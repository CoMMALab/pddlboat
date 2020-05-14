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

Z3Planner::GroundedAction::GroundedAction(const ActionPtr &action, const Assignment &assignment)
  : action(action), assignment(assignment), string(action->toID(assignment))
{
}

Z3Planner::GroundedDerived::GroundedDerived(const DerivedPredicateDefinitionPtr &derived,
                                            const Assignment &assignment)
  : derived(derived), assignment(assignment), string(derived->toID(assignment))
{
}

Z3Planner::GroundedPredicate::GroundedPredicate(const PredicateDefinitionPtr &predicate,
                                                const Assignment &assignment)
  : predicate(predicate), assignment(assignment), string(predicate->toID(assignment))
{
}

Z3Planner::PredAction::PredAction(const GroundedActionPtr &action, bool relies, bool effects)
  : action(action), relies(relies), effects(effects)
{
}

Z3Planner::Z3Planner(const ProblemPtr problem, const Options &options)
  : Planner(problem), options_(options), ctx_(new z3::context), solver_(new z3::solver(*ctx_))
{
    checkRequirements();

    if (options_.parallel)
        z3::set_param("parallel.enable", true);

    // Generate the groundings for this problem.
    generateGroundings();
}

Z3Planner::~Z3Planner()
{
    delete solver_;
    delete ctx_;
}

std::ostream &Z3Planner::printSMT2(std::ostream &out) const
{
    out << solver_->to_smt2();
    return out;
}

Requirements::Keyword Z3Planner::getSupportedRequirements() const
{
    return Requirements::ADL | Requirements::DERIVED_PREDICATES;
}

void Z3Planner::addStep(Time step, bool incremental)
{
    // At each step, must add:
    // (1) Actions imply their preconditions are true and effects are true
    addActionImplications(step);

    // (2) Predicates do not change unless an action changes them
    addFrameAxioms(step);

    if (options_.simultaneous)
        // (3) Only actions that don't conflict, if enabled
        addMutualExclusionLinearSimultaneous(step);
    else
        // (3) Only one action at a time
        addMutualExclusionLinear(step);

    // (4) Derived predicates take on their value
    addDerivedPredicates(step + 1);

    // Solve and push assertions if solving incrementally
    if (incremental)
        solver_->check();

    // Push another frame onto the Z3 solver
    solver_->push();
}

bool Z3Planner::solve(Plan &plan)
{
    boost::timer::cpu_timer t;

    // setup initial problem state
    addStartState();
    addDerivedPredicates(0);
    solver_->push();

    // add all predicates up to the minimum horizon to consider
    for (Time step = 0; step < options_.horizon.min; ++step)
        addStep(step, false);

    // incrementally solve up to maximum horizon
    bool sat = false;
    for (Time step = options_.horizon.min; step < options_.horizon.max and not sat; ++step)
    {
        boost::timer::cpu_timer tl;

        addStep(step, true);
        addGoalExpression(step + 1);

        auto result = solver_->check();

        tl.stop();
        if (options_.diagnostics)
            std::cout << "Solving Step " << step << " Z3 Instance took: " << io::getTiming(tl) << std::endl;

        sat = result == z3::sat;

        // pop goal expression from solver and continue on if not satisfied
        if (not sat)
            solver_->pop();
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
        std::cout << solver_->statistics() << std::endl;
    }

    if (sat)
        return buildPlan(plan);

    return false;
}

std::ostream &Z3Planner::printModel(std::ostream &out, bool only_true) const
{
    if (not solver_)
        return out;

    auto m = solver_->get_model();

    std::vector<std::vector<std::pair<ast::z3Action, bool>>> actions(options_.horizon.max + 1);
    std::vector<std::vector<std::pair<ast::z3Predicate, bool>>> predicates(options_.horizon.max + 1);

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

    for (std::size_t i = 0; i <= options_.horizon.max; ++i)
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

bool Z3Planner::buildPlan(Plan &plan) const
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
    {
        try
        {
            plan.addStep(action.name, action.args);
        }
        catch (std::exception &e)
        {
            std::cerr << e.what() << std::endl;
            return false;
        }
    }

    return true;
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
    boost::timer::cpu_timer tl;

    ground_actions_.clear();
    ground_derived_.clear();
    ground_pred_.clear();
    predicate_actions_.clear();

    auto addToPredicate = [&](const std::string &predicate, const GroundedActionPtr &action, bool relies,
                              bool effects) {
        auto it = predicate_actions_.find(predicate);
        if (it == predicate_actions_.end())
            it = predicate_actions_.emplace(predicate, std::vector<PredAction>{}).first;

        it->second.emplace_back(action, relies, effects);
    };

    for (const auto &name_actionptr : problem_->domain->actions)
    {
        const auto &assignments = problem_->getGroundings(name_actionptr.second->parameters);
        for (const auto &assignment : assignments)
        {
            ground_actions_.emplace_back(std::make_shared<GroundedAction>(name_actionptr.second, assignment),
                                         true, true);

            // Get all predicates this grounded action effects
            std::set<std::string> effects, preconds;
            name_actionptr.second->precondition->getUsedPredicates(preconds, problem_->start, assignment);
            name_actionptr.second->effect->getAssignedPredicates(effects, problem_->start, assignment);

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
    for (const auto &name_predicateptr : problem_->domain->predicates)
    {
        if (problem_->domain->isDerivedPredicate(name_predicateptr.first))
            continue;

        const auto &assignments = problem_->getGroundings(name_predicateptr.second->variables);
        for (const auto &assignment : assignments)
        {
            auto it = predicate_actions_.find(name_predicateptr.second->toID(assignment));
            // check if predicate is never effected
            if (it != predicate_actions_.end())
            {
                bool effected = false;
                for (const auto &action : it->second)
                    effected |= action.effects;

                if (not effected)
                    ground_pred_.emplace_back(name_predicateptr.second, assignment);
            }
        }
    }

    // get all derived predicate groundings
    for (const auto &name_derivedptr : problem_->domain->derived)
    {
        const auto &assignments = problem_->getGroundings(name_derivedptr.second->parameters);
        for (const auto &assignment : assignments)
            ground_derived_.emplace_back(name_derivedptr.second, assignment);
    }

    if (options_.diagnostics)
        std::cout << "Generating Problem Groundings took: " << io::getTiming(tl) << std::endl;
}

void Z3Planner::addActionImplications(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Action Implications @ " << step << ":" << std::endl;

    action_implications_.start();

    // add clauses for preconditions and effects of actions
    for (const auto &pred : ground_actions_)
    {
        const auto &action = pred.action;

        z3::expr act = toBool(addTimeToString(action->string, step));
        z3::expr precond = exprAtTime(action->action->precondition, action->assignment, step);
        z3::expr effects = exprAtTime(action->action->effect, action->assignment, step + 1);

        // action -> preconditions_t0 and effects_t1
        z3::expr clause = imply(act, precond and effects);

        solver_->add(clause);

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

void Z3Planner::addDerivedPredicates(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Derived Predicates @ " << step << ":" << std::endl;

    derived_predicates_.start();

    for (const auto &derived : ground_derived_)
    {
        z3::expr pred = toBool(addTimeToString(derived.string, step));
        z3::expr expr = exprAtTime(derived.derived->expression, derived.assignment, step);

        z3::expr clause = pred == expr;
        solver_->add(clause);

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

void Z3Planner::addFrameAxioms(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Frame Axioms @ " << step << ":" << std::endl;

    frame_axioms_.start();
    for (const auto &[predicate, actions] : predicate_actions_)
    {
        // add frame axioms
        // if the prop changed, then an action changed it. it doesn't matter what the value
        // changes to, because the clauses for precoditions and effects will ensure
        // consistency
        z3::expr curr_prop = toBool(addTimeToString(predicate, step));
        z3::expr next_prop = toBool(addTimeToString(predicate, step + 1));

        z3::expr actionex = ctx_->bool_val(false);
        for (std::size_t i = 0; i < actions.size(); ++i)
            if (actions[i].effects)
                actionex = actionex or toBool(addTimeToString(actions[i].action->string, step));

        z3::expr clause = imply(curr_prop != next_prop, actionex);

        solver_->add(clause);

        frame_axioms_.inc();
        if (options_.dump_clauses)
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

void Z3Planner::addMutualExclusionLinearHelper(const std::vector<PredAction> &actions, Time step,
                                               const std::string &prefix, bool forward)
{
    const std::string pp = prefix + std::string(forward ? "F" : "B") + ":";

    std::size_t intermediate = 0;
    z3::expr current = toBool(addTimeToString(pp + log::format("%1%", intermediate), step));

    for (unsigned int i = 0; i < actions.size(); ++i)
    {
        std::size_t index = (forward) ? i : actions.size() - i - 1;
        const PredAction &pred = actions[index];
        const GroundedActionPtr &action = pred.action;

        z3::expr op = toBool(addTimeToString(action->string, step));
        z3::expr clause = ctx_->bool_val(true);

        if (pred.relies)
        {
            z3::expr next = toBool(addTimeToString(pp + log::format("%1%", ++intermediate), step));
            clause = clause and imply(current, not op) and imply(current, next);
            current = next;
        }

        if (pred.effects)
            clause = clause and imply(op, current);

        solver_->add(clause);

        mutual_exclusions_.inc();
        if (options_.dump_clauses)
            std::cout << clause << std::endl;
    }
}

void Z3Planner::addMutualExclusionLinearSimultaneous(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Mutual Exclusion Constraints @ " << step << ":" << std::endl;

    mutual_exclusions_.start();

    for (const auto &[predicate, actions] : predicate_actions_)
    {
        addMutualExclusionLinearHelper(actions, step, predicate, true);
        addMutualExclusionLinearHelper(actions, step, predicate, false);
    }

    mutual_exclusions_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Mutual Exclusion Constraints @ " << step
                  << " took: " << io::getTiming(mutual_exclusions_.onestep) << std::endl;
    }
}

void Z3Planner::addMutualExclusionLinear(Time step)
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

void Z3Planner::addMutualExclusion(Time step)
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

            z3::expr e = toBool(addTimeToString(action.string, step));
            z3::expr other_e = toBool(addTimeToString(other_action.string, step));
            z3::expr clause = (not e) or (not other_e);

            solver_->add(clause);

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

void Z3Planner::addGoalExpression(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Goal Expression @ " << step << ":" << std::endl;

    goal_expression_.start();

    // Add the goal
    Assignment empty_assgn(&*problem_);
    z3::expr clause = exprAtTime(problem_->goal, empty_assgn, step);
    solver_->add(clause);

    if (options_.dump_clauses)
        std::cout << clause << std::endl;

    goal_expression_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Goal Expression @ " << step
                  << " took: " << io::getTiming(goal_expression_.onestep) << std::endl;
    }
}

void Z3Planner::addStartState()
{
    if (options_.dump_clauses)
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
            if (options_.dump_clauses)
                std::cout << clause << std::endl;
        }
    }

    start_propositions_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Start State @ 0"
                  << " took: " << io::getTiming(start_propositions_.onestep) << std::endl;
    }
}
