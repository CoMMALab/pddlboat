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
        const Time horizon_;       ///< Planning horizon.
        const bool diagnostics_;   ///< If true, print diagnostics.
        const bool dump_clauses_;  ///< If true, print constituent clauses.

        z3::context *ctx_{nullptr};    ///< Z3 Solver Context.
        z3::solver *solver_{nullptr};  ///< Z3 Solver.

        // Diagnostic counters
        unsigned int action_imply_count_;       ///< Number of action implication clauses per step.
        unsigned int action_imply_total_;       ///< Number of action implications total.
        unsigned int frame_axiom_count_;        ///< Number of frame axioms per step.
        unsigned int frame_axiom_total_;        ///< Number of frame axioms total.
        unsigned int mutual_exclusion_count_;   ///< Number of mutual exclusion clauses per step.
        unsigned int mutual_exclusion_total_;   ///< Number of mutual exclusion clauses total.
        unsigned int start_proposition_count_;  ///< Number of start state propositions.

        std::vector<std::string> ground_actions_;  ///< All grounded actions (without time step).

    public:
        /** \brief Constructor.
         *  \param[in] problem Problem to solve.
         *  \param[in] horizon Maximum planning horizon.
         *  \param[in] diagnostics If true, print diagnostics.
         *  \param[in] dump_clauses If true, print all clauses in plan.
         */
        Z3Planner(const ProblemPtr problem, const Time horizon, bool diagnostics = false,
                  bool dump_clauses = false);

        /** \brief Solve the planning problem.
         *  \param[out] plan Found plan.
         *  \return True on success, false on failure.
         */
        bool solve(Plan &plan) override;

    private:
        /** \brief From a solved Z3 instance, create a plan.
         *  \param[out] plan Plan to fill in.
         */
        void buildPlan(Plan &plan);

        /** \brief Create a Z3 boolean from a string ID.
         *  \param[in] string String to convert.
         *  \return A Z3 boolean with the same ID as the string.
         */
        z3::expr toBool(const std::string &string);

        /** \brief Suffix a timestamp onto a string.
         *  \param[in] string String to suffix.
         *  \param[in] time Time to suffix.
         *  \return The time-suffixed string.
         */
        const std::string addTimeToString(const std::string &string, Time time);

        /** \brief Create a Z3 expression from an expression, given a grounding, at a time.
         *  \param[in] expr Expression to generate from.
         *  \param[in] assignment The grounding of the expression.
         *  \param[in] step Timestep to generate expression at.
         *  \return The generated Z3 expression.
         */
        z3::expr exprAtTime(const ExpressionPtr &expr, const Assignment &assignment, const Time step);

        /** \brief Returns true if a predicate string (from PredicateDefinition::toID()) is within a grounded
         * expression.
         *
         *  This is the part of the core bottleneck of frameAxioms(). Should be changed later.
         *
         *  \param[in] expr Expression to search.
         *  \param[in] assignment Grounding of the expression.
         *  \param[in] predicate Predicate ID to check for.
         *  \return True if grounded predicate is in grounded expression.
         */
        bool predicateInEffect(const ExpressionPtr &expr, const Assignment &assignment,
                               const std::string &predicate);

        /** \brief Retrieves an expression that is a disjunction of all actions that could change a predicate.
         *
         *  This is the part of the core bottleneck of frameAxioms(). Should be changed later.
         *
         *  \param[in] predicate Predicate ID string.
         *  \param[in] step_before The step before the predicate is changed.
         *  \return Disjunction of all actions that effect a predicate.
         */
        z3::expr getPossibleChangeActions(const std::string &predicate, const Time step_before);

        /** \brief Generate all action implication constraints for the plan.
         */
        void actionImplications();

        /** \brief Generate all frame axiom constraints for the plan.
         */
        void frameAxioms();

        /** \brief Generate all mutual exclusion constraints for the plan.
         */
        void mutualExclusion();

        /** \brief Generate goal requirements at horizon.
         */
        void goalExpression();

        /** \brief Generate start state requirements.
         */
        void startState();

        /** \brief Build Z3 plan.
         */
        void build();
    };

}  // namespace pddlboat

#endif  // include guard
