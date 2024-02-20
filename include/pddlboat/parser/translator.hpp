/** Author: Zachary Kingston */

#ifndef PDDLBOAT_TRANSLATOR_
#define PDDLBOAT_TRANSLATOR_

#include <string>
#include <pddlboat/macros.hpp>
#include <pddlboat/parser/ast.hpp>

namespace pddlboat
{
    PDDLBOAT_CLASS_FORWARD(Domain);
    PDDLBOAT_CLASS_FORWARD(Problem);
    PDDLBOAT_CLASS_FORWARD(Action);
    PDDLBOAT_CLASS_FORWARD(Axiom);
    PDDLBOAT_CLASS_FORWARD(Expression);
    PDDLBOAT_CLASS_FORWARD(DerivedPredicateDefinition);
    PDDLBOAT_CLASS_FORWARD(Variable);

    std::vector<Variable> getVariables(const std::vector<ast::TypedList> &parsed);
    ExpressionPtr toExpression(const ast::Expression &expression, const std::vector<Variable> &variables,
                               const DomainPtr &domain, const ProblemPtr &problem = nullptr);
    ActionPtr toAction(const ast::Action &action, const DomainPtr &domain);
    DerivedPredicateDefinitionPtr toDerived(const ast::Action &derived, DomainPtr &domain);
    AxiomPtr toAxiom(const ast::Action &axiom, DomainPtr &domain);
    DomainPtr toDomain(const ast::Domain &domain);
    ProblemPtr toProblem(const ast::Problem &problem, const DomainPtr &domain);

    DomainPtr parseDomain(const std::string &domain_file);
    ProblemPtr parseProblem(const DomainPtr &domain, const std::string &problem_file);
    ExpressionPtr parseExpression(const ProblemPtr &problem, const std::string &expression_string);
}  // namespace pddlboat

#endif
