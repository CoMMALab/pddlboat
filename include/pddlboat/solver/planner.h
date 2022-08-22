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
    PDDLBOAT_CLASS_FORWARD(PlanDependencyGraph)
    /** \endcond */

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
        class Step
        {
        public:
            ActionPtr action;       ///< Action taken in this step.
            Assignment assignment;  ///< Assignment to this action for this step.
            StatePtr prior;         ///< State of the world before this action.
            StatePtr state;         ///< State of the world after this action.

            /** \brief Constructor.
             *  \param[in] action Action to take.
             *  \param[in] assignment Assignment to action.
             *  \param[in] state State of the world after this step.
             */
            Step(const ActionPtr &action, const Assignment &assignment);

            /** \brief Serialize this step.
             *  \param[in,out] Stream to output this step to.
             *  \return The stream.
             */
            std::ostream &toString(std::ostream &out) const;

            /** \brief Comparator. Allows us to build a seequence of steps.
             *  \param[in] step_obj Other step to compare against.
             *  \return If this object is "less than" the other (lexicographically).
             */
            bool operator<(const Step &step_obj) const;
        };

        /** \brief Constructor.
         *  \param[in] problem The problem this plan is for.
         */
        Plan(const ProblemPtr &problem);

        /** \brief Copy Constructor.
         *  \param[in] plan Plan to copy.
         */
        Plan(const Plan &plan);

        /** \name Adding steps to the plan.
            \{ */

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

        /** \brief Add a step to the end of an existing plan.
         *  \param[in] action Action to take.
         *  \param[in] assignment Assignment to the action.
         */
        void addStep(const ActionPtr &action, const Assignment &assignment);

        /** \brief Insert a step in the middle of an existing plan.
         *  \param[in] action Action to take.
         *  \param[in] assignment Assignment to the action.
         *  \param[in] index Desired index of insertion.
         */
        void insertStep(const ActionPtr &action, const Assignment &assignment, std::size_t index);

        /** \} */

        /** \name Retrieving steps from the plan
            \{ */

        /** \brief Get a step at an index.
         *  \param[in] index The index of the step to retrieve.
         *  \return The step at a specified index.
         */
        Step &getStep(std::size_t index);

        /** \brief Get a step at an index.
         *  \param[in] index The index of the step to retrieve.
         *  \return The step at a specified index.
         */
        const Step &getStepConst(std::size_t index) const;

        /** \brief Get the (flattened) steps of this plan.
         *  \return Vector of all plan steps.
         */
        std::vector<Step> getSteps() const;

        /** \brief Get the total makespan of the plan. The makespan is the minimum number of timesteps
         *  required to execute the plan, assuming that actions that can be executed in parallel (that is,
         *  they do not have any conflicting preconditions or effects).
         *  \return The makespan of the plan. Throws an error if the dependency graph has not been computed.
         */
        std::size_t getMakespan() const;

        /** \brief Get the set of plan steps that are \a makespan_depth from the start of the plan. That is,
         *  the set of steps that require at least \a makespan_depth prior steps that directly effect the
         *  preconditions of the action.
         *  \param[in] makespan_depth The makespan depth to get the set of steps for.
         *  \return A vector of indices to plan steps that can be retrieved with getStep().
         */
        const std::vector<Step> &getStepsAtMakespan(std::size_t makespan_depth) const;

        /** \brief Get the set of plan steps that are at each level of makespan depth (see
         *  getStepsAtMakespan()).
         *  \return The set of steps organized by makespan.
         */
        const std::vector<std::vector<Step>> &getMakespanSteps() const;

        /** \brief Get the number of steps in a plan.
         *  \return The number of steps in the plan.
         */
        std::size_t getNumSteps() const;

        /** \} */

        /** \brief Validates if this plan is correct. Also propagates internal action states if steps were
         * removed or added.
         * \return True if this is a feasible plan, false otherwise.
         */
        bool validate();

        /** \brief Serialize this plan.
         *  \param[in,out] Stream to output this plan to.
         *  \param[in] steps Output the makespan this action takes place
         *  \return The stream.
         */
        std::ostream &toString(std::ostream &out, bool steps = false) const;

        /** \brief Get the problem this plan is for.
         *  \return Problem.
         */
        const ProblemPtr &getProblem() const;

    private:
        std::pair<ActionPtr, Assignment> getAction(const std::string &action_name,
                                                   const std::vector<std::string> &objects) const;

        std::vector<std::vector<Step>> steps_;  ///< The steps of the plan.
        const ProblemPtr problem_;              ///< Problem this plan is for.
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

        /** \name Problem Solving
            \{ */

        /** \brief Solve the planning problem.
         *  \param[out] plan Found plan.
         *  \return True on success, false on failure.
         */
        virtual bool solve(Plan &plan) = 0;

        /** \brief Solve the planning problem.
         *  \param[out] plan Found plan.
         *  \param[in] timeout Planning time (in ms).
         *  \return True on success, false on failure.
         */
        virtual bool solve(Plan &plan, unsigned int timeout) = 0;

        /** \brief Reset the internal planning problem.
         */
        virtual void reset();

        /** \} */

        /** \name Adding State/Action Constraints
            \{ */

        /** \brief Block the action at a step of a plan given the state in the plan.
         *  \param[in] plan Plan to block the step from.
         *  \param[in] index Index of step to block.
         */
        void blockPlanStep(const Plan &plan, std::size_t index);

        /** \brief Block a grounded action from being used in a specific state.
         *  \param[in] action Action to prevent.
         *  \param[in] assignment Grounding of action.
         *  \param[in] state The state the action cannot be applied in.
         */
        void blockStateAction(const ActionPtr &action, const Assignment &assignment, const StatePtr &state);

        /** \brief Block a set of actions from being used in a specific state.
         *  \param[in] actions Action and grounding pairs to prevent.
         *  \param[in] state The state the actions cannot be applied in.
         */
        virtual void
        blockStateActions(const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
                          const StatePtr &state);

        void blockExpressionAction(const ActionPtr &action, const Assignment &act_assignment,
                                   const ExpressionPtr &expression, const Assignment &exp_assignment);

        virtual void
        blockExpressionActions(const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
                               const ExpressionPtr &expression, const Assignment &exp_assignment);

        enum ConstraintTime
        {
            AT_END = 1 << 0,   // Constraint applies only to the final state of the plan.
            OVER_ALL = 1 << 1  // Constraint applies to all states of the plan.
        };

        /** \brief Add a state constraint to the problem specification.
         *  \param[in] time Specification of what times the constraint applies.
         *  \param[in] expression Expression (fully grounded) that determines constraint.
         *  \param[in] value Value that the constraint should evaluate to.
         * */
        virtual void addConstraint(ConstraintTime time, const ExpressionPtr &expression, bool value = false);

        /** \brief Block a set of actions from being used *simultaneously* in a specific state.
         *  \param[in] actions Actions and grounding pairs to prevent from occurring simultaneously.
         *  \param[in] state The state the actions cannot occur together in.
         */
        virtual void blockStateActionsSimultaneous(
            const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
            const StatePtr &state);

        /** \} */

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
