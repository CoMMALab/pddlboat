#ifndef PDDLBOAT_PROBLEM_BUILDER_
#define PDDLBOAT_PROBLEM_BUILDER_

#include <z3++.h>
#include "expressions.h"

namespace pddlboat
{
    class SimpleState : public State
    {
    public:
        SimpleState(size_t id) : State()
        {
        }

        bool isTrue(const Predicate &predicate) const override
        {
            return false;
        }

        std::string evalFun(const Function &function) const override
        {
            return "";
        }
    };

    std::string propAtTime(std::string proposition, size_t time_step)
    {
        return std::string("time_") + std::to_string(time_step) + std::string("_prop_") + proposition;
    }

    std::string actionAtTime(std::string action, size_t time_step)
    {
        return std::string("time_") + std::to_string(time_step) + std::string("_action_") + action;
    }

    // For frame axioms, we need to know all actions that could change a proposition
    // the time step is when the action is executed, so the change will be at the next time step
    z3::expr getPossibleChangeActions(std::string state_proposition, const std::vector<Action> &actions,
                                      z3::context *context_ptr, const size_t time_step_before)
    {
        return context_ptr->bool_const(actionAtTime("my_action", time_step_before).c_str());
    }

    // Recurses down the tree and builds a z3::expr for the expression, replacing atoms with their
    // z3 bool_constants. We require that the expression does not contian any actions (all terminals are state
    // props).
    z3::expr getExprAtTime(const ExpressionPtr expr, z3::context *context_ptr, const size_t time_step)
    {
        switch (expr->getExprType())
        {
            case ATOM:
            {
                const auto atom_expr = std::static_pointer_cast<Atom>(expr);
                std::string str = propAtTime(atom_expr->name, time_step);
                return context_ptr->bool_const(str.c_str());
            }
            break;
            case AND:
            {
                const auto and_expr = std::static_pointer_cast<And>(expr);
                z3::expr car = getExprAtTime(and_expr->getTerms().back(), context_ptr, time_step);
                if (and_expr->getTerms().size() > 1)
                {
                    auto cdr_conj = makeAnd({});
                    for (size_t i = 0; i < and_expr->getTerms().size() - 1; i++)
                    {
                        cdr_conj->addTerm(and_expr->getTerms()[i]);
                    }

                    // auto cdr_conj = makeAnd(cdr_vec);
                    z3::expr cdr = getExprAtTime(cdr_conj, context_ptr, time_step);
                    z3::expr clause = car && cdr;
                    return clause;
                }
                else
                    return car;
            }
            break;
            case OR:
            {
                const auto or_expr = std::static_pointer_cast<Or>(expr);
                z3::expr car = getExprAtTime(or_expr->getTerms().back(), context_ptr, time_step);
                if (or_expr->getTerms().size() > 1)
                {
                    auto cdr_disj = makeOr({});
                    for (size_t i = 0; i < or_expr->getTerms().size() - 1; i++)
                    {
                        cdr_disj->addTerm(or_expr->getTerms()[i]);
                    }

                    // auto cdr_disj = makeOr(cdr_vec);
                    z3::expr cdr = getExprAtTime(cdr_disj, context_ptr, time_step);
                    z3::expr clause = car || cdr;
                    return clause;
                }
                else
                    return car;
            }
            break;
            case NOT:
            {
                const auto not_expr = std::static_pointer_cast<Not>(expr);
                z3::expr exp = getExprAtTime(not_expr->getTerm(), context_ptr, time_step);
                return !exp;
            }
            break;
            case EXPRESSION:
            {
                std::cout << "Error, we do not currently support Expression types here." << std::endl;
            }
                [[fallthrough]];  // sweet c++17 feature
            default:
                std::cout << "We had an error in parsing the pddlboat expression to an expression for Z3."
                          << std::endl;
                return context_ptr->bool_const("a");
        }
    }  // namespace pddlboat

    void build_instance(const StatePtr start_state, const ExpressionPtr goal_expr,
                        const std::vector<Action> &actions, const size_t planning_horizon)
    {
        // First we build the context with all boolean variables we will need
        // these are the propositons from the initial state at every timestep
        // as well as bools for the actions at each time step
        z3::context c;
        // We need to build the formula for each time step we consider.
        for (size_t time_step = 0; time_step < planning_horizon + 1; time_step++)
        {
            // add propositions for each variable in start state:
            for (auto prop_assgn : start_state->assignments)
            {
                std::string str = propAtTime(prop_assgn.first, time_step);
                z3::expr x = c.bool_const(str.c_str());
            }

            // add boolean vars for each action this timestep, but we don't do this for the last time step
            if (time_step != planning_horizon)
            {
                for (auto action : actions)
                {
                    std::string str = actionAtTime(action.name, time_step);
                    z3::expr x = c.bool_const(str.c_str());
                }
            }
        }

        // Now that we have built the context for this time step, we need to add constraints
        // for the problem. These are clauses that ensure:
        // (1) an action implies its precondition is true and its effects will be true next time step
        // (2) if a proposition changes, then an action must have changed it
        // (3) only one action is taken at each time step
        z3::solver s(c);
        for (size_t time_step = 0; time_step < planning_horizon; time_step++)
        {
            // add clauses for preconditions and effects of actions
            for (auto action : actions)
            {
                z3::expr pre_cond = getExprAtTime(action.preconditions, &c, time_step);
                z3::expr effects = getExprAtTime(action.effects, &c, time_step + 1);
                std::string str = actionAtTime(action.name, time_step);
                z3::expr act = c.bool_const(str.c_str());
                z3::expr clause =
                    (!act) || (pre_cond && effects);  // action -> preconditions_t0 and effects_t1

                s.add(clause);
            }

            // add frame axioms
            for (auto prop_assgn : start_state->assignments)
            {
                std::string curr_prop_str = propAtTime(prop_assgn.first, time_step);
                z3::expr curr_prop = c.bool_const(curr_prop_str.c_str());
                std::string next_prop_str = propAtTime(prop_assgn.first, time_step+1);
                z3::expr next_prop = c.bool_const(next_prop_str.c_str());

                // if the prop changed, then an action changed it. it doesn't matter what the value changes
                // to, because the clauses for precoditions and effects will ensure consistency
                z3::expr clause = !(curr_prop != next_prop) ||
                                  (getPossibleChangeActions(prop_assgn.first, actions, &c, time_step));
                s.add(clause);
            }

            // add mutual exclusion axioms, i.e., we only take one action each time step
            for (auto action : actions)
            {
                for (auto other_action : actions)
                {
                    if (action.name != other_action.name)
                    {
                        // For each pair of distinct actions, one action needs to be false.
                        std::string str = actionAtTime(action.name, time_step);
                        std::string other_str = actionAtTime(other_action.name, time_step);
                        z3::expr e = c.bool_const(str.c_str());
                        z3::expr other_e = c.bool_const(str.c_str());
                        z3::expr clause = !e || !other_e;
                        s.add(clause);
                    }
                }
            }
        }

        // Add the goal
        s.add(getExprAtTime(goal_expr, &c, planning_horizon));

        // Add the start state
        for (auto prop_assgn : start_state->assignments)
        {
            z3::expr p = c.bool_const(propAtTime(prop_assgn.first, 0).c_str());
            if (prop_assgn.second == true)
            {
                s.add(z3::expr(p));
            }
            else
            {
                s.add(z3::expr(!p));
            }
        }

        // Solve the problem
        std::cout << "solver: " << std::endl;

        std::cout << s << "\n";

        std::cout << "solver to smt2" << std::endl;

        std::cout << s.to_smt2() << "\n";
        switch (s.check())
        {
            case z3::unsat:
                std::cout << "planning instance is not solved\n";
                break;
            case z3::sat:
                std::cout << "planning instance is solved\n";
                break;
            case z3::unknown:
                std::cout << "unknown\n";
                break;
        }
    }

}  // namespace pddlboat

#endif  // include guard
