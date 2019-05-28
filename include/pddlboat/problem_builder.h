#ifndef PDDLBOAT_PROBLEM_BUILDER_
#define PDDLBOAT_PROBLEM_BUILDER_

#include <z3++.h>

#include "utility.h"
#include "expressions.h"

namespace pddlboat
{
    class Planner
    {
    public:
        Planner(const StatePtr &start, const ExpressionPtr &goal, const std::vector<ActionPtr> &actions,
                const Time horizon)
          : start_(start), goal_(goal), actions_(actions), horizon_(horizon)
        {
        }

        virtual bool solve() = 0;

    protected:
        const StatePtr start_;
        const ExpressionPtr goal_;
        const std::vector<ActionPtr> actions_;
        const Time horizon_;
    };

    class Z3Planner : public Planner
    {
    public:
        Z3Planner(const StatePtr &start, const ExpressionPtr &goal, const std::vector<ActionPtr> &actions,
                  const Time horizon) : Planner(start, goal, actions, horizon)
        {
            build();
        }

        bool solve() override
        {
            if (not solver_)
                return false;

            // Solve the problem
            std::cout << "solver: " << std::endl;
            std::cout << *solver_ << "\n";
            std::cout << "solver to smt2" << std::endl;
            std::cout << solver_->to_smt2() << "\n";

            switch (solver_->check())
            {
            case z3::sat:
                std::cout << "planning instance is solved\n";
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
        z3::expr makeBool(const std::string &string)
        {
            return ctx_->bool_const(string.c_str());
        }

        /**
         * For frame axioms, we need to know all actions that could change a proposition
         * the time step is when the action is executed, so the change will be at the next time step
         */
        z3::expr getPossibleChangeActions(std::string proposition, const Time step_before)
        {
            z3::expr changeActions = ctx_->bool_val(false);
            for (const auto &action : actions_)
            {
                // We assume this is time-step agnostic
                if (action->effects->containsProposition(proposition))
                    changeActions = changeActions or makeBool(action->atTime(step_before));
                else
                    std::cout << log::format("Action %1% does not contain prop. %2%.", action->name,
                                             proposition)
                              << std::endl;
            }

            return changeActions;
        }

        /**
         * Recurses down the tree and builds a z3::expr for the expression, replacing atoms with their
         * z3 bool_constants. We require that the expression does not contian any actions (all terminals
         * are state props).
         */
        z3::expr exprAtTime(const ExpressionPtr &expr, const Time step)
        {
            switch (expr->getExprType())
            {
                case ATOM:
                {
                    const auto &atom_expr = std::static_pointer_cast<Atom>(expr);
                    return makeBool(atom_expr->atTime(step));
                }
                case AND:
                {
                    const auto &and_expr = std::static_pointer_cast<And>(expr);
                    z3::expr car = exprAtTime(makeCAR(and_expr), step);
                    auto cdr_expr = makeCDR(and_expr);
                    if (cdr_expr)
                    {
                        z3::expr cdr = exprAtTime(cdr_expr, step);
                        z3::expr clause = car and cdr;
                        return clause;
                    }
                    else
                        return car;
                }
                case OR:
                {
                    const auto &or_expr = std::static_pointer_cast<Or>(expr);
                    z3::expr car = exprAtTime(or_expr->getTerms().back(), step);
                    auto cdr_expr = makeCDR(or_expr);
                    if (or_expr)
                    {
                        z3::expr cdr = exprAtTime(cdr_expr, step);
                        z3::expr clause = car or cdr;
                        return clause;
                    }
                    else
                        return car;
                }
                case NOT:
                {
                    const auto &not_expr = std::static_pointer_cast<Not>(expr);
                    z3::expr exp = exprAtTime(not_expr->getTerm(), step);
                    return not exp;
                }
                case EXPRESSION:
                    std::cout << "Error, we do not currently support Expression types here." << std::endl;
                    [[fallthrough]];  // sweet c++17 feature
                default:
                    std::cout << "We had an error in parsing the pddlboat expression to an expression for Z3."
                              << std::endl;
                    throw std::runtime_error("whoops!");
            }
        }

        void build()
        {
            // First we build the context with all boolean variables we will need
            // these are the propositons from the initial state at every timestep
            // as well as bools for the actions at each time step
            ctx_ = new z3::context;

            // We need to build the formula for each time step we consider.
            for (Time step = 0; step <= horizon_; ++step)
            {
                // add propositions for each variable in start state:
                for (const auto &prop : start_->assignments)
                    makeBool(propAtTime(prop.first, step));

                // add boolean vars for each action this timestep, but we don't do this for the last time step
                if (step != horizon_)
                    for (const auto &action : actions_)
                        makeBool(action->atTime(step));
            }

            solver_ = new z3::solver(*ctx_);

            // Now that we have built the context for this time step, we need to add constraints
            // for the problem. These are clauses that ensure:
            // (1) an action implies its precondition is true and its effects will be true next time step
            // (2) if a proposition changes, then an action must have changed it
            // (3) only one action is taken at each time step
            for (Time step = 0; step < horizon_; ++step)
            {
                // add clauses for preconditions and effects of actions
                for (const auto &action : actions_)
                {
                    z3::expr precond = exprAtTime(action->preconditions, step);
                    z3::expr effects = exprAtTime(action->effects, step + 1);
                    z3::expr act = makeBool(action->atTime(step));

                    // action -> preconditions_t0 and effects_t1
                    z3::expr clause = (not act) or (precond and effects);

                    solver_->add(clause);
                }

                // add frame axioms
                for (const auto &prop_assgn : start_->assignments)
                {
                    z3::expr curr_prop = makeBool(propAtTime(prop_assgn.first, step));
                    z3::expr next_prop = makeBool(propAtTime(prop_assgn.first, step + 1));

                    // if the prop changed, then an action changed it. it doesn't matter what the value
                    // changes to, because the clauses for precoditions and effects will ensure consistency
                    z3::expr clause =
                        not(curr_prop != next_prop) or (getPossibleChangeActions(prop_assgn.first, step));
                    solver_->add(clause);
                }

                // add mutual exclusion axioms, i.e., we only take one action each time step
                for (const auto &action : actions_)
                    for (const auto &other_action : actions_)
                        if (action->name != other_action->name)
                        {
                            // For each pair of distinct actions, one action needs to be false.
                            z3::expr e = makeBool(action->atTime(step));
                            z3::expr other_e = makeBool(other_action->atTime(step));
                            z3::expr clause = (not e) or (not other_e);
                            solver_->add(clause);
                        }
            }

            // Add the goal
            solver_->add(exprAtTime(goal_, horizon_));

            // Add the start state
            for (const auto &assgn : start_->assignments)
            {
                z3::expr p = makeBool(propAtTime(assgn.first, 0));
                solver_->add(z3::expr((assgn.second) ? p : not p));
            }
        }

        z3::context *ctx_{nullptr};
        z3::solver *solver_{nullptr};
    };

}  // namespace pddlboat

#endif  // include guard
