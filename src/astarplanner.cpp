/* Author: Zachary Kingston */

#include <pddlboat/solver/astarplanner.h>

using namespace pddlboat;

//
// AStarPlanner::ParameterSpace
//

bool AStarPlanner::ParameterSpace::Params::isEqual(const Params &other) const
{
    return true;
}

std::vector<AStarPlanner::ParameterSpace::ParamsPtr> AStarPlanner::ParameterSpace::getNeighbors(
    const StatePtr &state, const StatePtr &next, const GroundedAction &action)
{
    return std::vector<ParamsPtr>{std::make_shared<Params>()};
}

//
// PlanData::Node::Key
//

AStarPlanner::PlanData::Node::Key::Key(const Node *node) : state(node->state), params(node->params)
{
}

AStarPlanner::PlanData::Node::Key::Key(const StatePtr &state, const ParameterSpace::ParamsPtr &params)
  : state(state), params(params)
{
}

bool AStarPlanner::PlanData::Node::Key::operator==(const Key &other) const
{
    return state->isEqual(other.state) and params->isEqual(*other.params);
}

//
//
//

AStarPlanner::PlanData::Node::Node(const StatePtr &state, const ParameterSpace::ParamsPtr &params)
  : state(state), params(params)
{
}

//
// PlanData
//

const AStarPlanner::PlanData::TransitionDistance AStarPlanner::PlanData::DEFAULT_TRANSITION_DISTANCE =
    [](const Node *node, const GroundedAction &action, const StatePtr &next,
       const ParameterSpace::ParamsPtr &params) -> double { return 1.; };

const AStarPlanner::PlanData::TransitionDistance AStarPlanner::PlanData::TOTAL_COST_TRANSITION_DISTANCE =
    [](const Node *node, const GroundedAction &action, const StatePtr &next,
       const ParameterSpace::ParamsPtr &params) -> double {
    double previous = node->state->getValue("total-cost", {});
    double current = next->getValue("total-cost", {});
    return current - previous;
};

const AStarPlanner::PlanData::Heuristic AStarPlanner::PlanData::NULL_HEURISTIC =
    [](const Node *node) -> double { return 0.; };

AStarPlanner::PlanData::PlanData(const AStarPlanner *planner)
  : planner_(planner)
  , frontier_(
        [](const Node *a, const Node *b) { return (a->cost + a->heuristic) > (b->cost + b->heuristic); })
  , nodes_(0, [](const Node::Key &key) { return key.state->hash(); })
{
}

AStarPlanner::PlanData::~PlanData()
{
    for (const auto &pair : nodes_)
        delete pair.second;
}

AStarPlanner::PlanData::Node *AStarPlanner::PlanData::addState(const StatePtr &state,
                                                               const ParameterSpace::ParamsPtr &params,
                                                               const GroundedAction *action,
                                                               const Node *prior, double distance)
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

    Node::Key key(state, params);
    Node *node;

    // If node already exists in map, update information rather than create new information
    auto it = nodes_.find(key);
    if (it != nodes_.end())
    {
        node = it->second;

        if (planner_->options.verbose)
            std::cout << "    NODE EXISTED: OLD SCORE: " << node->cost << std::endl;
    }
    else
    {
        node = new Node(state, params);
        nodes_.emplace(key, node);
        node->id = id_++;
    }

    // If prior state (all except start), propagate information
    if (prior)
    {
        node->prior = prior;
        node->action = action;
        node->distance = prior->distance + 1;
        node->cost = prior->cost + distance;
        node->heuristic = planner_->options.heuristic(node);

        if (planner_->options.verbose)
            std::cout << "    NEW SCORE: " << node->cost << std::endl;
    }

    frontier_.emplace(node);

    // Track current best node.
    if (not best_ or best_->cost < node->cost)
        best_ = node;

    return node;
}

AStarPlanner::PlanData::Node *AStarPlanner::PlanData::popFrontier()
{
    if (frontier_.empty())
        return nullptr;

    Node *node = frontier_.top();
    frontier_.pop();

    return node;
}

AStarPlanner::PlanData::Node *AStarPlanner::PlanData::getCurrentBest() const
{
    return best_;
}

AStarPlanner::PlanData::Node *AStarPlanner::PlanData::findNode(const StatePtr &state,
                                                               const ParameterSpace::ParamsPtr &params) const
{
    auto existing = nodes_.find(Node::Key(state, params));
    if (existing == nodes_.end())
        return nullptr;

    return existing->second;
}

void AStarPlanner::PlanData::buildPlan(Plan &plan, const Node *goal) const
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
        plan.addStep(node->action->action, node->action->assignment);
}

//
// AStarPlanner
//

AStarPlanner::AStarPlanner(const ProblemPtr &problem, const Options &options)
  : Planner(problem), options(options)
{
    initialize();
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
    Assignment empty(problem_);  // Empty assignment for goal evaluation

    // Add start to frontier
    data.addState(problem_->start->cleanClone(), options.start_param);

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
            top->goal_checked = problem_->goal->evaluate(state, empty);

        if (top->goal_checked)
        {
            data.buildPlan(plan, top);
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

            // Check all possible parameter transitions from this state
            for (const auto &params : options.param_space->getNeighbors(state, next, grounded))
            {
                // Compute distance to next node
                double distance = options.transition_distance(top, grounded, next, params);

                // If node already has been explored, only re-expand if cost is better
                auto existing = data.findNode(next, params);
                if (not existing or existing->cost > top->cost + distance)
                {
                    data.addState(next, params, &grounded, top, distance);
                    expanded++;
                }

                neighbors++;
            }
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
    Assignment empty(problem_);  // Empty assignment for goal evaluation

    // Add start to frontier
    data.addState(problem_->start->cleanClone(), options.start_param);

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
            auto existing = data.findNode(next, options.start_param);
            if (not existing)
                existing = data.addState(next, options.start_param, &grounded, top, 1.);

            out << "<edge id=\"e" << top->id << "_" << existing->id << "\" source=\"n" << top->id << "\" target=\"n" << existing->id << "\">";
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
           Requirements::NUMERIC_FLUENTS;
}

void AStarPlanner::initialize()
{
    if (not options.transition_distance)
    {
        if (problem_->domain->isFunction("total-cost"))
            options.transition_distance = PlanData::TOTAL_COST_TRANSITION_DISTANCE;
        else
            options.transition_distance = PlanData::DEFAULT_TRANSITION_DISTANCE;
    }

    if (not options.heuristic)
        options.heuristic = PlanData::NULL_HEURISTIC;

    for (const auto &pair : problem_->domain->actions)
    {
        const auto &assignments = problem_->getGroundings(pair.second->parameters);
        for (const auto &assignment : assignments)
            ground_actions_.emplace_back(pair.second, assignment);
    }
}

//
// PlannerHeuristic
//

PlannerHeuristicFactory::PlannerHeuristicFactory(const ProblemPtr &problem,
                                                 const PlannerAllocator &planner_allocator)
  : problem_(std::make_shared<Problem>(*problem)), planner_allocator_(planner_allocator)
{
}

double PlannerHeuristicFactory::value(const AStarPlanner::PlanData::Node *node)
{
    auto it = distance_cache_.find(node->state);
    if (it == distance_cache_.end())
    {
        problem_->start = node->state;
        auto planner = planner_allocator_(problem_);

        Plan plan(problem_);
        if (planner->solve(plan))
        {
            const std::size_t n = plan.getNumSteps();
            addToCache(node->state, n);
            for (std::size_t i = 0; i < n; ++i)
                addToCache(plan.getStep(i).state, n - i - 1);

            return n;
        }

        return 10000;  // Huge number, should functionally be infinite, but hey
    }
    else
        return it->second;
}

void PlannerHeuristicFactory::addToCache(const StatePtr &state, std::size_t value)
{
    auto copy = problem_->makeState();
    copy->fold(state);
    copy->clean();

    distance_cache_.emplace(copy, value);
}

AStarPlanner::PlanData::Heuristic PlannerHeuristicFactory::make(
    const ProblemPtr &problem, const PlannerHeuristicFactory::PlannerAllocator &allocator)
{
    auto heuristic = std::make_shared<PlannerHeuristicFactory>(problem, allocator);
    return [heuristic](const AStarPlanner::PlanData::Node *node) { return heuristic->value(node); };
}
