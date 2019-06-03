#ifndef PDDLBOAT_PROBLEM_BUILDER_
#define PDDLBOAT_PROBLEM_BUILDER_

#include <z3++.h>

#include <pddlboat/utility.h>
#include <pddlboat/parser/parser.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/domain.h>

namespace pddlboat
{
    using Time = size_t;

    class Plan
    {
    public:
        struct Step
        {
            Step(const ActionPtr &action, const Assignment &assignment)
              : action(action), assignment(assignment)
            {
            }

            ActionPtr action;
            Assignment assignment;
        };

        Plan(const ProblemPtr &problem) : problem_(problem)
        {
        }

        void addStep(const std::string &action_name, const std::vector<std::string> &objects)
        {
            ActionPtr action = problem_->domain->getAction(action_name);
            if (not action)
                throw std::invalid_argument(log::format("Action %1% does not exist!", action_name));

            if (action->arity() != objects.size())
                throw std::invalid_argument(log::format("Action %1% has arity %2%, not %3%!", action_name,
                                                        action->arity(), objects.size()));

            Assignment assignment(problem_.get());
            for (unsigned int i = 0; i < objects.size(); ++i)
            {
                const auto &object_name = objects[i];
                ObjectPtr obj;
                if (not(obj = problem_->getObject(object_name)))
                {
                    if (not(obj = problem_->domain->getConstant(object_name)))
                    {
                        throw std::invalid_argument(
                            log::format("Cannot do action with object %1%, does not exist!", object_name));
                    }
                }

                if (action->parameters[i].type != obj->type)
                    throw std::invalid_argument(  //
                        log::format("Cannot do action with object %1%, is type %2%, not %3%!", object_name,
                                    obj->type, action->parameters[i].type));

                assignment.assign(action->parameters[i].name, objects[i]);
            }

            plan.emplace_back(action, assignment);
        }

        std::vector<Step> plan;

    private:
        const ProblemPtr problem_;
    };

    class Planner
    {
    public:
        /* Planner(const StatePtr &start, const ExpressionPtr &goal, const std::vector<ActionPtr> &actions, */
        /*         const Time horizon) */
        /*   : start_(start), goal_(goal), actions_(actions), horizon_(horizon) */
        /* { */
        /* } */

        Planner(const ProblemPtr &problem) : problem_(problem)
        {
        }

        virtual bool solve(Plan &plan) = 0;

    protected:
        /* const StatePtr start_; */
        /* const ExpressionPtr goal_; */
        /* const std::vector<ActionPtr> actions_; */
        const ProblemPtr problem_;
    };

    class FastDownwardPlanner : public Planner
    {
    public:
        FastDownwardPlanner(const ProblemPtr &problem)
          : Planner(problem)
          , uuid(io::generateUUID())
          , translator(io::resolvePath(log::format("%1%/translate/translate.py", directory)))
          , solver(io::resolvePath(log::format("%1%/downward", directory)))
          , domain_file(log::format(domain_file_template, uuid))
          , problem_file(log::format(problem_file_template, uuid))
          , sas_file(log::format(sas_file_template, uuid))
          , output_file(log::format(output_file_template, uuid))
          , output_file_1(output_file + ".1")
        {
            std::stringstream cs;
            cs << solver;

            // Output file
            cs << " --internal-plan-file " << output_file;

            // Evaluators
            cs << " --evaluator \"hcea=cea(transform=adapt_costs(plusone))\"";
            cs << " --evaluator \"hcg=cg(transform=adapt_costs(one))\"";
            cs << " --evaluator \"hgc=goalcount(transform=adapt_costs(plusone))\"";
            cs << " --evaluator \"hff=ff()\"";

            // Autotune Fast Downward Search
            cs << " --search ";
            cs << "\"";
            cs << "iterated([";
            cs << "ehc(hcea,preferred=[hcea],preferred_usage=0,cost_type=normal),";
            cs << "lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),";
            cs << "single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),";
            cs << "single(sum([weight(g(),2),weight(hcg,3)])),";
            cs << "single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),";
            cs << "single(sum([weight(g(),2),weight(hcea,3)])),";
            cs << "single(sum([weight(g(),2),weight(hcea,3)]),pref_only=true),";
            cs << "single(sum([weight(g(),2),weight(hgc,3)])),";
            cs << "single(sum([weight(g(),2),weight(hgc,3)]),pref_only=true)],";
            cs << "boost=200),";
            cs << "preferred=[hcea,hgc],reopen_closed=false,cost_type=one),";
            cs << "lazy(alt([single(sum([g(),weight(hff,5)])),";
            cs << "single(sum([g(),weight(hff,5)]),pref_only=true),";
            cs << "single(sum([g(),weight(hcg,5)])),";
            cs << "single(sum([g(),weight(hcg,5)]),pref_only=true),";
            cs << "single(sum([g(),weight(hcea,5)])),";
            cs << "single(sum([g(),weight(hcea,5)]),pref_only=true),";
            cs << "single(sum([g(),weight(hgc,5)])),";
            cs << "single(sum([g(),weight(hgc,5)]),pref_only=true)],";
            cs << "boost=5000),";
            cs << "preferred=[hcea,hgc],reopen_closed=true,cost_type=normal),";
            cs << "lazy(alt([single(sum([g(),weight(hff,2)])),";
            cs << "single(sum([g(),weight(hff,2)]),pref_only=true),";
            cs << "single(sum([g(),weight(hcg,2)])),";
            cs << "single(sum([g(),weight(hcg,2)]),pref_only=true),";
            cs << "single(sum([g(),weight(hcea,2)])),";
            cs << "single(sum([g(),weight(hcea,2)]),pref_only=true),";
            cs << "single(sum([g(),weight(hgc,2)])),";
            cs << "single(sum([g(),weight(hgc,2)]),pref_only=true)],";
            cs << "boost=1000),";
            cs << "preferred=[hcea,hgc],reopen_closed=true,cost_type=one)";
            cs << "],repeat_last=true,continue_on_fail=true)";
            cs << "\"";

            // Tell command what SAS file to use
            cs << " < " << sas_file;

            solver_command = cs.str();

            std::stringstream ts;
            ts << translator;
            ts << " --sas-file " << sas_file << " " << domain_file << " " << problem_file;
            translator_command = ts.str();

            update();
        }

        ~FastDownwardPlanner()
        {
            io::deleteFile(domain_file);
            io::deleteFile(problem_file);
            io::deleteFile(sas_file);
            io::deleteFile(output_file_1);
        }

        void update(bool domain = true, bool problem = true) const
        {
            // Serialize problem and domain

            if (domain)
            {
                std::ofstream domain_out;
                io::createFile(domain_out, domain_file);
                problem_->domain->toPDDL(domain_out);
                domain_out.close();
            }

            if (problem)
            {
                std::ofstream problem_out;
                io::createFile(problem_out, problem_file);
                problem_->toPDDL(problem_out);
                problem_out.close();
            }

            if (domain or problem)
            {
                // Call fast downward
                auto tr = io::runCommand(translator_command);
                if (not tr)
                    throw std::runtime_error("Failed to translate problem and domain!");
            }
        }

        bool solve(Plan &plan) override
        {
            auto pr = io::runCommand(solver_command);
            if (not pr)
                return false;

            // Parse output file
            ast::Plan parsed;
            parseFile(output_file_1, parsed);

            for (const auto &action : parsed.actions)
            {
                std::vector<std::string> objects;
                for (const auto &tl : action.variables)
                    for (const auto &t : tl.types)
                        objects.emplace_back(t);

                plan.addStep(action.name, objects);
            }

            return true;
        }

    private:
        const std::string directory{PDDLBOAT_FASTDOWNWARD_LOCATION};
        const std::string domain_file_template{"/tmp/pddlboat_domain_%1%.pddl"};
        const std::string problem_file_template{"/tmp/pddlboat_problem_%1%.pddl"};
        const std::string sas_file_template{"/tmp/pddlboat_sas_%1%"};
        const std::string output_file_template{"/tmp/pddlboat_out_%1%"};

        const std::string uuid;
        const std::string translator;
        const std::string solver;
        const std::string domain_file;
        const std::string problem_file;
        const std::string sas_file;
        const std::string output_file;
        const std::string output_file_1;

        std::string solver_command;
        std::string translator_command;
    };

    class Z3Planner : public Planner
    {
        // We store a set of grounded actions and predicates (without the timestep)
        std::set<std::string> grounded_actions_;
        std::set<std::string> grounded_predicates_;
        const Time horizon_;

        z3::context *ctx_{nullptr};
        z3::solver *solver_{nullptr};

    public:
        /* Z3Planner(const StatePtr &start, const ExpressionPtr &goal, const std::vector<ActionPtr> &actions,
         */
        /*           const Time horizon) : Planner(start, goal, actions, horizon) */
        /* { */
        /*     build(); */
        /* } */

        Z3Planner(const ProblemPtr problem, const Time horizon) : Planner(problem), horizon_(horizon)
        {
            build();
        }

        bool solve(Plan &plan) override
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
                                                  const Time step)
        {
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
                    // TODO this should be a bound predicate
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
                        z3::expr precond =
                            exprAtTime(name_actionptr.second->precondition, args_to_action, step);
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
                    z3::expr p = makeBool(createGroundedPredicateString(
                        problem_->domain->predicates[assgn.first.first], assgn.first.second, 0));
                    solver_->add(z3::expr((assgn.second) ? p : not p));
                }
            }
        }
    };

}  // namespace pddlboat

#endif  // include guard
