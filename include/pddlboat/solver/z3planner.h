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
        // We store a set of grounded actions and predicates (without the timestep)
        std::set<std::string> grounded_actions_;
        std::set<std::string> grounded_predicates_;
        const Time horizon_;

        z3::context *ctx_{nullptr};
        z3::solver *solver_{nullptr};

    public:
        Z3Planner(const ProblemPtr problem, const Time horizon) : Planner(problem), horizon_(horizon)
        {
            build();
        }

        std::vector<std::string> getVectorOfArgs(std::string string)
        {

            std::string str =
                string.substr(string.find("ARGS:") + 5, string.find("TIME:") - (string.find("ARGS:") + 5));
            std::vector<std::string> vec;

            std::string s = "";
            for (size_t i = 0; i < str.length(); i++)
            {
              std::string new_c = std::string(1, str.at(i));
              if(new_c != ",")
                s += new_c;
              else {
                vec.push_back(s);
                s = "";
              }
            }
            vec.push_back(s);

            return vec;
        }

        void buildPlan(Plan &my_plan)
        {
            auto m = solver_->get_model();
            for (size_t i = 0; i < m.num_consts(); i++)
            {
                z3::expr f = m.get_const_interp(m.get_const_decl(i));

                if (f.is_true())
                {
                    std::ostringstream oss;
                    oss << m.get_const_decl(i);
                    std::string complete_act_string = oss.str();
                    if (complete_act_string.find("ACTION:") != std::string::npos)
                    {
                        std::string act_string = complete_act_string.substr(
                            complete_act_string.find("ACTION:") + 7,
                            complete_act_string.find("ARGS:") - (complete_act_string.find("ACTION:") + 7));
                        std::vector<std::string> args_strings = getVectorOfArgs(complete_act_string);
                        my_plan.addStep(act_string, args_strings);
                    }
                }
            }
        }

        bool solve(Plan &plan) override
        {
            if (not solver_)
                return false;

            // Solve the problem
            // std::cout << "solver: " << std::endl;
            // std::cout << *solver_ << "\n";
            // std::cout << "solver to smt2" << std::endl;
            // std::cout << solver_->to_smt2() << "\n";

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
        bool isValidStringForZ3(const std::string &s)
        {
            if (s.find("ARGS:") == std::string::npos)
            {  // At least one ARGS:
                return false;
            }
            if (s.find("TIME:") == std::string::npos)
            {  // At least one TIME:
                return false;
            }
            if (s.find("ACTION:") == std::string::npos && s.find("PRED:") == std::string::npos)
            {  // At least one ACTION: or PRED:
                return false;
            }
            if (s.substr(s.find("ARGS:") + 5).find("ARGS:") != std::string::npos)
            {  // At most one ARGS:
                return false;
            }
            if (s.substr(s.find("TIME:") + 6).find("TIME:") != std::string::npos)
            {  // At most one ARGS:
                return false;
            }
            if (s.find("ACTION:") != std::string::npos && s.find("PRED:") != std::string::npos)
            {  // not (ACTION: and PRED:)
                return false;
            }
            if (s.substr(s.find("ACTION:") + 7).find("ACTION:") != std::string::npos)
            {  // At most one ACTION:
                return false;
            }
            if (s.substr(s.find("PRED:") + 6).find("PRED:") != std::string::npos)
            {  // At most one PRED:
                return false;
            }
            return true;
        }

        z3::expr makeBool(const std::string &string)  // Should be renamed. Only makes new if doesn't exist.
        {
            if (!isValidStringForZ3(string))
            {
                std::cout << "We have a malformed name for a Z3 variable: " << string << std::endl;
                throw std::runtime_error("whoops!");
            }
            return ctx_->bool_const(string.c_str());
        }

        const std::string addTimeToString(std::string string, Time time)
        {
            return string + "TIME:" + std::to_string(time);
        }

        std::unordered_map<std::string, std::string> createMapping(std::vector<std::string> v1,
                                                                   std::vector<std::string> v2)
        {
            std::unordered_map<std::string, std::string> my_map;
            assert(v1.size() + 1 == v2.size());
            for (size_t i = 0; i < v1.size(); i++)
            {
                my_map[v1[i]] = v2[i + 1];
            }
            return my_map;
        }

        // Map of names <from, to>
        std::string createGroundedPredicateString(
            const BoundPredicatePtr predicate, const std::unordered_map<std::string, std::string> &args_map,
            const Time step)
        {
            std::string str;

            for (size_t i = 0; i < predicate->bindings.size(); i++)
            {
                if (args_map.find(predicate->bindings[i].name) != args_map.end())
                {
                    auto s = args_map.find(predicate->bindings[i].name);
                    str += s->second + ",";
                }
                else
                {
                    std::cout << "We couldn't find variable " << predicate->bindings[i].name << std::endl;
                }
            }

            str = str.substr(0, str.length()-1);
            return "PRED:" + std::string(predicate->getPredicateName()) + "ARGS:" + str +
                   "TIME:" + std::to_string(step);
        }

        std::string createGroundedPredicateString(const PredicatePtr predicate,
                                                  const std::vector<std::string> &args_to_pred,
                                                  const Time step)
        {
            std::string str;
            for (auto const &s : args_to_pred) {
              if(s != "") {
                str += s + ",";
              }
            }

            str = str.substr(0, str.length()-1);
            return "PRED:" + std::string(predicate->name) + "ARGS:" + str + "TIME:" + std::to_string(step);
        }

        std::string createGroundedActionString(const ActionPtr action,
                                               const std::vector<std::string> &args_to_action,
                                               const Time step)
        {
            std::string str;
            for (auto const &s : args_to_action) {
              if(s != "") {
                str += s + ",";
              }
            }

            str = str.substr(0, str.length()-1);
            return "ACTION:" + std::string(action->name) + "ARGS:" + str + "TIME:" + std::to_string(step);
        }

        // TODO: How should this differ from the function for non-bound predicates
        std::string createGroundedPredicateString(const BoundPredicatePtr predicate,
                                                  const std::vector<std::string> &args_to_pred,
                                                  const Time step)
        {
            std::string str;
            for (auto const &s : args_to_pred) {
              if(s != "") {
                str += s + ",";
              }
            }

            str = str.substr(0, str.length()-1);
            return "PRED:" + std::string(predicate->getPredicateName()) + "ARGS:" + str +
                   "TIME:" + std::to_string(step);
        }

        // TODO: How should this differ from the function for non-bound predicates
        std::string createGroundedPredicateString(const BoundPredicatePtr predicate, const Time step)
        {
            std::string str;
            for (auto const &val : predicate->bindings)
            {
                str += val.name + ",";
            }
            str = str.substr(0, str.length()-1);
            return "PRED:" + std::string(predicate->getPredicateName()) + "ARGS:" + str +
                   "TIME:" + std::to_string(step);
        }

        // TODO version that does type checking
        std::vector<std::vector<std::string>> getNAryArgList(const VariableList parameters,
                                                             size_t start_mod = 0)
        {
            std::vector<std::vector<std::string>> list = {};

            size_t num_args = parameters.size() - start_mod;
            if (num_args == 0)
            {
                return list = {{""}};
            }
            else
            {
                std::vector<std::vector<std::string>> list_of_list_of_names =
                    getNAryArgList(parameters, start_mod + 1);
                for (auto l : list_of_list_of_names)
                {
                    for (auto obj : problem_->objects)
                    {
                        std::vector<std::string> new_list_of_names(l);
                        new_list_of_names.emplace_back(obj->name);
                        list.emplace_back(new_list_of_names);
                    }
                }
                return list;
            }
        }

        std::string substituteString(std::string main_string, std::string find, std::string replace,
                                     size_t start_index = 0,
                                     size_t end_index = std::numeric_limits<size_t>::max())
        {
            std::string str = main_string;
            size_t curr_index = start_index;
            end_index = end_index == std::numeric_limits<size_t>::max() ? str.length() : end_index;
            size_t i = 0;
            while ((i = str.find(find, curr_index)) != std::string::npos)
            {
                if (i >= end_index)
                {
                    break;
                }
                std::string s1 = str.substr(0, i);
                std::string s2 = str.substr(i + find.length());
                str = s1 + replace + s2;
                curr_index = i + replace.length();
            }
            return str;
        }

        /**
         * Recurses down the tree and builds a z3::expr for the expression, replacing atoms with their
         * z3 bool_constants. We require that the expression does not contain any actions (all terminals
         * are state props).
         */
      z3::expr exprAtTime(const ExpressionPtr &expr, const Assignment &assignment,
                            const Time step)
        {
            switch (expr->type())
            {
                case Expression::Type::PREDICATE:
                {
                    const auto &predicate_expr = std::static_pointer_cast<BoundPredicate>(expr);
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

        std::vector<std::string> getPredicateStrings(const ExpressionPtr &expr, const Time step)
        {
            switch (expr->type())
            {
                case Expression::Type::PREDICATE:
                {
                    const auto &predicate_expr = std::static_pointer_cast<BoundPredicate>(expr);
                    return {createGroundedPredicateString(predicate_expr, step)};
                }
                case Expression::Type::AND:
                {
                    const auto &and_expr = std::static_pointer_cast<And>(expr);
                    std::vector<std::string> car = getPredicateStrings(makeCAR(and_expr), step);
                    auto cdr_expr = makeCDR(and_expr);
                    if (cdr_expr)
                    {
                        std::vector<std::string> cdr = getPredicateStrings(cdr_expr, step);
                        car.insert(car.end(), cdr.begin(), cdr.end());
                        return car;
                    }
                    else
                        return car;
                }
                case Expression::Type::OR:
                {
                    const auto &or_expr = std::static_pointer_cast<Or>(expr);
                    std::vector<std::string> car = getPredicateStrings(or_expr->getTerms().back(), step);
                    auto cdr_expr = makeCDR(or_expr);
                    if (or_expr)
                    {
                        std::vector<std::string> cdr = getPredicateStrings(cdr_expr, step);
                        car.insert(car.end(), cdr.begin(), cdr.end());
                        return car;
                    }
                    else
                        return car;
                }
                case Expression::Type::NOT:
                {
                    const auto &not_expr = std::static_pointer_cast<Not>(expr);
                    return getPredicateStrings(not_expr->getTerm(), step);
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

        std::vector<std::string> getEffectStringsFromGroundedAction(
            const ActionPtr &action, const std::vector<std::string> &action_args, const Time step_before)
        {
            std::vector<std::string> vec_str = getPredicateStrings(action->effect, step_before + 1);
            std::vector<Variable> var_list = action->parameters;

            // We have an empty string as the base case for the arg list, so we add one.
            if (var_list.size() + 1 != action_args.size())
            {
                std::cout << "Arg list size mismatch: " << var_list.size() << " vs " << action_args.size()
                          << std::endl;
                std::cout << var_list[0].name << std::endl;
                std::cout << action_args[0] << std::endl;
                std::cout << action_args[1] << std::endl;

                std::cout << action->name << std::endl;
                throw std::runtime_error("whoops!");
            }

            for (size_t i = 0; i < vec_str.size(); i++)
            {
                assert(action_args[0].length() == 0);  // The empty string should always be the first one
                for (size_t j = 0; j < var_list.size(); j++)
                {
                    // Don't do anything for the empty string
                    if (action_args[j + 1].length() > 0)
                    {
                        vec_str[i] = substituteString(vec_str[i], var_list[j].name, action_args[j + 1],
                                                      vec_str[i].find("ARGS:") + 4, vec_str[i].find("TIME:"));
                    }
                }
            }

            return vec_str;
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
                std::vector<std::vector<std::string>> nary_arg_assignment_list =
                    getNAryArgList(name_actionptr.second->parameters);
                for (const auto args_to_action : nary_arg_assignment_list)
                {
                    std::vector<std::string> effect_strs = getEffectStringsFromGroundedAction(
                        name_actionptr.second, args_to_action, step_before);
                    if (std::find(effect_strs.begin(), effect_strs.end(),
                                  addTimeToString(proposition, step_before + 1)) != effect_strs.end())
                    {
                        change_actions = change_actions or
                                         makeBool(createGroundedActionString(name_actionptr.second,
                                                                             args_to_action, step_before));
                    }
                }
            }
            return change_actions;
        }

        // We need to ground predicates for each timestep and each applicable set of object:
        void groundPredicates()
        {
            for (Time step = 0; step <= horizon_; ++step)
            {
                for (const auto name_predptr : problem_->domain->predicates)
                {
                    std::vector<std::vector<std::string>> nary_object_list =
                        getNAryArgList(name_predptr.second->variables);
                    for (const auto args_to_pred : nary_object_list)
                    {
                        // create a predicate:
                        // predicate name, arg0, arg1, ..., timestep
                        std::string pred_string =
                            createGroundedPredicateString(name_predptr.second, args_to_pred, step);
                        makeBool(pred_string);
                        if (step == 0)
                        {
                            grounded_predicates_.insert(pred_string.substr(0, pred_string.find("TIME:")));
                        }
                    }
                }
            }
        }

        // We need to ground predicates for each timestep and each applicable set of objects:
        void groundActions()
        {
            // stop at the second to last time step
            for (Time step = 0; step < horizon_; ++step)
            {
                for (const auto name_actionptr : problem_->domain->actions)
                {
                    std::vector<std::vector<std::string>> nary_arg_assignment_list =
                        getNAryArgList(name_actionptr.second->parameters);
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
                            grounded_actions_.insert(action_string.substr(0, action_string.find("TIME:")));
                        }
                    }
                }
            }
        }

        void build()
        {
            // First we build the context, grounding all actions and predicates.
            ctx_ = new z3::context;
            solver_ = new z3::solver(*ctx_);

            groundPredicates();
            groundActions();

            // Now that we have built the context, we need to add constraints. These are clauses that ensure:
            // (1) an action implies its precondition is true and its effects will be true next time step
            // (2) if a proposition changes, then an action must have changed it
            // (3) only one action is taken at each time step
            for (Time step = 0; step < horizon_; ++step)
            {
                // add clauses for preconditions and effects of actions
                for (const auto &name_actionptr : problem_->domain->actions)
                {
                    const auto& assignments = problem_->getGroundings(name_actionptr.second->parameters);
                    for (const auto &assignment : assignments)
                    {
                        // std::unordered_map<std::string, std::string> arg_map;
                        // for (const auto &[var, object] : assignment.varToObj)
                        //     arg_map.emplace(var, object->name);

                        // create an action:
                        // actions have variable names and assignments
                        // action name, arg0, arg1, ..., timestep
                        // std::string action_string =
                        //     createGroundedActionString(name_actionptr.second, args_to_action, step);

                        std::string action_string = addTimeToString(name_actionptr.second->toID(assignment), step);
                        z3::expr act = makeBool(action_string);
                        z3::expr precond = exprAtTime(name_actionptr.second->precondition, assignment, step);
                        z3::expr effects = exprAtTime(name_actionptr.second->effect, assignment, step + 1);
                        // action -> preconditions_t0 and effects_t1
                        z3::expr clause = (not act) or (precond and effects);

                        solver_->add(clause);
                    }
                }

                // add frame axioms
                // if the prop changed, then an action changed it. it doesn't matter what the value
                // changes to, because the clauses for precoditions and effects will ensure
                // consistency
                for (const auto &predicate : grounded_predicates_)
                {
                    z3::expr curr_prop = makeBool(addTimeToString(predicate, step));
                    z3::expr next_prop = makeBool(addTimeToString(predicate, step + 1));
                    z3::expr clause =
                        not(curr_prop != next_prop) or (getPossibleChangeActions(predicate, step));
                    solver_->add(clause);
                }

                // add mutual exclusion axioms, i.e., we only take one action each time step
                for (const auto &action : grounded_actions_)
                {
                    for (const auto &other_action : grounded_actions_)
                    {
                        if (action != other_action)
                        {
                            z3::expr e = makeBool(addTimeToString(action, step));
                            z3::expr other_e = makeBool(addTimeToString(other_action, step));
                            z3::expr clause = (not e) or (not other_e);
                            solver_->add(clause);
                        }
                    }
                }
            }  // end for timestep

            // Add the goal
            Assignment empty_assgn(&*problem_);
            solver_->add(exprAtTime(problem_->goal, empty_assgn, horizon_));

            // Add the start state
            for (const auto name_predptr : problem_->domain->predicates)
            {
                std::vector<std::vector<std::string>> nary_arg_assignment_list =
                    getNAryArgList(name_predptr.second->variables);

                auto pred = *name_predptr.second;
                for (const auto arg_list : nary_arg_assignment_list)
                {
                    std::vector<std::string> new_arg_list;
                    for (auto s : arg_list)
                    {
                        if (s != "")
                        {
                            new_arg_list.push_back(s);
                        }
                    }

                    if (problem_->start->isTrue(pred.name, new_arg_list))
                    {
                        z3::expr p =
                            makeBool(createGroundedPredicateString(name_predptr.second, new_arg_list, 0));
                        solver_->add(z3::expr(p));
                    }
                    else
                    {
                        z3::expr p =
                            makeBool(createGroundedPredicateString(name_predptr.second, new_arg_list, 0));
                        solver_->add(z3::expr(not p));
                    }
                }
            }
        }
    };

}  // namespace pddlboat

#endif  // include guard
