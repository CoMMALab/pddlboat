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
        /** \brief Planner options.
         */
        struct Options
        {
            struct
            {
                Time max{20};  ///< Maximum planning horizon.
                Time min{0};   ///< Minimum planning horizon for incremental solving.
            } horizon;         ///< Planning horizons.

            bool parallel{false};      ///< Solve in parallel is supported by Z3.
            bool diagnostics{false};   ///< Print diagnostics.
            bool dump_clauses{false};  ///< Dump generated Z3 clauses.
            bool incremental{true};    ///< Solve the problem incrementally.
            bool simultaneous{true};   ///< Allow simultaneous actions.

            Options() = default;
        };

        /** \brief Constructor.
         *  \param[in] problem Problem to solve.
         *  \param[in] options Solver options.
         */
        Z3Planner(const ProblemPtr problem, const Options &options);

        /** \brief Returns the supported domain requirements.
         *  \return The supported requirements.
         */
        Requirements::Keyword getSupportedRequirements() const override;

        /** \brief Solve the planning problem.
         *  \param[out] plan Found plan.
         *  \return True on success, false on failure.
         */
        bool solve(Plan &plan) override;

        /** \brief Print out the current solver model.
         *  \param[in,out] Output stream.
         *  \param[in] only_true Only print true assignments.
         *  \return The output stream.
         */
        std::ostream &printModel(std::ostream &out, bool only_true = true) const;

    private:
        /** \brief Add a step to the plan.
         *  \param[in] step Timestep to add.
         */
        void addStep(Time step, bool incremental = true);

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

        /** \brief Generate all mutual exclusion constraints for the plan using a linear implication chain,
         * but only on conflicting predicates.
         *  \param[in] step Step to generate mutual exclusions for.
         */
        void addMutualExclusionLinearSimultaneous(Time step);

        /** \brief Generate all mutual exclusion constraints for the plan using a linear implication chain.
         *  \param[in] step Step to generate mutual exclusions for.
         */
        void addMutualExclusionLinear(Time step);

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

        Options options_;

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

        /** \brief A grounded action.
         */
        PDDLBOAT_CLASS_FORWARD(GroundedAction)
        class GroundedAction
        {
        public:
            ActionPtr action;       ///< Action
            Assignment assignment;  ///< The grounding.
            std::string string;     ///< String representation of this assigned action.

            GroundedAction(const ActionPtr &action, const Assignment &assignment);
        };

        /** \brief A grounded predicate.
         */
        struct GroundedPredicate
        {
            PredicateDefinitionPtr predicate;  ///< Derived predicate
            Assignment assignment;             ///< The grounding.
            std::string string;                ///< String representation of this assigned action.

            GroundedPredicate(const PredicateDefinitionPtr &action, const Assignment &assignment);
        };

        /** \brief A grounded derived predicate.
         */
        struct GroundedDerived
        {
            DerivedPredicateDefinitionPtr derived;  ///< Derived predicate
            Assignment assignment;                  ///< The grounding.
            std::string string;                     ///< String representation of this assigned action.

            GroundedDerived(const DerivedPredicateDefinitionPtr &action, const Assignment &assignment);
        };

        std::vector<GroundedDerived> ground_derived_;  ///< All grounded derived predicates.
        std::vector<GroundedPredicate> ground_pred_;   ///< All grounded unaffected predicates.

        struct PredAction
        {
            GroundedActionPtr action;
            bool relies;
            bool effects;

            PredAction(const GroundedActionPtr &action, bool relies, bool effects);
        };

        std::vector<PredAction> ground_actions_;  ///< All grounded actions.
        std::map<std::string, std::vector<PredAction>> predicate_actions_;

        /** \brief Helper function to add linear action implication chain.
         *  \param[in] step Step to generate mutual exclusions for.
         *  \param[in] forward Direction of chain.
         */
        void addMutualExclusionLinearHelper(const std::vector<PredAction> &actions, Time step,
                                            const std::string &prefix, bool forward);
    };

}  // namespace pddlboat

#endif  // include guard
