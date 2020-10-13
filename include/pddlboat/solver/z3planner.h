/* Author: Andrew Wells and Zachary Kingston */

#ifndef PDDLBOAT_Z3PLANNER_
#define PDDLBOAT_Z3PLANNER_

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

    PDDLBOAT_CLASS_FORWARD(GroundedAction)
    /** \brief A grounded action.
     */
    class GroundedAction
    {
    public:
        ActionPtr action;       ///< Action
        Assignment assignment;  ///< The grounding.
        std::string string;     ///< String representation of this assigned action.

        GroundedAction(const ActionPtr &action, const Assignment &assignment);
    };

    PDDLBOAT_CLASS_FORWARD(GroundedPredicate)
    /** \brief A grounded predicate.
     */
    struct GroundedPredicate
    {
        PredicateDefinitionPtr predicate;  ///< Derived predicate
        Assignment assignment;             ///< The grounding.
        std::string string;                ///< String representation of this assigned action.

        GroundedPredicate(const PredicateDefinitionPtr &action, const Assignment &assignment);
    };

    PDDLBOAT_CLASS_FORWARD(GroundedFluent)
    /** \brief A grounded fluent.
     */
    struct GroundedFluent
    {
        ObjectFunctionDefinitionPtr fluent;  ///< Derived predicate
        Assignment assignment;               ///< The grounding.
        std::string string;                  ///< String representation of this assigned action.

        GroundedFluent(const ObjectFunctionDefinitionPtr &fluent, const Assignment &assignment);
    };

    PDDLBOAT_CLASS_FORWARD(GroundedDerived)
    /** \brief A grounded derived predicate.
     */
    struct GroundedDerived
    {
        DerivedPredicateDefinitionPtr derived;  ///< Derived predicate
        Assignment assignment;                  ///< The grounding.
        std::string string;                     ///< String representation of this assigned action.

        GroundedDerived(const DerivedPredicateDefinitionPtr &action, const Assignment &assignment);
    };

    /** \brief A grounded action with information on whether it effects a predicate or not.
     */
    struct PredAction
    {
        GroundedActionPtr action;
        bool relies;
        bool effects;

        PredAction(const GroundedActionPtr &action, bool relies, bool effects);
    };

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

            bool parallel{true};       ///< Parallel solve through Z3.
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

        /** \brief Destructor.
         */
        ~Z3Planner();

        /** \brief Reset the planner.
         */
        void reset() override;

        /** \brief Clear internal data structures.
         */
        void clear();

        /** \brief Returns the supported domain requirements.
         *  \return The supported requirements.
         */
        Requirements::Keyword getSupportedRequirements() const override;

        /** \brief Solve the planning problem.
         *  \param[out] plan Found plan.
         *  \return True on success, false on failure.
         */
        bool solve(Plan &plan) override;

        /** \brief Block a state/action pair from being in the plan.
         *  \param[in] action Action to block.
         *  \param[in] assignment Grounding of action.
         *  \param[in] state State that action cannot occur on.
         */
        void blockStateAction(const ActionPtr &action, const Assignment &assignment,
                              const StatePtr &state) override;

        void blockStateActionSimultaneous(const ActionPtr &action1, const Assignment &assignment1, const ActionPtr &action2, const Assignment &assignment2,
                              const StatePtr &state);

        /** \brief Print out the current solver model.
         *  \param[in,out] Output stream.
         *  \param[in] only_true Only print true assignments.
         *  \return The output stream.
         */
        std::ostream &printModel(std::ostream &out, bool only_true = true) const;

        /** \brief Print out the current solver model as a smt2 file.
         *  \param[in,out] Output stream.
         *  \return The output stream.
         */
        std::ostream &printSMT2(std::ostream &out) const;

    private:
        /** \brief Add a step to the plan.
         *  \param[in] step Timestep to add.
         */
        void addStep(Time step, bool incremental = true);

        /** \brief From a solved Z3 instance, create a plan.
         *  \param[out] plan Plan to fill in.
         */
        bool buildPlan(Plan &plan, Time max) const;

        /** \brief Create a Z3 expression from an expression, given a grounding, at a time.
         *  \param[in] expr Expression to generate from.
         *  \param[in] assignment The grounding of the expression.
         *  \param[in] step Timestep to generate expression at.
         *  \return The generated Z3 expression.
         */
        z3::expr getExpression(const ExpressionPtr &expr, const Assignment &assignment,
                               const Time step) const;

        /** \brief Generate all action groundings and predicate effects.
         */
        void generateGroundings();

        /** \brief Generate all Z3 functions necessary.
         */
        void generateFunctions();

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

        /** \brief Helper function to add linear action implication chain.
         *  \param[in] step Step to generate mutual exclusions for.
         *  \param[in] forward Direction of chain.
         */
        void addMutualExclusionLinearHelper(const std::vector<PredAction> &actions, Time step,
                                            const std::string &prefix, bool forward);

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

        /** \brief Early termination step.
         */
        void addEarlyTermination(Time step);

        /** \brief Generate goal requirements at a step.
         */
        void addGoalExpression(Time step);

        /** \brief Generate all constraints at a step.
         */
        void addConstraints(Time step);

        /** \brief Generate start state requirements at first step.
         */
        void addStartState();

        Options options_;

        // diagnostic timers

        Diagnostics action_implications_;
        Diagnostics derived_predicates_;
        Diagnostics frame_axioms_;
        Diagnostics mutual_exclusions_;
        Diagnostics start_propositions_;
        Diagnostics early_termination_;
        Diagnostics constraint_clauses_;
        Diagnostics goal_expression_;

        // groundings

        std::vector<GroundedDerived> ground_derived_;      ///< All grounded derived predicates.
        std::vector<GroundedPredicate> ground_predicate_;  ///< All grounded unaffected predicates.
        std::vector<GroundedFluent> ground_fluent_;        ///< All grounded unaffected fluents.
        std::map<std::string, GroundedPredicate> predicate_map_;
        std::map<std::string, GroundedFluent> fluent_map_;

        /** \brief Add a state/action pair constraint at a time step.
         */
        void addConstraint(const ActionPtr &action, const Assignment &assignment, const StatePtr &state, Time step);

        void addConstraintSimultaneous(const ActionPtr &action1, const Assignment &assignment1, const ActionPtr &action2, const Assignment &assignment2, const StatePtr &state, Time step);


        std::vector<std::tuple<ActionPtr, Assignment, StatePtr>> constraints_;  ///< State/action constraints
        std::vector<std::tuple<ActionPtr, Assignment, ActionPtr, Assignment, StatePtr>> constraints_simultaneous_;  ///< State/action constraints


        std::vector<PredAction> ground_actions_;  ///< All grounded actions.
        std::map<std::string, std::vector<PredAction>> action_effects_;


        size_t push_count_{0}; // The number of push() calls since the last pop.

        /// Z3 Stuff

        z3::context *ctx_;      ///< Z3 Solver Context.
        z3::solver *solver_;    ///< Z3 Solver.
        Time current_step_{0};  ///< Current step the solve is on.

        PDDLBOAT_CLASS_FORWARD(Z3Function)
        PDDLBOAT_CLASS_FORWARD(Z3DataType)

        class Z3Function
        {
        public:
            std::string name;                 ///< Function name.
            std::vector<Z3DataTypePtr> args;  ///< Typed arguments.
            z3::sort_vector domains;          ///< Argument domain sorts.
            z3::sort range;                   ///< Return sort.
            z3::func_decl function;           ///< The function itself.

            void addTypeArgument(const Z3DataTypePtr &type);
            void addBoolArgument();
            void addIntArgument();
            void setTypeReturn(const Z3DataTypePtr &type);
            void make();

            z3::expr operator()() const;
            z3::expr operator()(const z3::expr &arg) const;
            z3::expr operator()(const z3::expr_vector &args) const;

            Z3Function(const std::string &name, const Z3Planner *planner);
            const Z3Planner *planner;
        };
        friend class Z3Function;

        class Z3DataType
        {
        public:
            std::string name;                           ///< Name of type.
            std::map<std::string, std::size_t> values;  ///< Values and indices of type members.
            z3::func_decl_vector consts;                ///< Constant functions.
            z3::func_decl_vector testers;               ///< Tester functions.
            z3::sort sort;                              ///< Domain sort.
            std::string parent{PDDLBOAT_OBJECT_CLASS};  ///< Parent class type.
            Z3Function upcast;                          ///< Function that casts type to parent type.

            z3::expr getConstant(const std::string &object) const;
            z3::expr getVariable(const std::string &name) const;

            Z3DataType(const std::string &string, const Z3Planner *planner);
            const Z3Planner *planner;
        };
        friend class Z3DataType;

        void addZ3DataType(const std::string &type);
        Z3DataTypePtr makeZ3DataType(const std::string &type) const;
        const Z3DataTypePtr &getZ3DataType(const std::string &type) const;
        z3::expr castZ3Type(const std::string &to, const z3::expr &e) const;
        void castZ3TypeArgs(z3::expr_vector &args, const Z3Function &function) const;

        Z3FunctionPtr makeZ3Function(const std::string &name, const VariableList &arguments) const;
        const Z3FunctionPtr &getZ3Predicate(const std::string &name) const;
        const Z3FunctionPtr &getZ3Action(const std::string &name) const;
        const Z3FunctionPtr &getZ3Fluent(const std::string &name) const;

        z3::expr getZ3Value(const Value &value, const Assignment &assignment, Time step) const;
        z3::expr_vector getZ3Values(const std::vector<Value> &bindings, const Assignment &assignment,
                                    Time step) const;

        z3::expr getZ3Value(const Variable &variable, const Assignment &assignment) const;
        z3::expr_vector getZ3Values(const VariableList &variables, const Assignment &assignment) const;

        z3::expr_vector getZ3Variables(const VariableList &parameters, Assignment &assignment) const;

        z3::expr toZ3Predicate(const PredicatePtr &predicate, const Assignment &assignment, Time step) const;
        z3::expr toZ3Predicate(const PredicateDefinitionPtr &predicate, const Assignment &assignment,
                               Time step) const;
        z3::expr toZ3Predicate(const DerivedPredicatePtr &predicate, const Assignment &assignment,
                               Time step) const;
        z3::expr toZ3Predicate(const GroundedPredicate &grounding, Time step) const;
        z3::expr toZ3Predicate(const GroundedDerived &grounding, Time step) const;
        z3::expr toZ3Action(const ActionPtr &action, const Assignment &assignment, Time step) const;
        z3::expr toZ3Fluent(const ObjectFunctionPtr &fluent, const Assignment &assignment, Time step) const;
        z3::expr toZ3Fluent(const ObjectFunctionDefinitionPtr &fluent, const Assignment &assignment,
                            Time step) const;
        z3::expr toZ3Fluent(const GroundedFluent &grounding, Time step) const;

        z3::expr toZ3Time(Time time) const;

        z3::expr getState(const StatePtr &state, Time step) const;

        std::map<std::string, Z3DataTypePtr> z3_types_;
        std::map<std::string, Z3FunctionPtr> z3_actions_;
        std::map<std::string, Z3FunctionPtr> z3_fluents_;
        std::map<std::string, Z3FunctionPtr> z3_predicates_;
        std::map<std::string, Z3FunctionPtr> z3_effect_exclusion_;
        Z3FunctionPtr mutex_auxilliary_;
    };

}  // namespace pddlboat

#endif  // include guard
