/* Author: Zachary Kingston */

#ifndef PDDLBOAT_SOLVER_PLANNER_
#define PDDLBOAT_SOLVER_PLANNER_

#include <pddlboat/macros.h>
#include <pddlboat/parser/keywords.h>
#include <pddlboat/solver/domain.h>
#include <string>
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

        /** \brief Copy Constructor.
         *  \param[in] plan Plan to copy.
         */
        Plan(const Plan &plan);

        /** \brief Add a step to this plan.
         *  \param[in] action_name Name of action to take.
         *  \param[in] objects Arguments to the action. Must be a valid assignment.
         */
        void addStep(const std::string &action_name, const std::vector<std::string> &objects);

        /** \brief Insert a step in the middle of an existing plan.
         *  \param[in] action_name Name of action to take.
         *  \param[in] objects Arguments to the action. Must be a valid assignment.
         *  \param[in] index Desired index of insertion.
         */
        void insertStep(const std::string &action_name, const std::vector<std::string> &objects,
                        std::size_t index);

        /** \brief Insert a step in the middle of an existing plan.
         *  \param[in] action Action to take.
         *  \param[in] assignment Assignment to the action.
         *  \param[in] index Desired index of insertion.
         */
        void insertStep(const ActionPtr &action, const Assignment &assignment, std::size_t index);

        /** \brief Removes step at \a index in the plan.
         *  \param[in] index Index of step to remove.
         */
        void removeStep(std::size_t index);

        /** \brief Validates if this plan is correct. Also propagates internal action states if steps were
         * removed or added.
         * \return True if this is a feasible plan, false otherwise.
         */
        bool validate();

        /** \brief Serialize this plan.
         *  \param[in,out] Stream to output this plan to.
         *  \return The stream.
         */
        std::ostream &toString(std::ostream &out) const;

        /** \brief Get the problem this plan is for.
         *  \return Problem.
         */
        const ProblemPtr &getProblem() const;

        /** \brief Get the steps of this plan.
         *  \return Vector of plan steps.
         */
        const std::vector<Step> &getSteps() const;

        /** \brief Get the number of steps in a plan.
         *  \return The number of steps in the plan.
         */
        std::size_t getNumSteps() const;

    private:
        StatePtr propagateAction(const ActionPtr &action, const Assignment &assignment,
                                 std::size_t index) const;

        std::pair<ActionPtr, Assignment> getAction(const std::string &action_name,
                                                   const std::vector<std::string> &objects) const;

        std::vector<Step> steps_;   ///< The steps of the plan.
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

        /** \brief Reset the internal planning problem.
         */
        virtual void reset();

        /** \brief Block the action at a step of a plan give the state in the plan.
         *  \param[in] plan Plan to block the step from.
         *  \param[in] index Index of step to block.
         */
        void blockPlanStep(const Plan &plan, std::size_t index);

        /** \brief Block a plan from being used.
         *  \param[in] action Action to prevent.
         *  \param[in] assignment Grounding of action.
         *  \param[in] state The state the action cannot be applied in.
         */
        virtual void blockStateAction(const ActionPtr &action, const Assignment &assignment, const StatePtr &state);

        /** \brief Returns the set of supported domain requirements the planner can solve.
         *  \return The set of supported requirements.
         */
        virtual Requirements::Keyword getSupportedRequirements() const = 0;

        /** \brief Checks if this planner supports the problem domain.
         *  \throws std::runtime_error if the planner does not support the planner.
         */
        void checkRequirements() const;

    protected:
        const ProblemPtr problem_;  ///< Problem to solve.
    };
}  // namespace pddlboat

#endif  // include guard
