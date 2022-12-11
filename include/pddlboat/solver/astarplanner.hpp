/* Author: Zachary Kingston */

#ifndef PDDLBOAT_SOLVER_ASTARPLANNER_
#define PDDLBOAT_SOLVER_ASTARPLANNER_

#include <queue>
#include <mutex>
#include <unordered_map>
#include <boost/functional/hash.hpp>

#include <pddlboat/utility.hpp>
#include <pddlboat/solver/planner.hpp>
#include <pddlboat/solver/fdplanner.hpp>

namespace pddlboat
{
    /** \brief Simple planner using PDDLboat constructs to perform A* search over the implicit graph of
     * symbolic states. This planner supports custom heuristics and distance functions, as well as searching
     * over the Cartesian product of the symbolic space and an auxiliary parameter space (ParameterSpace). */
    class AStarPlanner : public Planner
    {
    public:
        /** \brief Transition distance function.
         *  \param[in] node Source node.
         *  \param[in] action Transition action.
         *  \param[in] next Next state reached by action.
         *  \return Distance from node to next state.
         */
        using TransitionDistance = std::function<double(const StatePtr &current,       //
                                                        const GroundedAction &action,  //
                                                        const StatePtr &next)>;

        /** \brief Heuristic computation function.
         *  \param[in] node Node to compute heuristic for.
         *  \return Heuristic value.
         */
        using Heuristic = std::function<double(const StatePtr &current)>;

        /** \brief Uniform distance for all transitions.
         */
        static const TransitionDistance DEFAULT_TRANSITION_DISTANCE;

        /** \brief If `total-cost` is a numeric function, uses its value as the transition distance.
         */
        static const TransitionDistance TOTAL_COST_TRANSITION_DISTANCE;
        static const Heuristic NULL_HEURISTIC;  ///< Null heuristic, 0 for all nodes.

        /** \brief Planner options.
         */
        struct Options
        {
            /** \brief Constructor.
             */
            Options(){};

            bool verbose{false};  ///< Verbose output.

            TransitionDistance transition_distance;
            Heuristic heuristic;
        };

        /** \brief Constructor.
         *  \param[in] problem Problem to solve.
         *  \param[in] options Planner options.
         */
        AStarPlanner(const ProblemPtr &problem, const Options &options = Options());

        /** \brief Get the set of states that are adjacent to the starting state in the problem.
         *  \return The set of adjacent states (as steps, which include the action taken).
         */
        std::vector<Plan::Step> getStartNeighbors();

        /** \brief Take a random walk in the domain for \a steps.
         *  \param[in] steps Number of steps to randomly walk.
         *  \param[in] seed Random seed to set. If 0, will randomly seed.
         *  \param[in] weighted Weight randomness by cost of node (bias towards goal).
         *  \return The random sequence of steps visited.
         */
        std::vector<Plan::Step> randomWalk(std::size_t steps, std::size_t seed = 0, bool weighted = true);

        /** \brief Initialize planner.
         */
        void initialize();

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

        /** \brief Block an action from being used in a certain state.
         *  \param[in] actions Action and grounding pairs to prevent.
         *  \param[in] state The state the actions cannot be applied in.
         */
        void blockStateActions(const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
                               const StatePtr &state) override;

        /** \brief Generate a directed graph corresponding to the transition system defined by the problem.
         *  \param[in] filename Filename to save GraphML formatted file to.
         */
        void generateGraphML(const std::string &filename) const;

        /** \brief Returns the set of supported domain requirements the planner can solve.
         *  \return The set of supported requirements.
         */
        Requirements::Keyword getSupportedRequirements() const override;

        Options options;  ///< Options for computation.

    private:
        std::vector<GroundedAction> ground_actions_;          ///< Set of grounded actions.
        StateMap<std::vector<std::size_t>> blocked_actions_;  ///< Map of states to disallowed actions.
    };                                                        // namespace pddlboat
}  // namespace pddlboat

#endif
