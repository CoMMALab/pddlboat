/* Author: Zachary Kingston */

#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/functional/hash.hpp>

#include <pddlboat/utility.hpp>
#include <pddlboat/solver/domain.hpp>
#include <pddlboat/solver/expressions.hpp>

using namespace pddlboat;

// static const bool debug = true;
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
  : name(boost::algorithm::to_lower_copy(name)), type(boost::algorithm::to_lower_copy(type))
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
    if (it == variables.end())
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
    auto it = variables.find(name);
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

Assignment::Assignment(const Assignment &other) : varToObj(other.varToObj)
{
}

Assignment &Assignment::operator=(const Assignment &other)
{
    if (this != &other)
    {
        varToObj = other.varToObj;
        varToType = other.varToType;
    }

    return *this;
}

void Assignment::fold(const Assignment &other)
{
    for (const auto &[name, obj] : other.varToObj)
        varToObj[name] = obj;
}

unsigned int Assignment::arity() const
{
    return varToObj.size();
}

void Assignment::assign(const Problem &problem, const std::string &var, const std::string &obj)
{
    auto object = problem.getObject(obj);
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

void Assignment::type(const std::string &var, const std::string &type)
{
    auto [it, r] = varToType.emplace(var, type);
    if (not r)
        it->second = type;
}

std::string Assignment::typed(const std::string &var) const
{
    auto it = varToType.find(var);
    if (it == varToType.end())
        throw std::invalid_argument(log::format("Variable %1% not assigned!", var));

    return it->second;
}

bool Assignment::isAssigned(const std::string &var) const
{
    auto it = varToObj.find(var);
    if (it == varToObj.end())
        return false;
    return true;
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

    return bindValues(getValues(bound, variables, problem, domain));
}

PredicatePtr PredicateDefinition::bind(const std::vector<std::string> &bound, const Problem *problem,
                                       const Domain *domain) const
{
    if (variables.size() != bound.size())
        throw std::invalid_argument("Cannot bind variables, size mismatch!");

    return bindValues(getValues(bound, variables, problem, domain));
}

PredicatePtr PredicateDefinition::bindValues(const std::vector<Value> &values) const
{
    if (variables.size() != values.size())
        throw std::invalid_argument("Cannot bind variables, size mismatch!");

    auto copy = values;
    for (std::size_t i = 0; i < values.size(); ++i)
        copy[i].type = variables[i].type;
    // if (values[i].type != variables[i].type)
    //     throw std::invalid_argument(log::format("Cannot bind parameters in %1%, type mismatch %2% -
    //     %3%!",
    //                                             name, values[i].type, variables[i].type));

    return std::make_shared<Predicate>(this, copy);
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

bool Predicate::evaluate(const StatePtr &state, const Assignment &vars) const
{
    bool r = state->isTrue(predicate->name, getValueObjects(bindings, state, vars));

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

void Predicate::assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    out->assignN(predicate->name, getValueObjects(bindings, in, vars), true);
}

Expression::Type Predicate::type() const
{
    return Expression::PREDICATE;
}

void Predicate::getVariables(VariableSet &vars) const
{
    for (const auto &binding : bindings)
        binding.getVariables(vars);
}

void Predicate::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                  const Assignment &vars) const
{
    predicates.emplace(toID(state, vars));
}

void Predicate::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                      const Assignment &vars) const
{
    predicates.emplace(toID(state, vars));
}

void Predicate::getEffects(Effect &effect, const StatePtr &state, const Assignment &vars) const
{
    effect.add.emplace(toID(state, vars));
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
        if (v.containsVariable(variable))
            return true;

    return false;
}

bool Predicate::containsPredicate(const std::string &pred) const
{
    return predicate->name == pred;
}

void Predicate::renameVariable(const std::string &old, const std::string &name)
{
    for (auto &binding : bindings)
        binding.renameVariable(old, name);
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
        binding.toPDDL(out);
    }

    out << ")";
    return out;
}

std::string Predicate::toID(const StatePtr &state, const Assignment &vars) const
{
    std::stringstream ss;
    ss << "PRED:" << predicate->name;

    ss << ":ARGS:";

    auto objects = getValueObjects(bindings, state, vars);
    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        ss << objects[i];
        if (i < objects.size() - 1)
            ss << ",";
    }

    return ss.str();
}

const std::string &Predicate::name() const
{
    return predicate->name;
}

const PredicateDefinition *Predicate::getDefinition() const
{
    return predicate;
}

///
/// DerivedPredicateDefinition
///

DerivedPredicateDefinition::DerivedPredicateDefinition(const std::string &name,
                                                       const VariableList &parameters,
                                                       const ExpressionPtr &expression)
  : name(name), parameters(parameters), expression(expression)
{
}

void DerivedPredicateDefinition::setFormula(const ExpressionPtr &formula)
{
    expression = formula;
}

DerivedPredicatePtr DerivedPredicateDefinition::bind() const
{
    return bind(std::vector<std::string>{});
}

DerivedPredicatePtr DerivedPredicateDefinition::bind(const std::vector<std::string> &bound,
                                                     const ProblemPtr &problem, const DomainPtr &domain) const
{
    if (parameters.size() != bound.size())
        throw std::invalid_argument("Cannot bind variables, size mismatch!");

    return bindValues(getValues(bound, parameters, problem, domain));
}

DerivedPredicatePtr DerivedPredicateDefinition::bindValues(const std::vector<Value> &values) const
{
    if (parameters.size() != values.size())
        throw std::invalid_argument("Cannot bind variables, size mismatch!");

    // for (std::size_t i = 0; i < values.size(); ++i)
    //     if (values[i].type != parameters[i].type)
    //         throw std::invalid_argument(log::format("Cannot bind parameters in %1%, type mismatch %2% -
    //         %3%!",
    //                                                 name, values[i].type, parameters[i].type));

    return std::make_shared<DerivedPredicate>(this, values);
}

bool DerivedPredicateDefinition::isValidAssignment(const Problem *problem,
                                                   const std::vector<std::string> &objects) const
{
    if (objects.size() != arity())
        return false;

    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        ObjectPtr obj;
        if (not(obj = problem->getObject(objects[i])))
            return false;

        if (not problem->domain->isTypeOf(obj->type, parameters[i].type))
            return false;
    }

    return true;
}

unsigned int DerivedPredicateDefinition::arity() const
{
    return parameters.size();
}

std::ostream &DerivedPredicateDefinition::toPDDL(std::ostream &out, unsigned int indent) const
{
    const std::string SPACE = std::string(indent, ' ');

    out << SPACE << "(:derived (" << name;
    for (const auto &variable : parameters)
    {
        out << " ";
        out << "?" << variable.name;
        if (variable.type != PDDLBOAT_OBJECT_CLASS)
            out << " - " << variable.type;
    }
    out << ")" << std::endl;
    expression->toPDDL(out, indent, 2) << std::endl;
    out << SPACE << ")";
    return out;
}

std::string DerivedPredicateDefinition::toID(const Assignment &vars) const
{
    std::stringstream ss;
    ss << "PRED:" << name;

    ss << ":ARGS:";
    for (unsigned int i = 0; i < parameters.size(); ++i)
    {
        ss << vars.assigned(parameters[i].name)->name;
        if (i < parameters.size() - 1)
            ss << ",";
    }

    return ss.str();
}

DerivedPredicateDefinitionPtr pddlboat::makeDerivedPred(const std::string &name,
                                                        const VariableList &variables,
                                                        const ExpressionPtr &expression)
{
    return std::make_shared<DerivedPredicateDefinition>(name, variables, expression);
}

DerivedPredicate::DerivedPredicate(const DerivedPredicateDefinition *predicate,
                                   const std::vector<Value> &bindings)
  : bindings(bindings), predicate(predicate)
{
}

Assignment DerivedPredicate::getBinding(const StatePtr &state, const Assignment &vars) const
{
    Assignment derived;
    auto objects = getValueObjects(bindings, state, vars);

    for (std::size_t i = 0; i < predicate->parameters.size(); ++i)
        derived.assign(*state->getProblem(), predicate->parameters[i].name, objects[i]);

    return derived;
}

bool DerivedPredicate::evaluate(const StatePtr &state, const Assignment &vars) const
{
    auto binding = getBinding(state, vars);
    bool r = predicate->expression->evaluate(state, binding);

    if (debug)
    {
        std::cout << "Expression is " << r << std::endl;
        toPDDL(std::cout);
        std::cout << std::endl;
        binding.toString(std::cout);
        std::cout << std::endl;
        vars.toString(std::cout);
        std::cout << std::endl;
    }

    return r;
}

Expression::Type DerivedPredicate::type() const
{
    return Expression::DERIVED;
}

void DerivedPredicate::getVariables(VariableSet &vars) const
{
    for (const auto &binding : bindings)
        binding.getVariables(vars);
}

void DerivedPredicate::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                         const Assignment &vars) const
{
    const auto &id = toID(state, vars);
    if (predicates.find(id) == predicates.end())
    {
        predicates.emplace(id);
        predicate->expression->getUsedPredicates(predicates, state, getBinding(state, vars));
        predicates.erase(predicates.find(id));
    }
}

VariableList DerivedPredicate::arguments() const
{
    VariableList list;
    for (const auto &binding : bindings)
        if (binding.isVar)
            list.emplace_back(binding.name, binding.type);

    return list;
}

bool DerivedPredicate::containsVariable(const std::string &variable) const
{
    for (const auto &v : bindings)
        if (v.containsVariable(variable))
            return true;

    return false;
}

bool DerivedPredicate::containsPredicate(const std::string &pred) const
{
    if (predicate->name == pred)
        return true;

    return false;
    // return predicate->expression->containsPredicate(pred);
}

void DerivedPredicate::renameVariable(const std::string &old, const std::string &name)
{
    for (auto &binding : bindings)
        binding.renameVariable(old, name);
}

std::ostream &DerivedPredicate::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent, ' ');
    for (unsigned int i = 0; i < level; ++i)
        out << SPACE;

    out << "(" << predicate->name;
    for (const auto &binding : bindings)
    {
        out << " ";
        binding.toPDDL(out);
    }

    out << ")";
    return out;
}

const std::string &DerivedPredicate::name() const
{
    return predicate->name;
}

std::string DerivedPredicate::toID(const StatePtr &state, const Assignment &vars) const
{
    std::stringstream ss;
    ss << "PRED:" << predicate->name;

    ss << ":ARGS:";

    auto objects = getValueObjects(bindings, state, vars);
    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        ss << objects[i];
        if (i < objects.size() - 1)
            ss << ",";
    }

    return ss.str();
}

///
/// Functions
///

FunctionDefinition::FunctionDefinition(const std::string &name, const VariableList &parameters,
                                       const std::string &type)
  : name(name), parameters(parameters), type(type)
{
}

bool FunctionDefinition::isValidAssignment(const Problem *problem,
                                           const std::vector<std::string> &objects) const
{
    if (objects.size() != arity())
        return false;

    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        ObjectPtr obj;
        if (not(obj = problem->getObject(objects[i])))
            return false;

        if (not problem->domain->isTypeOf(obj->type, parameters[i].type))
            return false;
    }

    return true;
}

unsigned int FunctionDefinition::arity() const
{
    return parameters.size();
}

std::ostream &FunctionDefinition::toPDDL(std::ostream &out) const
{
    out << "(" << name;
    for (const auto &variable : parameters)
    {
        out << " ";
        out << "?" << variable.name;
        if (variable.type != PDDLBOAT_OBJECT_CLASS)
            out << " - " << variable.type;
    }
    out << ")";

    if (type != PDDLBOAT_NUMERIC_CLASS)
        out << " - " << type;
    return out;
}

std::string FunctionDefinition::toID(const Assignment &vars) const
{
    std::stringstream ss;
    ss << "FUNC:" << name;

    ss << ":ARGS:";
    for (unsigned int i = 0; i < parameters.size(); ++i)
    {
        ss << vars.assigned(parameters[i].name)->name;
        if (i < parameters.size() - 1)
            ss << ",";
    }

    return ss.str();
}

// Numeric

NumericFunctionDefinition::NumericFunctionDefinition(const std::string &name, const VariableList &parameters)
  : FunctionDefinition(name, parameters, PDDLBOAT_NUMERIC_CLASS)
{
}

NumericFunctionPtr NumericFunctionDefinition::bind() const
{
    return bind(std::vector<std::string>{});
}

NumericFunctionPtr NumericFunctionDefinition::bind(const std::vector<std::string> &bound,
                                                   const ProblemPtr &problem, const DomainPtr &domain) const
{
    if (parameters.size() != bound.size())
        throw std::invalid_argument("Cannot bind parameters, size mismatch!");

    return bindValues(getValues(bound, parameters, problem, domain));
}

NumericFunctionPtr NumericFunctionDefinition::bindValues(const std::vector<Value> &values) const
{
    if (parameters.size() != values.size())
        throw std::invalid_argument("Cannot bind parameters, size mismatch!");

    // for (std::size_t i = 0; i < values.size(); ++i)
    //     if (values[i].type != parameters[i].type)
    //         throw std::invalid_argument(log::format("Cannot bind parameters in %1%, type mismatch %2% -
    //         %3%!",
    //                                                 name, values[i].type, parameters[i].type));

    return std::make_shared<NumericFunction>(this, values);
}

NumericFunctionDefinitionPtr pddlboat::makeFunction(const std::string &name, const VariableList &variables)
{
    return std::make_shared<NumericFunctionDefinition>(name, variables);
}

NumericFunction::NumericFunction(const NumericFunctionDefinition *function,
                                 const std::vector<Value> &bindings)
  : bindings(bindings), function(function)
{
}

std::string NumericFunction::getName() const
{
    return function->name;
}

double NumericFunction::getValue(const StatePtr &state, const Assignment &vars) const
{
    return state->getValue(function->name, getValueObjects(bindings, state, vars));
}

Expression::Type NumericFunction::type() const
{
    return Expression::NUMERIC_FUNCTION;
}

void NumericFunction::getVariables(VariableSet &vars) const
{
    for (const auto &binding : bindings)
        binding.getVariables(vars);
}

bool NumericFunction::containsVariable(const std::string &variable) const
{
    for (const auto &v : bindings)
        if (v.containsVariable(variable))
            return true;

    return false;
}

void NumericFunction::renameVariable(const std::string &old, const std::string &name)
{
    for (auto &binding : bindings)
        binding.renameVariable(old, name);
}

void NumericFunction::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                        const Assignment &vars) const
{
    predicates.emplace(toID(state, vars));
    for (const auto &v : bindings)
        v.getUsedFunctions(predicates, state, vars);
}

void NumericFunction::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                            const Assignment &vars) const
{
    predicates.emplace(toID(state, vars));
}

std::string NumericFunction::toID(const StatePtr &state, const Assignment &vars) const
{
    std::stringstream ss;
    ss << "FUNC:" << function->name;

    ss << ":ARGS:";

    auto objects = getValueObjects(bindings, state, vars);
    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        ss << objects[i];
        if (i < objects.size() - 1)
            ss << ",";
    }

    return ss.str();
}

std::ostream &NumericFunction::toPDDL(std::ostream &out, unsigned int /* indent */,
                                      unsigned int /* level */) const
{
    out << "(" << function->name;
    for (const auto &binding : bindings)
    {
        out << " ";
        binding.toPDDL(out);
    }
    out << ")";

    return out;
}

// Object

ObjectFunctionDefinition::ObjectFunctionDefinition(const std::string &name, const VariableList &parameters,
                                                   const std::string &type)
  : FunctionDefinition(name, parameters, type)
{
}

ObjectFunctionPtr ObjectFunctionDefinition::bind() const
{
    return bind(std::vector<std::string>{});
}

ObjectFunctionPtr ObjectFunctionDefinition::bind(const std::vector<std::string> &bound,
                                                 const ProblemPtr &problem, const DomainPtr &domain) const
{
    if (parameters.size() != bound.size())
        throw std::invalid_argument("Cannot bind parameters, size mismatch!");

    return bindValues(getValues(bound, parameters, problem, domain));
}

ObjectFunctionPtr ObjectFunctionDefinition::bindValues(const std::vector<Value> &values) const
{
    if (parameters.size() != values.size())
        throw std::invalid_argument("Cannot bind parameters, size mismatch!");

    // for (std::size_t i = 0; i < values.size(); ++i)
    //     if (values[i].type != parameters[i].type)
    //         throw std::invalid_argument(log::format("Cannot bind parameters in %1%, type mismatch %2% -
    //         %3%!",
    //                                                 name, values[i].type, parameters[i].type));

    return std::make_shared<ObjectFunction>(this, values);
}

ObjectFunctionDefinitionPtr pddlboat::makeObjectFunction(const std::string &name, const std::string &type,
                                                         const VariableList &variables)
{
    return std::make_shared<ObjectFunctionDefinition>(name, variables, type);
}

ObjectFunction::ObjectFunction(const ObjectFunctionDefinition *function, const std::vector<Value> &bindings)
  : bindings(bindings), function(function)
{
}

std::string ObjectFunction::getName() const
{
    return function->name;
}

std::string ObjectFunction::getReturnType() const
{
    return function->type;
}

std::string ObjectFunction::getObject(const StatePtr &state, const Assignment &vars) const
{
    return state->getObject(function->name, getValueObjects(bindings, state, vars));
}

Expression::Type ObjectFunction::type() const
{
    return Expression::OBJECT_FUNCTION;
}

void ObjectFunction::getVariables(VariableSet &vars) const
{
    for (const auto &binding : bindings)
        binding.getVariables(vars);
}

bool ObjectFunction::containsVariable(const std::string &variable) const
{
    for (const auto &v : bindings)
        if (v.containsVariable(variable))
            return true;

    return false;
}

void ObjectFunction::renameVariable(const std::string &old, const std::string &name)
{
    for (auto &binding : bindings)
        binding.renameVariable(old, name);
}

void ObjectFunction::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                       const Assignment &vars) const
{
    predicates.emplace(toID(state, vars));
    for (const auto &v : bindings)
        v.getUsedFunctions(predicates, state, vars);
}

void ObjectFunction::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                           const Assignment &vars) const
{
    predicates.emplace(toID(state, vars));
}

void ObjectFunction::getEffects(Effect &effect, const StatePtr &state, const Assignment &vars) const
{
    effect.assigned.emplace(toID(state, vars));
}

std::string ObjectFunction::toID(const StatePtr &state, const Assignment &vars) const
{
    std::stringstream ss;
    ss << "FUNC:" << function->name;

    ss << ":ARGS:";

    auto objects = getValueObjects(bindings, state, vars);
    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        ss << objects[i];
        if (i < objects.size() - 1)
            ss << ",";
    }

    return ss.str();
}

std::ostream &ObjectFunction::toPDDL(std::ostream &out, unsigned int /* indent */,
                                     unsigned int /* level */) const
{
    out << "(" << function->name;
    for (const auto &binding : bindings)
    {
        out << " ";
        binding.toPDDL(out);
    }
    out << ")";

    return out;
}

///
/// Domain
///

Domain::Domain(const std::string &name) : name(boost::algorithm::to_lower_copy(name))
{
    addRequirement(Requirements::STRIPS);
}

void Domain::addRequirement(const Requirements::Keyword &requirement)
{
    requirements = requirements | requirement;
}

bool Domain::hasRequirements(const Requirements::Keyword &requirement) const
{
    return Requirements::isSubset(requirement, requirements);
}

bool Domain::supportsRequirements(const Requirements::Keyword &requirement) const
{
    return Requirements::isSubset(requirements, requirement);
}

const ObjectPtr Domain::getConstant(const std::string &name) const
{
    if (constants.empty())
        return nullptr;

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
        if (not isType(object->type))
            throw std::invalid_argument(  //
                log::format("Tried adding constant %1%, but is of invalid type %2%", object->name,
                            object->type));

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
    addRequirement(Requirements::TYPING);

    if (not isType(parent))
        throw std::runtime_error(log::format("Invalid parent type %1%!", parent));

    auto [it, r] = types.emplace(type, parent);
    if (not r)
        it->second = parent;
}

std::string Domain::getParent(const std::string &type) const
{
    if (not isType(type))
        throw std::runtime_error(log::format("Invalid type %1%!", type));

    if (type == PDDLBOAT_OBJECT_CLASS)
        return type;

    auto it = types.find(type);
    return it->second;
}

std::vector<std::string> Domain::getTypes() const
{
    std::vector<std::string> ret;
    for (const auto &pair : types)
        ret.emplace_back(pair.first);

    std::sort(ret.begin(), ret.end(),
              [&](const auto &a, const auto &b)
              {
                  if (isTypeOf(a, b))
                      return false;
                  if (isTypeOf(b, a))
                      return true;

                  if (getParent(a) == getParent(b))
                      return a < b;

                  return getParent(a) > getParent(b);
              });

    return ret;
}

bool Domain::isType(const std::string &type) const
{
    return type == PDDLBOAT_OBJECT_CLASS  //
           or types.find(type) != types.end();
}

bool Domain::isTypeOf(const std::string &type, const std::string &other) const
{
    bool r = false;

    auto view = std::cref(type);
    while (not((r = view.get() == other)  //
               or view.get() == PDDLBOAT_OBJECT_CLASS))
    {
        auto it = types.find(view);
        view = std::cref(it->second);
    }

    return r;
}

std::string Domain::findCommonType(const std::string &a, const std::string &b) const
{
    std::string temp;
    std::vector<std::string> atypes = {a};
    std::vector<std::string> btypes = {b};

    while (atypes.back() != PDDLBOAT_OBJECT_CLASS)
        atypes.emplace_back(getParent(atypes.back()));

    while (btypes.back() != PDDLBOAT_OBJECT_CLASS)
        btypes.emplace_back(getParent(btypes.back()));

    for (const auto &other : btypes)
    {
        auto it = std::find(atypes.begin(), atypes.end(), other);
        if (it != atypes.end())
            return *it;
    }

    return PDDLBOAT_OBJECT_CLASS;
}

void Domain::addPredicate(const PredicateDefinitionPtr &predicate)
{
    auto [it, r] = predicates.emplace(predicate->name, predicate);
    if (not r)
        it->second = predicate;
}

void Domain::removePredicate(const std::string &name)
{
    auto it = predicates.find(name);
    if (it != predicates.end())
        predicates.erase(it);
}

void Domain::addDerivedPredicate(const DerivedPredicateDefinitionPtr &predicate)
{
    // check if predicate exists
    addRequirement(Requirements::DERIVED_PREDICATES);
    if (predicate->expression)
        addRequirementsFromExpression(predicate->expression);

    auto [it, r] = derived.emplace(predicate->name, predicate);
    if (not r)
        it->second = predicate;

    if (not isPredicate(predicate->name))
        addPredicate(makePred(predicate->name, predicate->parameters));
}

bool Domain::isPredicate(const std::string &name) const
{
    auto it = predicates.find(name);
    return it != predicates.end();
}

const PredicateDefinitionPtr Domain::getPredicate(const std::string &name) const
{
    auto it = predicates.find(name);
    if (it == predicates.end())
        throw std::invalid_argument(log::format("Predicate %1% does not exist!", name));

    return it->second;
}

bool Domain::isDerivedPredicate(const std::string &name) const
{
    auto it = derived.find(name);
    return it != derived.end();
}

const DerivedPredicateDefinitionPtr Domain::getDerivedPredicate(const std::string &name) const
{
    auto it = derived.find(name);
    if (it == derived.end())
        throw std::invalid_argument(log::format("Derived predicate %1% does not exist!", name));

    return it->second;
}

void Domain::addFunction(const FunctionDefinitionPtr &function)
{
    if (function->type == PDDLBOAT_NUMERIC_CLASS and function->name == "total-cost")
        addRequirement(Requirements::ACTION_COSTS);
    else if (function->type == PDDLBOAT_NUMERIC_CLASS)
        addRequirement(Requirements::NUMERIC_FLUENTS);
    else
        addRequirement(Requirements::OBJECT_FLUENTS);

    auto [it, r] = functions.emplace(function->name, function);
    if (not r)
        it->second = function;
}

void Domain::removeFunction(const std::string &name)
{
    auto it = functions.find(name);
    if (it != functions.end())
        functions.erase(it);
}

bool Domain::isFunction(const std::string &name) const
{
    auto it = functions.find(name);
    return it != functions.end();
}

bool Domain::isObjectFunction(const std::string &name) const
{
    return isFunction(name) and std::dynamic_pointer_cast<ObjectFunctionDefinition>(getFunction(name));
}

bool Domain::isNumericFunction(const std::string &name) const
{
    return isFunction(name) and std::dynamic_pointer_cast<NumericFunctionDefinition>(getFunction(name));
}

FunctionDefinitionPtr Domain::getFunction(const std::string &name) const
{
    auto it = functions.find(name);
    if (it == functions.end())
        throw std::invalid_argument(log::format("Function %1% does not exist!", name));

    return it->second;
}

ObjectFunctionDefinitionPtr Domain::getObjectFunction(const std::string &name) const
{
    return std::dynamic_pointer_cast<ObjectFunctionDefinition>(getFunction(name));
}

NumericFunctionDefinitionPtr Domain::getNumericFunction(const std::string &name) const
{
    return std::dynamic_pointer_cast<NumericFunctionDefinition>(getFunction(name));
}

void Domain::addAction(const ActionPtr &action)
{
    addRequirementsFromExpression(action->precondition);
    addRequirementsFromExpression(action->effect);

    auto [it, r] = actions.emplace(action->name, action);
    if (not r)
        it->second = action;
}

const ActionPtr Domain::getAction(const std::string &name) const
{
    auto it = actions.find(name);
    if (it == actions.end())
        throw std::invalid_argument(log::format("Action %1% does not exist!", name));

    return it->second;
}

void Domain::addRequirementsFromExpression(const ExpressionPtr &expression)
{
    auto type = expression->getTypes();
    if (isIn(Expression::NOT, type))
        addRequirement(Requirements::NEGATIVE_PRECONDITIONS);

    if (isIn(Expression::OR, type))
        addRequirement(Requirements::DISJUNCTIVE_PRECONDITIONS);

    if (isIn(Expression::EQUALS, type))
        addRequirement(Requirements::EQUALITY);

    if (isIn(Expression::EXISTS, type))
        addRequirement(Requirements::EXISTENTIAL_PRECONDITIONS);

    if (isIn(Expression::FORALL, type))
        addRequirement(Requirements::UNIVERSAL_PRECONDITIONS);

    if (isIn(Expression::WHEN, type))
        addRequirement(Requirements::CONDITIONAL_EFFECTS);

    if (isIn(Expression::OBJECT_FUNCTION, type))
        addRequirement(Requirements::OBJECT_FLUENTS);

    bool numeric = isIn(Expression::MINUS, type)          //
                   or isIn(Expression::DIVIDE, type)      //
                   or isIn(Expression::MULTIPLY, type)    //
                   or isIn(Expression::PLUS, type)        //
                   or isIn(Expression::ASSIGN, type)      //
                   or isIn(Expression::SCALE_UP, type)    //
                   or isIn(Expression::SCALE_DOWN, type)  //
                   or isIn(Expression::DECREASE, type);
    if (isIn(Expression::NUMERIC_FUNCTION, type) and numeric)
        addRequirement(Requirements::NUMERIC_FLUENTS);
}

std::ostream &Domain::toPDDL(std::ostream &out) const
{
    const unsigned int INDENT = 2;
    const std::string SPACE = std::string(INDENT, ' ');

    out << "(define (domain " << name << ")" << std::endl;
    out << SPACE << "(:requirements" << std::endl;
    for (const auto &string : Requirements::toStrings(requirements))
        out << SPACE << SPACE << string << std::endl;
    out << SPACE << ")" << std::endl << std::endl;

    if (not types.empty())
    {
        out << SPACE << "(:types" << std::endl;
        for (const auto &name : getTypes())
        {
            out << SPACE << SPACE << name;

            const auto &it = types.find(name);
            if (it->second != PDDLBOAT_OBJECT_CLASS)
                out << " - " << it->second;

            out << std::endl;
        }

        out << SPACE << ")" << std::endl << std::endl;
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
        out << SPACE << ")" << std::endl << std::endl;
    }

    if (not predicates.empty())
    {
        out << SPACE << "(:predicates " << std::endl;
        for (const auto &key : predicates)
        {
            out << SPACE << SPACE;
            key.second->toPDDL(out) << std::endl;
        }
        out << SPACE << ")" << std::endl << std::endl;
    }

    if (not functions.empty())
    {
        out << SPACE << "(:functions " << std::endl;
        for (const auto &key : functions)
        {
            out << SPACE << SPACE;
            key.second->toPDDL(out) << std::endl;
        }
        out << SPACE << ")" << std::endl << std::endl;
    }

    std::set<std::string> toPrint;
    for (const auto &key : derived)
        toPrint.emplace(key.first);

    while (not toPrint.empty())
    {
        DerivedPredicateDefinitionPtr ptr;
        for (const auto &key : toPrint)
        {
            ptr = getDerivedPredicate(key);
            bool clean = true;
            for (const auto &other : derived)
            {
                if (key != other.first)
                    clean &= not ptr->expression->containsPredicate(other.first);
            }
            if (clean)
                break;
        }

        ptr->toPDDL(out, INDENT) << std::endl << std::endl;
        toPrint.erase(toPrint.find(ptr->name));
    }

    for (const auto &key : actions)
        key.second->toPDDL(out, INDENT) << std::endl << std::endl;

    out << ")";

    return out;
}

///
/// Problem
///

Problem::Problem(const std::string &name, const DomainPtr &domain)
  : name(boost::algorithm::to_lower_copy(name)), domain(domain), start(makeState())
{
    if (domain->isFunction("total-cost"))
    {
        minimize = true;
        metric = domain->getNumericFunction("total-cost")->bind();
    }
}

Problem::Problem(const Problem &problem)
  : name(problem.name)
  , domain(problem.domain)
  , objects(problem.objects)
  , start(problem.start)
  , goal(problem.goal)
  , minimize(problem.minimize)
  , metric(problem.metric)
{
}

Problem::Problem(const ProblemPtr &problem) : Problem(*problem)
{
}

const ObjectPtr Problem::getObject(const std::string &name) const
{
    const auto &constant = domain->getConstant(name);
    if (constant)
        return constant;

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
        Assignment toAdd;
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
            toAdd.assign(*this, car.name, object);
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

    if (metric)
    {
        out << SPACE << "(:metric ";
        if (minimize)
            out << "minimize ";
        else
            out << "maximize ";
        metric->toPDDL(out, INDENT, 2) << std::endl;
        out << SPACE << ")" << std::endl;
    }

    out << ")";

    return out;
}

///
/// State
///

State::State(const Problem *problem) : problem_(const_cast<Problem *>(problem))
{
}

StatePtr State::clone() const
{
    auto state = problem_->makeState();
    state->fold(*this);

    return state;
}

StatePtr State::cleanClone() const
{
    auto state = clone();
    state->clean();

    return state;
}

void State::setProblem(Problem *problem)
{
    problem_ = problem;
}

const Problem *State::getProblem() const
{
    return problem_;
}

void State::clear()
{
    assignments.clear();
    numeric.clear();
    values.clear();
}

void State::complement()
{
    for (const auto &[key, value] : assignments)
        assignN(key.first, key.second, not value);
}

void State::fold(const StatePtr &other)
{
    fold(*other);
}

void State::fold(const State &other)
{
    for (const auto &[key, value] : other.assignments)
    {
        auto it = assignments.find(key);
        if (it == assignments.end())
            assignN(key.first, key.second, value);
    }

    for (const auto &[key, value] : other.numeric)
    {
        auto it = numeric.find(key);
        if (it == numeric.end())
            assignN(key.first, key.second, value);
    }

    for (const auto &[key, value] : other.values)
    {
        auto it = values.find(key);
        if (it == values.end())
            assignN(key.first, key.second, value);
    }
}

void State::clean()
{
    std::vector<Key> keys;
    for (const auto &[key, value] : assignments)
        if (not value)
            keys.emplace_back(key);

    for (const auto &key : keys)
        assignments.erase(key);
}

std::size_t State::hash() const
{
    std::size_t seed = 0;
    // for (const auto &[key, value] : assignments)
    // {
    //     if (value)
    //     {
    //         boost::hash_combine(seed, key.first);
    //         for (const auto &object : key.second)
    //             boost::hash_combine(seed, object);

    //         boost::hash_combine(seed, value);
    //     }
    // }

    // // TODO: Implement a dirty tag so we don't compute this everytime
    std::size_t assignments_hash = boost::hash_range(assignments.begin(), assignments.end());
    std::size_t numeric_hash = boost::hash_range(numeric.begin(), numeric.end());
    std::size_t values_hash = boost::hash_range(values.begin(), values.end());

    boost::hash_combine(seed, assignments_hash);
    boost::hash_combine(seed, numeric_hash);
    boost::hash_combine(seed, values_hash);

    return seed;
}

bool State::isEqual(const StatePtr &other) const
{
    return isEqual(*other);
}

bool State::isEqual(const State &other) const
{
    return assignments == other.assignments and numeric == other.numeric and values == other.values;
}

bool State::isTrue(const std::string &predicate, const std::vector<std::string> &objects) const
{
    auto it = assignments.find(std::make_pair(predicate, objects));
    return (it != assignments.end()) ? it->second : false;
}

double State::getValue(const std::string &function, const std::vector<std::string> &objects) const
{
    auto it = numeric.find(std::make_pair(function, objects));
    return (it != numeric.end()) ? it->second : 0;
}

std::string State::getObject(const std::string &function, const std::vector<std::string> &objects) const
{
    auto it = values.find(std::make_pair(function, objects));
    return (it != values.end()) ? it->second : "";
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

    assignN(predicate, {object}, value);
}

// Assign n-ary pred.
void State::assignN(const std::string &predicate, const std::vector<std::string> &objects, bool value)
{
    const auto &pred = problem_->domain->getPredicate(predicate);

    auto [it, r] =
        assignments.emplace(std::piecewise_construct,  //
                            std::forward_as_tuple(pred->name, objects), std::forward_as_tuple(value));
    if (not r)
        it->second = value;
}

std::ostream &State::toPDDL(std::ostream &out, unsigned int indent, bool negatives) const
{
    const std::string SPACE = std::string(indent, ' ');

    // if (negatives)
    // {
    //     for (const auto &[name, preddef] : problem_->domain->predicates)
    //     {
    //         for (const auto &assignment : problem_->getGroundings(preddef->variables))
    //         {
    //             std::vector<std::string> objects;
    //             for (const auto &var : preddef->variables)
    //                 objects.emplace_back(assignment.assigned(var.name)->name);

    //             bool sv = isTrue(name, objects);

    //             out << SPACE;
    //             if (not sv)
    //                 out << "(not ";
    //             out << "(" << assignment.first.first;
    //             for (const auto &obj : assignment.first.second)
    //                 out << " " << obj;
    //             out << ")";
    //             if (not sv)
    //                 out << ")";
    //             out << std::endl;
    //         }
    //     }
    // }
    // else
    // {
    for (const auto &assignment : assignments)
    {
        if (not negatives)
        {
            if (assignment.second)
            {
                out << SPACE << "(" << assignment.first.first;
                for (const auto &obj : assignment.first.second)
                    out << " " << obj;
                out << ")" << std::endl;
            }
        }
        else
        {
            out << SPACE;
            if (not assignment.second)
                out << "(not ";
            out << "(" << assignment.first.first;
            for (const auto &obj : assignment.first.second)
                out << " " << obj;
            out << ")";
            if (not assignment.second)
                out << ")";
            out << std::endl;
        }
    }
    // }

    for (const auto &assignment : numeric)
    {
        out << SPACE << "(= (" << assignment.first.first;
        for (const auto &obj : assignment.first.second)
            out << " " << obj;
        out << ") " << assignment.second << ")" << std::endl;
    }

    for (const auto &assignment : values)
    {
        out << SPACE << "(= (" << assignment.first.first;
        for (const auto &obj : assignment.first.second)
            out << " " << obj;
        out << ") " << assignment.second << ")" << std::endl;
    }

    return out;
}

void State::assign0(const std::string &function, double value)
{
    assignN(function, {}, value);
}

void State::assign1(const std::string &function, const std::string &object, double value)
{
    const auto &func = problem_->domain->getFunction(function);
    if (func->arity() != 1)
        throw std::invalid_argument(log::format("Function %1% is not 1-ary!", function));

    assignN(function, {object}, value);
}

void State::assignN(const std::string &function, const std::vector<std::string> &objects, double value)
{
    const auto &func = problem_->domain->getFunction(function);

    auto [it, r] = numeric.emplace(std::piecewise_construct,  //
                                   std::forward_as_tuple(func->name, objects), std::forward_as_tuple(value));
    if (not r)
        it->second = value;
}

void State::assign0(const std::string &function, const std::string &value)
{
    assignN(function, {}, value);
}

void State::assign1(const std::string &function, const std::string &object, const std::string &value)
{
    const auto &func = problem_->domain->getFunction(function);
    if (func->arity() != 1)
        throw std::invalid_argument(log::format("Function %1% is not 1-ary!", function));

    assignN(function, {object}, value);
}

void State::assignN(const std::string &function, const std::vector<std::string> &objects,
                    const std::string &value)
{
    const auto &func = problem_->domain->getFunction(function);

    auto [it, r] = values.emplace(std::piecewise_construct,  //
                                  std::forward_as_tuple(func->name, objects), std::forward_as_tuple(value));
    if (not r)
        it->second = value;
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

bool Action::effected(const StatePtr &state, const Assignment &vars) const
{
    return effect->evaluate(state, vars);
}

bool Action::doAction(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    if (not holds(in, vars))
        return false;

    out->clear();
    effect->assign(out, in, vars);
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

std::string Action::toStepID(const Assignment &vars) const
{
    std::stringstream ss;
    ss << "(" << name;

    ss << " ";
    for (unsigned int i = 0; i < parameters.size(); ++i)
    {
        ss << vars.assigned(parameters[i].name)->name;
        if (i < parameters.size() - 1)
            ss << " ";
    }
    ss << ")";

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

//
// Groundings
//

GroundedAction::GroundedAction(const ActionPtr &action, const Assignment &assignment)
  : action(action), assignment(assignment), string(action->toID(assignment))
{
}

bool GroundedAction::operator==(const GroundedAction &other) const
{
    return string == other.string;
}

GroundedPredicate::GroundedPredicate(const PredicateDefinitionPtr &predicate, const Assignment &assignment)
  : predicate(predicate), assignment(assignment), string(predicate->toID(assignment))
{
}

GroundedFunction::GroundedFunction(const FunctionDefinitionPtr &fluent, const Assignment &assignment)
  : fluent(fluent), assignment(assignment), string(fluent->toID(assignment))
{
}

GroundedObjectFunction::GroundedObjectFunction(const ObjectFunctionDefinitionPtr &fluent,
                                               const Assignment &assignment)
  : fluent(fluent), assignment(assignment), string(fluent->toID(assignment))
{
}

GroundedNumericFunction::GroundedNumericFunction(const NumericFunctionDefinitionPtr &fluent,
                                                 const Assignment &assignment)
  : fluent(fluent), assignment(assignment), string(fluent->toID(assignment))
{
}

GroundedDerivedPredicate::GroundedDerivedPredicate(const DerivedPredicateDefinitionPtr &derived,
                                                   const Assignment &assignment)
  : derived(derived), assignment(assignment), string(derived->toID(assignment))
{
}

GroundedExpression::GroundedExpression(const ExpressionPtr &expression, const Assignment &assignment)
  : expression(expression), assignment(assignment)
{
}
