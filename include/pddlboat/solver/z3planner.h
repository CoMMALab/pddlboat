/* Author: Andrew Wells and Zachary Kingston */

#ifndef PDDLBOAT_Z3_PLANNER_
#define PDDLBOAT_Z3_PLANNER_

#include <iostream>
#include <sstream>

#include <z3++.h>

#include <pddlboat/utility.h>
#include <pddlboat/parser/parser.h>
#include <pddlboat/solver/domain.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/planner.h>

namespace pddlboat
{
    /** \brief Time step in a plan.
     */
    using Time = std::size_t;

    /** \brief A planner that uses Z3 to solve SAT plan representations of a planning problem.
     */
    class Z3Planner : public Planner
    {
    public:
        /** \brief Constructor.
         *  \param[in] problem Problem to solve.
         *  \param[in] max_horizon Maximum planning horizon.
         *  \param[in] min_horizon Minimum planning horizon.
         *  \param[in] parallel Use Z3's built in parallel solver.
         *  \param[in] diagnostics If true, print diagnostics.
         *  \param[in] dump_clauses If true, print all clauses in plan.
         */
        Z3Planner(const ProblemPtr problem, Time max_horizon, Time min_horizon = 0, bool parallel = false,
                  bool diagnostics = false, bool dump_clauses = false);

        /** \brief Returns the supported domain requirements.
         *  \return The supported requirements.
         */
        Requirements::Keyword getSupportedRequirements() const override;

        /** \brief Solve the planning problem.
         *  \param[out] plan Found plan.
         *  \return True on success, false on failure.
         */
        bool solve(Plan &plan) override;

    private:
        /** \brief From a solved Z3 instance, create a plan.
         *  \param[out] plan Plan to fill in.
         */
        void buildPlan(Plan &plan) const;

        /** \brief Create a Z3 boolean from a string ID.
         *  \param[in] string String to convert.
         *  \return A Z3 boolean with the same ID as the string.
         */
        z3::expr toBool(const std::string &string) const;

        /** \brief Returns true if a Z3 expression is true.
         *  \param[in] expr Expression to check if true.
         *  \return True if the expression is true, false otherwise.
         */
        bool isTrue(const z3::expr &expr) const;

        /** \brief Suffix a timestamp onto a string.
         *  \param[in] string String to suffix.
         *  \param[in] time Time to suffix.
         *  \return The time-suffixed string.
         */
        std::string addTimeToString(const std::string &string, Time time) const;

        /** \brief Create a Z3 expression from an expression, given a grounding, at a time.
         *  \param[in] expr Expression to generate from.
         *  \param[in] assignment The grounding of the expression.
         *  \param[in] step Timestep to generate expression at.
         *  \return The generated Z3 expression.
         */
        z3::expr exprAtTime(const ExpressionPtr &expr, const Assignment &assignment, const Time step) const;

        /** \brief Generate all action groundings and predicate effects.
         */
        void generateGroundings();

        /** \brief Generate action implication constraints for the plan at a step.
         *  \param[in] step Step to generate implications for.
         */
        void addActionImplications(Time step);

        /** \brief Generate all derived predicate conditions.
         *  \param[in] step Step to generate derived predicates for.
         */
        void addDerivedPredicates(Time step);

        /** \brief Generate all frame axiom constraints for the plan at a step.
         *  \param[in] step Step to generate frame axioms for.
         */
        void addFrameAxioms(Time step);

        /** \brief Generate all mutual exclusion constraints for the plan using integer cardinality.
         *  \param[in] step Step to generate mutual exclusions for.
         */
        void addMutualExclusionCardinality(Time step);

        /** \brief Generate all mutual exclusion constraints for the plan.
         *  \param[in] step Step to generate mutual exclusions for.
         */
        void addMutualExclusion(Time step);

        /** \brief Generate goal requirements at a step.
         */
        void addGoalExpression(Time step);

        /** \brief Generate start state requirements at first step.
         */
        void addStartState();

        const Time max_horizon_;   ///< Planning horizon.
        const Time min_horizon_;   ///< Planning horizon.
        const bool diagnostics_;   ///< If true, print diagnostics.
        const bool dump_clauses_;  ///< If true, print constituent clauses.

        z3::context *ctx_{nullptr};    ///< Z3 Solver Context.
        z3::solver *solver_{nullptr};  ///< Z3 Solver.

        /** \brief Diagnostic Information
         */
        struct Diagnostics
        {
            boost::timer::cpu_timer onestep;
            boost::timer::cpu_timer totaltime;
            std::size_t count;
            std::size_t total;

            Diagnostics()
            {
                stop();
            }

            void start()
            {
                onestep.start();
                totaltime.resume();
                count = 0;
            }

            void inc()
            {
                count++;
                total++;
            }

            void stop()
            {
                onestep.stop();
                totaltime.stop();
            }
        };

        Diagnostics action_implications_;
        Diagnostics derived_predicates_;
        Diagnostics frame_axioms_;
        Diagnostics mutual_exclusions_;
        Diagnostics start_propositions_;
        Diagnostics goal_expression_;

        std::map<std::string, std::vector<std::string>> predicate_effects_;  ///< Mapping of predicates to
                                                                             ///< actions which effect them.

        /** \brief A grounded action.
         */
        struct GroundedAction
        {
            ActionPtr action;       ///< Action
            Assignment assignment;  ///< The grounding.
            std::string string;     ///< String representation of this assigned action.

            GroundedAction(const ActionPtr &action, const Assignment &assignment, const std::string &string);
        };

        /** \brief A grounded derived predicate.
         */
        struct GroundedDerived
        {
            DerivedPredicateDefinitionPtr derived;  ///< Derived predicate
            Assignment assignment;                  ///< The grounding.
            std::string string;                     ///< String representation of this assigned action.

            GroundedDerived(const DerivedPredicateDefinitionPtr &action, const Assignment &assignment,
                            const std::string &string);
        };

        std::vector<GroundedAction> ground_actions_;   ///< All grounded actions (without time step).
        std::vector<GroundedDerived> ground_derived_;  ///< All grounded derived predicates (without time
                                                       ///< step).
    };

}  // namespace pddlboat

#endif  // include guard
