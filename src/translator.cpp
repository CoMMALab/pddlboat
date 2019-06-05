/** Author: Zachary Kingston */

#include <pddlboat/utility.h>

#include <pddlboat/parser/ast.h>
#include <pddlboat/parser/translator.h>
#include <pddlboat/solver/domain.h>

using namespace pddlboat;

pddlboat::ExpressionPtr pddlboat::toExpression(const ast::Expression &expression,
                                               const std::vector<Variable> &variables,
                                               const DomainPtr &domain, const ProblemPtr &problem)
{
    switch (expression.op.toOp())
    {
        case Operators::LITERAL:
        {
            const auto &s = expression.op.toString();

            auto pred = domain->getPredicate(s);
            if (pred)
            {
                std::vector<std::string> bindings;

                for (const auto &term : expression.terms)
                {
                    if (term.op.toOp() != Operators::LITERAL)
                        throw std::invalid_argument(log::format("Invalid predicate %1%!", s));

                    const auto &name = term.op.toString();
                    auto vit = std::find_if(variables.begin(), variables.end(),
                                            [&name](const auto &var) { return var.name == name; });
                    if (vit != variables.end())
                    {
                        bindings.emplace_back("?" + name);
                        continue;
                    }

                    if (problem)
                    {
                        auto obj = problem->getObject(name);
                        if (obj)
                        {
                            bindings.emplace_back(name);
                            continue;
                        }
                    }

                    throw std::invalid_argument(log::format("Invalid term %2% in predicate %1%!", s, name));
                }

                return pred->bind(bindings, problem);
            }
            else
                throw std::invalid_argument(log::format("Invalid literal %1%!", s));
        }
        case Operators::AND:
        {
            std::vector<ExpressionPtr> terms;
            for (const auto &term : expression.terms)
                terms.emplace_back(toExpression(term, variables, domain, problem));

            return pddlboat::makeAnd(terms);
        }
        case Operators::OR:
        {
            std::vector<ExpressionPtr> terms;
            for (const auto &term : expression.terms)
                terms.emplace_back(toExpression(term, variables, domain, problem));

            return pddlboat::makeOr(terms);
        }
        case Operators::NOT:
        {
            if (expression.terms.size() != 1)
                throw std::invalid_argument("Expression Not can only have one term!");

            return pddlboat::makeNot(toExpression(expression.terms[0], variables, domain, problem));
        }
        case Operators::IMPLY:
        {
            if (expression.terms.size() != 2)
                throw std::invalid_argument("Expression Imply must have two terms!");

            return pddlboat::makeImply(toExpression(expression.terms[0], variables, domain, problem),
                                       toExpression(expression.terms[1], variables, domain, problem));
        }
        default:
            throw std::invalid_argument(
                log::format("Expression type %1% not supported!", expression.op.toString()));
    };

    return nullptr;
}

pddlboat::ActionPtr pddlboat::toAction(const ast::Action &action, const DomainPtr &domain)
{
    std::vector<Variable> variables;

    for (const auto &list : action.parameters)
    {
        const auto &parent = (list.parent != "") ? list.parent : PDDLBOAT_OBJECT_CLASS;
        for (const auto &variable : list.types)
            variables.emplace_back(variable, parent);
    }

    return makeAction(action.name, variables,  //
                      toExpression(action.precondition, variables, domain),
                      toExpression(action.effect, variables, domain));
}

pddlboat::DomainPtr pddlboat::toDomain(const ast::Domain &domain)
{
    auto r = std::make_shared<Domain>(domain.name);

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
        r->addAction(toAction(action, r));

    return r;
}

pddlboat::ProblemPtr pddlboat::toProblem(const ast::Problem &problem, const DomainPtr &domain)
{
    auto r = std::make_shared<Problem>(problem.name, domain);
    if (domain->name != problem.domain)
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
