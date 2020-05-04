#include <queue>
#include <algorithm>

#include <boost/graph/dag_shortest_paths.hpp>

#include <pddlboat/solver/dependency.h>

using namespace pddlboat;

static const bool debug = true;

///
/// PlanDependencyGraph ActionNode
///

PlanDependencyGraph::ActionNode::ActionNode(const Plan::Step &step, std::size_t index)
  : action(step.action), assignment(step.assignment), plan_index(index)
{
    // Get all predicates for precondition and effects this actions causes.
    action->precondition->getUsedPredicates(pre_pred, step.state, assignment);
    action->effect->getAssignedPredicates(eff_pred, step.state, assignment);
}

void PlanDependencyGraph::ActionNode::setState(const StatePtr &state)
{
    pre_state = state->clone();
    eff_state = state->clone();

    action->doAction(eff_state, pre_state, assignment);
}

bool PlanDependencyGraph::ActionNode::isEffected(const ActionNodePtr &pre) const
{
    for (const auto &predicate : pre_pred)
    {
        auto it = pre->eff_pred.find(predicate);
        if (it != pre->eff_pred.end())
            return true;
    }

    return false;
}

bool PlanDependencyGraph::ActionNode::isPrior(const ActionNodePtr &eff) const
{
    for (const auto &predicate : eff_pred)
    {
        auto it = eff->pre_pred.find(predicate);
        if (it != eff->pre_pred.end())
            return true;
    }

    return false;
}

std::ostream &PlanDependencyGraph::ActionNode::toString(std::ostream &out) const
{
    out << "(" << action->name;
    for (const auto &parameter : action->parameters)
        out << " " << assignment.assigned(parameter.name)->name;

    out << ")";

    return out;
}

///
/// PlanDependencyGraph
///

PlanDependencyGraph::PlanDependencyGraph(const PlanPtr &plan)
  : plan_(plan), problem_(plan->getProblem()), dag_()
{
    initialize();
}

void PlanDependencyGraph::initialize()
{
    const auto &steps = plan_->getSteps();
    nodes_.resize(steps.size());

    for (std::size_t i = 0; i < steps.size(); ++i)
    {
        // iterate backwards to incrementally build DAG
        std::size_t index = steps.size() - i - 1;

        // can't use make_shared because of private constructor
        auto toAdd = std::shared_ptr<ActionNode>(new ActionNode(steps[index], index));

        // add to step -> node mapping
        nodes_[index] = toAdd;

        // setup vertex mapping
        toAdd->vertex = boost::add_vertex(dag_);
        getActionNode(toAdd->vertex) = toAdd;

        if (debug)
        {
            std::cout << "Adding Step ";
            steps[index].toString(std::cout) << " to DAG: " << toAdd->vertex;
            std::cout << std::endl;
        }

        // check against all other nodes that come after in the plan
        const auto &others = getNodesByMakespan();
        for (const auto &other : others)
        {
            if (other == toAdd)
                continue;

            // always check if this node effects others as it comes before in the plan
            bool effects = other->isEffected(toAdd);

            // only check effect if greater or equal in makespan (could be reordered)
            bool isEffected = other->makespan >= toAdd->makespan and toAdd->isEffected(other);

            // only add edges if there does not already exist a dependency
            bool depends = isDescendant(toAdd, other);

            bool add = (effects or isEffected) and not depends;

            if (debug)
            {
                std::cout << "  Connecting ";
                other->toString(std::cout) << " ";
                std::cout << ((effects) ? "Effected" : "Not Effected") << ", ";
                std::cout << ((isEffected) ? "Is Effected" : "Is Not Effected") << ", ";
                std::cout << ((depends) ? "Depends" : "Does Not Depend");

                if (add)
                    std::cout << " ADDED!";

                std::cout << std::endl;
            }

            if (add)
                addDependency(toAdd, other);
        }
    }

    computeLeafDistance();
}

void PlanDependencyGraph::computeLeafDistance()
{
    auto nodes = getNodesByMakespan();
    for (auto &node : nodes)
        node->leafdist = 0;

    for (auto &node : nodes)
    {
        auto set = getParents(node);
        for (const auto &other : set)
        {
            if (other->leafdist + 1 > node->leafdist)
                node->leafdist = other->leafdist + 1;
        }
    }
}

PlanDependencyGraph::ActionNodePtr PlanDependencyGraph::getPlanStep(std::size_t index) const
{
    if (index >= nodes_.size())
        return nullptr;

    return nodes_[index];
}

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getNodes() const
{
    return getNodesFilter();
}

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getNodesFilter(const NodeFunction &function) const
{
    ActionNodeList copy;

    auto [vi, ve] = boost::vertices(dag_);
    for (; vi != ve; ++vi)
    {
        const auto &node = getActionNodeConst(*vi);
        if (not function or (function and function(node)))
            copy.emplace_back(node);
    }

    return copy;
}

// check functions

bool PlanDependencyGraph::isParent(const ActionNodePtr &node, const ActionNodePtr &parent) const
{
    return isEdge(parent->vertex, node->vertex);
}

bool PlanDependencyGraph::isChild(const ActionNodePtr &node, const ActionNodePtr &child) const
{
    return isEdge(node->vertex, child->vertex);
}

bool PlanDependencyGraph::isDescendant(const ActionNodePtr &node, const ActionNodePtr &descendant) const
{
    return isDependentOn(node->vertex, descendant->vertex);
}

bool PlanDependencyGraph::isAncestor(const ActionNodePtr &node, const ActionNodePtr &ancestor) const
{
    return isDependentOn(ancestor->vertex, node->vertex);
}

// get functions

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getPredecessors(const ActionNodePtr &node) const
{
    ActionNodeList nodes;
    std::queue<ActionNodePtr> queue;
    queue.push(node);

    while (not queue.empty())
    {
        auto next = queue.front();
        queue.pop();

        for (const auto &parent : getParents(next))
        {
            nodes.emplace_back(parent);
            queue.emplace(parent);
        }
    }

    return nodes;
}

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getDescendants(const ActionNodePtr &node) const
{
    ActionNodeList nodes;
    std::queue<ActionNodePtr> queue;
    queue.push(node);

    while (not queue.empty())
    {
        auto next = queue.front();
        queue.pop();

        for (const auto &parent : getChildren(next))
        {
            nodes.emplace_back(parent);
            queue.emplace(parent);
        }
    }

    return nodes;
}

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getParents(const ActionNodePtr &node) const
{
    ActionNodeList nodes;
    auto parents = getParents(node->vertex);
    for (const auto &other : parents)
        nodes.emplace_back(getActionNodeConst(other));

    return nodes;
}

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getChildren(const ActionNodePtr &node) const
{
    ActionNodeList nodes;
    auto children = getChildren(node->vertex);
    for (const auto &other : children)
        nodes.emplace_back(getActionNodeConst(other));

    return nodes;
}

// makespan functions

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getNodesWithMakespan(std::size_t makespan) const
{
    auto copy = getNodesFilter([&makespan](const ActionNodePtr &node) { return node->makespan == makespan; });
    return copy;
}

PlanDependencyGraph::ActionNodeList
PlanDependencyGraph::getNodesWithGreaterMakespan(std::size_t makespan) const
{
    auto copy = getNodesFilter([&makespan](const ActionNodePtr &node) { return node->makespan > makespan; });
    return copy;
}

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getNodesWithLessMakespan(std::size_t makespan) const
{
    auto copy = getNodesFilter([&makespan](const ActionNodePtr &node) { return node->makespan < makespan; });
    return copy;
}

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getNodesByMakespan() const
{
    auto copy = getNodes();
    std::sort(copy.begin(), copy.end(),
              [](const ActionNodePtr &a, const ActionNodePtr &b) { return a->makespan > b->makespan; });
    return copy;
}

// leaf distance functions

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getNodesWithLeafDistance(std::size_t leafdist) const
{
    auto copy = getNodesFilter([&leafdist](const ActionNodePtr &node) { return node->leafdist == leafdist; });
    return copy;
}

PlanDependencyGraph::ActionNodeList
PlanDependencyGraph::getNodesWithGreaterLeafDistance(std::size_t leafdist) const
{
    auto copy = getNodesFilter([&leafdist](const ActionNodePtr &node) { return node->leafdist > leafdist; });
    return copy;
}

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getNodesWithLessLeafDistance(std::size_t leafdist) const
{
    auto copy = getNodesFilter([&leafdist](const ActionNodePtr &node) { return node->leafdist < leafdist; });
    return copy;
}

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getNodesByLeafDistance() const
{
    auto copy = getNodes();
    std::sort(copy.begin(), copy.end(),
              [](const ActionNodePtr &a, const ActionNodePtr &b) { return a->leafdist < b->leafdist; });
    return copy;
}

PlanDependencyGraph::ActionNodeList PlanDependencyGraph::getLeafs() const
{
    return getNodesWithLeafDistance(0);
}

std::ostream &PlanDependencyGraph::toDotFile(std::ostream &out, bool makespan) const
{
    out << "digraph {" << std::endl;
    out << "node[shape=plaintext,fontname=\"courier\",colorscheme=\"dark23\"]" << std::endl;

    // Rank track on the left
    out << (makespan ? "Makespan" : "\"Leaf Distance\"") << "->";
    for (std::size_t i = 0; i <= maxMakespan_; ++i)
    {
        std::size_t index = i;
        if (makespan)
            index = maxMakespan_ - i;

        out << index;
        if (i != maxMakespan_)
            out << "->";
    }
    out << "[style=invis];" << std::endl;

    // Nodes
    auto nodes = getNodes();
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        const auto &node = nodes[i];
        out << "n" << node->vertex << "[label=\"";
        out << node->plan_index << ". ";
        node->toString(out);
        out << "\"";
        if (node->leafdist == 0 and node->makespan == 0)
            out << ",fontcolor=1";
        else if (node->leafdist == 0)
            out << ",fontcolor=2";
        else if (node->makespan == 0)
            out << ",fontcolor=3";
        out << "];" << std::endl;
    }

    // Rank constraints
    for (std::size_t i = 0; i <= maxMakespan_; ++i)
    {
        out << "{rank=same;" << i << ";";
        ActionNodeList makes;
        if (makespan)
            makes = getNodesWithMakespan(i);
        else
            makes = getNodesWithLeafDistance(i);

        for (const auto &node : makes)
            out << "n" << node->vertex << ";";
        out << "}" << std::endl;
    }

    // Edges
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        auto children = getChildren(i);
        if (not children.empty())
        {
            out << "n" << i << "->";
            if (children.size() > 1)
                out << "{";
            for (std::size_t j = 0; j < children.size(); ++j)
                out << (j ? " " : "") << "n" << children[j];
            if (children.size() > 1)
                out << "}";
            out << ";" << std::endl;
        }
    }

    out << "}" << std::endl;
    return out;
}

void PlanDependencyGraph::computeNodeStates()
{
    auto nodes = getNodesByMakespan();
    for (auto &node : nodes)
    {
        auto predecessors = getPredecessors(node);
        auto state = problem_->start->clone();
        for (const auto &pred : predecessors)
        {
            auto out = state->clone();
            pred->action->doAction(out, state, pred->assignment);
            state = out;
        }

        node->setState(state);
    }
}

bool PlanDependencyGraph::removeNode(const ActionNodePtr &node)
{
    if (not getParents(node).empty())
        return false;

    boost::remove_vertex(node->vertex, dag_);
    computeLeafDistance();
    return true;
}

void PlanDependencyGraph::addDependency(ActionNodePtr pre, ActionNodePtr post)
{
    addEdge(pre->vertex, post->vertex);

    std::size_t v = post->makespan + 1;
    if (v > pre->makespan)
        pre->makespan = v;

    if (pre->makespan > maxMakespan_)
        maxMakespan_ = pre->makespan;
}

void PlanDependencyGraph::addEdge(Vertex pre, Vertex post)
{
    boost::add_edge(pre, post, 1., dag_);
}

bool PlanDependencyGraph::isDependentOn(Vertex pre, Vertex eff) const
{
    std::vector<Vertex> predecessors(boost::num_vertices(dag_), getNullVertex());
    boost::dag_shortest_paths(dag_, pre, boost::predecessor_map(&predecessors[0]));
    return predecessors[eff] != eff;
}

bool PlanDependencyGraph::isEdge(Vertex src, Vertex dst) const
{
    auto edge = boost::edge(src, dst, dag_);
    return edge.second;
}

std::vector<PlanDependencyGraph::Vertex> PlanDependencyGraph::getChildren(Vertex src) const
{
    std::vector<Vertex> children;
    auto [ei, end] = boost::out_edges(src, dag_);
    for (; ei != end; ++ei)
        children.emplace_back(boost::target(*ei, dag_));
    return children;
}

std::vector<PlanDependencyGraph::Vertex> PlanDependencyGraph::getParents(Vertex src) const
{
    std::vector<Vertex> parents;
    auto [ei, end] = boost::in_edges(src, dag_);
    for (; ei != end; ++ei)
        parents.emplace_back(boost::source(*ei, dag_));
    return parents;
}

PlanDependencyGraph::ActionNodePtr &PlanDependencyGraph::getActionNode(Vertex vertex)
{
    return get(boost::vertex_bundle, dag_, vertex);
}

const PlanDependencyGraph::ActionNodePtr &PlanDependencyGraph::getActionNodeConst(Vertex vertex) const
{
    return get(boost::vertex_bundle, dag_, vertex);
}

PlanDependencyGraph::Vertex PlanDependencyGraph::getNullVertex()
{
    return boost::graph_traits<Graph>::null_vertex();
}
