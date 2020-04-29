/* Author: Zachary Kingston */

#include <sstream>
#include <boost/algorithm/string.hpp>

#include <pddlboat/utility.h>
#include <pddlboat/solver/domain.h>
#include <pddlboat/solver/expressions.h>

using namespace pddlboat;

static const bool debug = false;

///
/// Objects
///

Object::Object(const std::string &name, const std::string &type)
  : name(boost::algorithm::to_lower_copy(name)), type(boost::algorithm::to_lower_copy(type))
{
}

ObjectPtr pddlboat::makeObj(const std::string &name, const std::string &type)
{
    return std::make_shared<Object>(name, type);
}

///
/// VariableSet
///

Variable::Variable(const std::pair<std::string, std::string> &both) : Variable(both.first, both.second)
{
}

Variable::Variable(const std::string &name, const std::string &type)
  : name(boost::algorithm::to_lower_copy(name)), type(type)
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

VariableSet::operator VariableList() const
{
    return std::vector<Variable>(variables.begin(), variables.end());
}

void VariableSet::merge(const VariableSet &other)
{
    for (const auto &variable : other.variables)
        addVariable(variable);
}

void VariableSet::remove(const VariableSet &other)
{
    for (const auto &variable : other.variables)
        removeVariable(variable);
}

void VariableSet::remove(const VariableList &other)
{
    for (const auto &variable : other)
        removeVariable(variable);
}

void VariableSet::addVariable(const Variable &var)
{
    auto it = std::find_if(variables.begin(), variables.end(),
                           [&var](const Variable &other) { return var.name == other.name; });
    if (it != variables.end())
    {
        // if (it->type != PDDLBOAT_OBJECT_CLASS and var.type != it->type)
        //     throw std::invalid_argument(
        //         log::format("Variable %1% has inconsistent types! %2% / %3%", var.name, var.type,
        //         it->type));
    }
    else
        variables.emplace(var);
}

bool VariableSet::removeVariable(const Variable &var)
{
    auto it = std::find_if(variables.begin(), variables.end(),
                           [&var](const Variable &other) { return var.name == other.name; });
    if (it != variables.end())
    {
        variables.erase(it);
        return true;
    }

    return false;
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
    auto it = variables.find(boost::algorithm::to_lower_copy(name));
    if (it != variables.end())
        return *it;

    throw std::invalid_argument(log::format("Variable %1% doesn't exist!", name));
}

std::ostream &VariableSet::toPDDL(std::ostream &out) const
{
    bool first = true;
    out << "(";
    for (const auto &variable : variables)
    {
        if (not first)
            out << " ";

        out << "?" << variable.name;
        if (variable.type != PDDLBOAT_OBJECT_CLASS)
            out << " - " << variable.type;

        first = false;
    }
    out << ")";

    return out;
}

bool VariableSet::operator==(const VariableSet &other) const
{
    return variables == other.variables;
}

bool VariableSet::operator!=(const VariableSet &other) const
{
    return variables != other.variables;
}

bool VariableSet::contains(const VariableSet &other) const
{
    for (const auto &ov : other.variables)
    {
        auto it = std::find_if(variables.begin(), variables.end(),
                               [&ov](const Variable &var) { return var.name == ov.name; });
        if (it == variables.end())
            return false;
    }

    return true;
}

///
/// Assignment
///

Assignment::Assignment(const Problem *problem) : problem(problem)
{
}

Assignment::Assignment(const Assignment &other) : problem(other.problem), varToObj(other.varToObj)
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

void Assignment::unassign(const std::string &var)
{
    auto it = varToObj.find(var);
    if (it != varToObj.end())
        varToObj.erase(it);
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

std::ostream &Assignment::toString(std::ostream &out) const
{
    out << "(assignment";
    for (const auto &[var, object] : varToObj)
        out << " (" << var << " " << object->name << ")";
    out << ")";
    return out;
}

///
/// Predicates
///

PredicateDefinition::PredicateDefinition(const std::string &name, const VariableList &variables)
  : name(boost::algorithm::to_lower_copy(name)), variables(variables)
{
}

PredicatePtr PredicateDefinition::bind() const
{
    return bind(std::vector<std::string>{});
}

PredicatePtr PredicateDefinition::bind(const std::vector<std::string> &bound, const ProblemPtr &problem,
                                       const DomainPtr &domain) const
{
    if (variables.size() != bound.size())
        throw std::invalid_argument("Cannot bind variables, size mismatch!");

    std::vector<Value> toUse;
    for (unsigned int i = 0; i < bound.size(); ++i)
        toUse.emplace_back(getValue(bound[i], &variables[i], problem, domain));

    return std::make_shared<Predicate>(this, toUse);
}

bool PredicateDefinition::isValidAssignment(const Problem *problem,
                                            const std::vector<std::string> &objects) const
{
    if (objects.size() != arity())
        return false;

    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        ObjectPtr obj;
        if (not(obj = problem->getObject(objects[i])))
            return false;

        if (not problem->domain->isTypeOf(obj->type, variables[i].type))
            return false;
    }

    return true;
}

unsigned int PredicateDefinition::arity() const
{
    return variables.size();
}

std::ostream &PredicateDefinition::toPDDL(std::ostream &out) const
{
    out << "(";
    out << name;
    for (const auto &variable : variables)
    {
        out << " ";
        out << "?" << variable.name;
        if (variable.type != PDDLBOAT_OBJECT_CLASS)
            out << " - " << variable.type;
    }

    out << ")";
    return out;
}

std::string PredicateDefinition::toID(const Assignment &vars) const
{
    std::stringstream ss;
    ss << "PRED:" << name;

    ss << ":ARGS:";
    for (unsigned int i = 0; i < variables.size(); ++i)
    {
        ss << vars.assigned(variables[i].name)->name;
        if (i < variables.size() - 1)
            ss << ",";
    }

    return ss.str();
}

PredicateDefinitionPtr pddlboat::makePred(const std::string &name, const VariableList &variables)
{
    return std::make_shared<PredicateDefinition>(name, variables);
}

Predicate::Predicate(const PredicateDefinition *predicate, const std::vector<Value> &bindings)
  : bindings(bindings), predicate(predicate)
{
}

std::vector<std::string> Predicate::getObjects(const Assignment &vars) const
{
    std::vector<std::string> objects;
    for (const auto &binding : bindings)
    {
        if (binding.isVar)
            objects.emplace_back(vars.assigned(binding.name)->name);
        else
            objects.emplace_back(binding.name);
    }

    return objects;
}

bool Predicate::evaluate(const StatePtr &state, const Assignment &vars) const
{
    bool r = state->isTrue(predicate->name, getObjects(vars));

    if (debug)
    {
        std::cout << "Expression is " << r << std::endl;
        toPDDL(std::cout);
        std::cout << std::endl;
        vars.toString(std::cout);
        std::cout << std::endl;
    }

    return r;
}

void Predicate::assign(StatePtr &out, const Assignment &vars) const
{
    out->assignN(predicate->name, getObjects(vars), true);
}

Expression::Type Predicate::type() const
{
    return Expression::PREDICATE;
}

void Predicate::getVariables(VariableSet &vars) const
{
    for (const auto &binding : bindings)
        if (binding.isVar)
            vars.addVariable({binding.name, binding.type});
}

VariableList Predicate::arguments() const
{
    VariableList list;
    for (const auto &binding : bindings)
        if (binding.isVar)
            list.emplace_back(binding.name, binding.type);

    return list;
}

bool Predicate::containsVariable(const std::string &variable) const
{
    for (const auto &v : bindings)
        if (v.isVar and v.name == variable)
            return true;

    return false;
}

bool Predicate::containsPredicate(const std::string &pred) const
{
    return predicate->name == pred;
}

std::ostream &Predicate::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent, ' ');
    for (unsigned int i = 0; i < level; ++i)
        out << SPACE;

    out << "(" << predicate->name;
    for (const auto &binding : bindings)
    {
        out << " ";
        if (binding.isVar)
            out << "?";
        out << binding.name;
    }

    out << ")";
    return out;
}

std::string Predicate::toID(const Assignment &vars) const
{
    std::stringstream ss;
    ss << "PRED:" << predicate->name;

    ss << ":ARGS:";
    for (unsigned int i = 0; i < bindings.size(); ++i)
    {
        if (bindings[i].isVar)
            ss << vars.assigned(bindings[i].name)->name;
        else
            ss << bindings[i].name;

        if (i < bindings.size() - 1)
            ss << ",";
    }

    return ss.str();
}

const std::string &Predicate::name() const
{
    return predicate->name;
}

///
/// Domain
///

Domain::Domain(const std::string &name) : name(boost::algorithm::to_lower_copy(name))
{
}

const ObjectPtr Domain::getConstant(const std::string &name) const
{
    if (constants.empty())
        return nullptr;

    const std::string lower = boost::algorithm::to_lower_copy(name);

    auto it = std::find_if(constants.begin(), constants.end(),
                           [&lower](const ObjectPtr &other) { return lower == other->name; });

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
    const std::string typel = boost::algorithm::to_lower_copy(type);
    const std::string parentl = boost::algorithm::to_lower_copy(parent);

    auto [it, r] = types.emplace(typel, parentl);
    if (not r)
        it->second = parentl;
}

bool Domain::isType(const std::string &type) const
{
    return types.find(boost::algorithm::to_lower_copy(type)) != types.end();
}

bool Domain::isTypeOf(const std::string &type, const std::string &other) const
{
    bool r = false;

    const std::string typel = boost::algorithm::to_lower_copy(type);
    const std::string otherl = boost::algorithm::to_lower_copy(other);

    // std::cout << "ITO: start" << std::endl;
    auto view = std::cref(typel);
    // std::cout << "ITO: " << view.get() << " " << otherl << std::endl;
    while (not((r = view.get() == otherl)  //
               or view.get() == PDDLBOAT_OBJECT_CLASS))
    {
        auto it = types.find(view);
        view = std::cref(it->second);
        // std::cout << "ITO: " << view.get() << " " << otherl << std::endl;
    }

    // std::cout << "ITO: " << r << std::endl;

    return r;
}

void Domain::addPredicate(const PredicateDefinitionPtr &predicate)
{
    auto [it, r] = predicates.emplace(predicate->name, predicate);
    if (not r)
        it->second = predicate;
}

const PredicateDefinitionPtr Domain::getPredicate(const std::string &name) const
{
    const std::string namel = boost::algorithm::to_lower_copy(name);

    auto it = predicates.find(namel);
    if (it == predicates.end())
        throw std::invalid_argument(log::format("Predicate %1% does not exist!", namel));

    return it->second;
}

void Domain::addAction(const ActionPtr &action)
{
    auto [it, r] = actions.emplace(action->name, action);
    if (not r)
        it->second = action;
}

const ActionPtr Domain::getAction(const std::string &name) const
{
    const std::string namel = boost::algorithm::to_lower_copy(name);

    auto it = actions.find(namel);
    if (it == actions.end())
        throw std::invalid_argument(log::format("Action %1% does not exist!", namel));

    return it->second;
}

std::ostream &Domain::toPDDL(std::ostream &out) const
{
    const unsigned int INDENT = 2;
    const std::string SPACE = std::string(INDENT, ' ');

    out << "(define (domain " << name << ")" << std::endl;
    out << SPACE << "(:requirements :strips)" << std::endl;  // TODO
    if (not types.empty())
    {
        out << SPACE << "(:types" << std::endl;
        std::vector<std::string> names;
        for (const auto &type : types)
            names.emplace_back(type.first);

        std::sort(names.begin(), names.end(), [&](const auto &a, const auto &b) { return isTypeOf(a, b); });

        for (const auto &name : names)
        {
            out << SPACE << SPACE << name;

            const auto &it = types.find(name);
            if (it->second != PDDLBOAT_OBJECT_CLASS)
                out << " - " << it->second;

            out << std::endl;
        }

        out << SPACE << ")" << std::endl;
    }

    if (not constants.empty())
    {
        out << SPACE << "(:constants" << std::endl;
        for (const auto &constant : constants)
        {
            out << SPACE << SPACE << constant->name;
            if (constant->type != PDDLBOAT_OBJECT_CLASS)
                out << " - " << constant->type;
            out << std::endl;
        }
        out << SPACE << ")" << std::endl;
    }

    out << SPACE << "(:predicates " << std::endl;
    for (const auto &key : predicates)
    {
        out << SPACE << SPACE;
        key.second->toPDDL(out) << std::endl;
    }
    out << SPACE << ")" << std::endl;

    for (const auto &key : actions)
    {
        // out << SPACE << SPACE;
        key.second->toPDDL(out, INDENT) << std::endl;
    }

    out << ")";

    return out;
}

///
/// Problem
///

Problem::Problem(const std::string &name, const DomainPtr &domain)
  : name(boost::algorithm::to_lower_copy(name)), domain(domain), start(makeState())
{
}

const ObjectPtr Problem::getObject(const std::string &name) const
{
    const std::string &namel = boost::algorithm::to_lower_copy(name);

    const auto &constant = domain->getConstant(namel);
    if (constant)
        return constant;

    auto it = std::find_if(objects.begin(), objects.end(),
                           [&namel](const ObjectPtr &other) { return namel == other->name; });

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

std::vector<std::string> Problem::objectsOfType(const std::string &type) const
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

std::vector<Assignment> Problem::getGroundings(const VariableList &vars, unsigned int index) const
{
    std::vector<Assignment> assignments;

    unsigned int remaining = vars.size() - index;
    if (not remaining)
    {
        Assignment toAdd(this);
        assignments.emplace_back(toAdd);
        return assignments;
    }

    const auto &recurse = getGroundings(vars, index + 1);

    const auto &car = vars[index];
    for (const auto &object : objectsOfType(car.type))
    {
        for (const auto &assignment : recurse)
        {
            Assignment toAdd = assignment;
            toAdd.assign(car.name, object);
            assignments.emplace_back(toAdd);
        }
    }

    return assignments;
}

StatePtr Problem::makeState() const
{
    return std::make_shared<State>(this);
}

std::ostream &Problem::toPDDL(std::ostream &out) const
{
    const unsigned int INDENT = 4;
    const std::string SPACE = std::string(INDENT, ' ');

    out << "(define (problem " << name << ")" << std::endl;
    out << SPACE << "(:domain " << domain->name << ")" << std::endl;

    if (not objects.empty())
    {
        out << SPACE << "(:objects" << std::endl;
        for (const auto &object : objects)
        {
            out << SPACE << SPACE << object->name;
            if (object->type != PDDLBOAT_OBJECT_CLASS)
                out << " - " << object->type;
            out << std::endl;
        }
        out << SPACE << ")" << std::endl;
    }

    out << SPACE << "(:init " << std::endl;
    start->toPDDL(out, INDENT * 2) << SPACE << ")" << std::endl;

    if (goal)
    {
        out << SPACE << "(:goal " << std::endl;
        goal->toPDDL(out, INDENT, 2) << std::endl;
        out << SPACE << ")" << std::endl;
    }

    out << ")";

    return out;
}

///
/// State
///

State::State(const Problem *problem) : problem_(problem)
{
}

const Problem *State::getProblem() const
{
    return problem_;
}

void State::clear()
{
    assignments.clear();
}

void State::complement()
{
    for (const auto &[key, value] : assignments)
        assignN(key.first, key.second, not value);
}

void State::fold(const StatePtr &other)
{
    for (const auto &[key, value] : other->assignments)
    {
        auto it = assignments.find(key);
        if (it == assignments.end())
            assignN(key.first, key.second, value);
    }
}

bool State::isTrue(const PredicateDefinitionPtr &predicate, const Assignment &assignment) const
{
    std::vector<std::string> objects;
    for (const auto &variable : predicate->variables)
        objects.emplace_back(assignment.assigned(variable.name)->name);

    return isTrue(predicate->name, objects);
}

bool State::isTrue(const std::string &predicate, const Assignment &assignment) const
{
    return isTrue(problem_->domain->getPredicate(predicate), assignment);
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

    std::vector<std::string> objectsl;
    for (const auto &obj : objects)
        objectsl.emplace_back(boost::algorithm::to_lower_copy(obj));

    if (not pred->isValidAssignment(problem_, objectsl))
        throw std::invalid_argument(log::format("Invalid assignment to predicate %1%!", predicate));

    auto [it, r] =
        assignments.emplace(std::piecewise_construct,  //
                            std::forward_as_tuple(predicate, objectsl), std::forward_as_tuple(value));
    if (not r)
        it->second = value;
}

std::ostream &State::toPDDL(std::ostream &out, unsigned int indent) const
{
    const std::string SPACE = std::string(indent, ' ');
    for (const auto &assignment : assignments)
    {
        if (assignment.second)
        {
            out << SPACE << "(" << assignment.first.first;
            for (const auto &obj : assignment.first.second)
                out << " " << obj;
            out << ")" << std::endl;
        }
    }

    return out;
}

///
/// Actions
///

Action::Action(const std::string &name, const VariableList &parameters, const ExpressionPtr &precondition,
               const ExpressionPtr &effect)
  : name(boost::algorithm::to_lower_copy(name))
  , parameters(parameters)
  , precondition(precondition)
  , effect(effect)
{
    VariableSet conditions, input(parameters);

    if (precondition)
        precondition->getVariables(conditions);

    if (effect)
        effect->getVariables(conditions);

    // TODO: Validate that precondition and effect only use predicates from domain when added.

    if (not input.contains(conditions))
    {
        // input.toPDDL(std::cout);
        // conditions.toPDDL(std::cout);
        throw std::invalid_argument(
            log::format("Action %1%'s parameter set does not match expressions!", name));
    }
}

std::ostream &Action::toPDDL(std::ostream &out, unsigned int indent) const
{
    const std::string SPACE = std::string(indent, ' ');

    out << SPACE << "(:action " << name << std::endl;

    if (not parameters.empty())
    {
        out << SPACE << SPACE << ":parameters (";
        for (unsigned int i = 0; i < parameters.size(); ++i)
        {
            if (i > 0)
                out << " ";

            out << "?" << parameters[i].name;
            if (parameters[i].type != PDDLBOAT_OBJECT_CLASS)
                out << " - " << parameters[i].type;
        }
        out << ")" << std::endl;
    }

    if (precondition)
    {
        out << SPACE << SPACE << ":precondition" << std::endl;
        precondition->toPDDL(out, indent, 3) << std::endl;
    }

    if (effect)
    {
        out << SPACE << SPACE << ":effect" << std::endl;
        effect->toPDDL(out, indent, 3) << std::endl;
    }

    out << SPACE << ")";

    return out;
}

unsigned int Action::arity() const
{
    return parameters.size();
}

bool Action::holds(const StatePtr &state, const Assignment &vars) const
{
    return precondition->evaluate(state, vars);
}

bool Action::doAction(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    if (not holds(in, vars))
        return false;

    out->clear();
    effect->assign(out, vars);
    out->fold(in);

    return true;
}

std::string Action::toID(const Assignment &vars) const
{
    std::stringstream ss;
    ss << "ACTION:" << name;

    ss << ":ARGS:";
    for (unsigned int i = 0; i < parameters.size(); ++i)
    {
        ss << vars.assigned(parameters[i].name)->name;
        if (i < parameters.size() - 1)
            ss << ",";
    }

    return ss.str();
}

ActionPtr pddlboat::makeAction(const std::string &name, const VariableList &parameters,
                               const ExpressionPtr &preconditions, const ExpressionPtr &effects)
{
    return std::make_shared<Action>(name, parameters, preconditions, effects);
}

ActionPtr pddlboat::makeNoOpAction(const std::string &name)
{
    return makeAction(name, {{}}, makeAnd({}), makeAnd({}));
}
