/** Author: Zachary Kingston */

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <pddlboat/utility.h>

#include <pddlboat/parser/ast.h>
#include <pddlboat/parser/parser.h>
#include <pddlboat/parser/translator.h>
#include <pddlboat/solver/domain.h>
#include <pddlboat/solver/expressions.h>

using namespace pddlboat;

// const static bool debug = true;
const static bool debug = false;

namespace
{
    const Variable *isVariable(const std::string &name, const std::vector<Variable> &variables)
    {
        auto vit = std::find_if(variables.begin(), variables.end(),
                                [&name](const auto &var) { return var.name == name; });

        if (vit != variables.end())
            return &*vit;
        else
            return nullptr;
    }

    bool isObject(const std::string &name, const DomainPtr &domain, const ProblemPtr &problem)
    {
        if (domain)
        {
            auto constant = domain->getConstant(name);
            if (constant)
                return true;
        }

        if (problem)
        {
            auto obj = problem->getObject(name);
            if (obj)
                return true;
        }

        return false;
    }

    Value parseArgument(const ast::Expression &term, const std::vector<Variable> &variables,
                        const DomainPtr &domain, const ProblemPtr &problem)
    {
        if (term.getKeyword() != Operators::LITERAL)
            throw std::invalid_argument(log::format("Invalid op %1% in predicate!", term.op));

        const auto &name = boost::algorithm::to_lower_copy(term.op);
        const Variable *var = isVariable(name, variables);
        if (domain->isFunction(name))
        {
            auto exp = toExpression(term, variables, domain, problem);
            auto of = std::dynamic_pointer_cast<ObjectFunction>(exp);
            if (of)
                return Value(of);
            else
                throw std::invalid_argument(log::format("Invalid term %1% in predicate!", name));
        }
        else if (var or isObject(name, domain, problem))
        {
            auto temp = (var ? "?" : "") + name;
            return getValue(temp, var, problem.get(), domain.get());
        }

        throw std::invalid_argument(log::format("Invalid term %1% in predicate!", name));
    }
}  // namespace

std::vector<Variable> pddlboat::getVariables(const std::vector<ast::TypedList> &parsed)
{
    std::vector<Variable> variables;

    for (const auto &list : parsed)
    {
        const auto &parent =
            boost::algorithm::to_lower_copy((list.parent != "") ? list.parent : PDDLBOAT_OBJECT_CLASS);
        for (const auto &variable : list.types)
            variables.emplace_back(boost::algorithm::to_lower_copy(variable), parent);
    }

    return variables;
}

pddlboat::ExpressionPtr pddlboat::toExpression(const ast::Expression &expression,
                                               const std::vector<Variable> &variables,
                                               const DomainPtr &domain, const ProblemPtr &problem)
{
    switch (expression.getKeyword())
    {
        case Operators::LITERAL:
        {
            const auto &s = boost::algorithm::to_lower_copy(expression.op);

            if (debug)
                std::cout << "Translating Literal " << s << std::endl;

            bool basic = domain->isPredicate(s);
            bool function = domain->isFunction(s);
            bool derived = domain->isDerivedPredicate(s);
            if (basic or derived or function)
            {
                std::vector<Value> bindings;

                for (std::size_t i = 0; i < expression.terms.size(); ++i)
                {
                    auto &term = expression.terms[i];

                    const auto &value = parseArgument(term, variables, domain, problem);
                    bindings.emplace_back(value);
                }

                if (derived)
                {
                    auto derived = domain->getDerivedPredicate(s);
                    return derived->bindValues(bindings);
                }
                else if (function)
                {
                    if (domain->isNumericFunction(s))
                    {
                        auto function = domain->getNumericFunction(s);
                        return function->bindValues(bindings);
                    }
                    else
                    {
                        auto function = domain->getObjectFunction(s);
                        return function->bindValues(bindings);
                    }
                }
                else
                {
                    auto pred = domain->getPredicate(s);
                    return pred->bindValues(bindings);
                }
            }
            else
                throw std::invalid_argument(log::format("Invalid literal %1%!", s));

            return nullptr;
        }
        case Operators::AND:
        {
            if (debug)
                std::cout << "Translating And" << std::endl;

            std::vector<ExpressionPtr> terms;
            for (const auto &term : expression.terms)
                terms.emplace_back(toExpression(term, variables, domain, problem));

            return pddlboat::makeAnd(terms);
        }
        case Operators::OR:
        {
            if (debug)
                std::cout << "Translating Or" << std::endl;

            std::vector<ExpressionPtr> terms;
            for (const auto &term : expression.terms)
                terms.emplace_back(toExpression(term, variables, domain, problem));

            return pddlboat::makeOr(terms);
        }
        case Operators::NOT:
        {
            if (debug)
                std::cout << "Translating Not" << std::endl;

            if (expression.terms.size() != 1)
                throw std::invalid_argument("Expression Not can only have one term!");

            return pddlboat::makeNot(toExpression(expression.terms[0], variables, domain, problem));
        }
        case Operators::IMPLY:
        {
            if (debug)
                std::cout << "Translating Imply" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Imply must have two terms!");

            return pddlboat::makeImply(toExpression(expression.terms[0], variables, domain, problem),
                                       toExpression(expression.terms[1], variables, domain, problem));
        }
        case Operators::FORALL:
        {
            if (debug)
                std::cout << "Translating Forall" << std::endl;

            if (expression.parameters.empty())
                throw std::invalid_argument("Expression Forall must have parameters!");

            if (expression.terms.size() != 1)
                throw std::invalid_argument("Expression Forall must have one term!");

            const auto &quant = getVariables(expression.parameters);
            auto combined = quant;
            combined.insert(combined.end(), variables.begin(), variables.end());

            return pddlboat::makeForall(quant, toExpression(expression.terms[0], combined, domain, problem));
        }
        case Operators::EXISTS:
        {
            if (debug)
                std::cout << "Translating Exists" << std::endl;

            if (expression.parameters.empty())
                throw std::invalid_argument("Expression Exists must have parameters!");

            if (expression.terms.size() != 1)
                throw std::invalid_argument("Expression Exists must have one term!");

            const auto &quant = getVariables(expression.parameters);
            auto combined = quant;
            combined.insert(combined.end(), variables.begin(), variables.end());

            return pddlboat::makeExists(quant, toExpression(expression.terms[0], combined, domain, problem));
        }
        case Operators::WHEN:
        {
            if (debug)
                std::cout << "Translating When" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression When must have two terms!");

            return pddlboat::makeWhen(toExpression(expression.terms[0], variables, domain, problem),
                                      toExpression(expression.terms[1], variables, domain, problem));
        }
        case Operators::EQUAL:
        {
            if (debug)
                std::cout << "Translating Equal" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Equal must have two terms!");

            auto &e1 = expression.terms[0];
            auto &e2 = expression.terms[1];

            auto e1t = parseArgument(e1, variables, domain, problem);
            auto e2t = parseArgument(e2, variables, domain, problem);

            return pddlboat::makeEquals(e1t, e2t);
        }
        case Operators::MINUS:
        {
            if (debug)
                std::cout << "Translating Minus" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Minus must have two terms!");

            auto e1 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto e2 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));
            if (not e1 or not e2)
                throw std::invalid_argument("Expression Minus terms must be numeric!");

            return pddlboat::makeMinus(e1, e2);
        }
        case Operators::DIVIDE:
        {
            if (debug)
                std::cout << "Translating Divide" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Divide must have two terms!");

            auto e1 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto e2 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));
            if (not e1 or not e2)
                throw std::invalid_argument("Expression Divide terms must be numeric!");

            return pddlboat::makeDivide(e1, e2);
        }
        case Operators::MULTIPLY:
        {
            if (debug)
                std::cout << "Translating Multiply" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Multiply must have two terms!");

            auto e1 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto e2 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));
            if (not e1 or not e2)
                throw std::invalid_argument("Expression Multiply terms must be numeric!");

            return pddlboat::makeMultiply(e1, e2);
        }
        case Operators::PLUS:
        {
            if (debug)
                std::cout << "Translating Plus" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Plus must have two terms!");

            auto e1 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto e2 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));
            if (not e1 or not e2)
                throw std::invalid_argument("Expression Plus terms must be numeric!");

            return pddlboat::makePlus(e1, e2);
        }
        case Operators::GREATER:
        {
            if (debug)
                std::cout << "Translating Greater" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Greater must have two terms!");

            auto e1 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto e2 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));
            if (not e1 or not e2)
                throw std::invalid_argument("Expression Greater terms must be numeric!");

            return pddlboat::makeGreaterThan(e1, e2);
        }
        case Operators::LESS:
        {
            if (debug)
                std::cout << "Translating Less" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Less must have two terms!");

            auto e1 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto e2 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));
            if (not e1 or not e2)
                throw std::invalid_argument("Expression Less terms must be numeric!");

            return pddlboat::makeLessThan(e1, e2);
        }
        case Operators::GREATER_OR_EQUAL:
        {
            if (debug)
                std::cout << "Translating GreaterOrEqual" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression GreaterOrEqual must have two terms!");

            auto e1 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto e2 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));
            if (not e1 or not e2)
                throw std::invalid_argument("Expression GreaterOrEqual terms must be numeric!");

            return pddlboat::makeGreaterOrEqual(e1, e2);
        }
        case Operators::LESS_OR_EQUAL:
        {
            if (debug)
                std::cout << "Translating LessOrEqual" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression LessOrEqual must have two terms!");

            auto e1 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto e2 = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));
            if (not e1 or not e2)
                throw std::invalid_argument("Expression LessOrEqual terms must be numeric!");

            return pddlboat::makeLessOrEqual(e1, e2);
        }
        case Operators::ASSIGN:
        {
            if (debug)
                std::cout << "Translating Assign" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Assign must have two terms!");

            auto lhs = toExpression(expression.terms[0], variables, domain, problem);
            auto olhs = std::dynamic_pointer_cast<ObjectFunction>(lhs);
            auto nlhs = std::dynamic_pointer_cast<NumericFunction>(lhs);

            if (olhs)
            {
                const auto &value = parseArgument(expression.terms[1], variables, domain, problem);
                return pddlboat::makeAssign(olhs, value);
            }

            if (nlhs)
            {
                auto rhs = toExpression(expression.terms[1], variables, domain, problem);
                auto nrhs = std::dynamic_pointer_cast<NumericExpression>(rhs);
                return pddlboat::makeAssign(nlhs, nrhs);
            }

            throw std::invalid_argument("Error in Expression Assign!");
        }
        case Operators::SCALE_UP:
        {
            if (debug)
                std::cout << "Translating Scale Up" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Scale Up must have two terms!");

            auto lhs = std::dynamic_pointer_cast<NumericFunction>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto rhs = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));

            if (not lhs or not rhs)
                throw std::invalid_argument("Expression Scale Up terms must be numeric!");

            return pddlboat::makeScaleUp(lhs, rhs);
        }
        case Operators::SCALE_DOWN:
        {
            if (debug)
                std::cout << "Translating Scale Down" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Scale Down must have two terms!");

            auto lhs = std::dynamic_pointer_cast<NumericFunction>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto rhs = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));

            if (not lhs or not rhs)
                throw std::invalid_argument("Expression Scale Down terms must be numeric!");

            return pddlboat::makeScaleDown(lhs, rhs);
        }
        case Operators::INCREASE:
        {
            if (debug)
                std::cout << "Translating Increase" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Increase must have two terms!");

            auto lhs = std::dynamic_pointer_cast<NumericFunction>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto rhs = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));

            if (not lhs or not rhs)
                throw std::invalid_argument("Expression Increase terms must be numeric!");

            return pddlboat::makeIncrease(lhs, rhs);
        }
        case Operators::DECREASE:
        {
            if (debug)
                std::cout << "Translating Decrease" << std::endl;

            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Decrease must have two terms!");

            auto lhs = std::dynamic_pointer_cast<NumericFunction>(
                toExpression(expression.terms[0], variables, domain, problem));
            auto rhs = std::dynamic_pointer_cast<NumericExpression>(
                toExpression(expression.terms[1], variables, domain, problem));

            if (not lhs or not rhs)
                throw std::invalid_argument("Expression Decrease terms must be numeric!");

            return pddlboat::makeDecrease(lhs, rhs);
        }
        default:
            throw std::invalid_argument(log::format("Expression type %1% not supported!", expression.op));
    };

    return nullptr;
}

pddlboat::ActionPtr pddlboat::toAction(const ast::Action &action, const DomainPtr &domain)
{
    const auto &variables = getVariables(action.parameters);
    return makeAction(action.name, variables,  //
                      (action.precondition.getKeyword() != Operators::Keyword::NONE) ?
                          toExpression(action.precondition, variables, domain) :
                          makeAnd({}),
                      (action.effect.getKeyword() != Operators::Keyword::NONE) ?
                          toExpression(action.effect, variables, domain) :
                          makeAnd({}));
}

pddlboat::DerivedPredicateDefinitionPtr pddlboat::toDerived(const ast::Action &action, DomainPtr &domain)
{
    // need to add before parsing expression due to recursive definitions
    const auto &variables = getVariables(action.parameters);
    auto pred = makeDerivedPred(action.name, variables, nullptr);
    domain->addDerivedPredicate(pred);
    pred->expression = (action.precondition.getKeyword() != Operators::Keyword::NONE) ?
                           toExpression(action.precondition, variables, domain) :
                           makeOr({});

    return pred;
}

pddlboat::DomainPtr pddlboat::toDomain(const ast::Domain &domain)
{
    auto r = std::make_shared<Domain>(domain.name);

    for (const auto &keyword : domain.requirements)
        r->addRequirement(keyword);

    for (const auto &typelist : domain.types)
    {
        const auto &parent = (typelist.parent != "") ? typelist.parent : PDDLBOAT_OBJECT_CLASS;
        for (const auto &type : typelist.types)
            r->addType(type, parent);
    }

    for (const auto &constantlist : domain.constants)
    {
        const auto &parent = boost::algorithm::to_lower_copy(
            (constantlist.parent != "") ? constantlist.parent : PDDLBOAT_OBJECT_CLASS);
        for (const auto &constant : constantlist.types)
            r->addConstant(boost::algorithm::to_lower_copy(constant), parent);
    }

    for (const auto &predicate : domain.predicates)
    {
        std::vector<Variable> variables;

        for (const auto &list : predicate.variables)
        {
            const auto &parent =
                boost::algorithm::to_lower_copy((list.parent != "") ? list.parent : PDDLBOAT_OBJECT_CLASS);
            for (const auto &variable : list.types)
                variables.emplace_back(boost::algorithm::to_lower_copy(variable), parent);
        }

        r->addPredicate(pddlboat::makePred(boost::algorithm::to_lower_copy(predicate.name), variables));
    }

    for (const auto &functionlist : domain.functions)
    {
        const auto &listparent = boost::algorithm::to_lower_copy(
            (functionlist.parent != "") ? functionlist.parent : PDDLBOAT_NUMERIC_CLASS);
        for (const auto &function : functionlist.formulas)
        {
            std::vector<Variable> variables;

            for (const auto &list : function.variables)
            {
                const auto &parent = boost::algorithm::to_lower_copy(
                    (list.parent != "") ? list.parent : PDDLBOAT_OBJECT_CLASS);
                for (const auto &variable : list.types)
                    variables.emplace_back(boost::algorithm::to_lower_copy(variable), parent);
            }

            if (listparent == PDDLBOAT_NUMERIC_CLASS)
                r->addFunction(
                    pddlboat::makeFunction(boost::algorithm::to_lower_copy(function.name), variables));
            else
                r->addFunction(pddlboat::makeObjectFunction(boost::algorithm::to_lower_copy(function.name),
                                                            listparent, variables));
        }
    }

    for (const auto &action : domain.actions)
    {
        if (action.derived)
            toDerived(action, r);
        else
            r->addAction(toAction(action, r));
    }

    return r;
}

pddlboat::ProblemPtr pddlboat::toProblem(const ast::Problem &problem, const DomainPtr &domain)
{
    auto r = std::make_shared<Problem>(problem.name, domain);
    if (domain->name != boost::algorithm::to_lower_copy(problem.domain))
        throw std::invalid_argument(
            log::format("Problem domain %1% does not match %2%!", problem.domain, domain->name));

    for (const auto &objectlist : problem.objects)
    {
        auto parent = boost::algorithm::to_lower_copy((objectlist.parent != "") ? objectlist.parent :
                                                                                  PDDLBOAT_OBJECT_CLASS);

        if (not domain->isType(parent))
            throw std::invalid_argument(log::format("Invalid type `%1%` in problem!", parent));

        for (const auto &object : objectlist.types)
            r->addObject(boost::algorithm::to_lower_copy(object), parent);
    }

    for (const auto &formula : problem.initial)
    {
        std::vector<std::string> names;
        for (const auto &types : formula.formula.variables)
            for (const auto &type : types.types)
                names.emplace_back(boost::algorithm::to_lower_copy(type));

        std::string name = boost::algorithm::to_lower_copy(formula.formula.name);
        if (not formula.set)
            r->start->assignN(name, names, true);
        else
        {
            try
            {
                double x = boost::lexical_cast<double>(formula.object);
                r->start->assignN(name, names, x);
            }
            catch (...)
            {
                r->start->assignN(name, names, boost::algorithm::to_lower_copy(formula.object));
            }
        }
    }

    r->goal = toExpression(problem.goal, {}, domain, r);
    if (problem.metric.active)
    {
        r->minimize = problem.metric.minimize;
        r->metric = toExpression(problem.metric.function, {}, domain, r);
    }

    return r;
}

pddlboat::DomainPtr pddlboat::parseDomain(const std::string &domain_file)
{
    ast::Domain domain_ast;
    DomainPtr domain;

    // parse domain
    if (not parseFile(domain_file, domain_ast))
    {
        std::cerr << log::format("Failed to parse domain file %1%.", domain_file) << std::endl;
        return nullptr;
    }

    // translate domain
    try
    {
        domain = toDomain(domain_ast);
    }
    catch (const std::exception &e)
    {
        std::cerr << log::format("Caught exception translating domain: %1%", e.what()) << std::endl;
        return nullptr;
    }

    return domain;
}

pddlboat::ProblemPtr pddlboat::parseProblem(const DomainPtr &domain, const std::string &problem_file)
{
    ast::Problem problem_ast;
    ProblemPtr problem;

    // parse problem
    if (not parseFile(problem_file, problem_ast))
    {
        std::cerr << log::format("Failed to parse problem file %1%.", problem_file) << std::endl;
        return nullptr;
    }

    // translate problem
    try
    {
        problem = toProblem(problem_ast, domain);
    }
    catch (const std::exception &e)
    {
        std::cerr << log::format("Caught exception translating problem: %1%", e.what()) << std::endl;
        return nullptr;
    }

    return problem;
}
