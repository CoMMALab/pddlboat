/** Author: Zachary Kingston */

#include <boost/algorithm/string.hpp>

#include <pddlboat/utility.h>

#include <pddlboat/parser/ast.h>
#include <pddlboat/parser/translator.h>
#include <pddlboat/solver/domain.h>
#include <pddlboat/solver/expressions.h>

using namespace pddlboat;

// const static bool debug = true;
const static bool debug = false;

namespace
{
    std::vector<Variable> getVariables(const std::vector<ast::TypedList> &parsed)
    {
        std::vector<Variable> variables;

        for (const auto &list : parsed)
        {
            const auto &parent = (list.parent != "") ? list.parent : PDDLBOAT_OBJECT_CLASS;
            for (const auto &variable : list.types)
                variables.emplace_back(variable, parent);
        }

        return variables;
    }

    bool isVariable(const std::string &name, const std::vector<Variable> &variables)
    {
        auto vit = std::find_if(variables.begin(), variables.end(),
                                [&name](const auto &var) { return var.name == name; });

        return vit != variables.end();
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

    std::string parseArgument(const ast::Expression &term, const std::vector<Variable> &variables,
                              const DomainPtr &domain, const ProblemPtr &problem)
    {
        if (term.op.toOp() != Operators::LITERAL)
            return "";

        const auto &name = term.op.toString();
        if (isVariable(name, variables))
            return "?" + name;

        if (isObject(name, domain, problem))
            return name;

        return "";
    }
}  // namespace

pddlboat::ExpressionPtr pddlboat::toExpression(const ast::Expression &expression,
                                               const std::vector<Variable> &variables,
                                               const DomainPtr &domain, const ProblemPtr &problem)
{
    switch (expression.op.toOp())
    {
        case Operators::LITERAL:
        {
            const auto &s = expression.op.toString();

            if (debug)
                std::cout << "Translating Literal " << s << std::endl;

            bool basic = domain->isPredicate(s);
            bool derived = domain->isDerivedPredicate(s);
            if (basic or derived)
            {
                std::vector<std::string> bindings;

                for (const auto &term : expression.terms)
                {
                    const auto &name = parseArgument(term, variables, domain, problem);
                    if (name.empty())
                        throw std::invalid_argument(log::format("Invalid term %1% in predicate!", s));

                    bindings.emplace_back(name);
                }

                if (derived)
                {
                    auto derived = domain->getDerivedPredicate(s);
                    return derived->bind(bindings, problem, domain);
                }
                else
                {
                    auto pred = domain->getPredicate(s);
                    return pred->bind(bindings, problem, domain);
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
                throw std::invalid_argument("Expression Forall must have parameters!");

            if (expression.terms.size() != 1)
                throw std::invalid_argument("Expression Forall must have one term!");

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

            if (e1t.empty())
                throw std::invalid_argument(log::format("Invalid first Equal variable!"));
            if (e2t.empty())
                throw std::invalid_argument(log::format("Invalid second Equal variable!"));

            return pddlboat::makeEquals(getValue(e1t, nullptr, problem, domain),
                                        getValue(e2t, nullptr, problem, domain));
        }
        default:
            throw std::invalid_argument(
                log::format("Expression type %1% not supported!", expression.op.toString()));
    };

    return nullptr;
}

pddlboat::ActionPtr pddlboat::toAction(const ast::Action &action, const DomainPtr &domain)
{
    const auto &variables = getVariables(action.parameters);
    return makeAction(action.name, variables,  //
                      toExpression(action.precondition, variables, domain),
                      toExpression(action.effect, variables, domain));
}

pddlboat::DerivedPredicateDefinitionPtr pddlboat::toDerived(const ast::Action &action,
                                                            DomainPtr &domain)
{
    // need to add before parsing expression due to recursive definitions
    const auto &variables = getVariables(action.parameters);
    auto pred = makeDerivedPred(action.name, variables, nullptr);
    domain->addDerivedPredicate(pred);
    pred->expression = toExpression(action.precondition, variables, domain);

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
        const auto &parent = (constantlist.parent != "") ? constantlist.parent : PDDLBOAT_OBJECT_CLASS;
        for (const auto &constant : constantlist.types)
            r->addConstant(constant, parent);
    }

    for (const auto &predicate : domain.predicates)
    {
        std::vector<Variable> variables;

        for (const auto &list : predicate.variables)
        {
            const auto &parent = (list.parent != "") ? list.parent : PDDLBOAT_OBJECT_CLASS;
            for (const auto &variable : list.types)
                variables.emplace_back(variable, parent);
        }

        r->addPredicate(pddlboat::makePred(predicate.name, variables));
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
        const auto &parent = (objectlist.parent != "") ? objectlist.parent : PDDLBOAT_OBJECT_CLASS;
        for (const auto &object : objectlist.types)
            r->addObject(object, parent);
    }

    for (const auto &formula : problem.initial)
    {
        std::vector<std::string> names;
        for (const auto &types : formula.variables)
            for (const auto &type : types.types)
                names.emplace_back(type);

        r->start->assignN(formula.name, names);
    }

    r->goal = toExpression(problem.goal, {}, domain, r);

    return r;
}
