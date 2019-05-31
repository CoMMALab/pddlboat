#ifndef PDDLBOAT_PROBLEM_BUILDER_
#define PDDLBOAT_PROBLEM_BUILDER_

#include <z3++.h>

#include <pddlboat/utility.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/domain.h>

namespace pddlboat
{
    using Time = size_t;

    class Planner
    {
    public:
        /* Planner(const StatePtr &start, const ExpressionPtr &goal, const std::vector<ActionPtr> &actions, */
        /*         const Time horizon) */
        /*   : start_(start), goal_(goal), actions_(actions), horizon_(horizon) */
        /* { */
        /* } */

        Planner(const ProblemPtr problem, const Time horizon) : problem_(problem), horizon_(horizon)
        {
        }

        virtual bool solve() = 0;

    protected:
        /* const StatePtr start_; */
        /* const ExpressionPtr goal_; */
        /* const std::vector<ActionPtr> actions_; */
        const Time horizon_;

        const ProblemPtr problem_;
    };

    class Z3Planner : public Planner
    {
        // We store a set of grounded actions and predicates (without the timestep)
        std::set<std::string> grounded_actions_;
        std::set<std::string> grounded_predicates_;

        z3::context *ctx_{nullptr};
        z3::solver *solver_{nullptr};

    public:
        /* Z3Planner(const StatePtr &start, const ExpressionPtr &goal, const std::vector<ActionPtr> &actions,
         */
        /*           const Time horizon) : Planner(start, goal, actions, horizon) */
        /* { */
        /*     build(); */
        /* } */

        Z3Planner(const ProblemPtr problem, const Time horizon) : Planner(problem, horizon)
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
        z3::expr makeBool(const std::string &string)  // Should be renamed. Only makes new if doesn't exist.
        {
            return ctx_->bool_const(string.c_str());
        }

        bool effectContainsProposition(const ActionPtr action, const std::vector<std::string> args,
                                       std::string prop)
        {
            return false;
        }
        const ActionPtr getActionFromString(std::string string)
        {
            std::string str = string.substr(string.find("ACTION:") + 6, string.find("ARGS:"));
            return problem_->domain->actions[str];
        }

        const PredicatePtr getPredicateFromString(std::string string)
        {
            std::string str = string.substr(string.find("PREDICATE:") + 9, string.find("ARGS:"));
            return problem_->domain->predicates[str];
        }

        /* const std::vector<std::string> getArgsFromString(std::string string) */
        /* { */
        /*   std::string str = string.substr(string.find("ARGS:") + 4, string.find("TIME:")); */
        /*   std::istringstream iss(str); */
        /*   return std::vector<std::string>(); */
        /* } */

        const std::string addTimeToString(std::string string, Time time)
        {
            return string + "TIME:" + std::to_string(time);
        }

        std::string createGroundedPredicateString(const PredicatePtr predicate,
                                                  const std::vector<std::string> &args_to_pred,
                                                  const Time step)
        {
            std::string str;
            for (auto const &s : args_to_pred)
                str += s;
            return "PRED:" + std::string(predicate->name) + "ARGS:" + str + "TIME:" + std::to_string(step);
        }

        std::string createGroundedActionString(const ActionPtr action,
                                               const std::vector<std::string> &args_to_action,
                                               const Time step)
        {
            std::string str;
            for (auto const &s : args_to_action)
                str += s;
            return "ACTION:" + std::string(action->name) + "ARGS:" + str + "TIME:" + std::to_string(step);
        }

                std::string createGroundedPredicateString(const BoundPredicatePtr predicate,
                                                  const std::vector<std::string> &args_to_pred,
                                                          const Time step) {
                  return "";
                }

        /**
         * For frame axioms, we need to know all actions that could change a proposition
         * the time step is when the action is executed, so the change will be at the next time step
         */
        z3::expr getPossibleChangeActions(std::string proposition, const Time step_before)
        {
            z3::expr change_actions = ctx_->bool_val(false);
            for (const auto name_actionptr : problem_->domain->actions)
            {
                std::set<std::vector<std::string>> nary_arg_assignment_list = {};
                for (const auto args_to_action : nary_arg_assignment_list)
                {
                    if (effectContainsProposition(name_actionptr.second, args_to_action, proposition))
                    {
                        change_actions = change_actions or
                                         makeBool(createGroundedActionString(name_actionptr.second,
                                                                             args_to_action, step_before));
                    }
                    else
                    {
                        std::cout << log::format("Action %1% does not contain prop. %2%.",
                                                 name_actionptr.first, proposition)
                                  << std::endl;
                    }
                }
            }
            return change_actions;
        }

        /**
         * Recurses down the tree and builds a z3::expr for the expression, replacing atoms with their
         * z3 bool_constants. We require that the expression does not contain any actions (all terminals
         * are state props).
         */
        z3::expr exprAtTime(const ExpressionPtr &expr, std::vector<std::string> args_list, const Time step)
        {
            switch (expr->type())
            {
                case Expression::Type::PREDICATE:
                {
                  //TODO this should be a bound predicate
                    const auto &predicate_expr = std::static_pointer_cast<BoundPredicate>(expr);
                    return makeBool(createGroundedPredicateString(predicate_expr, args_list, step));
                }
                case Expression::Type::AND:
                {
                    const auto &and_expr = std::static_pointer_cast<And>(expr);
                    z3::expr car = exprAtTime(makeCAR(and_expr), args_list, step);
                    auto cdr_expr = makeCDR(and_expr);
                    if (cdr_expr)
                    {
                        z3::expr cdr = exprAtTime(cdr_expr, args_list, step);
                        z3::expr clause = car and cdr;
                        return clause;
                    }
                    else
                        return car;
                }
                case Expression::Type::OR:
                {
                    const auto &or_expr = std::static_pointer_cast<Or>(expr);
                    z3::expr car = exprAtTime(or_expr->getTerms().back(), args_list, step);
                    auto cdr_expr = makeCDR(or_expr);
                    if (or_expr)
                    {
                        z3::expr cdr = exprAtTime(cdr_expr, args_list, step);
                        z3::expr clause = car or cdr;
                        return clause;
                    }
                    else
                        return car;
                }
                case Expression::Type::NOT:
                {
                    const auto &not_expr = std::static_pointer_cast<Not>(expr);
                    z3::expr exp = exprAtTime(not_expr->getTerm(), args_list, step);
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

        // We need to ground predicates for each timestep and each applicable set of object:
        void groundPredicates()
        {
            for (Time step = 0; step < horizon_; ++step)
            {
                for (const auto name_predptr : problem_->domain->predicates)
                {
                    std::set<std::vector<std::string>> nary_object_list = {};
                    for (const auto args_to_pred : nary_object_list)
                    {
                        // create a predicate:
                        // predicate name, arg0, arg1, ..., timestep
                        std::string pred_string =
                            createGroundedPredicateString(name_predptr.second, args_to_pred, step);
                        makeBool(pred_string);
                        if (step == 0)
                        {
                            grounded_predicates_.insert(pred_string.substr(0, pred_string.find(":TIME")));
                        }
                    }
                }
            }
        }

        // We need to ground predicates for each timestep and each applicable set of objects:
        void groundActions()
        {
            // stop at the second to last time step
            for (Time step = 0; step < horizon_ - 1; ++step)
            {
                for (const auto name_actionptr : problem_->domain->actions)
                {
                    std::set<std::vector<std::string>> nary_arg_assignment_list = {};
                    for (const auto args_to_action : nary_arg_assignment_list)
                    {
                        // create an action:
                        // actions have variable names and assignments
                        // action name, arg0, arg1, ..., timestep
                        std::string action_string =
                            createGroundedActionString(name_actionptr.second, args_to_action, step);
                        makeBool(action_string);
                        if (step == 0)
                        {
                            grounded_actions_.insert(action_string.substr(0, action_string.find(":TIME")));
                        }
                    }
                }
            }
        }

        void build()
        {
            // First we build the context, grounding all actions and predicates.
            ctx_ = new z3::context;
            groundPredicates();
            groundActions();
            solver_ = new z3::solver(*ctx_);

            // Now that we have built the context, we need to add constraints. These are clauses that ensure:
            // (1) an action implies its precondition is true and its effects will be true next time step
            // (2) if a proposition changes, then an action must have changed it
            // (3) only one action is taken at each time step
            for (Time step = 0; step < horizon_; ++step)
            {
                // add clauses for preconditions and effects of actions
                for (const auto name_actionptr : problem_->domain->actions)
                {
                    std::set<std::vector<std::string>> nary_arg_assignment_list = {};
                    for (const auto args_to_action : nary_arg_assignment_list)
                    {
                        // create an action:
                        // actions have variable names and assignments
                        // action name, arg0, arg1, ..., timestep
                        std::string action_string =
                            createGroundedActionString(name_actionptr.second, args_to_action, step);
                        z3::expr act = makeBool(action_string);
                        z3::expr precond = exprAtTime(name_actionptr.second->precondition, args_to_action, step);
                        z3::expr effects = exprAtTime(name_actionptr.second->effect, args_to_action, step);

                        /* z3::expr precond = makeBool(precondition_string); */
                        /* z3::expr effects = makeBool(effect_string); */
                        /* z3::expr act = makeBool(action_string); */

                        // action -> preconditions_t0 and effects_t1
                        z3::expr clause = (not act) or (precond and effects);
                        solver_->add(clause);
                    }
                }

                // add frame axioms
                for (const auto &predicate : grounded_predicates_)
                {
                    z3::expr curr_prop = makeBool(addTimeToString(predicate, step));
                    z3::expr next_prop = makeBool(addTimeToString(predicate, step + 1));

                    // if the prop changed, then an action changed it. it doesn't matter what the value
                    // changes to, because the clauses for precoditions and effects will ensure
                    // consistency
                    z3::expr clause =
                        not(curr_prop != next_prop) or (getPossibleChangeActions("prop_string", step));
                    solver_->add(clause);
                }

                // add mutual exclusion axioms, i.e., we only take one action each time step
                for (const auto &action : grounded_actions_)
                {
                    for (const auto &other_action : grounded_actions_)
                    {
                        if (action != other_action)
                        {  // TODO: make sure this comparison works correctly
                            z3::expr e = makeBool(addTimeToString(action, step));
                            z3::expr other_e = makeBool(addTimeToString(other_action, step));
                            z3::expr clause = (not e) or (not other_e);
                            solver_->add(clause);
                        }
                    }
                }

                // Add the goal
                solver_->add(exprAtTime(problem_->goal, {""}, horizon_));

                // Add the start state
                for (const auto &assgn : problem_->start->assignments)
                {
                  z3::expr p = makeBool(createGroundedPredicateString(problem_->domain->predicates[assgn.first.first], assgn.first.second, 0));
                    solver_->add(z3::expr((assgn.second) ? p : not p));
                }
            }
        }
    };

    }  // namespace pddlboat

#endif  // include guard
