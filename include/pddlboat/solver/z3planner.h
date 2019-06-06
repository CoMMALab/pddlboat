#ifndef PDDLBOAT_Z3_PLANNER_
#define PDDLBOAT_Z3_PLANNER_

#include <z3++.h>
#include <pddlboat/utility.h>
#include <pddlboat/parser/parser.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/domain.h>
#include <pddlboat/solver/planner.h>

#include <iostream>
#include <sstream>
#include <unordered_map>

namespace pddlboat
{
    class Z3Planner : public Planner
    {
        const Time horizon_;

        z3::context *ctx_{nullptr};
        z3::solver *solver_{nullptr};

    public:
        Z3Planner(const ProblemPtr problem, const Time horizon) : Planner(problem), horizon_(horizon)
        {
            build();
        }

        void buildPlan(Plan &plan)
        {
            auto m = solver_->get_model();
            for (size_t i = 0; i < m.num_consts(); i++)
            {
                z3::expr f = m.get_const_interp(m.get_const_decl(i));

                if (f.is_true())
                {
                    const std::string name = m.get_const_decl(i).name().str();
                    ast::z3Action action;
                    if (parseZ3Action(name, action))
                        plan.addStep(action.name, action.args);
                }
            }
        }

        bool solve(Plan &plan) override
        {
            if (not solver_)
                return false;

            switch (solver_->check())
            {
                case z3::sat:
                    std::cout << "planning instance is solved\n";
                    buildPlan(plan);
                    std::cout << plan << std::endl;

                    return true;
                case z3::unsat:
                    std::cout << "planning instance is not solved\n";
                    return false;
                default:
                    std::cout << "unknown\n";
                    return false;
            }
        }

    private:
        z3::expr makeBool(const std::string &string)  // Should be renamed. Only makes new if doesn't exist.
        {
            return ctx_->bool_const(string.c_str());
        }

        const std::string addTimeToString(std::string string, Time time)
        {
            return string + ":TIME:" + std::to_string(time);
        }

        /**
         * Recurses down the tree and builds a z3::expr for the expression, replacing atoms with their
         * z3 bool_constants. We require that the expression does not contain any actions (all terminals
         * are state props).
         */
        z3::expr exprAtTime(const ExpressionPtr &expr, const Assignment &assignment, const Time step)
        {
            switch (expr->type())
            {
                case Expression::Type::PREDICATE:
                {
                    const auto &predicate_expr = std::static_pointer_cast<Predicate>(expr);
                    return makeBool(addTimeToString(predicate_expr->toID(assignment), step));
                }
                case Expression::Type::AND:
                {
                    const auto &and_expr = std::static_pointer_cast<And>(expr);
                    z3::expr car = exprAtTime(makeCAR(and_expr), assignment, step);
                    auto cdr_expr = makeCDR(and_expr);
                    if (cdr_expr)
                    {
                        z3::expr cdr = exprAtTime(cdr_expr, assignment, step);
                        z3::expr clause = car and cdr;
                        return clause;
                    }
                    else
                        return car;
                }
                case Expression::Type::OR:
                {
                    const auto &or_expr = std::static_pointer_cast<Or>(expr);
                    z3::expr car = exprAtTime(or_expr->getTerms().back(), assignment, step);
                    auto cdr_expr = makeCDR(or_expr);
                    if (or_expr)
                    {
                        z3::expr cdr = exprAtTime(cdr_expr, assignment, step);
                        z3::expr clause = car or cdr;
                        return clause;
                    }
                    else
                        return car;
                }
                case Expression::Type::NOT:
                {
                    const auto &not_expr = std::static_pointer_cast<Not>(expr);
                    z3::expr exp = exprAtTime(not_expr->getTerm(), assignment, step);
                    return not exp;
                }
                case Expression::Type::EXPRESSION:
                    std::cout << "Error, we do not currently support Expression types here." << std::endl;
                    [[fallthrough]];  // sweet c++17 feature
                default:
                    std::cout << "We had an error in parsing the pddlboat expression to an expression for Z3."
                              << std::endl;
                    throw std::runtime_error("whoops!");
            }
        }

        bool predicateInEffect(const ExpressionPtr &expr, const Assignment &assignment, const std::string &predicate)
        {
            switch (expr->type())
            {
                case Expression::Type::PREDICATE:
                {
                    const auto &predicate_expr = std::static_pointer_cast<Predicate>(expr);
                    return predicate_expr->toID(assignment) == predicate;
                }
                case Expression::Type::AND:
                {
                    const auto &and_expr = std::static_pointer_cast<And>(expr);
                    for (const auto &term : and_expr->getTerms())
                        if (predicateInEffect(term, assignment, predicate))
                            return true;

                    return false;
                }
                case Expression::Type::OR:
                {
                    const auto &or_expr = std::static_pointer_cast<Or>(expr);
                    for (const auto &term : or_expr->getTerms())
                        if (predicateInEffect(term, assignment, predicate))
                            return true;

                    return false;
                }
                case Expression::Type::NOT:
                {
                    const auto &not_expr = std::static_pointer_cast<Not>(expr);
                    return predicateInEffect(not_expr->getTerm(), assignment, predicate);
                }
                case Expression::Type::EXPRESSION:
                    std::cout << "Error, we do not currently support Expression types here." << std::endl;
                    [[fallthrough]];  // sweet c++17 feature
                default:
                    std::cout << "We had an error in parsing the pddlboat expression to an expression for Z3."
                              << std::endl;
                    throw std::runtime_error("whoops!");
            }
        }

        /**
         * For frame axioms, we need to know all actions that could change a proposition
         * the time step is when the action is executed, so the change will be at the next time step
         */
        z3::expr getPossibleChangeActions(const std::string &predicate, const Time step_before)
        {
            z3::expr change_actions = ctx_->bool_val(false);

            for (const auto &name_actionptr : problem_->domain->actions)
            {
                const auto &assignments = problem_->getGroundings(name_actionptr.second->parameters);
                for (const auto &assignment : assignments)
                {
                    if (predicateInEffect(name_actionptr.second->effect, assignment, predicate))
                    {
                        std::string action_string =
                            addTimeToString(name_actionptr.second->toID(assignment), step_before);
                        change_actions = change_actions or makeBool(action_string);
                    }
                }
            }

            return change_actions;
        }

        void build()
        {
            // First we build the context, grounding all actions and predicates.
            ctx_ = new z3::context;
            solver_ = new z3::solver(*ctx_);

            // We store a set of grounded actions and predicates (without the timestep)
            std::vector<std::string> ground_actions;

            // Now that we have built the context, we need to add constraints. These are clauses that ensure:
            // (1) an action implies its precondition is true and its effects will be true next time step
            // (2) if a proposition changes, then an action must have changed it
            // (3) only one action is taken at each time step
            for (Time step = 0; step < horizon_; ++step)
            {
                // add clauses for preconditions and effects of actions
                for (const auto &name_actionptr : problem_->domain->actions)
                {
                    const auto &assignments = problem_->getGroundings(name_actionptr.second->parameters);
                    for (const auto &assignment : assignments)
                    {
                        std::string action_string = name_actionptr.second->toID(assignment);
                        std::string action_string_time = addTimeToString(action_string, step);

                        z3::expr act = makeBool(action_string_time);
                        z3::expr precond = exprAtTime(name_actionptr.second->precondition, assignment, step);
                        z3::expr effects = exprAtTime(name_actionptr.second->effect, assignment, step + 1);

                        // action -> preconditions_t0 and effects_t1
                        z3::expr clause = (not act) or (precond and effects);

                        solver_->add(clause);

                        if (step == 0)
                            ground_actions.emplace_back(action_string);
                    }
                }

                for (const auto &name_predptr : problem_->domain->predicates)
                {
                    const auto &assignments = problem_->getGroundings(name_predptr.second->variables);
                    for (const auto &assignment : assignments)
                    {
                        std::string pred_string = name_predptr.second->toID(assignment);

                        // add frame axioms
                        // if the prop changed, then an action changed it. it doesn't matter what the value
                        // changes to, because the clauses for precoditions and effects will ensure
                        // consistency
                        z3::expr curr_prop = makeBool(addTimeToString(pred_string, step));
                        z3::expr next_prop = makeBool(addTimeToString(pred_string, step + 1));
                        z3::expr clause =
                            not(curr_prop != next_prop) or (getPossibleChangeActions(pred_string, step));

                        solver_->add(clause);
                    }
                }

                // add mutual exclusion axioms, i.e., we only take one action each time step
                for (unsigned int i = 0; i < ground_actions.size(); ++i)
                {
                    const auto &action = ground_actions[i];
                    for (unsigned int j = i + 1; j < ground_actions.size(); ++j)
                    {
                        const auto &other_action = ground_actions[j];

                        z3::expr e = makeBool(addTimeToString(action, step));
                        z3::expr other_e = makeBool(addTimeToString(other_action, step));
                        z3::expr clause = (not e) or (not other_e);

                        solver_->add(clause);
                    }
                }
            }  // end for timestep

            // Add the goal
            Assignment empty_assgn(&*problem_);
            solver_->add(exprAtTime(problem_->goal, empty_assgn, horizon_));

            // Add the start state
            for (const auto &name_predptr : problem_->domain->predicates)
            {
                const auto &assignments = problem_->getGroundings(name_predptr.second->variables);
                for (const auto &assignment : assignments)
                {
                    std::string pred_string = name_predptr.second->toID(assignment);
                    z3::expr p = makeBool(addTimeToString(pred_string, 0));

                    if (problem_->start->isTrue(name_predptr.second, assignment))
                        solver_->add(z3::expr(p));
                    else
                        solver_->add(z3::expr(not p));
                }
            }
        }
    };

}  // namespace pddlboat

#endif  // include guard
