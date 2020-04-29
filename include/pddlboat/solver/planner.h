/* Author: Zachary Kingston */

#ifndef PDDLBOAT_SOLVER_PLANNER_
#define PDDLBOAT_SOLVER_PLANNER_

#include <pddlboat/macros.h>
#include <pddlboat/solver/domain.h>
#include <string>
// #include <iostream>
#include <sstream>

namespace pddlboat
{
    /** \cond IGNORE */
    PDDLBOAT_CLASS_FORWARD(Plan)
    /** \endcond */

    /** \class pddlboat::PlanPtr
        \brief A shared pointer wrapper for pddlboat::Plan. */

    /** \class pddlboat::PlanConstPtr
        \brief A const shared pointer wrapper for pddlboat::Plan. */

    /** \brief A class that stores the step of a plan to solve a problem.
     */
    class Plan
    {
    public:
        /** \brief A step within a plan.
         */
        struct Step
        {
            ActionPtr action;       ///< Action taken in this step.
            Assignment assignment;  ///< Assignment to this action for this step.
            StatePtr state;         ///< State of the world after this action.

            /** \brief Constructor.
             *  \param[in] action Action to take.
             *  \param[in] assignment Assignment to action.
             *  \param[in] state State of the world after this step.
             */
            Step(const ActionPtr &action, const Assignment &assignment, const StatePtr &state);

            /** \brief Serialize this step.
             *  \param[in,out] Stream to output this step to.
             *  \return The stream.
             */
            std::ostream &toString(std::ostream &out) const;

            /**
             * To allow us to build sets of Steps.
             */
            bool operator<(const Step &step_obj) const
            {
                std::ostringstream myObjectStream;
                toString(myObjectStream);
                std::string mySerializedObject = myObjectStream.str();

                std::ostringstream myObjectStream2;
                step_obj.toString(myObjectStream2);
                std::string mySerializedObject2 = myObjectStream2.str();

                return mySerializedObject < mySerializedObject2;
            }
        };

        /** \brief Constructor.
         *  \param[in] problem The problem this plan is for.
         */
        Plan(const ProblemPtr &problem);

        /** \brief Add a step to this plan.
         *  \param[in] action_name Name of action to take.
         *  \param[in] objects Arguments to the action. Must be a valid assignment.
         */
        void addStep(const std::string &action_name, const std::vector<std::string> &objects);

        /** \brief Serialize this plan.
         *  \param[in,out] Stream to output this plan to.
         *  \return The stream.
         */
        std::ostream &toString(std::ostream &out) const;

        std::vector<Step> steps;  ///< The steps of the plan.

    private:
        const ProblemPtr problem_;  ///< Problem this plan is for.
    };

    /** \brief Output stream operator for a step in the plan.
     *  \param[in, out] out Output stream to use.
     *  \param[in] step Step to output.
     *  \return The stream.
     */
    std::ostream &operator<<(std::ostream &out, const Plan::Step &step);

    /** \brief Output stream operator for a plan.
     *  \param[in, out] out Output stream to use.
     *  \param[in] plan Plan to output.
     *  \return The stream.
     */
    std::ostream &operator<<(std::ostream &out, const Plan &plan);

    /** \cond IGNORE */
    PDDLBOAT_CLASS_FORWARD(Planner)
    /** \endcond */

    /** \class pddlboat::PlannerPtr
        \brief A shared pointer wrapper for pddlboat::Planner. */

    /** \class pddlboat::PlannerConstPtr
        \brief A const shared pointer wrapper for pddlboat::Planner. */

    /** \brief An abstract planner that can solve planning problems.
     */
    class Planner
    {
    public:
        /** \brief Constructor.
         *  \param[in] problem Problem to solve.
         */
        Planner(const ProblemPtr &problem);

        /** \brief Solve the planning problem.
         *  \param[out] plan Found plan.
         *  \return True on success, false on failure.
         */
        virtual bool solve(Plan &plan) = 0;

    protected:
        const ProblemPtr problem_;  ///< Problem to solve.
    };
}  // namespace pddlboat

#endif  // include guard
