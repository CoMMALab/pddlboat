/* Author: Andrew Wells and Zachary Kingston */

#include <pddlboat/solver/z3planner.h>

using namespace pddlboat;

namespace
{
    z3::expr imply(const z3::expr &a, const z3::expr &b)
    {
        return (not a) or b;
    }
}  // namespace

Z3Planner::GroundedAction::GroundedAction(const ActionPtr &action, const Assignment &assignment,
                                          const std::string &string)
  : action(action), assignment(assignment), string(string)
{
}

Z3Planner::GroundedDerived::GroundedDerived(const DerivedPredicateDefinitionPtr &derived,
                                            const Assignment &assignment, const std::string &string)
  : derived(derived), assignment(assignment), string(string)
{
}

Z3Planner::GroundedPredicate::GroundedPredicate(const PredicateDefinitionPtr &predicate,
                                                const Assignment &assignment, const std::string &string)
  : predicate(predicate), assignment(assignment), string(string)
{
}

Z3Planner::Z3Planner(const ProblemPtr problem, Time max_horizon, Time min_horizon, bool parallel,
                     bool diagnostics, bool dump_clauses)
  : Planner(problem)
  , max_horizon_(max_horizon)
  , min_horizon_(min_horizon)
  , diagnostics_(diagnostics)
  , dump_clauses_(dump_clauses)
{
    checkRequirements();

    if (parallel)
        z3::set_param("parallel.enable", true);

    // Generate the groundings for this problem.
    generateGroundings();
}

Requirements::Keyword Z3Planner::getSupportedRequirements() const
{
    return Requirements::ADL | Requirements::DERIVED_PREDICATES;
}

bool Z3Planner::solve(Plan &plan)
{
    boost::timer::cpu_timer t;

    if (ctx_)
        delete ctx_;

    ctx_ = new z3::context;

    if (solver_)
        delete solver_;

    solver_ = new z3::solver(*ctx_);

    addStartState();
    addDerivedPredicates(0);
    solver_->push();

    for (Time step = 0; step < min_horizon_; ++step)
    {
        addActionImplications(step);
        addFrameAxioms(step);
        addMutualExclusionLinear(step);
        addDerivedPredicates(step + 1);

        solver_->push();
    }

    bool sat = false;
    for (Time step = min_horizon_; step < max_horizon_ and not sat; ++step)
    {
        boost::timer::cpu_timer tl;

        addActionImplications(step);
        addFrameAxioms(step);
        addMutualExclusionLinear(step);
        addDerivedPredicates(step + 1);

        // Solve now to create assertions that won't be popped.
        solver_->check();
        solver_->push();

        addGoalExpression(step + 1);

        auto result = solver_->check();

        tl.stop();
        if (diagnostics_)
            std::cout << "Solving Step " << step << " Z3 Instance took: " << io::getTiming(tl) << std::endl;

        sat = result == z3::sat;

        // Pop goal expression from solver.
        if (not sat)
            solver_->pop();
    }

    if (diagnostics_)
    {
        t.stop();

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
        std::cout << solver_->statistics() << std::endl;
    }

    if (sat)
    {
        buildPlan(plan);
        return true;
    }

    return false;
}

std::ostream &Z3Planner::printModel(std::ostream &out, bool only_true) const
{
    if (not solver_)
        return out;

    auto m = solver_->get_model();

    std::vector<std::vector<std::pair<ast::z3Action, bool>>> actions(max_horizon_ + 1);
    std::vector<std::vector<std::pair<ast::z3Predicate, bool>>> predicates(max_horizon_ + 1);

    for (std::size_t i = 0; i < m.num_consts(); i++)
    {
        z3::expr f = m.get_const_interp(m.get_const_decl(i));
        const std::string name = m.get_const_decl(i).name().str();

        ast::z3Action action;
        if (parseZ3Action(name, action))
        {
            action.conv = std::stoi(action.time);

            bool t = isTrue(f);
            if (not only_true or (only_true and t))
                actions[action.conv].emplace_back(action, t);
        }

        ast::z3Predicate predicate;
        if (parseZ3Predicate(name, predicate))
        {
            predicate.conv = std::stoi(predicate.time);

            bool t = isTrue(f);
            if (not only_true or (only_true and t))
                predicates[predicate.conv].emplace_back(predicate, t);
        }
    }

    for (std::size_t i = 0; i <= max_horizon_; ++i)
    {
        std::sort(actions[i].begin(), actions[i].end(),
                  [](const auto &a, const auto &b) { return a.first.name < b.first.name; });
        std::sort(predicates[i].begin(), predicates[i].end(),
                  [](const auto &a, const auto &b) { return a.first.name < b.first.name; });

        bool isa = not actions[i].empty();
        bool isp = not predicates[i].empty();

        if (isa or isp)
            out << "Time Step " << i << std::endl;

        if (isa)
        {
            out << "Actions: " << std::endl;
            for (const auto &action : actions[i])
            {
                out << "  ";
                if (action.second)
                    out << "  ";
                action.first.toPDDL(out) << ": " << (action.second ? "true" : "false") << std::endl;
            }
        }

        if (isp)
        {
            out << "Predicates: " << std::endl;
            for (const auto &predicate : predicates[i])
            {
                out << "  ";
                if (predicate.second)
                    out << "  ";
                predicate.first.toPDDL(out) << ": " << (predicate.second ? "true" : "false") << std::endl;
            }
        }

        if (isa or isp)
            out << std::endl;
    }

    return out;
}

std::ostream &Z3Planner::printUnsatCore(std::ostream &out) const
{
    if (not solver_)
        return out;

    auto core = solver_->unsat_core();
    out << "Unsat Core:" << std::endl;
    for (std::size_t i = 0; i < core.size(); ++i)
        out << core[i] << std::endl;
    out << "End Unsat Core" << std::endl;

    return out;
}

void Z3Planner::buildPlan(Plan &plan) const
{
    auto m = solver_->get_model();

    // As actions may appear `out of order`, we first collect them all.
    std::vector<ast::z3Action> actions;
    for (std::size_t i = 0; i < m.num_consts(); i++)
    {
        z3::expr f = m.get_const_interp(m.get_const_decl(i));
        if (isTrue(f))
        {
            const std::string name = m.get_const_decl(i).name().str();

            // Parse name of Z3 boolean to a grounded action.
            ast::z3Action action;
            if (parseZ3Action(name, action))
            {
                action.conv = std::stoi(action.time);
                actions.emplace_back(action);
            }
        }
    }

    // Sort the actions according to time.
    std::sort(actions.begin(), actions.end(), [](const auto &a, const auto &b) { return a.conv < b.conv; });

    // Add each step to return plan.
    for (const auto &action : actions)
        plan.addStep(action.name, action.args);
}

z3::expr Z3Planner::toBool(const std::string &string) const
{
    return ctx_->bool_const(string.c_str());
}

bool Z3Planner::isTrue(const z3::expr &expr) const
{
    // As is_true() is apparently a bleeding edge feature
    return expr.is_app() and Z3_OP_TRUE == expr.decl().decl_kind();
}

std::string Z3Planner::addTimeToString(const std::string &string, Time time) const
{
    return string + ":TIME:" + std::to_string(time);
}

/**
 * Recurses down the tree and builds a z3::expr for the expression, replacing atoms with their
 * z3 bool_constants. We require that the expression does not contain any actions (all terminals
 * are state props).
 */
z3::expr Z3Planner::exprAtTime(const ExpressionPtr &expr, const Assignment &assignment, Time step) const
{
    switch (expr->type())
    {
        case Expression::Type::PREDICATE:
        {
            const auto &predicate_expr = std::static_pointer_cast<Predicate>(expr);
            return toBool(addTimeToString(predicate_expr->toID(nullptr, assignment), step));
        }
        case Expression::Type::DERIVED:
        {
            const auto &predicate_expr = std::static_pointer_cast<DerivedPredicate>(expr);
            return toBool(addTimeToString(predicate_expr->toID(nullptr, assignment), step));
        }
        case Expression::Type::AND:
        {
            const auto &and_expr = std::static_pointer_cast<And>(expr);

            z3::expr exp = ctx_->bool_val(true);
            for (const auto &term : and_expr->getTerms())
                exp = exp and exprAtTime(term, assignment, step);

            return exp;
        }
        case Expression::Type::OR:
        {
            const auto &or_expr = std::static_pointer_cast<Or>(expr);

            z3::expr exp = ctx_->bool_val(false);
            for (const auto &term : or_expr->getTerms())
                exp = exp or exprAtTime(term, assignment, step);

            return exp;
        }
        case Expression::Type::NOT:
        {
            const auto &not_expr = std::static_pointer_cast<Not>(expr);
            z3::expr exp = exprAtTime(not_expr->getTerm(), assignment, step);
            return not exp;
        }
        case Expression::Type::IMPLY:
        {
            const auto &imply_expr = std::static_pointer_cast<Imply>(expr);
            return imply(exprAtTime(imply_expr->getLeft(), assignment, step),
                         exprAtTime(imply_expr->getRight(), assignment, step));
        }
        case Expression::Type::WHEN:
        {
            const auto &when_expr = std::static_pointer_cast<When>(expr);
            return imply(exprAtTime(when_expr->getLeft(), assignment, step),
                         exprAtTime(when_expr->getRight(), assignment, step));
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
                exp = exp or exprAtTime(exists_expr->getFormula(), copy, step);
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
                exp = exp and exprAtTime(forall_expr->getFormula(), copy, step);
            }

            return exp;
        }
        case Expression::Type::EQUALS:
        {
            const auto &equals_expr = std::static_pointer_cast<Equals>(expr);
            const auto &[t1, t2] = equals_expr->getTerms();

            auto t1o = t1.getObject(nullptr, assignment);
            auto t2o = t2.getObject(nullptr, assignment);

            return ctx_->bool_val(t1o == t2o);
        }
        default:
            throw std::runtime_error(log::format("We had an error in parsing the pddlboat expression to an "
                                                 "expression for Z3. Unsupported expression %1%",
                                                 expr->type()));
    }
}

void Z3Planner::generateGroundings()
{
    ground_actions_.clear();
    predicate_effects_.clear();

    for (const auto &name_actionptr : problem_->domain->actions)
    {
        const auto &assignments = problem_->getGroundings(name_actionptr.second->parameters);
        for (const auto &assignment : assignments)
        {
            // Add grounded action
            std::string action_string = name_actionptr.second->toID(assignment);
            ground_actions_.emplace_back(name_actionptr.second, assignment, action_string);

            // Get all predicates this grounded action effects
            std::set<std::string> predicates;
            name_actionptr.second->effect->getAssignedPredicates(predicates, problem_->start, assignment);
            for (const auto &predicate : predicates)
            {
                auto it = predicate_effects_.find(predicate);
                if (it == predicate_effects_.end())
                    it = predicate_effects_.emplace(predicate, std::vector<std::string>{}).first;

                it->second.emplace_back(name_actionptr.second->toID(assignment));
            }
        }
    }

    // get all predicates that are unassigned
    for (const auto &name_predicateptr : problem_->domain->predicates)
    {
        if (problem_->domain->isDerivedPredicate(name_predicateptr.first))
            continue;

        const auto &assignments = problem_->getGroundings(name_predicateptr.second->variables);
        for (const auto &assignment : assignments)
        {
            std::string predicate_string = name_predicateptr.second->toID(assignment);
            if (predicate_effects_.find(predicate_string) == predicate_effects_.end())
                ground_pred_.emplace_back(name_predicateptr.second, assignment, predicate_string);
        }
    }

    // get all derived predicate groundings
    for (const auto &name_derivedptr : problem_->domain->derived)
    {
        const auto &assignments = problem_->getGroundings(name_derivedptr.second->parameters);
        for (const auto &assignment : assignments)
        {
            // Add grounded derived
            std::string derived_string = name_derivedptr.second->toID(assignment);
            ground_derived_.emplace_back(name_derivedptr.second, assignment, derived_string);
        }
    }
}

void Z3Planner::addActionImplications(Time step)
{
    if (dump_clauses_)
        std::cout << ";; Adding Action Implications @ " << step << ":" << std::endl;

    action_implications_.start();

    // add clauses for preconditions and effects of actions
    for (const auto &action : ground_actions_)
    {
        z3::expr act = toBool(addTimeToString(action.string, step));
        z3::expr precond = exprAtTime(action.action->precondition, action.assignment, step);
        z3::expr effects = exprAtTime(action.action->effect, action.assignment, step + 1);

        // action -> preconditions_t0 and effects_t1
        z3::expr clause = imply(act, precond and effects);

        solver_->add(clause);

        action_implications_.inc();
        if (dump_clauses_)
            std::cout << clause << std::endl;
    }

    action_implications_.stop();
    if (diagnostics_)
    {
        std::cout << "Building Z3 Action Implications @ " << step
                  << " took: " << io::getTiming(action_implications_.onestep) << std::endl;
    }
}

void Z3Planner::addDerivedPredicates(Time step)
{
    if (dump_clauses_)
        std::cout << ";; Adding Derived Predicates @ " << step << ":" << std::endl;

    derived_predicates_.start();

    for (const auto &derived : ground_derived_)
    {
        z3::expr ax1 = toBool(addTimeToString("AX:" + derived.string, step));
        z3::expr ax2 = toBool(addTimeToString("BX:" + derived.string, step));

        z3::expr pred = toBool(addTimeToString(derived.string, step));
        z3::expr expr = exprAtTime(derived.derived->expression, derived.assignment, step);

        z3::expr clause = pred == expr;
        // z3::expr clause = not(pred != expr);
        solver_->add(clause);

        derived_predicates_.inc();
        if (dump_clauses_)
            std::cout << clause << std::endl;
    }

    derived_predicates_.stop();
    if (diagnostics_)
    {
        std::cout << "Building Z3 Derived Predicates @ " << step
                  << " took: " << io::getTiming(derived_predicates_.onestep) << std::endl;
    }
}

void Z3Planner::addFrameAxioms(Time step)
{
    if (dump_clauses_)
        std::cout << ";; Adding Frame Axioms @ " << step << ":" << std::endl;

    frame_axioms_.start();
    for (const auto &[predicate, actions] : predicate_effects_)
    {
        // add frame axioms
        // if the prop changed, then an action changed it. it doesn't matter what the value
        // changes to, because the clauses for precoditions and effects will ensure
        // consistency
        z3::expr curr_prop = toBool(addTimeToString(predicate, step));
        z3::expr next_prop = toBool(addTimeToString(predicate, step + 1));

        z3::expr actionex = toBool(addTimeToString(actions[0], step));
        for (std::size_t i = 1; i < actions.size(); ++i)
            actionex = actionex or toBool(addTimeToString(actions[i], step));

        z3::expr clause = imply(curr_prop != next_prop, actionex);

        solver_->add(clause);

        frame_axioms_.inc();
        if (dump_clauses_)
            std::cout << clause << std::endl;
    }

    for (const auto &predicate : ground_pred_)
    {
        // imply that the predicate remains the same as the next time step
        z3::expr curr_prop = toBool(addTimeToString(predicate.string, step));
        z3::expr next_prop = toBool(addTimeToString(predicate.string, step + 1));
        z3::expr clause = curr_prop == next_prop;

        solver_->add(clause);

        frame_axioms_.inc();
        if (dump_clauses_)
            std::cout << clause << std::endl;
    }

    frame_axioms_.stop();
    if (diagnostics_)
    {
        std::cout << "Building Z3 Frame Axioms @ " << step
                  << " took: " << io::getTiming(frame_axioms_.onestep) << std::endl;
    }
}

void Z3Planner::addMutualExclusionCardinality(Time step)
{
    if (dump_clauses_)
        std::cout << ";; Adding Mutual Exclusion Constraints @ " << step << ":" << std::endl;

    mutual_exclusions_.start();

    z3::expr clause = ctx_->int_val(0);
    for (const auto &action : ground_actions_)
    {
        z3::expr e = toBool(addTimeToString(action.string, step));
        clause = clause + z3::ite(e, ctx_->int_val(1), ctx_->int_val(0));
    }

    solver_->add(clause == 1);

    mutual_exclusions_.inc();
    if (dump_clauses_)
        std::cout << clause << std::endl;

    mutual_exclusions_.stop();
    if (diagnostics_)
    {
        std::cout << "Building Z3 Mutual Exclusion Constraints @ " << step
                  << " took: " << io::getTiming(mutual_exclusions_.onestep) << std::endl;
    }
}

void Z3Planner::addMutualExclusionLinearHelper(Time step, bool forward)
{
    const std::string prefix = std::string(forward ? "F" : "B") + ":";

    for (unsigned int i = 0; i < ground_actions_.size(); ++i)
    {
        std::size_t index = (forward) ? i : ground_actions_.size() - i - 1;
        const GroundedAction &action = ground_actions_[index];

        z3::expr intermediate = toBool(addTimeToString(prefix + action.string, step));
        z3::expr op = toBool(addTimeToString(action.string, step));

        z3::expr clause = ctx_->bool_val(true);

        if (i != ground_actions_.size() - 1)
            clause = clause and imply(op, intermediate);

        if (i)
        {
            std::size_t pindex = (forward) ? i - 1 : ground_actions_.size() - i;
            const GroundedAction &prior = ground_actions_[pindex];

            z3::expr prev = toBool(addTimeToString(prefix + prior.string, step));
            clause = clause and imply(prev, intermediate) and imply(prev, not op);
        }

        solver_->add(clause);

        mutual_exclusions_.inc();
        if (dump_clauses_)
            std::cout << clause << std::endl;
    }
}

void Z3Planner::addMutualExclusionLinear(Time step)
{
    if (dump_clauses_)
        std::cout << ";; Adding Mutual Exclusion Constraints @ " << step << ":" << std::endl;

    mutual_exclusions_.start();

    addMutualExclusionLinearHelper(step, true);
    addMutualExclusionLinearHelper(step, false);

    mutual_exclusions_.stop();
    if (diagnostics_)
    {
        std::cout << "Building Z3 Mutual Exclusion Constraints @ " << step
                  << " took: " << io::getTiming(mutual_exclusions_.onestep) << std::endl;
    }
}

void Z3Planner::addMutualExclusion(Time step)
{
    if (dump_clauses_)
        std::cout << ";; Adding Mutual Exclusion Constraints @ " << step << ":" << std::endl;

    mutual_exclusions_.start();

    // add mutual exclusion axioms, i.e., we only take one action each time step
    for (unsigned int i = 0; i < ground_actions_.size(); ++i)
    {
        const auto &action = ground_actions_[i];
        for (unsigned int j = i + 1; j < ground_actions_.size(); ++j)
        {
            const auto &other_action = ground_actions_[j];

            z3::expr e = toBool(addTimeToString(action.string, step));
            z3::expr other_e = toBool(addTimeToString(other_action.string, step));
            z3::expr clause = (not e) or (not other_e);

            solver_->add(clause);

            mutual_exclusions_.inc();
            if (dump_clauses_)
                std::cout << clause << std::endl;
        }
    }

    mutual_exclusions_.stop();
    if (diagnostics_)
    {
        std::cout << "Building Z3 Mutual Exclusion Constraints @ " << step
                  << " took: " << io::getTiming(mutual_exclusions_.onestep) << std::endl;
    }
}

void Z3Planner::addGoalExpression(Time step)
{
    if (dump_clauses_)
        std::cout << ";; Adding Goal Expression @ " << step << ":" << std::endl;

    goal_expression_.start();

    // Add the goal
    Assignment empty_assgn(&*problem_);
    z3::expr clause = exprAtTime(problem_->goal, empty_assgn, step);
    solver_->add(clause);

    if (dump_clauses_)
        std::cout << clause << std::endl;

    goal_expression_.stop();
    if (diagnostics_)
    {
        std::cout << "Building Z3 Goal Expression @ " << step
                  << " took: " << io::getTiming(goal_expression_.onestep) << std::endl;
    }
}

void Z3Planner::addStartState()
{
    if (dump_clauses_)
        std::cout << ";; Adding Start State @ 0:" << std::endl;

    start_propositions_.start();

    // Add the start state
    for (const auto &name_predptr : problem_->domain->predicates)
    {
        if (problem_->domain->isDerivedPredicate(name_predptr.first))
            continue;
        const auto &assignments = problem_->getGroundings(name_predptr.second->variables);
        for (const auto &assignment : assignments)
        {
            std::string pred_string = name_predptr.second->toID(assignment);
            z3::expr p = toBool(addTimeToString(pred_string, 0));

            // Anything that is not true is false within the initial state.

            std::vector<std::string> objects;
            for (const auto &variable : name_predptr.second->variables)
                objects.emplace_back(assignment.assigned(variable.name)->name);

            z3::expr clause = (problem_->start->isTrue(name_predptr.first, objects)) ? p : not p;

            solver_->add(clause);

            start_propositions_.inc();
            if (dump_clauses_)
                std::cout << clause << std::endl;
        }
    }

    start_propositions_.stop();
    if (diagnostics_)
    {
        std::cout << "Building Z3 Start State @ 0"
                  << " took: " << io::getTiming(start_propositions_.onestep) << std::endl;
    }
}
