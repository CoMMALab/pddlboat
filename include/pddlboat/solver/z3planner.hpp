/* Author: Andrew Wells and Zachary Kingston */

#ifndef PDDLBOAT_Z3PLANNER_
#define PDDLBOAT_Z3PLANNER_

#include <iostream>
#include <sstream>

#include <pddlboat/utility.hpp>
#include <pddlboat/parser/parser.hpp>
#include <pddlboat/solver/domain.hpp>
#include <pddlboat/solver/expressions.hpp>
#include <pddlboat/solver/planner.hpp>

namespace pddlboat
{
    /** \cond IGNORE */
    PDDLBOAT_CLASS_FORWARD(Z3PlannerInternal)
    /** \endcond */

    /** \brief Time step in a plan.
     */
    using Time = std::size_t;

    /** \brief A planner that uses Z3 to solve SAT plan representations of a planning problem.
     */
    class Z3Planner : public Planner
    {
    public:
        /** \brief Planner options.
         */
        struct Options
        {
            Time horizon_max{20};      ///< Maximum planning horizon.
            Time horizon_min{0};       ///< Minimum planning horizon for incremental solving.
            bool parallel{true};       ///< Parallel solve through Z3.
            bool diagnostics{false};   ///< Print diagnostics.
            bool dump_clauses{false};  ///< Dump generated Z3 clauses.
            bool incremental{true};    ///< Solve the problem incrementally.
            bool simultaneous{true};   ///< Allow simultaneous actions.
            bool optimize{true};       ///< If the problem has a metric, optimize.

            Options() = default;
        };

        /** \brief Constructor.
         *  \param[in] problem Problem to solve.
         *  \param[in] options Solver options.
         */
        Z3Planner(const ProblemPtr &problem, const Options &options);

        /** \brief Reset the planner.
         */
        void reset() override;

        /** \brief Returns the supported domain requirements.
         *  \return The supported requirements.
         */
        Requirements::Keyword getSupportedRequirements() const override;

        /** \brief Solve the planning problem.
         *  \param[out] plan Found plan.
         *  \return True on success, false on failure.
         */
        bool solve(Plan &plan) override;

        /** \brief Solve the planning problem.
         *  \param[out] plan Found plan.
         *  \return True on success, false on failure.
         */
        bool solve(Plan &plan, unsigned int timeout) override;

        /** \brief Block a set of state/action pairs from being in the plan.
         *  \param[in] actions Action and grounding pairs to prevent.
         *  \param[in] state The state the actions cannot be applied in.
         */
        void blockStateActions(const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
                               const StatePtr &state) override;

        /** \brief Block a set of state/action pairs from being in the plan simultaneously.
         *  \param[in] actions Action and grounding pairs to prevent.
         *  \param[in] state The state the actions cannot be applied in.
         */
        void blockStateActionsSimultaneous(
            const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
            const StatePtr &state) override;

        void
        blockExpressionActions(const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
                               const ExpressionPtr &expression, const Assignment &exp_assignment) override;

        void addConstraint(Planner::ConstraintTime time, const ExpressionPtr &expression,
                           bool value) override;

        /** \brief Print out the current solver model.
         *  \param[in,out] Output stream.
         *  \param[in] only_true Only print true assignments.
         *  \return The output stream.
         */
        std::ostream &printModel(std::ostream &out, bool only_true = true) const;

        /** \brief Print out the current solver model as a smt2 file.
         *  \param[in,out] Output stream.
         *  \return The output stream.
         */
        std::ostream &printSMT2(std::ostream &out) const;

    private:
        Z3PlannerInternalPtr internal_;
    };

}  // namespace pddlboat

#endif  // include guard
