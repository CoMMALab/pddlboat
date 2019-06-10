/* Author: Andrew Wells and Zachary Kingston */

#include <pddlboat/solver/z3planner.h>

using namespace pddlboat;

Z3Planner::Z3Planner(const ProblemPtr problem, const Time horizon, bool diagnostics, bool dump_clauses)
  : Planner(problem), horizon_(horizon), diagnostics_(diagnostics), dump_clauses_(dump_clauses)
{
    build();
}
bool Z3Planner::solve(Plan &plan)
{
    boost::timer::cpu_timer t;
    if (not solver_)
        return false;

    auto result = solver_->check();

    if (diagnostics_)
    {
        t.stop();
        std::cout << "Solving Z3 Instance took: " << io::getTiming(t) << std::endl;
        std::cout << solver_->statistics() << std::endl;
    }

    switch (result)
    {
        case z3::sat:
            buildPlan(plan);
            return true;
        case z3::unsat:
            return false;
        default:
            return false;
    }
}

void Z3Planner::buildPlan(Plan &plan) const
{
    auto m = solver_->get_model();

    // As actions may appear `out of order`, we first collect them all.
    std::vector<ast::z3Action> actions;
    actions.reserve(horizon_);

    for (size_t i = 0; i < m.num_consts(); i++)
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
            return toBool(addTimeToString(predicate_expr->toID(assignment), step));
        }
        case Expression::Type::AND:
        {
            const auto &and_expr = std::static_pointer_cast<And>(expr);
            z3::expr car = exprAtTime(makeCAR(and_expr), assignment, step);
            auto cdr_expr = makeCDR(and_expr);
            if (cdr_expr)
            {
                z3::expr cdr = exprAtTime(cdr_expr, assignment, step);
                z3::expr clause = car and cdr;
                return clause;
            }
            else
                return car;
        }
        case Expression::Type::OR:
        {
            const auto &or_expr = std::static_pointer_cast<Or>(expr);
            z3::expr car = exprAtTime(or_expr->getTerms().back(), assignment, step);
            auto cdr_expr = makeCDR(or_expr);
            if (or_expr)
            {
                z3::expr cdr = exprAtTime(cdr_expr, assignment, step);
                z3::expr clause = car or cdr;
                return clause;
            }
            else
                return car;
        }
        case Expression::Type::NOT:
        {
            const auto &not_expr = std::static_pointer_cast<Not>(expr);
            z3::expr exp = exprAtTime(not_expr->getTerm(), assignment, step);
            return not exp;
        }
        case Expression::Type::EXPRESSION:
            std::cout << "Error, we do not currently support Expression types here." << std::endl;
            [[fallthrough]];  // sweet c++17 feature
        default:
            std::cout << "We had an error in parsing the pddlboat expression to an expression for Z3."
                      << std::endl;
            throw std::runtime_error("whoops!");
    }
}

void Z3Planner::actionImplications()
{
    boost::timer::cpu_timer t;
    action_imply_count_ = action_imply_total_ = 0;

    if (dump_clauses_)
        std::cout << "Action Implications:" << std::endl;

    // add clauses for preconditions and effects of actions
    for (const auto &name_actionptr : problem_->domain->actions)
    {
        const auto &assignments = problem_->getGroundings(name_actionptr.second->parameters);
        for (const auto &assignment : assignments)
        {
            std::string action_string = name_actionptr.second->toID(assignment);

            for (Time step = 0; step < horizon_; ++step)
            {
                std::string action_string_time = addTimeToString(action_string, step);

                z3::expr act = toBool(action_string_time);
                z3::expr precond = exprAtTime(name_actionptr.second->precondition, assignment, step);
                z3::expr effects = exprAtTime(name_actionptr.second->effect, assignment, step + 1);

                // action -> preconditions_t0 and effects_t1
                z3::expr clause = (not act) or (precond and effects);

                solver_->add(clause);

                action_imply_total_++;
                if (dump_clauses_)
                    std::cout << clause << std::endl;
            }

            ground_actions_.emplace_back(action_string);
            action_imply_count_++;
        }
    }

    if (diagnostics_)
    {
        t.stop();
        std::cout << "Building Z3 Action Implications took: " << io::getTiming(t) << std::endl;
    }
}

bool Z3Planner::predicateInEffect(const ExpressionPtr &expr, const Assignment &assignment,
                                  const std::string &predicate) const
{
    switch (expr->type())
    {
        case Expression::Type::PREDICATE:
        {
            const auto &predicate_expr = std::static_pointer_cast<Predicate>(expr);
            return predicate_expr->toID(assignment) == predicate;
        }
        case Expression::Type::AND:
        {
            const auto &and_expr = std::static_pointer_cast<And>(expr);
            for (const auto &term : and_expr->getTerms())
                if (predicateInEffect(term, assignment, predicate))
                    return true;

            return false;
        }
        case Expression::Type::OR:
        {
            const auto &or_expr = std::static_pointer_cast<Or>(expr);
            for (const auto &term : or_expr->getTerms())
                if (predicateInEffect(term, assignment, predicate))
                    return true;

            return false;
        }
        case Expression::Type::NOT:
        {
            const auto &not_expr = std::static_pointer_cast<Not>(expr);
            return predicateInEffect(not_expr->getTerm(), assignment, predicate);
        }
        case Expression::Type::EXPRESSION:
            std::cout << "Error, we do not currently support Expression types here." << std::endl;
            [[fallthrough]];  // sweet c++17 feature
        default:
            std::cout << "We had an error in parsing the pddlboat expression to an expression for Z3."
                      << std::endl;
            throw std::runtime_error("whoops!");
    }
}

/**
 * For frame axioms, we need to know all actions that could change a proposition
 * the time step is when the action is executed, so the change will be at the next time step
 */
z3::expr Z3Planner::getPossibleChangeActions(const std::string &predicate, const Time step_before) const
{
    z3::expr change_actions = ctx_->bool_val(false);

    for (const auto &name_actionptr : problem_->domain->actions)
    {
        const auto &assignments = problem_->getGroundings(name_actionptr.second->parameters);
        for (const auto &assignment : assignments)
        {
            if (predicateInEffect(name_actionptr.second->effect, assignment, predicate))
            {
                std::string action_string =
                    addTimeToString(name_actionptr.second->toID(assignment), step_before);
                change_actions = change_actions or toBool(action_string);
            }
        }
    }

    return change_actions;
}

void Z3Planner::frameAxioms()
{
    boost::timer::cpu_timer t;
    frame_axiom_count_ = frame_axiom_total_ = 0;

    if (dump_clauses_)
        std::cout << "Frame Axioms:" << std::endl;

    for (const auto &name_predptr : problem_->domain->predicates)
    {
        const auto &assignments = problem_->getGroundings(name_predptr.second->variables);
        for (const auto &assignment : assignments)
        {
            std::string pred_string = name_predptr.second->toID(assignment);

            for (Time step = 0; step < horizon_; ++step)
            {
                // add frame axioms
                // if the prop changed, then an action changed it. it doesn't matter what the value
                // changes to, because the clauses for precoditions and effects will ensure
                // consistency
                z3::expr curr_prop = toBool(addTimeToString(pred_string, step));
                z3::expr next_prop = toBool(addTimeToString(pred_string, step + 1));
                z3::expr clause =
                    not(curr_prop != next_prop) or (getPossibleChangeActions(pred_string, step));

                solver_->add(clause);

                frame_axiom_total_++;
                if (dump_clauses_)
                    std::cout << clause << std::endl;
            }

            frame_axiom_count_++;
        }
    }

    if (diagnostics_)
    {
        t.stop();
        std::cout << "Building Z3 Frame Axioms took: " << io::getTiming(t) << std::endl;
    }
}

void Z3Planner::mutualExclusion()
{
    boost::timer::cpu_timer t;
    mutual_exclusion_count_ = mutual_exclusion_total_ = 0;

    if (dump_clauses_)
        std::cout << "Mutual Exclusion:" << std::endl;

    // add mutual exclusion axioms, i.e., we only take one action each time step
    for (unsigned int i = 0; i < ground_actions_.size(); ++i)
    {
        const auto &action = ground_actions_[i];
        for (unsigned int j = i + 1; j < ground_actions_.size(); ++j)
        {
            const auto &other_action = ground_actions_[j];

            for (Time step = 0; step < horizon_; ++step)
            {
                z3::expr e = toBool(addTimeToString(action, step));
                z3::expr other_e = toBool(addTimeToString(other_action, step));
                z3::expr clause = (not e) or (not other_e);

                solver_->add(clause);

                mutual_exclusion_total_++;
                if (dump_clauses_)
                    std::cout << clause << std::endl;
            }

            mutual_exclusion_count_++;
        }
    }

    if (diagnostics_)
    {
        t.stop();
        std::cout << "Building Z3 Mutual Exclusion Clauses took: " << io::getTiming(t) << std::endl;
    }
}

void Z3Planner::goalExpression()
{
    boost::timer::cpu_timer t;

    // Add the goal
    Assignment empty_assgn(&*problem_);
    z3::expr clause = exprAtTime(problem_->goal, empty_assgn, horizon_);
    solver_->add(clause);

    if (dump_clauses_)
    {
        std::cout << "Goal:" << std::endl;
        std::cout << clause << std::endl;
    }

    if (diagnostics_)
    {
        t.stop();
        std::cout << "Building Z3 Goal Expression took: " << io::getTiming(t) << std::endl;
    }
}

void Z3Planner::startState()
{
    boost::timer::cpu_timer t;

    start_proposition_count_ = 0;

    if (dump_clauses_)
        std::cout << "Start State:" << std::endl;

    // Add the start state
    for (const auto &name_predptr : problem_->domain->predicates)
    {
        const auto &assignments = problem_->getGroundings(name_predptr.second->variables);
        for (const auto &assignment : assignments)
        {
            std::string pred_string = name_predptr.second->toID(assignment);
            z3::expr p = toBool(addTimeToString(pred_string, 0));

            // Anything that is not true is false within the initial state.
            z3::expr clause = (problem_->start->isTrue(name_predptr.second, assignment)) ? p : not p;

            solver_->add(clause);

            start_proposition_count_++;
            if (dump_clauses_)
                std::cout << clause << std::endl;
        }
    }

    if (diagnostics_)
    {
        t.stop();
        std::cout << "Building Z3 Start State took: " << io::getTiming(t) << std::endl;
    }
}

void Z3Planner::build()
{
    boost::timer::cpu_timer t;

    // First we build the context.
    if (ctx_)
        delete ctx_;

    ctx_ = new z3::context;

    if (solver_)
        delete solver_;

    solver_ = new z3::solver(*ctx_);

    // Now that we have built the context, we need to add constraints. These are clauses that ensure:
    // (1) an action implies its precondition is true and its effects will be true next time step
    actionImplications();

    // (2) if a proposition changes, then an action must have changed it
    frameAxioms();

    // (3) only one action is taken at each time step
    mutualExclusion();

    // (4) that we reach the goal at the time horizon, and
    goalExpression();

    // (5) that we start with the grounded predicates as declared in the start state.
    startState();

    if (diagnostics_)
    {
        t.stop();
        std::cout << "Building Total Z3 Instance took: " << io::getTiming(t) << std::endl;

        std::cout << log::format("Z3 has %1% action implication clauses per step (%2% total)",
                                 action_imply_count_, action_imply_total_)
                  << std::endl;
        std::cout << log::format("Z3 has %1% frame axiom clauses per step (%2% total)", frame_axiom_count_,
                                 frame_axiom_total_)
                  << std::endl;
        std::cout << log::format("Z3 has %1% mutual exclusion clauses per step (%2% total)",
                                 mutual_exclusion_count_, mutual_exclusion_total_)
                  << std::endl;
        std::cout << log::format("Z3 has %1% start state clauses", start_proposition_count_) << std::endl;
    }
}
