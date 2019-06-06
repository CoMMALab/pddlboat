/* Author: Zachary Kingston */

#ifndef PDDLBOAT_SOLVER_PLANNER_
#define PDDLBOAT_SOLVER_PLANNER_

#include <pddlboat/macros.h>
#include <pddlboat/solver/domain.h>

namespace pddlboat
{
    using Time = std::size_t;

    PDDLBOAT_CLASS_FORWARD(Plan)
    class Plan
    {
    public:
        struct Step
        {
            ActionPtr action;
            Assignment assignment;
            StatePtr state;

            Step(const ActionPtr &action, const Assignment &assignment, const StatePtr &state);
            std::ostream &toString(std::ostream &out) const;
        };

        Plan(const ProblemPtr &problem);

        void addStep(const std::string &action_name, const std::vector<std::string> &objects);
        std::ostream &toString(std::ostream &out) const;

        std::vector<Step> steps;

    private:
        const ProblemPtr problem_;
    };

    std::ostream &operator<<(std::ostream &out, const Plan::Step &step);
    std::ostream &operator<<(std::ostream &out, const Plan &plan);

    PDDLBOAT_CLASS_FORWARD(Planner)
    class Planner
    {
    public:
        Planner(const ProblemPtr &problem);

        virtual bool solve(Plan &plan) = 0;

    protected:
        const ProblemPtr problem_;
    };

}  // namespace pddlboat

#endif  // include guard
