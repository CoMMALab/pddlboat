#ifndef PDDLBOAT_PROBLEM_BUILDER_
#define PDDLBOAT_PROBLEM_BUILDER_

#include <z3++.h>

#include "utility.h"
#include "expressions.h"

namespace pddlboat
{
    class SimpleState : public State
    {
    public:
        SimpleState(Time id) : State()
        {
        }

        bool isTrue(const Predicate &predicate) const override
        {
            return false;
        }

        ExpressionPtr function(const Function &function) const override
        {
            return nullptr;
        }
    };

    z3::expr makeZ3Bool(z3::context *context, const std::string &string)
    {
        return context->bool_const(string.c_str());
    }

    // For frame axioms, we need to know all actions that could change a proposition
    // the time step is when the action is executed, so the change will be at the next time step
    z3::expr getPossibleChangeActions(std::string proposition, const std::vector<Action> &actions,
                                      z3::context *context, const Time step_before)
    {
        z3::expr changeActions = context->bool_val(false);
        for (const auto &a : actions)
        {
            // We assume this is time-step agnostic
            if (a.effects->containsProposition(proposition))
                changeActions = changeActions or makeZ3Bool(context, a.atTime(step_before));
            else
                std::cout << log::format("Action %1% does not contain prop. %2%.", a.name, proposition)
                          << std::endl;
        }

        return changeActions;
    }

    // Recurses down the tree and builds a z3::expr for the expression, replacing atoms with their
    // z3 bool_constants. We require that the expression does not contian any actions (all terminals are state
    // props).
    z3::expr getExprAtTime(const ExpressionPtr expr, z3::context *context, const Time step)
    {
        switch (expr->getExprType())
        {
            case ATOM:
            {
                const auto &atom_expr = std::static_pointer_cast<Atom>(expr);
                return makeZ3Bool(context, atom_expr->atTime(step));
            }
            case AND:
            {
                const auto &and_expr = std::static_pointer_cast<And>(expr);
                z3::expr car = getExprAtTime(makeCAR(and_expr), context, step);
                auto cdr_expr = makeCDR(and_expr);
                if (cdr_expr)
                {
                    z3::expr cdr = getExprAtTime(cdr_expr, context, step);
                    z3::expr clause = car and cdr;
                    return clause;
                }
                else
                    return car;
            }
            case OR:
            {
                const auto or_expr = std::static_pointer_cast<Or>(expr);
                z3::expr car = getExprAtTime(or_expr->getTerms().back(), context, step);
                auto cdr_expr = makeCDR(or_expr);
                if (or_expr)
                {
                    z3::expr cdr = getExprAtTime(cdr_expr, context, step);
                    z3::expr clause = car or cdr;
                    return clause;
                }
                else
                    return car;
            }
            case NOT:
            {
                const auto not_expr = std::static_pointer_cast<Not>(expr);
                z3::expr exp = getExprAtTime(not_expr->getTerm(), context, step);
                return not exp;
            }
            case EXPRESSION:
                std::cout << "Error, we do not currently support Expression types here." << std::endl;
                [[fallthrough]];  // sweet c++17 feature
            default:
                std::cout << "We had an error in parsing the pddlboat expression to an expression for Z3."
                          << std::endl;
                return makeZ3Bool(context, "a");
        }
    }  // namespace pddlboat

    void build_instance(const StatePtr &start_state, const ExpressionPtr &goal_expr,
                        const std::vector<Action> &actions, const Time planning_horizon)
    {
        // First we build the context with all boolean variables we will need
        // these are the propositons from the initial state at every timestep
        // as well as bools for the actions at each time step
        z3::context c;
        // We need to build the formula for each time step we consider.
        for (Time step = 0; step <= planning_horizon; ++step)
        {
            // add propositions for each variable in start state:
            for (const auto &prop : start_state->assignments)
                makeZ3Bool(&c, propAtTime(prop.first, step));

            // add boolean vars for each action this timestep, but we don't do this for the last time step
            if (step != planning_horizon)
                for (const auto &action : actions)
                    makeZ3Bool(&c, action.atTime(step));
        }

        // Now that we have built the context for this time step, we need to add constraints
        // for the problem. These are clauses that ensure:
        // (1) an action implies its precondition is true and its effects will be true next time step
        // (2) if a proposition changes, then an action must have changed it
        // (3) only one action is taken at each time step
        z3::solver s(c);
        for (Time step = 0; step < planning_horizon; ++step)
        {
            // add clauses for preconditions and effects of actions
            for (const auto &action : actions)
            {
                z3::expr precond = getExprAtTime(action.preconditions, &c, step);
                z3::expr effects = getExprAtTime(action.effects, &c, step + 1);
                z3::expr act = makeZ3Bool(&c, action.atTime(step));

                // action -> preconditions_t0 and effects_t1
                z3::expr clause = (not act) or (precond and effects);

                s.add(clause);
            }

            // add frame axioms
            for (const auto &prop_assgn : start_state->assignments)
            {
                z3::expr curr_prop = makeZ3Bool(&c, propAtTime(prop_assgn.first, step));
                z3::expr next_prop = makeZ3Bool(&c, propAtTime(prop_assgn.first, step + 1));

                // if the prop changed, then an action changed it. it doesn't matter what the value changes
                // to, because the clauses for precoditions and effects will ensure consistency
                z3::expr clause = not(curr_prop != next_prop) or
                                  (getPossibleChangeActions(prop_assgn.first, actions, &c, step));
                s.add(clause);
            }

            // add mutual exclusion axioms, i.e., we only take one action each time step
            for (const auto &action : actions)
                for (const auto &other_action : actions)
                    if (action.name != other_action.name)
                    {
                        // For each pair of distinct actions, one action needs to be false.
                        z3::expr e = makeZ3Bool(&c, action.atTime(step));
                        z3::expr other_e = makeZ3Bool(&c, other_action.atTime(step));
                        z3::expr clause = (not e) or (not other_e);
                        s.add(clause);
                    }
        }

        // Add the goal
        s.add(getExprAtTime(goal_expr, &c, planning_horizon));

        // Add the start state
        for (const auto &assgn : start_state->assignments)
        {
            z3::expr p = makeZ3Bool(&c, propAtTime(assgn.first, 0));
            s.add(z3::expr((assgn.second) ? p : not p));
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
