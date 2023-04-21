/* Author: Andrew Wells and Zachary Kingston */

#include <z3++.h>
#include <pddlboat/solver/z3planner.hpp>

//
// Anonymous functions
//

namespace
{
    z3::expr imply(const z3::expr &a, const z3::expr &b)
    {
        return (not a) or b;
    }

    bool isTrue(const z3::expr &expr)
    {
        // As is_true() is apparently a bleeding edge feature
        return expr.is_app() and Z3_OP_TRUE == expr.decl().decl_kind();
    }

    z3::expr_vector copy(const z3::expr_vector &v)
    {
        z3::expr_vector r(v.ctx());
        for (std::size_t i = 0; i < v.size(); ++i)
            r.push_back(v[i]);
        return r;
    }

    std::string getType(const z3::expr &e)
    {
        auto sort = e.get_sort();

        Z3_symbol s = Z3_get_sort_name(sort.ctx(), sort);
        auto sc = z3::symbol(sort.ctx(), s);
        return sc.str();
    }
}  // namespace

namespace pddlboat
{
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

    /** \brief A grounded action with information on whether it effects a predicate or not.
     */
    struct PredAction
    {
        GroundedActionPtr action;
        bool relies;
        bool effects;

        PredAction(const GroundedActionPtr &action, bool relies, bool effects)
          : action(action), relies(relies), effects(effects)
        {
        }
    };

    class Z3PlannerInternal
    {
    public:
        Z3PlannerInternal(const ProblemPtr &problem, const Z3Planner::Options &options)
          : problem_(problem), options_(options)
        {
            reset();
        }

        ~Z3PlannerInternal()
        {
            clear();
        }

        bool solve(Plan &plan, unsigned int timeout);
        void reset();
        void clear();

        void blockStateActions(const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
                               const StatePtr &state);

        void
        blockExpressionActions(const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
                               const ExpressionPtr &expression, const Assignment &exp_assignment);

        void blockStateActionSimultaneous(
            const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
            const StatePtr &state);

        void addConstraint(Planner::ConstraintTime time, const ExpressionPtr &expression, bool value = false);

        std::ostream &printModelReadable(std::ostream &out, bool only_true) const;
        std::ostream &printModel(std::ostream &out, bool only_true) const
        {
            return printModelReadable(out, only_true);
            // auto model = getSolverModel();
            // out << model << std::endl;

            // return out;
        }

        std::ostream &printSMT2(std::ostream &out) const
        {
            if (solver_)
                out << solver_->to_smt2();
            return out;
        }

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

        void addDomainAxioms(Time step);

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
        void addStateActionConstraints(Time step);

        /** \brief Generate start state requirements at first step.
         */
        void addStartState();

        ProblemPtr problem_;          ///< Problem to solve.
        Z3Planner::Options options_;  ///< Options for computation.

        // diagnostic timers

        Diagnostics action_implications_;
        Diagnostics derived_predicates_;
        Diagnostics domain_axioms_;
        Diagnostics frame_axioms_;
        Diagnostics mutual_exclusions_;
        Diagnostics start_propositions_;
        Diagnostics early_termination_;
        Diagnostics constraint_clauses_;
        Diagnostics goal_expression_;

        // groundings

        std::vector<GroundedAxiom> ground_axioms_;              ///< All grounded domain axioms.
        std::vector<GroundedDerivedPredicate> ground_derived_;  ///< All grounded derived predicates.
        std::vector<GroundedPredicate> ground_predicate_;       ///< All grounded unaffected predicates.
        std::vector<GroundedFunction> ground_fluent_;           ///< All grounded unaffected fluents.

        std::map<std::string, GroundedPredicate> predicate_map_;  ///< Map of IDs to grounded predicates.
        std::map<std::string, GroundedFunction> fluent_map_;      ///< Map of IDs to fluents.

        /** \brief Add a state/actions pair constraint at a time step.
         */
        void addStateActionConstraint(const StatePtr &state, const std::set<size_t> &ground_indices,
                                      Time step, bool simultaneous = false);

        void addExpConstraint(const GroundedExpression &exp, const std::set<size_t> &ground_indices,
                              Time step);
        void addExpStateConstraint(const ExpressionPtr &exp, bool value, Time step);

        /** \brief Map of state to disallowed actions. Set is of indices into ground_actions_ (the list of all
         * ground actions). */
        StateMap<std::set<std::size_t>> constraints_;

        /** \brief Pairs of states to the set of actions that cannot occur simultaneously at that state. */
        std::vector<std::pair<StatePtr, std::set<std::size_t>>> simultaneous_constraints_;

        /** \brief Pairs of expressions that, if true, imply these actions cannot occur. */
        std::vector<std::pair<GroundedExpression, std::set<std::size_t>>> expression_constraints_;

        /** \brief State constraint expressions, with time specifiers and values. */
        std::vector<std::pair<ExpressionPtr, bool>> end_state_constraints_;
        std::vector<std::pair<ExpressionPtr, bool>> all_state_constraints_;

        /** get the index of a grounded action in the grounded action list */
        std::size_t getGroundActionIndex(const ActionPtr &action, const Assignment &assignment) const;

        std::vector<PredAction> ground_actions_;  ///< All grounded actions.
        std::map<std::string, std::vector<PredAction>> action_effects_;

        // Z3 Stuff
        z3::context *ctx_{nullptr};         ///< Z3 Solver Context.
        z3::solver *solver_{nullptr};       ///< Z3 Solver.
        z3::optimize *optimizer_{nullptr};  ///< Z3 Optimizer.

        // Add a clause to either the solver / optimizer
        void addClause(const z3::expr &clause) const;
        bool solverCheck() const;

        void solverPush() const;
        void solverPop() const;

        z3::model getSolverModel() const;
        z3::stats getSolverStats() const;

        bool solved_{false};
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

            void setBoolReturn();
            void setIntReturn();
            void setTypeReturn(const Z3DataTypePtr &type);

            void make();

            z3::expr operator()() const;
            z3::expr operator()(const z3::expr &arg) const;
            z3::expr operator()(const z3::expr_vector &args) const;

            Z3Function(const std::string &name, const Z3PlannerInternal *planner);
            const Z3PlannerInternal *planner;
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

            Z3DataType(const std::string &string, const Z3PlannerInternal *planner);
            const Z3PlannerInternal *planner;
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

        z3::expr toZ3Expr(const Z3FunctionPtr &function, z3::expr_vector &values, Time step) const;

        z3::expr toZ3Predicate(const PredicatePtr &predicate, const Assignment &assignment, Time step) const;
        z3::expr toZ3Predicate(const PredicateDefinitionPtr &predicate, const Assignment &assignment,
                               Time step) const;
        z3::expr toZ3Predicate(const DerivedPredicatePtr &predicate, const Assignment &assignment,
                               Time step) const;
        z3::expr toZ3Predicate(const GroundedPredicate &grounding, Time step) const;
        z3::expr toZ3Predicate(const GroundedDerivedPredicate &grounding, Time step) const;

        z3::expr toZ3Action(const ActionPtr &action, const Assignment &assignment, Time step) const;

        z3::expr toZ3Fluent(const std::string &fluent, const std::vector<Value> &values,
                            const Assignment &assignment, Time step) const;
        z3::expr toZ3Fluent(const std::string &fluent, const VariableList &variables,
                            const Assignment &assignment, Time step) const;

        z3::expr toZ3Fluent(const NumericFunctionPtr &fluent, const Assignment &assignment, Time step) const;
        z3::expr toZ3Fluent(const NumericFunctionDefinitionPtr &fluent, const Assignment &assignment,
                            Time step) const;
        z3::expr toZ3Fluent(const GroundedNumericFunction &grounding, Time step) const;

        z3::expr toZ3Fluent(const ObjectFunctionPtr &fluent, const Assignment &assignment, Time step) const;
        z3::expr toZ3Fluent(const ObjectFunctionDefinitionPtr &fluent, const Assignment &assignment,
                            Time step) const;
        z3::expr toZ3Fluent(const GroundedObjectFunction &grounding, Time step) const;

        z3::expr toZ3Fluent(const FunctionDefinitionPtr &fluent, const Assignment &assignment,
                            Time step) const;
        z3::expr toZ3Fluent(const GroundedFunction &grounding, Time step) const;

        z3::expr toZ3Time(Time time) const;
        z3::expr toZ3Integer(double value) const;

        z3::expr getState(const StatePtr &state, Time step) const;

        std::map<std::string, Z3DataTypePtr> z3_types_;
        std::map<std::string, Z3FunctionPtr> z3_actions_;
        std::map<std::string, Z3FunctionPtr> z3_fluents_;
        std::map<std::string, Z3FunctionPtr> z3_predicates_;
        std::map<std::string, Z3FunctionPtr> z3_effect_exclusion_;
        Z3FunctionPtr mutex_auxilliary_;
    };
}  // namespace pddlboat

using namespace pddlboat;

//
// Z3Planner
//

Z3Planner::Z3Planner(const ProblemPtr &problem, const Options &options)
  : Planner(problem), internal_(std::make_shared<Z3PlannerInternal>(problem, options))
{
    checkRequirements();
}

void Z3Planner::reset()
{
    internal_->reset();
}

Requirements::Keyword Z3Planner::getSupportedRequirements() const
{
    return Requirements::ADL                   //
           | Requirements::DERIVED_PREDICATES  //
           | Requirements::OBJECT_FLUENTS      //
           | Requirements::NUMERIC_FLUENTS | Requirements::DOMAIN_AXIOMS;
}

bool Z3Planner::solve(Plan &plan)
{
    return solve(plan, 0);
}

bool Z3Planner::solve(Plan &plan, unsigned int timeout)
{
    return internal_->solve(plan, timeout);
}

void Z3Planner::blockStateActions(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions, const StatePtr &state)
{
    internal_->blockStateActions(actions, state);
}

void Z3Planner::blockExpressionActions(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
    const ExpressionPtr &expression, const Assignment &exp_assignment)
{
    internal_->blockExpressionActions(actions, expression, exp_assignment);
}

void Z3Planner::blockStateActionsSimultaneous(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions, const StatePtr &state)
{
    internal_->blockStateActionSimultaneous(actions, state);
}

void Z3Planner::addConstraint(Planner::ConstraintTime time, const ExpressionPtr &expression, bool value)
{
    internal_->addConstraint(time, expression, value);
}

std::ostream &Z3Planner::printModel(std::ostream &out, bool only_true) const
{
    return internal_->printModel(out, only_true);
}

std::ostream &Z3Planner::printSMT2(std::ostream &out) const
{
    return internal_->printSMT2(out);
}

///
/// Z3PlannerInternal
///

void Z3PlannerInternal::reset()
{
    z3::set_param("parallel.enable", options_.parallel);

    clear();

    ctx_ = new z3::context;

    if (options_.optimize and problem_->metric)
        optimizer_ = new z3::optimize(*ctx_);
    else
        solver_ = new z3::solver(*ctx_);

    generateFunctions();
    generateGroundings();

    // setup initial problem state
    addStartState();
    addDerivedPredicates(0);

    solverCheck();
    solverPush();

    // add all predicates up to the minimum horizon to consider
    for (Time step = 0; step < options_.horizon_min; ++step)
        addStep(step, false);

    current_step_ = options_.horizon_min;
    solved_ = false;
}

void Z3PlannerInternal::clear()
{
    z3_types_.clear();
    z3_actions_.clear();
    z3_fluents_.clear();
    z3_predicates_.clear();
    z3_effect_exclusion_.clear();
    mutex_auxilliary_.reset();

    if (optimizer_)
        delete optimizer_;

    if (solver_)
        delete solver_;

    delete ctx_;
}

bool Z3PlannerInternal::solve(Plan &plan, unsigned int timeout)
{
    // NOTE: This does not print diagnostics or log solve times for idempotent calls to solve. This
    // may not be desirable behavior?
    if (solved_)
    {
        return buildPlan(plan, current_step_);
    }

    if (timeout > 0)
    {
        if (solver_)
        {
            z3::params p(*ctx_);
            p.set("timeout", timeout);

            solver_->set(p);
        }

        if (optimizer_)
            std::cerr << "Z3 optimizer does not understand timeout, ignoring!" << std::endl;
    }

    boost::timer::cpu_timer t;

    // incrementally solve up to maximum horizon
    solved_ = false;

    // already have begun solving, incremental time
    if (current_step_ != 0)
    {
        solverCheck();
        solverPush();

        addGoalExpression(current_step_);

        auto result = solverCheck();
        solved_ = result == z3::sat;
        if (not solved_)
            solverPop();  // remove goal
    }

    for (; current_step_ < options_.horizon_max and not solved_; ++current_step_)
    {
        boost::timer::cpu_timer tl;

        addStep(current_step_, true);
        solverCheck();
        solverPush();

        addGoalExpression(current_step_ + 1);

        auto result = solverCheck();

        tl.stop();
        if (options_.diagnostics)
            std::cout << "Solving Step " << current_step_ << " Z3 Instance took: " << io::getTiming(tl)
                      << std::endl;

        solved_ = result == z3::sat;
        if (not solved_)
            solverPop();
    }

    t.stop();
    if (options_.diagnostics)
    {
        // Action Implications
        std::cout << log::format("Z3 has %1% action implication clauses per step (%2% total)",
                                 action_implications_.count, action_implications_.total)
                  << std::endl;
        std::cout << "Total action implication time took: " << io::getTiming(action_implications_.totaltime)
                  << std::endl;

        // Frame Axioms
        std::cout << log::format("Z3 has %1% frame axiom clauses per step (%2% total)", frame_axioms_.count,
                                 frame_axioms_.total)
                  << std::endl;
        std::cout << "Total frame axiom time took: " << io::getTiming(frame_axioms_.totaltime) << std::endl;

        // Derived Predicates
        std::cout << log::format("Z3 has %1% derived predicate clauses per step (%2% total)",
                                 derived_predicates_.count, derived_predicates_.total)
                  << std::endl;
        std::cout << "Total derived predicate time took: " << io::getTiming(frame_axioms_.totaltime)
                  << std::endl;

        // Mutual Exclusion
        std::cout << log::format("Z3 has %1% mutual exclusion clauses per step (%2% total)",
                                 mutual_exclusions_.count, mutual_exclusions_.total)
                  << std::endl;
        std::cout << "Total mutual exclusion time took: " << io::getTiming(mutual_exclusions_.totaltime)
                  << std::endl;

        std::cout << "Solving Z3 Instance took: " << io::getTiming(t) << std::endl;
        std::cout << getSolverStats() << std::endl;
    }

    plan.setTimeTaken("action_implications",
                      io::getElapsedMilliseconds(action_implications_.totaltime) / 1000.);
    plan.setTimeTaken("frame_axioms", io::getElapsedMilliseconds(frame_axioms_.totaltime) / 1000.);
    plan.setTimeTaken("mutual_exclusion", io::getElapsedMilliseconds(mutual_exclusions_.totaltime) / 1000.);

    if (solved_)
    {
        plan.setTimeTaken("solve", io::getElapsedMilliseconds(t) / 1000.);
        return buildPlan(plan, current_step_);
    }

    return false;
}

void Z3PlannerInternal::blockStateActions(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions, const StatePtr &state)
{
    if (solved_)
    {
        solverPop();
        solved_ = false;
    }

    // get indices of all actions to block in the ground action list
    std::set<std::size_t> toBlock;
    for (const auto &[action, assignment] : actions)
        toBlock.emplace(getGroundActionIndex(action, assignment));

    // get a clean clone of the state to access map
    const auto &copy = state->cleanClone();

    // either emplace or insert the indices to block
    auto [it, r] = constraints_.emplace(copy, toBlock);
    if (not r)
        it->second.insert(toBlock.begin(), toBlock.end());

    // block up till current time step
    for (Time step = 0; step < current_step_; ++step)
        addStateActionConstraint(it->first, it->second, step);
}

void Z3PlannerInternal::blockExpressionActions(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
    const ExpressionPtr &expression, const Assignment &exp_assignment)
{
    if (solved_)
    {
        solverPop();
        solved_ = false;
    }

    // get indices of all actions to block in the ground action list
    std::set<std::size_t> toBlock;
    for (const auto &[action, assignment] : actions)
        toBlock.emplace(getGroundActionIndex(action, assignment));

    // either emplace or insert the indices to block
    auto it = expression_constraints_.emplace_back(std::piecewise_construct,
                                                   std::forward_as_tuple(expression, exp_assignment),
                                                   std::forward_as_tuple(toBlock));

    // block up till current time step
    for (Time step = 0; step < current_step_; ++step)
        addExpConstraint(it.first, it.second, step);
}

void Z3PlannerInternal::blockStateActionSimultaneous(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions, const StatePtr &state)
{
    if (solved_)
    {
        solverPop();
        solved_ = false;
    }

    // get indices of all actions to block in the ground action list
    std::set<std::size_t> toBlock;
    for (const auto &[action, assignment] : actions)
        toBlock.emplace(getGroundActionIndex(action, assignment));

    // get a clean clone of the state to access map
    const auto &copy = state->cleanClone();
    simultaneous_constraints_.emplace_back(copy, toBlock);

    // block up till current time step
    for (Time step = 0; step < current_step_; ++step)
        addStateActionConstraint(copy, toBlock, step, true);
}

void Z3PlannerInternal::addConstraint(Planner::ConstraintTime time, const ExpressionPtr &expression,
                                      bool value)
{
    if (solved_)
    {
        solverPop();
        solved_ = false;
    }

    if (time == Planner::AT_END)
    {
        end_state_constraints_.emplace_back(expression, value);
        addExpStateConstraint(expression, value, current_step_);
    }
    else if (time == Planner::OVER_ALL)
    {
        all_state_constraints_.emplace_back(expression, value);

        for (Time step = 0; step < current_step_; ++step)
            addExpStateConstraint(expression, value, step);
    }
}

void Z3PlannerInternal::addStep(Time step, bool incremental)
{
    // At each step, must add:
    // (1) Actions imply their preconditions are true and effects are true
    addActionImplications(step);

    // (2) Predicates do not change unless an action changes them
    addFrameAxioms(step);

    addDomainAxioms(step);

    if (options_.simultaneous)
        // (3) Only actions that don't conflict, if enabled
        addMutualExclusionLinearSimultaneous(step);
    else
        // (3) Only one action at a time
        addMutualExclusionLinear(step);

    // (4) Early termination check
    if (not incremental)
        addEarlyTermination(step);

    // (5) Derived predicates take on their value
    addDerivedPredicates(step + 1);

    // We need to do this last so popping constraints works correctly?
    // (6) Add existing planning constraints
    addStateActionConstraints(step);
}

std::ostream &Z3PlannerInternal::printModelReadable(std::ostream &out, bool only_true) const
{
    auto model = getSolverModel();
    for (std::size_t i = 0; i <= current_step_; ++i)
    {
        out << "TIMESTEP:" << i << std::endl;
        for (const auto &[name, predicate] : problem_->domain->predicates)
        {
            if (problem_->domain->isDerivedPredicate(name))
                continue;

            const auto &assignments = problem_->getGroundings(predicate->variables);
            for (const auto &assignment : assignments)
            {
                if (isTrue(model.eval(toZ3Predicate(predicate, assignment, i))))
                    out << predicate->toID(assignment) << std::endl;
                else if (not only_true)
                    out << "not " << predicate->toID(assignment) << std::endl;
            }
        }
    }

    return out;
}

bool Z3PlannerInternal::buildPlan(Plan &plan, Time max) const
{
    auto model = getSolverModel();
    for (std::size_t i = 0; i < max; ++i)
    {
        for (const auto &[name, action] : problem_->domain->actions)
        {
            const auto &groundings = problem_->getGroundings(action->parameters);
            for (const auto &grounding : groundings)
            {
                if (isTrue(model.eval(toZ3Action(action, grounding, i))))
                {
                    std::vector<std::string> args;
                    for (const auto &parameter : action->parameters)
                        args.emplace_back(grounding.assigned(parameter.name)->name);

                    try
                    {
                        plan.insertStep(name, args, i);
                    }
                    catch (std::exception &e)
                    {
                        std::cerr << e.what() << std::endl;
                        return false;
                    }
                }
            }
        }
    }

    return plan.validate();
}

/**
 * Recurses down the tree and builds a z3::expr for the expression, replacing atoms with their
 * z3 bool_constants. We require that the expression does not contain any actions (all terminals
 * are state props).
 */
z3::expr Z3PlannerInternal::getExpression(const ExpressionPtr &expr, const Assignment &assignment,
                                          Time step) const
{
    const auto &type = expr->type();
    switch (type)
    {
        case Expression::Type::PREDICATE:
        {
            const auto &predicate_expr = std::static_pointer_cast<Predicate>(expr);
            return toZ3Predicate(predicate_expr, assignment, step);
        }
        case Expression::Type::DERIVED:
        {
            const auto &predicate_expr = std::static_pointer_cast<DerivedPredicate>(expr);
            return toZ3Predicate(predicate_expr, assignment, step);
        }
        case Expression::Type::AND:
        {
            const auto &and_expr = std::static_pointer_cast<And>(expr);

            z3::expr exp = ctx_->bool_val(true);
            for (const auto &term : and_expr->getTerms())
                exp = exp and getExpression(term, assignment, step);

            return exp;
        }
        case Expression::Type::OR:
        {
            const auto &or_expr = std::static_pointer_cast<Or>(expr);

            z3::expr exp = ctx_->bool_val(false);
            for (const auto &term : or_expr->getTerms())
                exp = exp or getExpression(term, assignment, step);

            return exp;
        }
        case Expression::Type::NOT:
        {
            const auto &not_expr = std::static_pointer_cast<Not>(expr);
            z3::expr exp = getExpression(not_expr->getTerm(), assignment, step);
            return not exp;
        }
        case Expression::Type::IMPLY:
        {
            const auto &imply_expr = std::static_pointer_cast<Imply>(expr);
            return imply(getExpression(imply_expr->getLeft(), assignment, step),
                         getExpression(imply_expr->getRight(), assignment, step));
        }
        case Expression::Type::WHEN:
        {
            const auto &when_expr = std::static_pointer_cast<When>(expr);
            return imply(getExpression(when_expr->getLeft(), assignment, step),
                         getExpression(when_expr->getRight(), assignment, step));
        }
        case Expression::Type::EXISTS:
        {
            const auto &exists_expr = std::static_pointer_cast<Exists>(expr);

            auto copy = assignment;
            const auto &groundings = problem_->getGroundings(exists_expr->getDomain());

            z3::expr exp = ctx_->bool_val(false);
            for (const auto &grounding : groundings)
            {
                copy.fold(grounding);
                exp = exp or getExpression(exists_expr->getFormula(), copy, step);
            }

            return exp;
        }
        case Expression::Type::FORALL:
        {
            const auto &forall_expr = std::static_pointer_cast<Forall>(expr);

            auto copy = assignment;
            const auto &groundings = problem_->getGroundings(forall_expr->getDomain());

            z3::expr exp = ctx_->bool_val(true);
            for (const auto &grounding : groundings)
            {
                copy.fold(grounding);
                exp = exp and getExpression(forall_expr->getFormula(), copy, step);
            }

            return exp;
        }
        case Expression::Type::EQUALS:
        {
            const auto &equals_expr = std::static_pointer_cast<Equals>(expr);
            const auto &[t1, t2] = equals_expr->getTerms();

            z3::expr t1e = getZ3Value(t1, assignment, step);
            z3::expr t2e = getZ3Value(t2, assignment, step);

            const auto &type = problem_->domain->findCommonType(getType(t1e), getType(t2e));
            return castZ3Type(type, t1e) == castZ3Type(type, t2e);
        }
        case Expression::Type::NUMERIC_FUNCTION:
        {
            const auto &numeric_function_expr = std::static_pointer_cast<NumericFunction>(expr);
            return toZ3Fluent(numeric_function_expr, assignment, step);
        }
        case Expression::Type::INCREASE:
        case Expression::Type::DECREASE:
        case Expression::Type::SCALE_UP:
        case Expression::Type::SCALE_DOWN:
        {
            const auto &numeric_expr = std::static_pointer_cast<AssignNumber>(expr);

            const auto &lhs = numeric_expr->getLHS();
            const auto &rhs = numeric_expr->getRHS();

            z3::expr set(*ctx_);
            z3::expr term = toZ3Fluent(lhs, assignment, step);
            z3::expr prior = toZ3Fluent(lhs, assignment, step - 1);
            z3::expr value = getExpression(rhs, assignment, step - 1);

            if (type == Expression::Type::INCREASE)
                set = term == (prior + value);
            else if (type == Expression::Type::DECREASE)
                set = term == (prior - value);
            else if (type == Expression::Type::SCALE_UP)
                set = term == (prior * value);
            else if (type == Expression::Type::SCALE_DOWN)
                set = term == (prior / value);

            return set;
        }
        case Expression::Type::GREATER_THAN:
        case Expression::Type::LESS_THAN:
        case Expression::Type::GREATER_OR_EQUAL:
        case Expression::Type::LESS_OR_EQUAL:
        case Expression::Type::MINUS:
        case Expression::Type::DIVIDE:
        case Expression::Type::MULTIPLY:
        case Expression::Type::PLUS:
        {
            const auto &binary_expr = std::static_pointer_cast<BinaryExpression>(expr);

            const auto &lhs = binary_expr->getLHS();
            const auto &rhs = binary_expr->getRHS();

            z3::expr set(*ctx_);
            z3::expr lhse = getExpression(lhs, assignment, step);
            z3::expr rhse = getExpression(rhs, assignment, step);

            if (type == Expression::Type::GREATER_THAN)
                set = lhse > rhse;
            else if (type == Expression::Type::LESS_THAN)
                set = lhse < rhse;
            else if (type == Expression::Type::GREATER_OR_EQUAL)
                set = lhse >= rhse;
            else if (type == Expression::Type::LESS_OR_EQUAL)
                set = lhse <= rhse;
            else if (type == Expression::Type::MINUS)
                set = lhse - rhse;
            else if (type == Expression::Type::DIVIDE)
                set = lhse / rhse;
            else if (type == Expression::Type::MULTIPLY)
                set = lhse * rhse;
            else if (type == Expression::Type::PLUS)
                set = lhse + rhse;

            return set;
        }
        default:
            throw std::runtime_error(log::format("We had an error in parsing the pddlboat expression to an "
                                                 "expression for Z3. Unsupported expression %1%",
                                                 expr->type()));
    }
}

void Z3PlannerInternal::generateGroundings()
{
    boost::timer::cpu_timer tl;

    auto addToPredicate =
        [&](const std::string &predicate, const GroundedActionPtr &action, bool relies, bool effects)
    {
        auto it = action_effects_.find(predicate);
        if (it == action_effects_.end())
            it = action_effects_.emplace(predicate, std::vector<PredAction>{}).first;

        it->second.emplace_back(action, relies, effects);
    };

    for (const auto &[name, action] : problem_->domain->actions)
    {
        const auto &assignments = problem_->getGroundings(action->parameters);
        for (const auto &assignment : assignments)
        {
            ground_actions_.emplace_back(std::make_shared<GroundedAction>(action, assignment), true, true);

            // Get all predicates this grounded action effects
            std::set<std::string> effects, preconds;
            action->precondition->getUsedPredicates(preconds, problem_->start, assignment);
            action->effect->getAssignedPredicates(effects, problem_->start, assignment);

            // Check all predicates that are preconditions of this action
            for (const auto &predicate : preconds)
                // Always relies on predicate if its in this list, just need to check if it effects
                addToPredicate(predicate, ground_actions_.back().action, true,
                               effects.find(predicate) != effects.end());

            // Now check all predicates that are only effected by this action
            for (const auto &predicate : effects)
                if (preconds.find(predicate) == preconds.end())
                    addToPredicate(predicate, ground_actions_.back().action, false, true);
        }
    }

    // get all predicates that are unassigned
    for (const auto &[name, predicate] : problem_->domain->predicates)
    {
        if (problem_->domain->isDerivedPredicate(name))
            continue;

        const auto &assignments = problem_->getGroundings(predicate->variables);
        for (const auto &assignment : assignments)
        {
            const auto &id = predicate->toID(assignment);
            predicate_map_.emplace(std::piecewise_construct,   //
                                   std::forward_as_tuple(id),  //
                                   std::forward_as_tuple(predicate, assignment));

            auto it = action_effects_.find(id);

            // check if predicate is never effected
            bool effected = false;
            if (it != action_effects_.end())
                for (const auto &action : it->second)
                    effected |= action.effects;

            if (not effected)
                ground_predicate_.emplace_back(predicate, assignment);
        }
    }

    // get all fluents unassigned
    for (const auto &[name, function] : problem_->domain->functions)
    {
        const auto &assignments = problem_->getGroundings(function->parameters);
        for (const auto &assignment : assignments)
        {
            const auto &id = function->toID(assignment);
            fluent_map_.emplace(std::piecewise_construct,   //
                                std::forward_as_tuple(id),  //
                                std::forward_as_tuple(function, assignment));

            // check if predicate is never effected
            bool effected = false;
            auto it = action_effects_.find(id);
            if (it != action_effects_.end())
            {
                for (const auto &action : it->second)
                    if ((effected |= action.effects))
                        break;
            }

            if (not effected)
                ground_fluent_.emplace_back(function, assignment);
        }
    }

    // get all axiom predicate groundings
    for (const auto &axiom : problem_->domain->axioms)
    {
        const auto &assignments = problem_->getGroundings(axiom->parameters);
        for (const auto &assignment : assignments)
            ground_axioms_.emplace_back(axiom, assignment);
    }

    // get all derived predicate groundings
    for (const auto &[name, derived] : problem_->domain->derived)
    {
        const auto &assignments = problem_->getGroundings(derived->parameters);
        for (const auto &assignment : assignments)
            ground_derived_.emplace_back(derived, assignment);
    }

    if (options_.diagnostics)
        std::cout << "Generating Problem Groundings took: " << io::getTiming(tl) << std::endl;
}

void Z3PlannerInternal::generateFunctions()
{
    // create all type data types
    addZ3DataType(PDDLBOAT_OBJECT_CLASS);
    for (const auto &type : problem_->domain->getTypes())
        addZ3DataType(type);

    // predicates and mutexes
    for (const auto &[name, predicate] : problem_->domain->predicates)
    {
        auto function = makeZ3Function(name, predicate->variables);
        if (not function)
            continue;

        z3_predicates_.emplace(name, function);

        auto mutex = makeZ3Function(name + "-mutex", predicate->variables);
        mutex->addBoolArgument();  // forward / backward?
        mutex->addIntArgument();   // index
        mutex->make();             // index
        z3_effect_exclusion_.emplace(name, mutex);
    }

    // general purpose mutex
    mutex_auxilliary_ = std::make_shared<Z3Function>("mutex", this);
    mutex_auxilliary_->addIntArgument();
    mutex_auxilliary_->addBoolArgument();  // forward / backward?
    mutex_auxilliary_->addIntArgument();   // index
    mutex_auxilliary_->make();             // index

    // actions
    for (const auto &[name, action] : problem_->domain->actions)
    {
        auto function = makeZ3Function(name, action->parameters);
        z3_actions_.emplace(name, function);
    }

    // fluents
    for (const auto &[name, fluent] : problem_->domain->functions)
    {
        auto of = std::dynamic_pointer_cast<ObjectFunctionDefinition>(fluent);
        if (of)
        {
            auto function = makeZ3Function(name, fluent->parameters);
            function->setTypeReturn(getZ3DataType(of->type));
            function->make();

            z3_fluents_.emplace(name, function);
        }
        else  // numeric
        {
            auto function = makeZ3Function(name, fluent->parameters);
            function->setIntReturn();
            function->make();

            z3_fluents_.emplace(name, function);
        }

        auto mutex = makeZ3Function(name + "-mutex", fluent->parameters);
        mutex->addBoolArgument();  // forward / backward?
        mutex->addIntArgument();   // index
        mutex->make();             // index
        z3_effect_exclusion_.emplace(name, mutex);
    }
}

void Z3PlannerInternal::addActionImplications(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Action Implications @ " << step << ":" << std::endl;

    action_implications_.start();

    // add clauses for preconditions and effects of actions
    for (const auto &pred : ground_actions_)
    {
        const auto &action = pred.action;

        z3::expr act = toZ3Action(action->action, action->assignment, step);
        z3::expr precond = getExpression(action->action->precondition, action->assignment, step);
        z3::expr effects = getExpression(action->action->effect, action->assignment, step + 1);

        // action -> preconditions_t0 and effects_t1
        z3::expr clause = imply(act, precond and effects);

        addClause(clause);

        action_implications_.inc();
        if (options_.dump_clauses)
            std::cout << clause << std::endl;
    }

    action_implications_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Action Implications @ " << step
                  << " took: " << io::getTiming(action_implications_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addDerivedPredicates(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Derived Predicates @ " << step << ":" << std::endl;

    derived_predicates_.start();

    for (const auto &derived : ground_derived_)
    {
        z3::expr pred = toZ3Predicate(derived, step);
        z3::expr expr = getExpression(derived.derived->expression, derived.assignment, step);

        z3::expr clause = pred == expr;
        addClause(clause);

        derived_predicates_.inc();
        if (options_.dump_clauses)
            std::cout << clause << std::endl;
    }

    derived_predicates_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Derived Predicates @ " << step
                  << " took: " << io::getTiming(derived_predicates_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addDomainAxioms(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Domain Axioms @ " << step << ":" << std::endl;

    domain_axioms_.start();

    for (const auto &axiom : ground_axioms_)
    {
        z3::expr expr = imply(getExpression(axiom.axiom->context, axiom.assignment, step),
                              getExpression(axiom.axiom->implies, axiom.assignment, step));
        addClause(expr);

        domain_axioms_.inc();
        if (options_.dump_clauses)
            std::cout << expr << std::endl;
    }

    domain_axioms_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Domain Axioms @ " << step
                  << " took: " << io::getTiming(domain_axioms_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addFrameAxioms(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Frame Axioms @ " << step << ":" << std::endl;

    frame_axioms_.start();

    if (options_.dump_clauses)
        std::cout << ";; Action Effects @ " << step << ":" << std::endl;
    // add frame axioms
    // if the prop changed, then an action changed it. it doesn't matter what the value
    // changes to, because the clauses for precoditions and effects will ensure
    // consistency
    for (const auto &[effect, actions] : action_effects_)
    {
        if (actions.empty())
            continue;

        z3::expr curr(*ctx_);
        z3::expr next(*ctx_);

        const auto &pit = predicate_map_.find(effect);
        if (pit != predicate_map_.end())
        {
            const auto &predicate = pit->second;
            curr = toZ3Predicate(predicate, step);
            next = toZ3Predicate(predicate, step + 1);
        }
        else
        {
            const auto &fit = fluent_map_.find(effect);
            if (fit != fluent_map_.end())
            {
                const auto &fluent = fit->second;
                curr = toZ3Fluent(fluent, step);
                next = toZ3Fluent(fluent, step + 1);
            }
        }

        bool extended = false;
        z3::expr actionex = ctx_->bool_val(false);
        for (std::size_t i = 0; i < actions.size(); ++i)
            if (actions[i].effects)
            {
                z3::expr actioni = toZ3Action(actions[i].action->action, actions[i].action->assignment, step);
                actionex = actionex or actioni;
                extended = true;
            }

        if (not extended)
            continue;

        z3::expr clause = imply(curr != next, actionex);

        addClause(clause);

        frame_axioms_.inc();
        if (options_.dump_clauses)
            std::cout << clause << std::endl;
    }

    if (options_.dump_clauses)
        std::cout << ";; Constant Predicates @ " << step << ":" << std::endl;

    for (const auto &predicate : ground_predicate_)
    {
        // imply that the predicate remains the same as the next time step
        z3::expr curr = toZ3Predicate(predicate, step);
        z3::expr next = toZ3Predicate(predicate, step + 1);

        z3::expr clause = curr == next;

        addClause(clause);

        frame_axioms_.inc();
        if (options_.dump_clauses)
            std::cout << clause << std::endl;
    }

    if (options_.dump_clauses)
        std::cout << ";; Constant Fluents @ " << step << ":" << std::endl;

    for (const auto &fluent : ground_fluent_)
    {
        // imply that the fluent remains the same as the next time step
        z3::expr curr = toZ3Fluent(fluent, step);
        z3::expr next = toZ3Fluent(fluent, step + 1);

        z3::expr clause = curr == next;

        addClause(clause);

        frame_axioms_.inc();
        if (options_.dump_clauses)
            std::cout << clause << std::endl;
    }

    frame_axioms_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Frame Axioms @ " << step
                  << " took: " << io::getTiming(frame_axioms_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addMutualExclusionLinearHelper(const std::vector<PredAction> &actions, Time step,
                                                       const std::string &prefix, bool forward)
{
    Z3FunctionPtr function = mutex_auxilliary_;
    z3::expr_vector args(*ctx_);

    // find mutex function to use
    if (not prefix.empty())
    {
        auto git = predicate_map_.find(prefix);
        if (git != predicate_map_.end())
        {
            const auto &predicate = git->second.predicate;
            const auto &assignment = git->second.assignment;
            auto fit = z3_effect_exclusion_.find(predicate->name);
            function = fit->second;

            auto pargs = getZ3Values(predicate->variables, assignment);
            for (std::size_t i = 0; i < pargs.size(); ++i)
                args.push_back(pargs[i]);
        }
        else
        {
            auto git = fluent_map_.find(prefix);
            if (git != fluent_map_.end())
            {
                const auto &fluent = git->second.fluent;
                const auto &assignment = git->second.assignment;
                auto fit = z3_effect_exclusion_.find(fluent->name);
                function = fit->second;

                auto pargs = getZ3Values(fluent->parameters, assignment);
                for (std::size_t i = 0; i < pargs.size(); ++i)
                    args.push_back(pargs[i]);
            }
        }
    }

    args.push_back(toZ3Time(step));
    args.push_back(ctx_->bool_val(forward));
    castZ3TypeArgs(args, *function);

    auto auxilliary = [&](std::size_t index) -> z3::expr
    {
        auto pargs = copy(args);
        pargs.push_back(ctx_->int_val((int)index));

        return (*function)(pargs);
    };

    std::size_t intermediate = 0;
    z3::expr current = auxilliary(intermediate);

    for (unsigned int i = 0; i < actions.size(); ++i)
    {
        std::size_t index = (forward) ? i : actions.size() - i - 1;
        const PredAction &pred = actions[index];
        const GroundedActionPtr &action = pred.action;

        z3::expr op = toZ3Action(action->action, action->assignment, step);
        z3::expr clause = ctx_->bool_val(true);

        if (pred.relies)
        {
            z3::expr next = auxilliary(++intermediate);
            clause = clause and imply(current, not op) and imply(current, next);
            current = next;
        }

        if (pred.effects)
            clause = clause and imply(op, current);

        addClause(clause);

        mutual_exclusions_.inc();
        if (options_.dump_clauses)
            std::cout << clause << std::endl;
    }
}

void Z3PlannerInternal::addMutualExclusionLinearSimultaneous(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Mutual Exclusion Constraints @ " << step << ":" << std::endl;

    mutual_exclusions_.start();

    for (const auto &[effect, actions] : action_effects_)
    {
        addMutualExclusionLinearHelper(actions, step, effect, true);
        addMutualExclusionLinearHelper(actions, step, effect, false);
    }

    mutual_exclusions_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Mutual Exclusion Constraints @ " << step
                  << " took: " << io::getTiming(mutual_exclusions_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addMutualExclusionLinear(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Mutual Exclusion Constraints @ " << step << ":" << std::endl;

    mutual_exclusions_.start();

    addMutualExclusionLinearHelper(ground_actions_, step, "", true);
    addMutualExclusionLinearHelper(ground_actions_, step, "", false);

    mutual_exclusions_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Mutual Exclusion Constraints @ " << step
                  << " took: " << io::getTiming(mutual_exclusions_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addMutualExclusion(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Mutual Exclusion Constraints @ " << step << ":" << std::endl;

    mutual_exclusions_.start();

    // add mutual exclusion axioms, i.e., we only take one action each time step
    for (unsigned int i = 0; i < ground_actions_.size(); ++i)
    {
        const auto &action = *ground_actions_[i].action;
        for (unsigned int j = i + 1; j < ground_actions_.size(); ++j)
        {
            const auto &other_action = *ground_actions_[j].action;

            z3::expr e = toZ3Action(action.action, action.assignment, step);
            z3::expr other_e = toZ3Action(other_action.action, other_action.assignment, step);
            z3::expr clause = (not e) or (not other_e);

            addClause(clause);

            mutual_exclusions_.inc();
            if (options_.dump_clauses)
                std::cout << clause << std::endl;
        }
    }

    mutual_exclusions_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Mutual Exclusion Constraints @ " << step
                  << " took: " << io::getTiming(mutual_exclusions_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addEarlyTermination(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Early Termination @ " << step << ":" << std::endl;

    early_termination_.start();

    // "or" over actions
    z3::expr actions = ctx_->bool_val(false);
    for (const auto &pred : ground_actions_)
    {
        const auto &action = pred.action;
        actions = actions or toZ3Action(action->action, action->assignment, step);
    }

    // Add the goal
    z3::expr clause = z3::ite(getExpression(problem_->goal, {}, step), not actions, actions);
    addClause(clause);

    if (options_.dump_clauses)
        std::cout << clause << std::endl;

    early_termination_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Early Termination @ " << step
                  << " took: " << io::getTiming(early_termination_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addGoalExpression(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Goal Expression @ " << step << ":" << std::endl;

    goal_expression_.start();

    // Add the goal
    z3::expr clause = getExpression(problem_->goal, {}, step);
    addClause(clause);

    if (options_.dump_clauses)
        std::cout << clause << std::endl;

    // Add constraints
    for (const auto &[expression, value] : end_state_constraints_)
        addExpStateConstraint(expression, value, step);

    // Add optimization criteria
    if (optimizer_)
    {
        if (options_.dump_clauses)
            std::cout << ";; Adding Optimization Criteria @ " << step << ":" << std::endl;

        z3::expr metric = getExpression(problem_->metric, {}, step);

        if (problem_->minimize)
            optimizer_->minimize(metric);
        else
            optimizer_->maximize(metric);

        if (options_.dump_clauses)
            std::cout << metric << std::endl;
    }

    goal_expression_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Goal Expression @ " << step
                  << " took: " << io::getTiming(goal_expression_.onestep) << std::endl;
    }
}

z3::expr Z3PlannerInternal::getState(const StatePtr &state, Time step) const
{
    z3::expr expr = ctx_->bool_val(true);

    // Add the start state
    for (const auto &[name, predicate] : problem_->domain->predicates)
    {
        if (problem_->domain->isDerivedPredicate(name))
            continue;

        const auto &assignments = problem_->getGroundings(predicate->variables);
        for (const auto &assignment : assignments)
        {
            z3::expr p = toZ3Predicate(predicate, assignment, step);

            // Anything that is not true is false within the initial state.
            std::vector<std::string> objects;
            for (const auto &variable : predicate->variables)
                objects.emplace_back(assignment.assigned(variable.name)->name);

            z3::expr clause = (state->isTrue(name, objects)) ? p : not p;
            expr = expr and clause;
        }
    }

    for (const auto &[name, function] : problem_->domain->functions)
    {
        auto of = std::dynamic_pointer_cast<ObjectFunctionDefinition>(function);

        const auto &assignments = problem_->getGroundings(function->parameters);
        for (const auto &assignment : assignments)
        {
            z3::expr p = toZ3Fluent(function, assignment, step);

            std::vector<std::string> objects;
            for (const auto &variable : function->parameters)
                objects.emplace_back(assignment.assigned(variable.name)->name);

            if (of)
            {
                auto dt = getZ3DataType(of->type);

                std::string value = state->getObject(name, objects);
                if (value != "")
                {
                    z3::expr clause = p == dt->getConstant(value);
                    expr = expr and clause;
                }
            }
            else  // is a numeric function
            {
                double number = state->getValue(name, objects);
                z3::expr clause = p == toZ3Integer(number);
                expr = expr and clause;
            }
        }
    }

    return expr;
}

void Z3PlannerInternal::addStartState()
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Start State @ 0:" << std::endl;

    start_propositions_.start();

    z3::expr clause = getState(problem_->start, 0);
    addClause(clause);

    start_propositions_.inc();
    if (options_.dump_clauses)
        std::cout << clause << std::endl;

    start_propositions_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Start State @ 0"
                  << " took: " << io::getTiming(start_propositions_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addStateActionConstraints(Time step)
{
    for (const auto &[state, set] : constraints_)
        addStateActionConstraint(state, set, step);

    for (const auto &[state, set] : simultaneous_constraints_)
        addStateActionConstraint(state, set, step, true);

    for (const auto &[expression, set] : expression_constraints_)
        addExpConstraint(expression, set, step);

    for (const auto &[expression, value] : all_state_constraints_)
        addExpStateConstraint(expression, value, step);
}

void Z3PlannerInternal::addExpStateConstraint(const ExpressionPtr &exp, bool value, Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Expression State Constraint @ :" << step << std::endl;

    constraint_clauses_.start();

    z3::expr clause = getExpression(exp, {}, step) == ctx_->bool_val(value);
    addClause(clause);

    constraint_clauses_.inc();
    if (options_.dump_clauses)
        std::cout << clause << std::endl;

    constraint_clauses_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Constraint @ " << step
                  << " took: " << io::getTiming(constraint_clauses_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addExpConstraint(const GroundedExpression &exp,
                                         const std::set<size_t> &ground_indices, Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Expression Constraint @ :" << step << std::endl;

    constraint_clauses_.start();

    z3::expr zexpr = getExpression(exp.expression, exp.assignment, step);

    z3::expr aexpr = ctx_->bool_val(false);
    for (const auto &index : ground_indices)
    {
        const auto &action = ground_actions_[index].action;
        aexpr = aexpr or toZ3Action(action->action, action->assignment, step);
    }

    z3::expr clause = imply(zexpr, not aexpr);
    addClause(clause);

    constraint_clauses_.inc();
    if (options_.dump_clauses)
        std::cout << clause << std::endl;

    constraint_clauses_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Constraint @ " << step
                  << " took: " << io::getTiming(constraint_clauses_.onestep) << std::endl;
    }
}

void Z3PlannerInternal::addStateActionConstraint(const StatePtr &state,
                                                 const std::set<size_t> &ground_indices, Time step,
                                                 bool simultaneous)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Constraint Simultaneous @ :" << step << std::endl;

    constraint_clauses_.start();

    z3::expr sexpr = getState(state, step);
    z3::expr aexpr = ctx_->bool_val(false);
    for (const auto &index : ground_indices)
    {
        const auto &action = ground_actions_[index].action;
        if (simultaneous)
            aexpr = aexpr and toZ3Action(action->action, action->assignment, step);
        else
            aexpr = aexpr or toZ3Action(action->action, action->assignment, step);
    }

    z3::expr clause = imply(sexpr, not aexpr);
    addClause(clause);

    constraint_clauses_.inc();
    if (options_.dump_clauses)
        std::cout << clause << std::endl;

    constraint_clauses_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Constraint @ " << step
                  << " took: " << io::getTiming(constraint_clauses_.onestep) << std::endl;
    }
}

std::size_t Z3PlannerInternal::getGroundActionIndex(const ActionPtr &action,
                                                    const Assignment &assignment) const
{
    const auto &id = action->toID(assignment);
    const auto &it = std::find_if(ground_actions_.begin(), ground_actions_.end(),
                                  [id](const PredAction &pa) { return pa.action->string == id; });

    if (it == ground_actions_.end())
        throw std::runtime_error(log::format("Attempted to block action `%1%`, which was not found.", id));

    return it - ground_actions_.begin();
}

void Z3PlannerInternal::addZ3DataType(const std::string &type)
{
    const auto &dt = makeZ3DataType(type);
    z3_types_.emplace(type, dt);
}

Z3PlannerInternal::Z3DataTypePtr Z3PlannerInternal::makeZ3DataType(const std::string &type) const
{
    auto dt = std::make_shared<Z3DataType>(type, this);

    if (type != PDDLBOAT_OBJECT_CLASS and not dt->values.empty())
    {
        const auto &pdt = getZ3DataType(dt->parent);
        dt->upcast.addTypeArgument(dt);
        dt->upcast.setTypeReturn(pdt);
        dt->upcast.make();

        if (options_.dump_clauses)
            std::cout << ";; Adding casts for " << type << " to " << dt->parent << std::endl;

        for (const auto &value : dt->values)
        {
            const auto &obj = value.first;
            z3::expr clause = dt->upcast.function(dt->getConstant(obj)) == pdt->getConstant(obj);

            if (options_.dump_clauses)
                std::cout << clause << std::endl;

            addClause(clause);
        }
    }

    return dt;
}

const Z3PlannerInternal::Z3DataTypePtr &Z3PlannerInternal::getZ3DataType(const std::string &type) const
{
    const auto &it = z3_types_.find(type);
    if (it != z3_types_.end())
        return it->second;

    throw std::runtime_error(log::format("Invalid type %1%", type));
}

z3::expr Z3PlannerInternal::castZ3Type(const std::string &to, const z3::expr &e) const
{
    z3::expr casted = e;
    std::string type = getType(e);

    while (type != to)
    {
        const auto &dt = getZ3DataType(type);
        casted = dt->upcast(casted);
        type = dt->parent;
    }

    return casted;
}

void Z3PlannerInternal::castZ3TypeArgs(z3::expr_vector &args, const Z3Function &function) const
{
    z3::expr_vector casted(*ctx_);

    // assignment to an expr_vector is apparently also new hotness...
    for (std::size_t i = 0; i < args.size(); ++i)
    {
        if (i < function.args.size() and function.args[i])
            casted.push_back(castZ3Type(function.args[i]->name, args[i]));
        else
            casted.push_back(args[i]);
    }

    args = casted;
}

Z3PlannerInternal::Z3FunctionPtr Z3PlannerInternal::makeZ3Function(const std::string &name,
                                                                   const VariableList &arguments) const
{
    auto function = std::make_shared<Z3Function>(name, this);
    for (const auto &arg : arguments)
    {
        const auto &dt = getZ3DataType(arg.type);
        if (dt->values.empty())
            return nullptr;

        function->addTypeArgument(dt);
    }

    function->addIntArgument();
    function->make();
    return function;
}

const Z3PlannerInternal::Z3FunctionPtr &Z3PlannerInternal::getZ3Predicate(const std::string &name) const
{
    const auto &it = z3_predicates_.find(name);
    if (it == z3_predicates_.end())
        throw std::runtime_error("Invalid predicate " + name);

    return it->second;
}

const Z3PlannerInternal::Z3FunctionPtr &Z3PlannerInternal::getZ3Fluent(const std::string &name) const
{
    const auto &it = z3_fluents_.find(name);
    if (it == z3_fluents_.end())
        throw std::runtime_error("Invalid fluent " + name);

    return it->second;
}

const Z3PlannerInternal::Z3FunctionPtr &Z3PlannerInternal::getZ3Action(const std::string &name) const
{
    const auto &it = z3_actions_.find(name);
    if (it == z3_actions_.end())
        throw std::runtime_error("Invalid action " + name);

    return it->second;
}

z3::expr Z3PlannerInternal::getZ3Value(const Value &value, const Assignment &assignment, Time step) const
{
    if (value.function)
        return toZ3Fluent(value.function, assignment, step);

    else if (value.isVar)
    {
        if (assignment.isAssigned(value.name))
        {
            const auto &assigned = assignment.assigned(value.name);
            const auto &dt = getZ3DataType(assigned->type);
            return dt->getConstant(assigned->name);
        }
        else
        {
            const auto &dt = getZ3DataType(assignment.typed(value.name));
            return dt->getConstant(value.name);
        }
    }
    else
    {
        const auto &dt = getZ3DataType(value.type);
        return dt->getConstant(value.name);
    }
}

z3::expr Z3PlannerInternal::getZ3Value(const Variable &variable, const Assignment &assignment) const
{
    const auto &assigned = assignment.assigned(variable.name);
    const auto &dt = getZ3DataType(assigned->type);
    return dt->getConstant(assigned->name);
}

z3::expr_vector Z3PlannerInternal::getZ3Values(const std::vector<Value> &bindings,
                                               const Assignment &assignment, Time step) const
{
    z3::expr_vector args(*ctx_);
    for (const auto &binding : bindings)
        args.push_back(getZ3Value(binding, assignment, step));

    return args;
}

z3::expr_vector Z3PlannerInternal::getZ3Values(const VariableList &variables,
                                               const Assignment &assignment) const
{
    z3::expr_vector args(*ctx_);
    for (const auto &variable : variables)
        args.push_back(getZ3Value(variable, assignment));

    return args;
}

z3::expr_vector Z3PlannerInternal::getZ3Variables(const VariableList &parameters,
                                                  Assignment &assignment) const
{
    z3::expr_vector args(*ctx_);
    for (const auto &parameter : parameters)
    {
        const auto &dt = getZ3DataType(parameter.type);
        args.push_back(dt->getVariable(parameter.name));
        assignment.type(parameter.name, parameter.type);
    }

    return args;
}

z3::expr Z3PlannerInternal::toZ3Expr(const Z3FunctionPtr &function, z3::expr_vector &args, Time step) const
{
    args.push_back(toZ3Time(step));
    castZ3TypeArgs(args, *function);
    return (*function)(args);
}

z3::expr Z3PlannerInternal::toZ3Predicate(const PredicatePtr &predicate, const Assignment &assignment,
                                          Time step) const
{
    const auto &function = getZ3Predicate(predicate->name());
    auto args = getZ3Values(predicate->bindings, assignment, step);
    return toZ3Expr(function, args, step);
}

z3::expr Z3PlannerInternal::toZ3Predicate(const PredicateDefinitionPtr &predicate,
                                          const Assignment &assignment, Time step) const
{
    const auto &function = getZ3Predicate(predicate->name);
    auto args = getZ3Values(predicate->variables, assignment);
    return toZ3Expr(function, args, step);
}

z3::expr Z3PlannerInternal::toZ3Predicate(const DerivedPredicatePtr &predicate, const Assignment &assignment,
                                          Time step) const
{
    const auto &function = getZ3Predicate(predicate->name());
    auto args = getZ3Values(predicate->bindings, assignment, step);
    return toZ3Expr(function, args, step);
}

z3::expr Z3PlannerInternal::toZ3Predicate(const GroundedPredicate &grounding, Time step) const
{
    const auto &predicate = grounding.predicate;
    const auto &function = getZ3Predicate(predicate->name);
    auto args = getZ3Values(predicate->variables, grounding.assignment);
    return toZ3Expr(function, args, step);
}

z3::expr Z3PlannerInternal::toZ3Predicate(const GroundedDerivedPredicate &grounding, Time step) const
{
    const auto &derived = grounding.derived;
    const auto &function = getZ3Predicate(derived->name);
    auto args = getZ3Values(derived->parameters, grounding.assignment);
    return toZ3Expr(function, args, step);
}

z3::expr Z3PlannerInternal::toZ3Action(const ActionPtr &action, const Assignment &assignment, Time step) const
{
    const auto &function = getZ3Action(action->name);
    auto args = getZ3Values(action->parameters, assignment);
    return toZ3Expr(function, args, step);
}

z3::expr Z3PlannerInternal::toZ3Fluent(const std::string &fluent, const std::vector<Value> &values,
                                       const Assignment &assignment, Time step) const
{
    const auto &function = getZ3Fluent(fluent);
    auto args = getZ3Values(values, assignment, step);
    return toZ3Expr(function, args, step);
}

z3::expr Z3PlannerInternal::toZ3Fluent(const std::string &fluent, const VariableList &variables,
                                       const Assignment &assignment, Time step) const
{
    const auto &function = getZ3Fluent(fluent);
    auto args = getZ3Values(variables, assignment);
    return toZ3Expr(function, args, step);
}

z3::expr Z3PlannerInternal::toZ3Fluent(const ObjectFunctionPtr &fluent, const Assignment &assignment,
                                       Time step) const
{
    return toZ3Fluent(fluent->getName(), fluent->bindings, assignment, step);
}

z3::expr Z3PlannerInternal::toZ3Fluent(const ObjectFunctionDefinitionPtr &fluent,
                                       const Assignment &assignment, Time step) const
{
    return toZ3Fluent(fluent->name, fluent->parameters, assignment, step);
}

z3::expr Z3PlannerInternal::toZ3Fluent(const GroundedObjectFunction &grounding, Time step) const
{
    const auto &fluent = grounding.fluent;
    return toZ3Fluent(fluent->name, fluent->parameters, grounding.assignment, step);
}

z3::expr Z3PlannerInternal::toZ3Fluent(const NumericFunctionPtr &fluent, const Assignment &assignment,
                                       Time step) const
{
    return toZ3Fluent(fluent->getName(), fluent->bindings, assignment, step);
}

z3::expr Z3PlannerInternal::toZ3Fluent(const NumericFunctionDefinitionPtr &fluent,
                                       const Assignment &assignment, Time step) const
{
    return toZ3Fluent(fluent->name, fluent->parameters, assignment, step);
}

z3::expr Z3PlannerInternal::toZ3Fluent(const GroundedNumericFunction &grounding, Time step) const
{
    const auto &fluent = grounding.fluent;
    return toZ3Fluent(fluent->name, fluent->parameters, grounding.assignment, step);
}

z3::expr Z3PlannerInternal::toZ3Fluent(const FunctionDefinitionPtr &fluent, const Assignment &assignment,
                                       Time step) const
{
    return toZ3Fluent(fluent->name, fluent->parameters, assignment, step);
}

z3::expr Z3PlannerInternal::toZ3Fluent(const GroundedFunction &grounding, Time step) const
{
    const auto &fluent = grounding.fluent;
    return toZ3Fluent(fluent->name, fluent->parameters, grounding.assignment, step);
}

z3::expr Z3PlannerInternal::toZ3Time(Time time) const
{
    return ctx_->int_val((int)time);
}

z3::expr Z3PlannerInternal::toZ3Integer(double value) const
{
    return ctx_->int_val((int)value);
}

void Z3PlannerInternal::addClause(const z3::expr &clause) const
{
    (optimizer_) ? optimizer_->add(clause) : solver_->add(clause);
}

bool Z3PlannerInternal::solverCheck() const
{
    return ((optimizer_) ? optimizer_->check() : solver_->check()) == z3::sat;
}

void Z3PlannerInternal::solverPush() const
{
    (optimizer_) ? optimizer_->push() : solver_->push();
}

void Z3PlannerInternal::solverPop() const
{
    (optimizer_) ? optimizer_->pop() : solver_->pop();
}

z3::model Z3PlannerInternal::getSolverModel() const
{
    return (optimizer_) ? optimizer_->get_model() : solver_->get_model();
}

z3::stats Z3PlannerInternal::getSolverStats() const
{
    return (optimizer_) ? optimizer_->statistics() : solver_->statistics();
}

//
// Z3 Constructs
//

Z3PlannerInternal::Z3Function::Z3Function(const std::string &name, const Z3PlannerInternal *planner)
  : name(name)
  , domains(*planner->ctx_)
  , range(planner->ctx_->bool_sort())
  , function(*planner->ctx_)
  , planner(planner)
{
}

void Z3PlannerInternal::Z3Function::addTypeArgument(const Z3DataTypePtr &type)
{
    args.push_back(type);
    domains.push_back(type->sort);
}

void Z3PlannerInternal::Z3Function::addIntArgument()
{
    args.push_back(nullptr);
    domains.push_back(planner->ctx_->int_sort());
}

void Z3PlannerInternal::Z3Function::addBoolArgument()
{
    args.push_back(nullptr);
    domains.push_back(planner->ctx_->bool_sort());
}

void Z3PlannerInternal::Z3Function::setBoolReturn()
{
    range = planner->ctx_->bool_sort();
}

void Z3PlannerInternal::Z3Function::setIntReturn()
{
    range = planner->ctx_->int_sort();
}

void Z3PlannerInternal::Z3Function::setTypeReturn(const Z3DataTypePtr &type)
{
    range = type->sort;
}

void Z3PlannerInternal::Z3Function::make()
{
    function = planner->ctx_->function(name.c_str(), domains, range);
}

z3::expr Z3PlannerInternal::Z3Function::operator()() const
{
    return function();
}

z3::expr Z3PlannerInternal::Z3Function::operator()(const z3::expr &arg) const
{
    return function(arg);
}

z3::expr Z3PlannerInternal::Z3Function::operator()(const z3::expr_vector &args) const
{
    return function(args);
}

Z3PlannerInternal::Z3DataType::Z3DataType(const std::string &type, const Z3PlannerInternal *planner)
  : name(type)
  , consts(*planner->ctx_)
  , testers(*planner->ctx_)
  , sort(*planner->ctx_)
  , parent(planner->problem_->domain->getParent(type))
  , upcast("to-" + parent, planner)
  , planner(planner)
{
    // get all objects
    const auto &objects = planner->problem_->objectsOfType(type);
    if (objects.empty())
        return;

    const char *names[objects.size()];
    for (std::size_t i = 0; i < objects.size(); ++i)
    {
        const auto &it = values.emplace(objects[i], i);
        names[i] = it.first->first.c_str();
    }

    sort = planner->ctx_->enumeration_sort(type.c_str(), values.size(), names, consts, testers);
}

z3::expr Z3PlannerInternal::Z3DataType::getConstant(const std::string &object) const
{
    const auto &it = values.find(object);
    if (it == values.end())
        throw std::runtime_error(log::format("Invalid object %1% in type %2%!", object, name));

    return consts[it->second]();
}

z3::expr Z3PlannerInternal::Z3DataType::getVariable(const std::string &name) const
{
    return planner->ctx_->constant(name.c_str(), sort);
}
