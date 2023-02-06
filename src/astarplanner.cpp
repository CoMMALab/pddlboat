/* Author: Zachary Kingston */

#include <random>
#include <fstream>
#include <pddlboat/solver/astarplanner.hpp>

using namespace pddlboat;

namespace
{
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
            unsigned int id;                        ///< Unique ID.
            StatePtr state;                         ///< Symbolic state.
            const Node *prior{nullptr};             ///< Prior node in search.
            const GroundedAction *action{nullptr};  ///< Action that transitioned into this state.
            bool goal_checked{false};  ///< Has this state already been evaluated for goal satisfaction?
            std::size_t distance{0};   ///< Distance (in terms of node hops) from start.
            double cost{0};            ///< Cost of node so far.
            double heuristic{0};       ///< Heuristic value of node.

            Node(const StatePtr &state);
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
         *  \param[in] action Action that transitioned into this state.
         *  \param[in] prior Prior node in the search tree.
         *  \param[in] distance Distance of transition to this new node.
         *  \return The newly created (or updated) search node.
         */
        Node *addState(const StatePtr &state,                   //
                       const GroundedAction *action = nullptr,  //
                       const Node *prior = nullptr,             //
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
         *  \return Node if found, nullptr otherwise.
         */
        Node *findNode(const StatePtr &state) const;

        /** \brief Build the resulting plan that reaches the provided goal node.
         *  \param[out] plan Plan to build.
         *  \param[in] goal Goal to build plant to.
         */
        void buildPlan(Plan &plan, const Node *goal) const;

    private:
        const AStarPlanner *planner_;  ///< Associated planner.

        std::priority_queue<Node *,               //
                            std::vector<Node *>,  //
                            std::function<bool(const Node *, const Node *)>>
            frontier_;  ///< Frontier set of states, ordered by cost.

        StateMap<Node *> nodes_;  ///< Map of explored nodes.

        Node *best_{nullptr};  ///< Current node with highest cost.
        unsigned int id_{0};   ///< Current node ID to allocate.
    };

    PlanData::Node::Node(const StatePtr &state) : state(state)
    {
    }

    PlanData::PlanData(const AStarPlanner *planner)
      : planner_(planner)
      , frontier_([](const Node *a, const Node *b)
                  { return (a->cost + a->heuristic) > (b->cost + b->heuristic); })
    {
    }

    PlanData::~PlanData()
    {
        for (const auto &pair : nodes_)
            delete pair.second;
    }

    PlanData::Node *PlanData::addState(const StatePtr &state, const GroundedAction *action, const Node *prior,
                                       double distance)
    {
        if (planner_->options.verbose)
        {
            std::cout << "    EXPANDING to " << state->hash();
            if (action)
            {
                std::cout << " ";
                std::cout << "(" << action->action->name;
                for (const auto &parameter : action->action->parameters)
                    std::cout << " " << action->assignment.assigned(parameter.name)->name;
                std::cout << ")";
            }
            std::cout << std::endl;

            state->toPDDL(std::cout, 4);
        }

        Node *node;

        // If node already exists in map, update information rather than create new information
        auto it = nodes_.find(state);
        if (it != nodes_.end())
        {
            node = it->second;

            if (planner_->options.verbose)
                std::cout << "    NODE EXISTED: OLD SCORE: " << node->cost << std::endl;
        }
        else
        {
            node = new Node(state);
            nodes_.emplace(state, node);
            node->id = id_++;
        }

        // If prior state (all except start), propagate information
        if (prior)
        {
            node->prior = prior;
            node->action = action;
            node->distance = prior->distance + 1;
            node->cost = prior->cost + distance;
            node->heuristic = planner_->options.heuristic(node->state);

            if (planner_->options.verbose)
                std::cout << "    NEW SCORE: " << node->cost << std::endl;
        }

        frontier_.emplace(node);

        // Track current best node.
        if (not best_ or best_->cost < node->cost)
            best_ = node;

        return node;
    }

    PlanData::Node *PlanData::popFrontier()
    {
        if (frontier_.empty())
            return nullptr;

        Node *node = frontier_.top();
        frontier_.pop();

        return node;
    }

    PlanData::Node *PlanData::getCurrentBest() const
    {
        return best_;
    }

    PlanData::Node *PlanData::findNode(const StatePtr &state) const
    {
        auto existing = nodes_.find(state);
        if (existing == nodes_.end())
            return nullptr;

        return existing->second;
    }

    void PlanData::buildPlan(Plan &plan, const Node *goal) const
    {
        std::vector<const Node *> node_list;

        const Node *it = goal;
        while (it->prior)
        {
            node_list.emplace_back(it);
            it = it->prior;
        }

        std::reverse(node_list.begin(), node_list.end());

        for (const auto &node : node_list)
            plan.addStepAction(node->action->action, node->action->assignment);
    }
}  // namespace

//
// AStarPlanner
//

const AStarPlanner::TransitionDistance AStarPlanner::DEFAULT_TRANSITION_DISTANCE =
    [](const StatePtr &current, const GroundedAction &action, const StatePtr &next) -> double { return 1.; };

const AStarPlanner::TransitionDistance AStarPlanner::TOTAL_COST_TRANSITION_DISTANCE =
    [](const StatePtr &current, const GroundedAction &action, const StatePtr &next) -> double
{
    return next->getValue("total-cost", {}) - current->getValue("total-cost", {});
};

const AStarPlanner::Heuristic AStarPlanner::NULL_HEURISTIC = [](const StatePtr &node) -> double
{ return 0.; };

AStarPlanner::AStarPlanner(const ProblemPtr &problem, const Options &options)
  : Planner(problem), options(options)
{
    initialize();
}

std::vector<Plan::Step> AStarPlanner::randomWalk(std::size_t steps, std::size_t seed, bool weighted)
{
    std::random_device rd;
    std::mt19937 gen((seed) ? seed : rd());  // If seed == 0, then use random seed.

    PlanData data(this);         // Search data

    data.addState(problem_->start->cleanClone());
    PlanData::Node *start = data.popFrontier();

    std::vector<Plan::Step> states;
    for (std::size_t i = 0; i < steps; ++i)
    {
        auto state = start->state;

        for (std::size_t j = 0; j < ground_actions_.size(); ++j)
        {
            const auto &grounded = ground_actions_[j];

            // Check if grounded action is feasible in this state
            if (not grounded.action->precondition->evaluate(state, grounded.assignment))
                continue;

            // Propagate action on this state
            StatePtr next = problem_->makeState();
            if (not grounded.action->doAction(next, state, grounded.assignment))
                throw std::runtime_error("Failed to apply action to state in planning.");

            next->clean();
            double distance = options.transition_distance(start->state, grounded, next);
            data.addState(next, &grounded, start, distance);
        }

        double mc = 0;
        std::vector<PlanData::Node *> nodes;
        while ((start = data.popFrontier()))
        {
            if (start->cost > mc)
                mc = start->cost;

            nodes.emplace_back(start);
        }

        if (nodes.empty())
            break;

        if (weighted)
        {
            double total = 0;
            std::vector<double> values;
            for (const auto &node : nodes)
            {
                total += mc - node->cost + 1;
                values.emplace_back(total);
            }

            std::uniform_real_distribution<double> urand(0, total);
            double r = urand(gen);

            for (std::size_t j = 0; j < values.size(); ++j)
                if (r < values[j])
                {
                    start = nodes[j];
                    Plan::Step step(start->action->action, start->action->assignment);
                    step.state = start->state;
                    states.emplace_back(step);
                    break;
                }
        }
        else
        {
            std::uniform_int_distribution<std::size_t> urand(0, nodes.size() - 1);
            std::size_t r = urand(gen);

            start = nodes[r];
            Plan::Step step(start->action->action, start->action->assignment);
            step.state = start->state;
            states.emplace_back(step);
        }
    }

    return states;
}

std::vector<Plan::Step> AStarPlanner::getStartNeighbors()
{
    PlanData data(this);         // Search data

    data.addState(problem_->start->cleanClone());
    PlanData::Node *start = data.popFrontier();
    auto state = start->state;

    // Get the blocked actions for this state
    const auto &blocked = blocked_actions_.find(state);

    // Check all possible transitions from this state
    for (std::size_t index = 0; index < ground_actions_.size(); ++index)
    {
        const auto &grounded = ground_actions_[index];

        // Check if action is in blocked list (i.e., its index appears)
        if (blocked != blocked_actions_.end() and
            std::find(blocked->second.begin(), blocked->second.end(), index) != blocked->second.end())
            continue;

        // Check if grounded action is feasible in this state
        if (not grounded.action->precondition->evaluate(state, grounded.assignment))
            continue;

        // Propagate action on this state
        StatePtr next = problem_->makeState();
        if (not grounded.action->doAction(next, state, grounded.assignment))
            throw std::runtime_error("Failed to apply action to state in planning.");
        next->clean();

        data.addState(next, &grounded, start);
    }

    std::vector<Plan::Step> states;
    PlanData::Node *node;
    while ((node = data.popFrontier()))
    {
        Plan::Step step(node->action->action, node->action->assignment);
        step.state = node->state;
        states.emplace_back(step);
    }

    return states;
}

bool AStarPlanner::solve(Plan &plan)
{
    return solve(plan, 0);
}

bool AStarPlanner::solve(Plan &plan, unsigned int timeout)
{
    std::size_t iteration = 0;
    boost::timer::cpu_timer timer;

    PlanData data(this);         // Search data

    // Add start to frontier
    data.addState(problem_->start->cleanClone());

    PlanData::Node *top;
    while ((top = data.popFrontier()))
    {
        const auto &state = top->state;

        // Check if allotted time has passed
        if (timeout and io::getElapsedMilliseconds(timer) > timeout)
        {
            std::cerr << log::format("Failure: Planner Timeout >%1$06dms", timeout) << std::endl;
            data.buildPlan(plan, data.getCurrentBest());
            return plan.validate();
        }

        if (options.verbose)
        {
            std::cout << log::format("A* Iteration: %1$05d", iteration) << std::endl;
            std::cout << log::format("State Hash: %1%", state->hash()) << std::endl;
            state->toPDDL(std::cout);
        }

        // Check if this state satisfies the goal
        if (not top->goal_checked)
            top->goal_checked = problem_->goal->evaluate(state, {});

        if (top->goal_checked)
        {
            data.buildPlan(plan, top);
            plan.setTimeTaken("solve", io::getElapsedMilliseconds(timer) / 1000.);
            return plan.validate();
        }

        // Get the blocked actions for this state
        const auto &blocked = blocked_actions_.find(state);

        std::size_t expanded = 0;
        std::size_t neighbors = 0;

        // Check all possible transitions from this state
        for (std::size_t index = 0; index < ground_actions_.size(); ++index)
        {
            const auto &grounded = ground_actions_[index];

            // Check if action is in blocked list (i.e., its index appears)
            if (blocked != blocked_actions_.end() and
                std::find(blocked->second.begin(), blocked->second.end(), index) != blocked->second.end())
            {
                if (options.verbose)
                    std::cout << log::format("Ignoring blocked action %s", grounded.string) << std::endl;

                continue;
            }

            // Check if grounded action is feasible in this state
            if (not grounded.action->precondition->evaluate(state, grounded.assignment))
                continue;

            // Propagate action on this state
            StatePtr next = problem_->makeState();
            if (not grounded.action->doAction(next, state, grounded.assignment))
                throw std::runtime_error("Failed to apply action to state in planning.");
            next->clean();

            // Compute distance to next node
            double distance = options.transition_distance(top->state, grounded, next);

            // If node already has been explored, only re-expand if cost is better
            auto existing = data.findNode(next);
            if (not existing or existing->cost > top->cost + distance)
            {
                data.addState(next, &grounded, top, distance);
                expanded++;
            }

            neighbors++;
        }

        if (options.verbose)
            std::cout << log::format("Expanded %1%/%2% neighbors!", expanded, neighbors) << std::endl;

        iteration++;
    }

    return false;
}

void AStarPlanner::generateGraphML(const std::string &filename) const
{
    std::ofstream out;
    io::createFile(out, filename);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    out << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
           "xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns "
           "http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">"
        << std::endl;
    out << "<key id=\"d0\" for=\"node\" attr.name=\"state\" attr.type=\"string\"></key>" << std::endl;
    out << "<key id=\"d1\" for=\"edge\" attr.name=\"action\" attr.type=\"string\"></key>" << std::endl;
    out << "<graph id=\"G\" edgedefault=\"directed\">" << std::endl;

    std::size_t iteration = 0;
    boost::timer::cpu_timer timer;

    PlanData data(this);         // Search data

    // Add start to frontier
    data.addState(problem_->start->cleanClone());

    PlanData::Node *top;
    while ((top = data.popFrontier()))
    {
        const auto &state = top->state;

        out << "<node id=\"n" << top->id << "\">";
        out << "<data key=\"d0\">";
        state->toPDDL(out, 0);
        out << "</data>"
            << "</node>" << std::endl;

        // Check all possible transitions from this state
        for (std::size_t index = 0; index < ground_actions_.size(); ++index)
        {
            const auto &grounded = ground_actions_[index];

            // Check if grounded action is feasible in this state
            if (not grounded.action->precondition->evaluate(state, grounded.assignment))
                continue;

            // Propagate action on this state
            StatePtr next = problem_->makeState();
            if (not grounded.action->doAction(next, state, grounded.assignment))
                throw std::runtime_error("Failed to apply action to state in planning.");
            next->clean();

            // If node already has been explored
            auto existing = data.findNode(next);
            if (not existing)
                existing = data.addState(next, &grounded, top, 1.);

            out << "<edge id=\"e" << top->id << "_" << existing->id << "\" source=\"n" << top->id
                << "\" target=\"n" << existing->id << "\">";
            out << "<data key=\"d1\">";
            out << grounded.action->toStepID(grounded.assignment);
            out << "</data>"
                << "</edge>" << std::endl;
        }

        iteration++;
    }

    out << "</graph>" << std::endl;
    out << "</graphml>" << std::endl;
    out.close();
}

void AStarPlanner::blockStateActions(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions, const StatePtr &state)
{
    const auto &copy = state->cleanClone();
    auto pair = blocked_actions_.emplace(copy, std::vector<std::size_t>());

    for (const auto &[action, assignment] : actions)
    {
        GroundedAction key(action, assignment);
        auto fit = std::find(ground_actions_.begin(), ground_actions_.end(), key);
        if (fit == ground_actions_.end())
            continue;

        std::size_t index = std::distance(ground_actions_.begin(), fit);
        auto &blocked = pair.first->second;

        // Insert if not already blocked
        if (std::find(blocked.begin(), blocked.end(), index) == blocked.end())
            blocked.emplace_back(index);
    }
}

Requirements::Keyword AStarPlanner::getSupportedRequirements() const
{
    return Requirements::ADL | Requirements::DERIVED_PREDICATES | Requirements::OBJECT_FLUENTS |
        Requirements::NUMERIC_FLUENTS | Requirements::DOMAIN_AXIOMS;
}

void AStarPlanner::initialize()
{
    if (not options.transition_distance)
    {
        if (problem_->domain->isFunction("total-cost"))
            options.transition_distance = TOTAL_COST_TRANSITION_DISTANCE;
        else
            options.transition_distance = DEFAULT_TRANSITION_DISTANCE;
    }

    if (not options.heuristic)
        options.heuristic = NULL_HEURISTIC;

    for (const auto &pair : problem_->domain->actions)
    {
        const auto &assignments = problem_->getGroundings(pair.second->parameters);
        for (const auto &assignment : assignments)
            ground_actions_.emplace_back(pair.second, assignment);
    }
}
