/* Author: Andrew Wells and Zachary Kingston */

#include <pddlboat/solver/z3planner.h>

using namespace pddlboat;

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

//
// Groundings
//

GroundedAction::GroundedAction(const ActionPtr &action, const Assignment &assignment)
  : action(action), assignment(assignment), string(action->toID(assignment))
{
}

GroundedFluent::GroundedFluent(const ObjectFunctionDefinitionPtr &fluent, const Assignment &assignment)
  : fluent(fluent), assignment(assignment), string(fluent->toID(assignment))
{
}

GroundedDerived::GroundedDerived(const DerivedPredicateDefinitionPtr &derived, const Assignment &assignment)
  : derived(derived), assignment(assignment), string(derived->toID(assignment))
{
}

GroundedPredicate::GroundedPredicate(const PredicateDefinitionPtr &predicate, const Assignment &assignment)
  : predicate(predicate), assignment(assignment), string(predicate->toID(assignment))
{
}

PredAction::PredAction(const GroundedActionPtr &action, bool relies, bool effects)
  : action(action), relies(relies), effects(effects)
{
}

//
// Z3 Constructs
//

Z3Planner::Z3Function::Z3Function(const std::string &name, const Z3Planner *planner)
  : name(name)
  , domains(*planner->ctx_)
  , range(planner->ctx_->bool_sort())
  , function(*planner->ctx_)
  , planner(planner)
{
}

void Z3Planner::Z3Function::addTypeArgument(const Z3DataTypePtr &type)
{
    args.push_back(type);
    domains.push_back(type->sort);
}

void Z3Planner::Z3Function::addIntArgument()
{
    args.push_back(nullptr);
    domains.push_back(planner->ctx_->int_sort());
}

void Z3Planner::Z3Function::addBoolArgument()
{
    args.push_back(nullptr);
    domains.push_back(planner->ctx_->bool_sort());
}

void Z3Planner::Z3Function::setTypeReturn(const Z3DataTypePtr &type)
{
    range = type->sort;
}

void Z3Planner::Z3Function::make()
{
    function = planner->ctx_->function(name.c_str(), domains, range);
}

z3::expr Z3Planner::Z3Function::operator()() const
{
    return function();
}

z3::expr Z3Planner::Z3Function::operator()(const z3::expr &arg) const
{
    return function(arg);
}

z3::expr Z3Planner::Z3Function::operator()(const z3::expr_vector &args) const
{
    return function(args);
}

Z3Planner::Z3DataType::Z3DataType(const std::string &type, const Z3Planner *planner)
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
        auto it = values.emplace(objects[i], i);
        names[i] = it.first->first.c_str();
    }

    sort = planner->ctx_->enumeration_sort(type.c_str(), values.size(), names, consts, testers);
}

z3::expr Z3Planner::Z3DataType::getConstant(const std::string &object) const
{
    auto it = values.find(object);
    if (it == values.end())
        throw std::runtime_error(log::format("Invalid object %1% in type %2%!", object, name));

    return consts[it->second]();
}

z3::expr Z3Planner::Z3DataType::getVariable(const std::string &name) const
{
    return planner->ctx_->constant(name.c_str(), sort);
}

void Z3Planner::addZ3DataType(const std::string &type)
{
    auto dt = makeZ3DataType(type);
    z3_types_.emplace(type, dt);
}

Z3Planner::Z3DataTypePtr Z3Planner::makeZ3DataType(const std::string &type) const
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

            solver_->add(clause);
        }
    }

    return dt;
}

const Z3Planner::Z3DataTypePtr &Z3Planner::getZ3DataType(const std::string &type) const
{
    auto it = z3_types_.find(type);
    if (it != z3_types_.end())
        return it->second;

    throw std::runtime_error(log::format("Invalid type %1%", type));
}

z3::expr Z3Planner::castZ3Type(const std::string &to, const z3::expr &e) const
{
    z3::expr casted = e;
    std::string type = getType(e);

    while (type != to)
    {
        auto dt = getZ3DataType(type);
        casted = dt->upcast(casted);
        type = dt->parent;
    }

    return casted;
}

void Z3Planner::castZ3TypeArgs(z3::expr_vector &args, const Z3Function &function) const
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

Z3Planner::Z3FunctionPtr Z3Planner::makeZ3Function(const std::string &name,
                                                   const VariableList &arguments) const
{
    auto function = std::make_shared<Z3Function>(name, this);
    for (const auto &arg : arguments)
        function->addTypeArgument(getZ3DataType(arg.type));

    function->addIntArgument();
    function->make();
    return function;
}

const Z3Planner::Z3FunctionPtr &Z3Planner::getZ3Predicate(const std::string &name) const
{
    auto it = z3_predicates_.find(name);
    if (it == z3_predicates_.end())
        throw std::runtime_error("Invalid predicate " + name);

    return it->second;
}

const Z3Planner::Z3FunctionPtr &Z3Planner::getZ3Fluent(const std::string &name) const
{
    auto it = z3_fluents_.find(name);
    if (it == z3_fluents_.end())
        throw std::runtime_error("Invalid fluent " + name);

    return it->second;
}

const Z3Planner::Z3FunctionPtr &Z3Planner::getZ3Action(const std::string &name) const
{
    auto it = z3_actions_.find(name);
    if (it == z3_actions_.end())
        throw std::runtime_error("Invalid action " + name);

    return it->second;
}

z3::expr Z3Planner::getZ3Value(const Value &value, const Assignment &assignment, Time step) const
{
    if (value.function)
        return toZ3Fluent(value.function, assignment, step);

    else if (value.isVar)
    {
        if (assignment.isAssigned(value.name))
        {
            auto assigned = assignment.assigned(value.name);
            auto dt = getZ3DataType(assigned->type);
            return dt->getConstant(assigned->name);
        }
        else
        {
            auto dt = getZ3DataType(assignment.typed(value.name));
            return dt->getConstant(value.name);
        }
    }
    else
    {
        auto dt = getZ3DataType(value.type);
        return dt->getConstant(value.name);
    }
}

z3::expr Z3Planner::getZ3Value(const Variable &variable, const Assignment &assignment) const
{
    auto assigned = assignment.assigned(variable.name);
    auto dt = getZ3DataType(assigned->type);
    return dt->getConstant(assigned->name);
}

z3::expr_vector Z3Planner::getZ3Values(const std::vector<Value> &bindings, const Assignment &assignment,
                                       Time step) const
{
    z3::expr_vector args(*ctx_);
    for (const auto &binding : bindings)
        args.push_back(getZ3Value(binding, assignment, step));

    return args;
}

z3::expr_vector Z3Planner::getZ3Values(const VariableList &variables, const Assignment &assignment) const
{
    z3::expr_vector args(*ctx_);
    for (const auto &variable : variables)
        args.push_back(getZ3Value(variable, assignment));

    return args;
}

z3::expr_vector Z3Planner::getZ3Variables(const VariableList &parameters, Assignment &assignment) const
{
    z3::expr_vector args(*ctx_);
    for (const auto &parameter : parameters)
    {
        auto dt = getZ3DataType(parameter.type);
        args.push_back(dt->getVariable(parameter.name));
        assignment.type(parameter.name, parameter.type);
    }

    return args;
}

z3::expr Z3Planner::toZ3Predicate(const PredicatePtr &predicate, const Assignment &assignment,
                                  Time step) const
{
    const auto &function = getZ3Predicate(predicate->name());
    auto args = getZ3Values(predicate->bindings, assignment, step);
    args.push_back(toZ3Time(step));

    return (*function)(args);
}

z3::expr Z3Planner::toZ3Predicate(const PredicateDefinitionPtr &predicate, const Assignment &assignment,
                                  Time step) const
{
    const auto &function = getZ3Predicate(predicate->name);
    auto args = getZ3Values(predicate->variables, assignment);
    args.push_back(toZ3Time(step));
    castZ3TypeArgs(args, *function);

    return (*function)(args);
}

z3::expr Z3Planner::toZ3Predicate(const DerivedPredicatePtr &predicate, const Assignment &assignment,
                                  Time step) const
{
    const auto &function = getZ3Predicate(predicate->name());
    auto args = getZ3Values(predicate->bindings, assignment, step);
    args.push_back(toZ3Time(step));
    castZ3TypeArgs(args, *function);

    return (*function)(args);
}

z3::expr Z3Planner::toZ3Predicate(const GroundedPredicate &grounding, Time step) const
{
    const auto &predicate = grounding.predicate;

    const auto &function = getZ3Predicate(predicate->name);
    auto args = getZ3Values(predicate->variables, grounding.assignment);
    args.push_back(toZ3Time(step));
    castZ3TypeArgs(args, *function);

    return (*function)(args);
}

z3::expr Z3Planner::toZ3Predicate(const GroundedDerived &grounding, Time step) const
{
    const auto &derived = grounding.derived;

    const auto &function = getZ3Predicate(derived->name);
    auto args = getZ3Values(derived->parameters, grounding.assignment);
    args.push_back(toZ3Time(step));
    castZ3TypeArgs(args, *function);

    return (*function)(args);
}

z3::expr Z3Planner::toZ3Action(const ActionPtr &action, const Assignment &assignment, Time step) const
{
    const auto &function = getZ3Action(action->name);
    auto args = getZ3Values(action->parameters, assignment);
    args.push_back(toZ3Time(step));
    castZ3TypeArgs(args, *function);

    return (*function)(args);
}

z3::expr Z3Planner::toZ3Fluent(const ObjectFunctionPtr &fluent, const Assignment &assignment, Time step) const
{
    const auto &function = getZ3Fluent(fluent->getName());
    auto args = getZ3Values(fluent->bindings, assignment, step);
    args.push_back(toZ3Time(step));
    castZ3TypeArgs(args, *function);

    return (*function)(args);
}

z3::expr Z3Planner::toZ3Fluent(const ObjectFunctionDefinitionPtr &fluent, const Assignment &assignment,
                               Time step) const
{
    const auto &function = getZ3Fluent(fluent->name);
    auto args = getZ3Values(fluent->parameters, assignment);
    args.push_back(toZ3Time(step));
    castZ3TypeArgs(args, *function);

    return (*function)(args);
}

z3::expr Z3Planner::toZ3Fluent(const GroundedFluent &grounding, Time step) const
{
    const auto &fluent = grounding.fluent;

    const auto &function = getZ3Fluent(fluent->name);
    auto args = getZ3Values(fluent->parameters, grounding.assignment);
    args.push_back(toZ3Time(step));
    castZ3TypeArgs(args, *function);

    return (*function)(args);
}

z3::expr Z3Planner::toZ3Time(Time time) const
{
    return ctx_->int_val((int)time);
}

//
// Planner
//

Z3Planner::Z3Planner(const ProblemPtr problem, const Options &options)
  : Planner(problem), options_(options), ctx_(new z3::context), solver_(new z3::solver(*ctx_))
{
    if (options_.parallel)
        z3::set_param("parallel.enable", true);

    checkRequirements();
    generateFunctions();
    generateGroundings();
}

Z3Planner::~Z3Planner()
{
    z3_types_.clear();
    z3_actions_.clear();
    z3_fluents_.clear();
    z3_predicates_.clear();
    z3_effect_exclusion_.clear();
    mutex_auxilliary_.reset();
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
    return Requirements::ADL | Requirements::DERIVED_PREDICATES | Requirements::OBJECT_FLUENTS;
    // return Requirements::ADL | Requirements::DERIVED_PREDICATES;
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

    // (4) Early termination check
    addEarlyTermination(step);

    // (5) Derived predicates take on their value
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
    Time step = options_.horizon.min;
    for (; step < options_.horizon.max and not sat; ++step)
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
        return buildPlan(plan, step);

    return false;
}

std::ostream &Z3Planner::printModel(std::ostream &out, bool only_true) const
{
    if (not solver_)
        return out;

    auto m = solver_->get_model();
    out << m << std::endl;

    return out;
}

bool Z3Planner::buildPlan(Plan &plan, Time max) const
{
    auto model = solver_->get_model();
    // std::cout << model << std::endl;

    for (std::size_t i = 0; i < max; ++i)
    {
        for (const auto &[name, action] : problem_->domain->actions)
        {
            const auto &groundings = problem_->getGroundings(action->parameters);
            for (const auto &grounding : groundings)
            {
                if (isTrue(model.eval(toZ3Action(action, grounding, i))))
                {
                    // std::cout << action->toID(grounding) << std::endl;
                    std::vector<std::string> args;
                    for (const auto &parameter : action->parameters)
                        args.emplace_back(grounding.assigned(parameter.name)->name);

                    try
                    {
                        plan.addStep(name, args);
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

    return true;
}

/**
 * Recurses down the tree and builds a z3::expr for the expression, replacing atoms with their
 * z3 bool_constants. We require that the expression does not contain any actions (all terminals
 * are state props).
 */
z3::expr Z3Planner::getExpression(const ExpressionPtr &expr, const Assignment &assignment, Time step) const
{
    switch (expr->type())
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
        default:
            throw std::runtime_error(log::format("We had an error in parsing the pddlboat expression to an "
                                                 "expression for Z3. Unsupported expression %1%",
                                                 expr->type()));
    }
}

void Z3Planner::generateGroundings()
{
    boost::timer::cpu_timer tl;

    auto addToPredicate = [&](const std::string &predicate, const GroundedActionPtr &action, bool relies,
                              bool effects) {
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
            if (it != action_effects_.end())
            {
                bool effected = false;
                for (const auto &action : it->second)
                    effected |= action.effects;

                if (not effected)
                    ground_predicate_.emplace_back(predicate, assignment);
            }
        }
    }

    // get all fluents unassigned
    for (const auto &[name, function] : problem_->domain->functions)
    {
        auto of = std::dynamic_pointer_cast<ObjectFunctionDefinition>(function);
        if (not of)
            continue;

        const auto &assignments = problem_->getGroundings(of->parameters);
        for (const auto &assignment : assignments)
        {
            const auto &id = of->toID(assignment);
            fluent_map_.emplace(std::piecewise_construct,   //
                                std::forward_as_tuple(id),  //
                                std::forward_as_tuple(of, assignment));

            auto it = action_effects_.find(id);
            // check if predicate is never effected
            if (it != action_effects_.end())
            {
                bool effected = false;
                for (const auto &action : it->second)
                    effected |= action.effects;

                if (not effected)
                    ground_fluent_.emplace_back(of, assignment);
            }
        }
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

void Z3Planner::generateFunctions()
{
    // create all type data types
    addZ3DataType(PDDLBOAT_OBJECT_CLASS);
    for (const auto &type : problem_->domain->getTypes())
        addZ3DataType(type);

    // predicates and mutexes
    for (const auto &[name, predicate] : problem_->domain->predicates)
    {
        auto function = makeZ3Function(name, predicate->variables);
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
            auto function = makeZ3Function(name, of->parameters);
            function->setTypeReturn(getZ3DataType(of->type));
            function->make();

            z3_fluents_.emplace(name, function);

            auto mutex = makeZ3Function(name + "-mutex", of->parameters);
            mutex->addBoolArgument();  // forward / backward?
            mutex->addIntArgument();   // index
            mutex->make();             // index
            z3_effect_exclusion_.emplace(name, mutex);
        }
    }
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

        z3::expr act = toZ3Action(action->action, action->assignment, step);
        z3::expr precond = getExpression(action->action->precondition, action->assignment, step);
        z3::expr effects = getExpression(action->action->effect, action->assignment, step + 1);

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
        z3::expr pred = toZ3Predicate(derived, step);
        z3::expr expr = getExpression(derived.derived->expression, derived.assignment, step);

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

    // add frame axioms
    // if the prop changed, then an action changed it. it doesn't matter what the value
    // changes to, because the clauses for precoditions and effects will ensure
    // consistency
    for (const auto &[effect, actions] : action_effects_)
    {
        z3::expr curr(*ctx_);
        z3::expr next(*ctx_);

        const auto &it = predicate_map_.find(effect);
        if (it != predicate_map_.end())
        {
            const auto &predicate = it->second;
            curr = toZ3Predicate(predicate, step);
            next = toZ3Predicate(predicate, step + 1);
        }
        else
        {
            const auto &it = fluent_map_.find(effect);
            if (it != fluent_map_.end())
            {
                const auto &fluent = it->second;
                curr = toZ3Fluent(fluent, step);
                next = toZ3Fluent(fluent, step + 1);
            }
        }

        z3::expr actionex = ctx_->bool_val(false);
        for (std::size_t i = 0; i < actions.size(); ++i)
            if (actions[i].effects)
            {
                z3::expr actioni = toZ3Action(actions[i].action->action, actions[i].action->assignment, step);
                actionex = actionex or actioni;
            }

        z3::expr clause = imply(curr != next, actionex);

        solver_->add(clause);

        frame_axioms_.inc();
        if (options_.dump_clauses)
            std::cout << clause << std::endl;
    }

    for (const auto &predicate : ground_predicate_)
    {
        // imply that the predicate remains the same as the next time step
        z3::expr curr = toZ3Predicate(predicate, step);
        z3::expr next = toZ3Predicate(predicate, step + 1);

        z3::expr clause = curr == next;

        solver_->add(clause);

        frame_axioms_.inc();
        if (options_.dump_clauses)
            std::cout << clause << std::endl;
    }

    for (const auto &fluent : ground_fluent_)
    {
        // imply that the fluent remains the same as the next time step
        z3::expr curr = toZ3Fluent(fluent, step);
        z3::expr next = toZ3Fluent(fluent, step + 1);

        z3::expr clause = curr == next;

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

    auto auxilliary = [&](std::size_t index) -> z3::expr {
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

            z3::expr e = toZ3Action(action.action, action.assignment, step);
            z3::expr other_e = toZ3Action(other_action.action, other_action.assignment, step);
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

void Z3Planner::addEarlyTermination(Time step)
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
    Assignment empty_assgn(&*problem_);
    z3::expr clause = z3::ite(getExpression(problem_->goal, empty_assgn, step), not actions, actions);
    solver_->add(clause);

    if (options_.dump_clauses)
        std::cout << clause << std::endl;

    early_termination_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Early Termination @ " << step
                  << " took: " << io::getTiming(early_termination_.onestep) << std::endl;
    }
}

void Z3Planner::addGoalExpression(Time step)
{
    if (options_.dump_clauses)
        std::cout << ";; Adding Goal Expression @ " << step << ":" << std::endl;

    goal_expression_.start();

    // Add the goal
    Assignment empty_assgn(&*problem_);
    z3::expr clause = getExpression(problem_->goal, empty_assgn, step);
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
    for (const auto &[name, predicate] : problem_->domain->predicates)
    {
        if (problem_->domain->isDerivedPredicate(name))
            continue;

        const auto &assignments = problem_->getGroundings(predicate->variables);
        for (const auto &assignment : assignments)
        {
            z3::expr p = toZ3Predicate(predicate, assignment, 0);

            // Anything that is not true is false within the initial state.
            std::vector<std::string> objects;
            for (const auto &variable : predicate->variables)
                objects.emplace_back(assignment.assigned(variable.name)->name);

            z3::expr clause = (problem_->start->isTrue(name, objects)) ? p : not p;

            solver_->add(clause);

            start_propositions_.inc();
            if (options_.dump_clauses)
                std::cout << clause << std::endl;
        }
    }

    for (const auto &[name, function] : problem_->domain->functions)
    {
        auto of = std::dynamic_pointer_cast<ObjectFunctionDefinition>(function);
        if (of)
        {
            const auto &assignments = problem_->getGroundings(of->parameters);
            for (const auto &assignment : assignments)
            {
                z3::expr p = toZ3Fluent(of, assignment, 0);

                std::vector<std::string> objects;
                for (const auto &variable : of->parameters)
                    objects.emplace_back(assignment.assigned(variable.name)->name);

                auto dt = getZ3DataType(of->type);

                std::string value = problem_->start->getObject(name, objects);
                if (value != "")
                {
                    z3::expr clause = p == dt->getConstant(value);
                    solver_->add(clause);

                    start_propositions_.inc();
                    if (options_.dump_clauses)
                        std::cout << clause << std::endl;
                }
                // else
                //     std::cerr << of->toID(assignment) << " not set!!" << std::endl;
            }
        }
    }

    start_propositions_.stop();
    if (options_.diagnostics)
    {
        std::cout << "Building Z3 Start State @ 0"
                  << " took: " << io::getTiming(start_propositions_.onestep) << std::endl;
    }
}
