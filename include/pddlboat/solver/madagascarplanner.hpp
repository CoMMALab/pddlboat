/* Madagascar-style SAT planner (Rintanen).
 *
 * Implements the planning-as-satisfiability approach of the Madagascar planner:
 *   - Purely propositional encoding (one boolean per ground atom / ground action per step).
 *   - ∃-step parallel semantics: actions may share a step if they can be executed in some
 *     linear order, enforced with disabling-graph SCC implication chains
 *     (Rintanen, Heljanko, Niemelä, "Planning as satisfiability: parallel plans and
 *     algorithms for plan search", AIJ 2006).
 *   - Invariant synthesis by iterative weakening from the initial state.
 *   - Algorithm B horizon scheduling: several horizons are solved concurrently with
 *     geometrically distributed effort; the first satisfiable horizon wins, so plans are
 *     not guaranteed to be makespan-optimal. Peak memory is roughly
 *     `window` × (size of the largest active encoding).
 *
 * Supports boolean STRIPS/ADL domains without conditional effects. Problems using numeric
 * or object fluents, domain axioms, derived predicates, or conditional effects (`when`)
 * are rejected at construction.
 */

#ifndef PDDLBOAT_MADAGASCARPLANNER_
#define PDDLBOAT_MADAGASCARPLANNER_

#include <pddlboat/utility.hpp>
#include <pddlboat/parser/parser.hpp>
#include <pddlboat/solver/domain.hpp>
#include <pddlboat/solver/expressions.hpp>
#include <pddlboat/solver/planner.hpp>

namespace pddlboat
{
    /** \cond IGNORE */
    PDDLBOAT_CLASS_FORWARD(MadagascarPlannerInternal)
    /** \endcond */

    /** \brief Time step in a plan.
     */
    using Time = std::size_t;

    /** \brief A planner using Madagascar-style planning as satisfiability with Z3 as the backend.
     */
    class MadagascarPlanner : public Planner
    {
    public:
        /** \brief Parallel plan semantics.
         */
        enum class Semantics
        {
            FORALL_STEP,  ///< Actions in a step must be non-interfering in every order.
            EXISTS_STEP   ///< Actions in a step must be executable in some order.
        };

        /** \brief Planner options.
         */
        struct Options
        {
            Time horizon_max{60};              ///< Maximum planning horizon.
            Time horizon_min{0};               ///< Minimum planning horizon.
            Time horizon_step{1};              ///< Gap between scheduled horizons.
            unsigned int window{8};            ///< Maximum concurrently active horizons (Algorithm B).
            double gamma{0.9};                 ///< Geometric effort rate for Algorithm B.
            unsigned int rlimit_base{500000};  ///< Z3 resource budget per round for the shortest horizon.
            Semantics semantics{Semantics::EXISTS_STEP};  ///< Parallel plan semantics.
            bool invariants{true};    ///< Synthesize and assert invariants.
            bool algorithm_b{true};   ///< Concurrent horizons; false solves horizons sequentially.
            bool parallel{false};     ///< Parallel solve through Z3.
            bool diagnostics{false};  ///< Print diagnostics.
            bool dump_clauses{false};  ///< Dump generated Z3 clauses.

            Options() = default;
        };

        /** \brief Constructor.
         *  \param[in] problem Problem to solve.
         *  \param[in] options Solver options.
         *  \throws std::runtime_error if the problem uses unsupported features.
         */
        MadagascarPlanner(const ProblemPtr &problem, const Options &options);

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
         *  \param[in] timeout Wall-clock budget in milliseconds (0 = none).
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

    private:
        MadagascarPlannerInternalPtr internal_;
    };

}  // namespace pddlboat

#endif  // include guard
