/* Author: Zachary Kingston */

#ifndef PDDLBOAT_SOLVER_DEPENDENCY_
#define PDDLBOAT_SOLVER_DEPENDENCY_

#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/dag_shortest_paths.hpp>
// #include <boost/graph/dijkstra_shortest_paths.hpp>

#include <pddlboat/macros.h>
#include <pddlboat/solver/domain.h>
#include <pddlboat/solver/planner.h>

#include <string>
#include <sstream>

namespace pddlboat
{
    /** \cond IGNORE */
    PDDLBOAT_CLASS_FORWARD(PlanDependencyGraph)
    /** \endcond */

    /** \class pddlboat::PlanDependencyPtr
        \brief A shared pointer wrapper for pddlboat::PlanDependency. */

    /** \class pddlboat::PlanDependencyConstPtr
        \brief A const shared pointer wrapper for pddlboat::PlanDependency. */

    class PlanDependencyGraph
    {
    public:
        PDDLBOAT_CLASS_FORWARD(ActionNode)

        class ActionNode
        {
        public:
            ActionPtr action;        ///< Action taken at this node.
            Assignment assignment;   ///< Assignment to this action for this node.
            std::size_t plan_index;  ///< Index of this step in the original plan.

            std::set<std::string> pre_pred;  ///< Precondition predicate set.
            std::set<std::string> eff_pred;  ///< Effect predicate set.

            StatePtr pre_state{nullptr};  ///< State of the world before this action.
            StatePtr eff_state{nullptr};  ///< State of the world after this action.

            bool initialized{false};
            std::size_t makespan{0};  ///< Distance to final action.
            std::size_t leafdist{0};  ///< Distance from leaves.

            ActionNode(const Plan::Step &step, std::size_t index)
              : action(step.action), assignment(step.assignment), plan_index(index)
            {
                // Get all predicates for precondition and effect.
                action->precondition->getUsedPredicates(pre_pred, step.state, assignment);
                action->effect->getAssignedPredicates(eff_pred, step.state, assignment);
            }

            void setPreState(const StatePtr &state)
            {
                pre_state = state->clone();
                eff_state = state->clone();

                action->doAction(eff_state, pre_state, assignment);
            }

            // true if this action depends on predicates from the other (pre)
            bool isEffected(const ActionNodePtr &pre) const
            {
                for (const auto &predicate : pre_pred)
                {
                    auto it = pre->eff_pred.find(predicate);
                    if (it != pre->eff_pred.end())
                        return true;
                }

                return false;
            }

            // true if this action changes predicates that the other depends on (eff)
            bool isPrior(const ActionNodePtr &eff) const
            {
                for (const auto &predicate : eff_pred)
                {
                    auto it = eff->pre_pred.find(predicate);
                    if (it != eff->pre_pred.end())
                        return true;
                }

                return false;
            }

            std::ostream &toString(std::ostream &out) const
            {
                out << "(" << action->name;
                for (const auto &parameter : action->parameters)
                    out << " " << assignment.assigned(parameter.name)->name;

                out << ")";

                return out;
            }
        };

        using Graph =                                                            //
            boost::adjacency_matrix<boost::directedS,                            //
                                    ActionNodePtr,                               //
                                    boost::property<boost::edge_weight_t, int>,  //
                                    boost::no_property>;
        using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;

        PlanDependencyGraph(const PlanPtr &plan)
          : plan_(plan), problem_(plan->getProblem()), dag_(plan->getNumSteps())
        {
            generateNodes();
            buildGraph();
        }

        std::set<ActionNodePtr> getNodesWithMakespan(std::size_t makespan) const
        {
            std::set<ActionNodePtr> set;
            for (const auto &node : nodes_)
            {
                if (node->initialized and node->makespan == makespan)
                    set.emplace(node);
            }

            return set;
        }

        std::set<ActionNodePtr> getNodesWithLeafDist(std::size_t leafdist) const
        {
            std::set<ActionNodePtr> set;
            for (const auto &node : nodes_)
            {
                if (node->initialized and node->leafdist == leafdist)
                    set.emplace(node);
            }

            return set;
        }

        bool isDirectlyDependentOn(const ActionNodePtr &pre, const ActionNodePtr &eff) const
        {
            return isEdge(pre->plan_index, eff->plan_index);
        }

        bool isDependentOn(const ActionNodePtr &pre, const ActionNodePtr &eff) const
        {
            return isDependentOn(pre->plan_index, eff->plan_index);
        }

        // get nodes in descending makespan, e.g., farthest first
        std::vector<ActionNodePtr> getNodesByMakespan() const
        {
            std::vector<ActionNodePtr> copy;

            // need this as this function is used in construction
            for (const auto &node : nodes_)
                if (node->initialized)
                    copy.emplace_back(node);

            std::sort(copy.begin(), copy.end(), [](const ActionNodePtr &a, const ActionNodePtr &b) {
                return a->makespan > b->makespan;
            });

            return copy;
        }

        // get nodes in ascending leaf dist, e.g., farthest first
        std::vector<ActionNodePtr> getNodesByLeafDist() const
        {
            auto copy = nodes_;
            std::sort(copy.begin(), copy.end(), [](const ActionNodePtr &a, const ActionNodePtr &b) {
                return a->leafdist < b->leafdist;
            });

            return copy;
        }

        std::vector<ActionNodePtr> getPredecessors(const ActionNodePtr &node) const
        {
            std::vector<ActionNodePtr> nodes;
            for (const auto &other : nodes_)
                if (isDependentOn(other, node))
                    nodes.emplace_back(other);

            return nodes;
        }

        std::vector<ActionNodePtr> getDescendants(const ActionNodePtr &node) const
        {
            std::vector<ActionNodePtr> nodes;
            for (const auto &other : nodes_)
                if (isDependentOn(node, other))
                    nodes.emplace_back(other);

            return nodes;
        }

        std::set<ActionNodePtr> getParents(const ActionNodePtr &node) const
        {
            std::set<ActionNodePtr> nodes;
            for (const auto &other : nodes_)
                if (isDirectlyDependentOn(other, node))
                    nodes.emplace(other);

            return nodes;
        }

        std::set<ActionNodePtr> getChildren(const ActionNodePtr &node) const
        {
            std::set<ActionNodePtr> nodes;
            for (const auto &other : nodes_)
                if (isDirectlyDependentOn(node, other))
                    nodes.emplace(other);

            return nodes;
        }

        std::ostream &toDotFile(std::ostream &out, bool makespan = true) const
        {
            out << "digraph {" << std::endl;

            out << "node[shape=plaintext,fontname=\"courier\",colorscheme=\"dark23\"]" << std::endl;

            out << (makespan ? "Makespan" : "\"Leaf Distance\"") << "->";
            for (std::size_t i = 0; i <= maxMakespan_; ++i)
            {
                out << i;
                if (i != maxMakespan_)
                    out << "->";
            }
            out << "[style=invis];" << std::endl;

            for (std::size_t i = 0; i < nodes_.size(); ++i)
            {
                out << "n" << i << " [label=\"";
                nodes_[i]->toString(out);
                out << "\"";
                if (nodes_[i]->leafdist == 0 and nodes_[i]->makespan == 0)
                    out << ",fontcolor=1";
                else if (nodes_[i]->leafdist == 0)
                    out << ",fontcolor=2";
                else if (nodes_[i]->makespan == 0)
                    out << ",fontcolor=3";
                out << "];" << std::endl;
            }

            for (std::size_t i = 0; i <= maxMakespan_; ++i)
            {
                out << "{rank=same;" << i << ";";
                std::set<ActionNodePtr> set;
                if (makespan)
                    set = getNodesWithMakespan(i);
                else
                    set = getNodesWithLeafDist(i);

                for (const auto &node : set)
                    out << "n" << node->plan_index << ";";
                out << "}" << std::endl;
            }

            for (std::size_t i = 0; i < nodes_.size(); ++i)
                for (std::size_t j = 0; j < nodes_.size(); ++j)
                {
                    if (isEdge(i, j))
                        out << "n" << i << "->"
                            << "n" << j << ";" << std::endl;
                }

            out << "}" << std::endl;
            return out;
        }

    private:
        void generateNodes()
        {
            const auto &steps = plan_->getSteps();
            for (std::size_t i = 0; i < steps.size(); ++i)
            {
                auto an = std::make_shared<ActionNode>(steps[i], i);
                nodes_.emplace_back(an);
                get(boost::vertex_bundle, dag_, i) = an;
            }
        }

        void buildGraph()
        {
            nodes_.back()->initialized = true;

            for (std::size_t i = 1; i < nodes_.size(); ++i)
            {
                auto toAdd = nodes_[nodes_.size() - i - 1];

                const auto &others = getNodesByMakespan();
                for (const auto &other : others)
                    if (other->isEffected(toAdd) and not isDependentOn(toAdd, other))
                        addDependency(toAdd, other);

                toAdd->initialized = true;
            }

            auto nodes = getNodesByMakespan();
            for (auto &node : nodes)
            {
                auto set = getParents(node);
                for (const auto &other : set)
                {
                    if (other->leafdist + 1 > node->leafdist)
                        node->leafdist = other->leafdist + 1;
                }
            }

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

                node->setPreState(state);
            }
        }

        void addDependency(ActionNodePtr pre, ActionNodePtr post)
        {
            boost::add_edge(pre->plan_index, post->plan_index, 1, dag_);

            std::size_t v = post->makespan + 1;
            if (v > pre->makespan)
                pre->makespan = v;

            if (pre->makespan > maxMakespan_)
                maxMakespan_ = pre->makespan;
        }

        bool isDependentOn(Vertex pre, Vertex eff) const
        {
            std::vector<Vertex> predecessors(nodes_.size(), getNullVertex());
            boost::dag_shortest_paths(dag_, pre, boost::predecessor_map(&predecessors[0]));
            return predecessors[eff] != eff;
        }

        bool isEdge(Vertex src, Vertex dst) const
        {
            auto edge = boost::edge(src, dst, dag_);
            return edge.second;
        }

        static Vertex getNullVertex()
        {
            return boost::graph_traits<Graph>::null_vertex();
        }

        std::vector<ActionNodePtr> nodes_;
        std::size_t maxMakespan_{0};

        const PlanPtr plan_;        ///< Plan to generate dependencies for.
        const ProblemPtr problem_;  ///< Problem domain.
        Graph dag_;                 ///< Dependency DAG
    };
}  // namespace pddlboat

#endif
