/* Author: Zachary Kingston */

#include <pddlboat/utility.h>
#include <pddlboat/solver/domain.h>

using namespace pddlboat;

///
/// Objects
///

Object::Object(const std::string &name, const std::string &type) : name(name), type(type)
{
}

ObjectPtr pddlboat::makeObj(const std::string &name, const std::string &type)
{
    return std::make_shared<Object>(name, type);
}

///
/// VariableSet
///

Variable::Variable(const std::pair<std::string, std::string> &both) : name(both.first), type(both.second)
{
}

Variable::Variable(const std::string &name, const std::string &type) : name(name), type(type)
{
}

bool Variable::operator<(const Variable &other) const
{
    return name < other.name;
}

bool Variable::operator==(const Variable &other) const
{
    return name == other.name and type == other.type;
}

bool Variable::operator!=(const Variable &other) const
{
    return name != other.name or type != other.type;
}

VariableSet::VariableSet() = default;

VariableSet::VariableSet(const VariableList &variables)
{
    for (const auto &variable : variables)
        addVariable(variable);
}

void VariableSet::merge(const VariableSet &other)
{
    for (const auto &variable : other.variables)
        addVariable(variable);
}

void VariableSet::addVariable(const Variable &var)
{
    auto it = std::find_if(variables.begin(), variables.end(),
                           [&var](const Variable &other) { return var.name == other.name; });
    if (it != variables.end())
    {
        if (var.type != it->type)
            throw std::invalid_argument(
                log::format("Variable %1% has inconsistent types! %2% / %3%", var.name, var.type, it->type));
    }
    else
        variables.emplace(var);
}

std::set<std::string> VariableSet::getNames() const
{
    std::set<std::string> names;
    for (const auto &variable : variables)
        names.emplace(variable.name);

    return names;
}

const Variable &VariableSet::getVariable(const std::string &name) const
{
    auto it = variables.find(name);
    if (it != variables.end())
        return *it;

    throw std::invalid_argument(log::format("Variable %1% doesn't exist!", name));
}

bool VariableSet::operator==(const VariableSet &other) const
{
    return variables == other.variables;
}

bool VariableSet::operator!=(const VariableSet &other) const
{
    return variables != other.variables;
}

///
/// Assignment
///

Assignment::Assignment(const Problem *problem) : problem(problem)
{
}

unsigned int Assignment::arity() const
{
    return varToObj.size();
}

void Assignment::assign(const std::string &var, const std::string &obj)
{
    auto object = problem->getObject(obj);
    if (not object)
        throw std::invalid_argument(log::format("Cannot assign %1% to %2%, %1% does not exist!", obj, var));

    auto [it, r] = varToObj.emplace(var, object);
    if (not r)
        it->second = object;
}

const ObjectPtr &Assignment::assigned(const std::string &var) const
{
    auto it = varToObj.find(var);
    if (it == varToObj.end())
        throw std::invalid_argument(log::format("Variable %1% not assigned!", var));

    return it->second;
}

std::set<std::string> Assignment::getVariables() const
{
    std::set<std::string> variables;
    for (const auto &assgn : varToObj)
        variables.emplace(assgn.first);

    return variables;
}

///
/// Predicates
///

Predicate::Predicate(const std::string &name, const VariableList &variables)
  : name(name), variables(variables)
{
}

BoundPredicatePtr Predicate::bind() const
{
    return bind(std::vector<std::string>{});
}

BoundPredicatePtr Predicate::bind(const std::string &bound) const
{
    return bind({bound});
}

BoundPredicatePtr Predicate::bind(const std::string &boundA, const std::string &boundB) const
{
    return bind({boundA, boundB});
}

BoundPredicatePtr Predicate::bind(const std::vector<std::string> &bound) const
{
    if (variables.size() != bound.size())
        throw std::invalid_argument("Cannot bind variables, size mismatch!");

    VariableList toUse;
    for (unsigned int i = 0; i < bound.size(); ++i)
        toUse.emplace_back(bound[i], variables[i].type);

    return std::make_shared<BoundPredicate>(this, toUse);
}

unsigned int Predicate::arity() const
{
    return variables.size();
}

PredicatePtr pddlboat::makePred(const std::string &name, const VariableList &variables)
{
    return std::make_shared<Predicate>(name, variables);
}

BoundPredicate::BoundPredicate(const Predicate *predicate, const VariableList &variables)
  : variables(variables), predicate(predicate)
{
}

bool BoundPredicate::evaluate(const StatePtr &state, const Assignment &vars) const
{
    std::vector<std::string> objects;
    for (const auto &variable : variables)
        objects.emplace_back(vars.assigned(variable.name)->name);

    return state->isTrue(predicate->name, objects);
}

Expression::Type BoundPredicate::type() const
{
    return Expression::PREDICATE;
}

void BoundPredicate::getVariables(VariableSet &vars) const
{
    for (const auto &variable : variables)
        vars.addVariable(variable);
}

bool BoundPredicate::containsVariable(const std::string &variable) const
{
    for (const auto &v : variables)
        if (v.name == variable)
            return true;

    return false;
}

bool BoundPredicate::containsPredicate(const std::string &pred) const
{
    return predicate->name == pred;
}

///
/// Domain
///

Domain::Domain(const std::string &name) : name(name)
{
}

const ObjectPtr Domain::getConstant(const std::string &name) const
{
    auto it = std::find_if(constants.begin(), constants.end(),
                           [&name](const ObjectPtr &other) { return name == other->name; });

    if (it != constants.end())
        return *it;

    return nullptr;
}

void Domain::addConstant(const ObjectPtr &object)
{
    const auto &other = getConstant(object->name);
    if (other)
    {
        if (object->type != other->type)
            throw std::invalid_argument(  //
                log::format("Tried adding constant %1% with type %2%, but already exists with type %3%",
                            object->name, object->type, other->type));
        else
            throw std::invalid_argument(  //
                log::format("Tried adding constant %1%, but already exists", object->name));
    }
    else
        constants.emplace_back(object);
}

void Domain::addConstant(const std::string &name, const std::string &type)
{
    addConstant(makeObj(name, type));
}

void Domain::addType(const std::string &type, const std::string &parent)
{
    auto [it, r] = types.emplace(type, parent);
    if (not r)
        it->second = parent;
}

bool Domain::isType(const std::string &type) const
{
    return types.find(type) != types.end();
}

bool Domain::isTypeOf(const std::string &type, const std::string &other) const
{
    bool r = false;
    auto view = std::cref(type);
    while (not(r = view.get() != other) and view.get() != PDDLBOAT_OBJECT_CLASS)
    {
        auto it = types.find(view);
        view = std::cref(it->second);
    }

    return r;
}

void Domain::addPredicate(const PredicatePtr &predicate)
{
    auto [it, r] = predicates.emplace(predicate->name, predicate);
    if (not r)
        it->second = predicate;
}

const PredicatePtr Domain::getPredicate(const std::string &name) const
{
    auto it = predicates.find(name);
    if (it == predicates.end())
        throw std::invalid_argument(log::format("Predicate %1% does not exist!", name));

    return it->second;
}

///
/// Problem
///

Problem::Problem(const DomainPtr &domain)
{
}

const ObjectPtr Problem::getObject(const std::string &name) const
{
    auto it = std::find_if(objects.begin(), objects.end(),
                           [&name](const ObjectPtr &other) { return name == other->name; });

    if (it != objects.end())
        return *it;

    return nullptr;
}

void Problem::addObject(const ObjectPtr &object)
{
    const auto &constant = domain->getConstant(object->name);
    if (constant)
    {
        if (object->type != constant->type)
            throw std::invalid_argument(  //
                log::format("Tried adding object %1% with type %2%, but constant exists with type %3%",
                            object->name, object->type, constant->type));
        else
            throw std::invalid_argument(  //
                log::format("Tried adding object %1%, but constant already exists", object->name));
    }
    else
    {
        const auto &other = getObject(object->name);
        if (other)
        {
            if (object->type != other->type)
                throw std::invalid_argument(  //
                    log::format("Tried adding object %1% with type %2%, but object exists with type %3%",
                                object->name, object->type, other->type));
            else
                throw std::invalid_argument(  //
                    log::format("Tried adding object %1%, but object already exists", object->name));
        }
        else
            objects.emplace_back(object);
    }
}

void Problem::addObject(const std::string &name, const std::string &type)
{
    addObject(makeObj(name, type));
}

const std::vector<std::string> Problem::objectsOfType(const std::string &type) const
{
    std::vector<std::string> r;
    for (const auto &object : objects)
    {
        if (domain->isTypeOf(object->type, type))
            r.emplace_back(object->name);
    }

    for (const auto &constant : domain->constants)
    {
        if (domain->isTypeOf(constant->type, type))
            r.emplace_back(constant->name);
    }

    return r;
}

StatePtr Problem::makeState() const
{
    return std::make_shared<State>(this);
}

///
/// State
///

State::State(const Problem *problem) : problem_(problem)
{
}

bool State::isTrue(const std::string &predicate, const std::vector<std::string> &objects) const
{
    auto it = assignments.find(std::make_pair(predicate, objects));
    return (it != assignments.end()) ? it->second : false;
}

// Assign 0-ary pred.
void State::assign0(const std::string &predicate, bool value)
{
    assignN(predicate, {}, value);
}

// Assign 1-ary pred.
void State::assign1(const std::string &predicate, const std::string &object, bool value)
{
    const auto &pred = problem_->domain->getPredicate(predicate);
    if (pred->arity() != 1)
        throw std::invalid_argument(log::format("Predicate %1% is not 1-ary!", predicate));

    // const auto &var = *pred->variables.getNames().begin();
    assignN(predicate, {object}, value);
}

// Assign n-ary pred.
void State::assignN(const std::string &predicate, const std::vector<std::string> &objects, bool value)
{
    const auto &pred = problem_->domain->getPredicate(predicate);

    // if (not pred->validAssignment(problem_, objects))
    //     throw std::invalid_argument(log::format("Invalid assignment to predicate %1%!", predicate));

    auto [it, r] =
        assignments.emplace(std::piecewise_construct,  //
                            std::forward_as_tuple(predicate, objects), std::forward_as_tuple(value));
    if (not r)
        it->second = value;
}

///
/// Actions
///

Action::Action(const std::string &name, const VariableSet &parameters, const ExpressionPtr &precondition,
               const ExpressionPtr &effect)
  : name(name), parameters(parameters), precondition(precondition), effect(effect)
{
    VariableSet in;
    precondition->getVariables(in);
    effect->getVariables(in);

    if (in != parameters)
        throw std::invalid_argument("Action parameter set does not match expressions!");
}

ActionPtr pddlboat::makeAction(const std::string &name, const VariableSet &parameters,
                               const ExpressionPtr &preconditions, const ExpressionPtr &effects)
{
    return std::make_shared<Action>(name, parameters, preconditions, effects);
}
