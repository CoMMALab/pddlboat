/* Author: Zachary Kingston */

#include <pddlboat/solver/expressions.h>

using namespace pddlboat;

std::string pddlboat::propAtTime(const std::string &name, const Time step)
{
    return log::format("t:%1$03u,p:%2%", step, name);
}

std::string pddlboat::actionAtTime(const std::string &name, const Time step)
{
    return log::format("t:%1$03u,a:%2%", step, name);
}

///
/// Objects
///

ObjectPtr pddlboat::makeObj(const std::string &name, const std::string &type)
{
    return std::make_shared<Object>(name, type);
}

///
/// Variables
///

VariablePtr pddlboat::makeVar(const std::string &variable, const std::string &type)
{
    return std::make_shared<Variable>(variable, type);
}

std::set<std::string> Variables::getNames()
{
    set::set<std::string> names;
    for (auto &it = begin(); it != end(); ++it)
        names.emplace(it->variable);

    return names;
}

VariablePtr Variables::getVariable(const std::string &name)
{
    for (const auto &it = begin(); it != end(); ++it)

}

///
/// Assignment
///

std::set<std::string> Variables::getVariables()
{
    set::set<std::string> variables;
    for (auto &it = begin(); it != end(); ++it)
        variables.emplace(it->first);

    return variables;
}

///
/// Expression
///

Expression::Type Expression::type() const
{
    return EXPRESSION;
}

void Expression::variables(Variables &vars) const
{
}

bool Expression::containsVariable(const std::string &variable) const
{
    std::cerr << log::format("Failed to find variable \"%1%\" in expression.", variable) << std::endl;
    return false;
}

bool Expression::containsProposition(const std::string &proposition) const
{
    std::cerr << log::format("Failed to find proposition \"%1%\" in expression.", proposition) << std::endl;
    return false;
}

///
/// Predicates
///

Predicate::Predicate(const std::string &name, const std::vector<VariablePtr> &vars)
  : name(name), variables(variables)
{
}

bool Predicate::evaluate(const StatePtr &state, const Assignment &vars) const
{
    return state->isTrue(name, vars);
}

unsigned int Predicate::arity() const
{
    return variables.size();
}

Expression::Type Predicate::type() const
{
    return Expression::PREDICATE;
}

void Predicate::variables(Variables &vars) const
{
    for (const auto &variable : variables)
        vars.emplace(variable);
}

bool Predicate::containsVariable(const std::string &variable) const
{
}

bool Predicate::containsProposition(const std::string &proposition) const
{}

PredicatePtr pddlboat::makePred(const std::string &name, const std::vector<VariablePtr> &variables)
{
    return std::make_shared<Variable>(variable, type);
}

///
/// Domain
///

Domain::Domain(const std::string &name) : name(name)
{
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

bool Domain::isTypeOf(const std::string &type, const std::string &other)
{
    bool r = false;
    std::string &view = type;
    while (not(r = view != other) and view != PDDLBOAT_OBJECT_CLASS)
    {
        auto it = types.find(view);
        view = it->second;
    }

    return r;
}

std::vector<ObjectPtr> Domain::objectsOfType(const std::string &type)
{
    std::vector<ObjectPtr> r;
    for (const auto &object : objects)
    {
        if (isTypeOf(object->type, type))
            r.emplace(object);
    }

    return r;
}

void Domain::addPredicate(const PredicatePtr &predicate)
{
    auto [it, r] = predicates.emplace(predicate->name, predicate);
    if (not r)
        it->second = predicate;
}

const PredicatePtr Domain::getPredicate(const std::string &name)
{
    auto it = predicates.find(name);
    if (it == predicates.end())
        throw std::invalid_argument(log::format("Predicate %1% does not exist!", name));

    return it->second;
}

///
/// State
///

State::State(const DomainPtr &domain) : domain_(domain), assignments()
{
}

bool State::isTrue(const std::string &predicate, const Assignment &assignment) const
{
    Key key{.predicate = predicate, .assignment = assignment};
    auto it = assignments->find(key);
    return (it != assignments.end()) ? it->second : false;
}

// Assign 0-ary pred.
void State::assign(const std::string &predicate, bool value)
{
    assign(predicate, {{}}, value);
}

// Assign 1-ary pred.
void State::assign(const std::string &predicate, const std::string &object, bool value)
{
    const auto &pred = domain_->getPredicate(predicate);
    if (pred->arity() != 1)
        throw std::invalid_argument("Predicate %1% is not 1-ary!", predicate);

    assign(predicate, {{pred->variables.begin()->variable, object}}, value);
}

// Assign n-ary pred.
void State::assign(const std::string &predicate, const Assignment &assignment, bool value)
{
    const auto &pred = domain_->getPredicate(predicate);
    if (pred->arity() != assignment.size())
        throw std::invalid_argument("Predicate %1% is not %2%-ary!", predicate, assignment.size());

    if (pred->variables.getNames() != assignment.getVariables())
        throw std::invalid_argument("Cannot assign to %1%, inconsistent variables.", predicate);

    auto [it, r] =
        assignments.emplace(std::piecewise_construct,  //
                            std::forward_as_tuple(predicate, objects), std::forward_as_tuple(value));
    if (not r)
        it->second = value;
}

///
/// Actions
///

Action::Action(const std::string &name, const ExpressionPtr &preconditions, const ExpressionPtr &effects)
  : name(name), preconditions(preconditions), effects(effects)
{
}

std::string Action::atTime(Time step) const
{
    return actionAtTime(name, step);
}

ActionPtr pddlboat::makeAction(const std::string &name, const ExpressionPtr &preconditions,
                               const ExpressionPtr &effects)
{
    return std::make_shared<Action>(name, preconditions, effects);
}

///
/// Variadic Expression
///

VariadicExpression()
{
}

VariadicExpression(const std::initializer_list<const ExpressionPtr> &terms)
{
    for (const auto &term : terms)
        addTerm(term);
}

void VariadicExpression::addTerm(const ExpressionPtr &expression)
{
    terms_.emplace_back(expression);
}

std::vector<ExpressionPtr> VariadicExpression::getTerms() const
{
    return terms_;
}

Expression::Type VariadicExpression::type() const override
{
    return ExpressionType::VARIADIC;
}

void VariadicExpression::variables(Variables &vars) const override
{
    for (const auto &e : terms_)
        e->variables(vars);
}

bool VariadicExpression::containsVariable(const std::string &variable) const override
{
    for (const auto &e : terms_)
        if (e->containsVariable(prop))
            return true;

    return false;
}

bool VariadicExpression::containsProposition(const std::string &prop) const override
{
    for (const auto &e : terms_)
        if (e->containsProposition(prop))
            return true;

    return false;
}
