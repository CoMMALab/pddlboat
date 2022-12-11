/* Author: Zachary Kingston */

#ifndef PDDLBOAT_SOLVER_DEPENDENCY_
#define PDDLBOAT_SOLVER_DEPENDENCY_

#include <boost/graph/adjacency_list.hpp>

#include <pddlboat/macros.hpp>
#include <pddlboat/solver/domain.hpp>
#include <pddlboat/solver/planner.hpp>

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

        using Graph =                                                             //
            boost::adjacency_list<boost::vecS,                                    //
                                  boost::vecS,                                    //
                                  boost::bidirectionalS,                          //
                                  ActionNodePtr,                                  //
                                  boost::property<boost::edge_weight_t, double>,  //
                                  boost::no_property>;

        using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;

        class ActionNode
        {
            friend class PlanDependencyGraph;

        public:
            const ActionPtr action;        ///< Action taken at this node.
            const Assignment assignment;   ///< Assignment to this action for this node.
            const std::size_t plan_index;  ///< Index of this step in the original plan.

            const std::set<std::string> pre_pred;  ///< Precondition predicate set.
            const std::set<std::string> eff_pred;  ///< Effect predicate set.

            StatePtr pre_state{nullptr};  ///< State of the world before this action.
            StatePtr eff_state{nullptr};  ///< State of the world after this action.

            std::size_t makespan{0};  ///< Distance to final action.
            std::size_t leafdist{0};  ///< Distance from leaves.

            bool isEffected(const ActionNodePtr &pre) const;
            bool isPrior(const ActionNodePtr &eff) const;
            std::set<std::string> getInteraction(const ActionNodePtr &eff) const;
            std::ostream &toString(std::ostream &out) const;

        private:
            ActionNode(const Plan::Step &step, std::size_t index);
            ActionNode(const ActionNodePtr &node);

            void setState(const StatePtr &state);

            Vertex vertex;  ///< Vertex in graph
        };

        using ActionNodeList = std::vector<ActionNodePtr>;

        PlanDependencyGraph(const Plan &plan);
        PlanDependencyGraph(const PlanPtr &plan);
        PlanDependencyGraph(const PlanDependencyGraph &graph);

        void initialize();

        ActionNodePtr getPlanStep(std::size_t index) const;
        ActionNodeList getNodes() const;

        bool removeNode(const ActionNodePtr &node);
        void computeNodeStates();

        bool isParent(const ActionNodePtr &node, const ActionNodePtr &parent) const;
        bool isChild(const ActionNodePtr &node, const ActionNodePtr &child) const;
        bool isDescendant(const ActionNodePtr &node, const ActionNodePtr &descendant) const;
        bool isAncestor(const ActionNodePtr &node, const ActionNodePtr &ancestor) const;

        ActionNodeList getPredecessors(const ActionNodePtr &node) const;
        ActionNodeList getDescendants(const ActionNodePtr &node) const;
        ActionNodeList getParents(const ActionNodePtr &node) const;
        ActionNodeList getChildren(const ActionNodePtr &node) const;

        ActionNodeList getNodesWithMakespan(std::size_t makespan) const;
        ActionNodeList getNodesWithGreaterMakespan(std::size_t makespan) const;
        ActionNodeList getNodesWithLessMakespan(std::size_t makespan) const;
        // get nodes in descending makespan, e.g., farthest first
        ActionNodeList getNodesByMakespan() const;
        std::size_t getMaxMakespan() const;

        ActionNodeList getNodesWithLeafDistance(std::size_t leafdist) const;
        ActionNodeList getNodesWithGreaterLeafDistance(std::size_t leafdist) const;
        ActionNodeList getNodesWithLessLeafDistance(std::size_t leafdist) const;
        // get nodes in ascending leaf dist, e.g., farthest first
        ActionNodeList getNodesByLeafDistance() const;
        ActionNodeList getLeafs() const;
        std::size_t getMaxLeafDistance() const;

        struct DotFileOptions
        {
            enum Rank
            {
                MAKESPAN,
                LEAFDIST,
            };

            DotFileOptions(){};

            Rank ranking{MAKESPAN};
            bool action{true};
            bool index{true};
            bool colorLeafs{true};
            bool colorTerminals{true};
            bool stateToolTips{true};
            bool edgeToolTips{true};
            bool drawStart{true};
            bool drawEnd{true};
        };

        std::ostream &toDotFile(std::ostream &out, DotFileOptions options = DotFileOptions()) const;

    private:
        using NodeFunction = std::function<bool(const ActionNodePtr &node)>;

        void computeLeafDistance();

        ActionNodeList getNodesFilter(const NodeFunction &function = {}) const;

        void addDependency(ActionNodePtr pre, ActionNodePtr post);
        void addEdge(Vertex pre, Vertex post);

        bool isDependentOn(Vertex pre, Vertex eff) const;
        bool isEdge(Vertex src, Vertex dst) const;

        std::vector<Vertex> getChildrenVertex(Vertex src) const;
        std::vector<Vertex> getParentsVertex(Vertex src) const;

        ActionNodePtr &getActionNode(Vertex vertex);
        const ActionNodePtr &getActionNodeConst(Vertex vertex) const;
        static Vertex getNullVertex();

        ActionNodeList nodes_;      ///< Nodes in task plan.
        Plan plan_;                 ///< Plan to generate dependencies for.
        const ProblemPtr problem_;  ///< Problem domain.
        Graph dag_;                 ///< Dependency DAG
    };
}  // namespace pddlboat

#endif
