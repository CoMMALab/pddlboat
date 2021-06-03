/* Author: Zachary Kingston */

#ifndef PDDLBOAT_SOLVER_ASTARPLANNER_
#define PDDLBOAT_SOLVER_ASTARPLANNER_

#include <queue>
#include <unordered_map>
#include <boost/functional/hash.hpp>

#include <pddlboat/utility.h>
#include <pddlboat/solver/planner.h>
#include <pddlboat/solver/fdplanner.h>

namespace pddlboat
{
    /** \brief Simple planner using PDDLboat constructs to perform A* search over the implicit graph of
     * symbolic states. This planner supports custom heuristics and distance functions, as well as searching
     * over the Cartesian product of the symbolic space and an auxiliary parameter space (ParameterSpace). */
    class AStarPlanner : public Planner
    {
    public:
        /** \cond IGNORE */
        PDDLBOAT_CLASS_FORWARD(ParameterSpace)
        /** \endcond */

        /** \class pddlboat::AStarPlanner::ParameterSpacePtr
            \brief A shared pointer wrapper for pddlboat::AStarPlanner::ParameterSpace. */

        /** \class pddlboat::AStarPlanner::ParameterSpaceConstPtr
            \brief A const shared pointer wrapper for pddlboat::AStarPlanner::ParameterSpace. */

        /** \brief Abstract base class of additional state data that can be accessed by the A* planner. To
         * implement a custom parameter space, inherit from this class and implement the `getNeighbors()`
         * method that returns, given a symbolic transition, the set of valid parameters (which are instances
         * of a class that inherits from ParameterSpace::Params). Note that you should also implement a new
         * transition distance function that takes into account your parameters.
         */
        class ParameterSpace
        {
        public:
            /** \cond IGNORE */
            PDDLBOAT_CLASS_FORWARD(Params)
            /** \endcond */

            /** \class pddlboat::AStarPlanner::ParameterSpace::ParamsPtr
                \brief A shared pointer wrapper for pddlboat::AStarPlanner::ParameterSpace::Params. */

            /** \class pddlboat::AStarPlanner::ParameterSpace::ParamsConstPtr
                \brief A const shared pointer wrapper for pddlboat::AStarPlanner::ParameterSpace::Params. */

            /** \brief Parameters from the parameter space.
             */
            class Params
            {
            public:
                /** \brief Comparator. Returns true if these are equal parameters.
                 *  \param[in] other Other parameter to compare against.
                 *  \return True if these are equivalent parameters.
                 */
                virtual bool isEqual(const Params &other) const;
            };

            /** \brief Get the neighboring parameters given a symbolic transition.
             *  \param[in] state The source state.
             *  \param[in] next The destination state.
             *  \param[in] action The transitioning action.
             *  \return The set of parameters for a composite state with next.
             */
            virtual std::vector<ParamsPtr> getNeighbors(const StatePtr &state, const StatePtr &next,
                                                        const GroundedAction &action);
        };

        /** \brief Data structures used in A* search.
         */
        class PlanData
        {
        public:
            /** \brief Node in the A* search tree.
             */
            class Node
            {
            public:
                /** \brief Key in the unordered node map.
                 */
                struct Key
                {
                    StatePtr state;                    ///< Symbolic state.
                    ParameterSpace::ParamsPtr params;  ///< State parameters.

                    /** \brief Constructor.
                     *  \param[in] node Node to key on.
                     */
                    Key(const Node *node);

                    /** \brief Constructor.
                     *  \param[in] state State to key on.
                     *  \param[in] params Parameters to key on.
                     */
                    Key(const StatePtr &state, const ParameterSpace::ParamsPtr &params);

                    /** \brief Equality operator. Compares state and parameters.
                     *  \param[in] other Key to compare against.
                     */
                    bool operator==(const Key &other) const;
                };

                StatePtr state;                         ///< Symbolic state.
                ParameterSpace::ParamsPtr params;       ///< State parameters.
                const Node *prior{nullptr};             ///< Prior node in search.
                const GroundedAction *action{nullptr};  ///< Action that transitioned into this state.
                bool goal_checked{false};  ///< Has this state already been evaluated for goal satisfaction?
                std::size_t distance{0};   ///< Distance (in terms of node hops) from start.
                double cost{0};            ///< Cost of node so far.
                double heuristic{0};       ///< Heuristic value of node.

                /** \brief Constructor.
                 *  \param[in] state Symbolic state.
                 *  \param[in] params State parameters.
                 */
                Node(const StatePtr &state, const ParameterSpace::ParamsPtr &params);
            };

            /** \brief Constructor.
             *  \param[in] planner Planner that is using this data structure.
             */
            PlanData(const AStarPlanner *planner);

            /** \brief Deconstructor.
             */
            ~PlanData();

            /** \brief Add a state to the search graph.
             *  \param[in] state Symbolic state to add to graph.
             *  \param[in] params State parameters.
             *  \param[in] action Action that transitioned into this state.
             *  \param[in] prior Prior node in the search tree.
             *  \param[in] distance Distance of transition to this new node.
             *  \return The newly created (or updated) search node.
             */
            Node *addState(const StatePtr &state,                    //
                           const ParameterSpace::ParamsPtr &params,  //
                           const GroundedAction *action = nullptr,   //
                           const Node *prior = nullptr,              //
                           double distance = 0);

            /** \brief Pop the best cost node from the front of the search frontier.
             *  \return The best cost node from the search frontier.
             */
            Node *popFrontier();

            /** \brief Get the current `best` node (highest cost).
             *  \return The highest cost node explored so far.
             */
            Node *getCurrentBest() const;

            /** \brief Find a node in the set of explored nodes.
             *  \param[in] state State to search for.
             *  \param[in] params Params to search for.
             *  \return Node if found, nullptr otherwise.
             */
            Node *findNode(const StatePtr &state, const ParameterSpace::ParamsPtr &params) const;

            /** \brief Build the resulting plan that reaches the provided goal node.
             *  \param[out] plan Plan to build.
             *  \param[in] goal Goal to build plant to.
             */
            void buildPlan(Plan &plan, const Node *goal) const;

            /** \brief Transition distance function.
             *  \param[in] node Source node.
             *  \param[in] action Transition action.
             *  \param[in] next Next state reached by action.
             *  \param[in] params Next state params.
             *  \return Distance from node to next state.
             */
            using TransitionDistance = std::function<double(const Node *node,              //
                                                            const GroundedAction &action,  //
                                                            const StatePtr &next,          //
                                                            const ParameterSpace::ParamsPtr &params)>;

            /** \brief Heuristic computation function.
             *  \param[in] node Node to compute heuristic for.
             *  \return Heuristic value.
             */
            using Heuristic = std::function<double(const Node *node)>;

            /** \brief Uniform distance for all transitions.
             */
            static const TransitionDistance DEFAULT_TRANSITION_DISTANCE;

            /** \brief If `total-cost` is a numeric function, uses its value as the transition distance.
             */
            static const TransitionDistance TOTAL_COST_TRANSITION_DISTANCE;
            static const Heuristic NULL_HEURISTIC;  ///< Null heuristic, 0 for all nodes.

        private:
            const AStarPlanner *planner_;  ///< Associated planner.

            std::priority_queue<Node *,               //
                                std::vector<Node *>,  //
                                std::function<bool(const Node *, const Node *)>>
                frontier_;  ///< Frontier set of states, ordered by cost.

            std::unordered_map<Node::Key,  //
                               Node *,     //
                               std::function<std::size_t(const Node::Key &key)>>
                nodes_;  ///< Map of explored nodes.

            Node *best_{nullptr};  ///< Current node with highest cost.
        };

        /** \brief Planner options.
         */
        struct Options
        {
            /** \brief Constructor.
             */
            Options(){};

            bool verbose{false};  ///< Verbose output.

            ParameterSpacePtr param_space = std::make_shared<ParameterSpace>();  ///< Parameter space to use.
            ParameterSpace::ParamsPtr start_param =
                std::make_shared<ParameterSpace::Params>();  ///< Start state parameters.
            PlanData::TransitionDistance transition_distance;
            PlanData::Heuristic heuristic;
        };

        /** \brief Constructor.
         *  \param[in] problem Problem to solve.
         *  \param[in] options Planner options.
         */
        AStarPlanner(const ProblemPtr &problem, const Options &options = Options());

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

        /** \brief Returns the set of supported domain requirements the planner can solve.
         *  \return The set of supported requirements.
         */
        Requirements::Keyword getSupportedRequirements() const override;

        Options options;  ///< Options for computation.

    private:
        std::vector<GroundedAction> ground_actions_;          ///< Set of grounded actions.
        StateMap<std::vector<std::size_t>> blocked_actions_;  ///< Map of states to disallowed actions.
    };

    /** \brief Factory class that creates a heuristic function that uses another pddlboat::Planner to evaluate
     * node cost-to-go.
     */
    class PlannerHeuristicFactory
    {
    public:
        /** \brief Planner allocator function. Creates a new planner instance for the provided problem.
         *  \param[in] problem Problem to use to create problem.
         *  \return Planner for the provided problem.
         */
        using PlannerAllocator = std::function<PlannerPtr(const ProblemPtr &problem)>;

        /** \brief Factory method. Creates a heuristic function given the base problem.
         *  \param[in] problem The base problem the heuristic will be used for.
         *  \param[in] allocator Planner allocator to use.
         *  \return The heuristic function.
         */
        static AStarPlanner::PlanData::Heuristic make(const ProblemPtr &problem,
                                                      const PlannerAllocator &allocator);

        /** \brief Constructor.
         *  \param[in] problem The base problem the heuristic will be used for.
         *  \param[in] allocator Planner allocator to use.
         */
        PlannerHeuristicFactory(const ProblemPtr &problem, const PlannerAllocator &planner_allocator);

        /** \brief The heuristic function. Given a node, returns the number of steps left to reach the goal
         * based on the result of another plan. Will also cache the results for other states visited in the
         * resulting path.
         *  \param[in] node Node to evaluate.
         *  \return The heuristic value of the node.
         */
        double value(const AStarPlanner::PlanData::Node *node);

    private:
        /** \brief Add a state to the cost-to-go cache.
         *  \param[in] state State to cache.
         *  \param[in] value Value of state.
         */
        void addToCache(const StatePtr &state, std::size_t value);

        ProblemPtr problem_;                        ///< Per-state problem used for evaluation.
        const PlannerAllocator planner_allocator_;  ///< Planner allocator.
        StateMap<std::size_t> distance_cache_;      ///< Cache of state cost-to-go.
    };

}  // namespace pddlboat

#endif
