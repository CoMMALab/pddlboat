#ifndef PDDLBOAT_PROBLEM_BUILDER_
#define PDDLBOAT_PROBLEM_BUILDER_

#include <pddlboat/utility.h>
#include <pddlboat/parser/parser.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/domain.h>

namespace pddlboat
{
    using Time = size_t;

    PDDLBOAT_CLASS_FORWARD(Plan)
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

            std::string toString() const
            {
                std::stringstream ss;
                ss << "(" << action->name;
                for (const auto &parameter : action->parameters)
                {
                    ss << " ";
                    ss << assignment.assigned(parameter.name)->name;
                }
                ss << ")";

                return ss.str();
            }
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

        std::string toString() const
        {
            std::stringstream ss;

            for (const auto &step : plan)
                ss << step.toString() << std::endl;

            return ss.str();
        }

        std::vector<Step> plan;

    private:
        const ProblemPtr problem_;
    };

    PDDLBOAT_CLASS_FORWARD(Planner)
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

}  // namespace pddlboat

#endif  // include guard
