/** Author: Zachary Kingston */

#ifndef PDDLBOAT_TRANSLATOR_
#define PDDLBOAT_TRANSLATOR_

#include <string>
#include <pddlboat/macros.h>
#include <pddlboat/parser/ast.h>

namespace pddlboat
{
    PDDLBOAT_CLASS_FORWARD(Domain);
    PDDLBOAT_CLASS_FORWARD(Problem);
    PDDLBOAT_CLASS_FORWARD(Action);
    PDDLBOAT_CLASS_FORWARD(Expression);
    PDDLBOAT_CLASS_FORWARD(DerivedPredicateDefinition);
    PDDLBOAT_CLASS_FORWARD(Variable);

    ExpressionPtr toExpression(const ast::Expression &expression, const std::vector<Variable> &variables,
                               const DomainPtr &domain, const ProblemPtr &problem = nullptr);
    ActionPtr toAction(const ast::Action &action, const DomainPtr &domain);
    DerivedPredicateDefinitionPtr toDerived(const ast::Action &derived, DomainPtr &domain);
    DomainPtr toDomain(const ast::Domain &domain);
    ProblemPtr toProblem(const ast::Problem &problem, const DomainPtr &domain);
}  // namespace pddlboat

#endif
