/* Author: Zachary Kingston */

#include <pddlboat/solver/expressions.h>

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
/// Variables
///

Variable::Variable(const std::string &name, const std::string &type) : name(name), type(type)
{
}

VariablePtr pddlboat::makeVar(const std::string &variable, const std::string &type)
{
    return std::make_shared<Variable>(variable, type);
}

Variables::Variables() = default;

Variables::Variables(const std::initializer_list<VariablePtr> &variables)
{
    for (const auto &variable : variables)
        addVariable(variable);
}

void Variables::merge(const Variables &other)
{
    for (const auto &variable : other.variables)
        addVariable(variable.second);
}

void Variables::addVariable(const VariablePtr &var)
{
    auto [it, r] = variables.emplace(var->name, var);
    if (not r)
        it->second = var;
}

std::set<std::string> Variables::getNames() const
{
    std::set<std::string> names;
    for (const auto &variable : variables)
        names.emplace(variable.first);

    return names;
}

const VariablePtr Variables::getVariable(const std::string &name) const
{
    auto it = variables.find(name);
    if (it != variables.end())
        return it->second;

    return nullptr;
}

bool Variables::operator==(const Variables &other) const
{
    if (variables.size() != other.variables.size())
        return false;

    for (const auto &variable : variables)
    {
        auto var = getVariable(variable.first);
        if (not var)
            return false;

        if (var->type != variable.second->type)
            return false;
    }

    return true;
}

bool Variables::operator!=(const Variables &other) const
{
    return not this->operator==(other);
}

///
/// Assignment
///

Assignment::Assignment()
{
}

Assignment::Assignment(const Assignment &other) : varToObj(other.varToObj)
{
}

Assignment::Assignment(const std::map<std::string, std::string> &assgn) : varToObj(assgn)
{
}

unsigned int Assignment::arity() const
{
    return varToObj.size();
}

void Assignment::assign(const std::string &var, const std::string &obj)
{
    auto [it, r] = varToObj.emplace(var, obj);
    if (not r)
    {
        objects.erase(objects.find(obj));
        it->second = obj;
    }

    variables.emplace(var);
    objects.emplace(obj);
}

const std::string &Assignment::assigned(const std::string &var) const
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

bool Assignment::operator<(const Assignment &rhs) const
{
    return variables < rhs.variables or objects < rhs.objects;
}

///
/// Expression
///

Expression::Type Expression::type() const
{
    return EXPRESSION;
}

void Expression::getVariables(Variables &vars) const
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

Predicate::Predicate(const std::string &name, const Variables &variables) : name(name), variables(variables)
{
}

bool Predicate::evaluate(const StatePtr &state, const Assignment &vars) const
{
    return state->isTrue(name, vars);
}

unsigned int Predicate::arity() const
{
    return variables.variables.size();
}

Expression::Type Predicate::type() const
{
    return Expression::PREDICATE;
}

void Predicate::getVariables(Variables &vars) const
{
    vars.merge(variables);
}

bool Predicate::containsVariable(const std::string &variable) const
{
    return variables.getVariable(variable) != nullptr;
}

bool Predicate::containsProposition(const std::string &proposition) const
{
    return name == proposition;
}

bool Predicate::validAssignment(const Problem *problem, const Assignment &assignment) const
{
    if (arity() > assignment.arity())
    {
        const auto &msg = log::format("Predicate %1% is %2%-ary, not %3%-ary!",  //
                                      name, arity(), assignment.arity());
        std::cerr << msg << std::endl;
        return false;
    }

    if (variables.getNames() != assignment.getVariables())
    {
        const auto &msg = log::format("Cannot assign to %1%, inconsistent variables.", name);
        std::cerr << msg << std::endl;
        return false;
    }

    for (const auto &[name, variable] : variables.variables)
    {
        const auto &object_name = assignment.assigned(variable->name);
        const auto &object = problem->getObject(object_name);

        if (not problem->domain->isTypeOf(object->type, variable->type))
        {
            const auto &msg = log::format("In Predicate %1%: "
                                          "Object %1% is of type %2%, but variable %3% is of type %4%!",
                                          name, object_name, object->type, variable->name, variable->type);
            std::cerr << msg << std::endl;
            return false;
        }
    }

    return true;
}

PredicatePtr pddlboat::makePred(const std::string &name, const Variables &variables)
{
    return std::make_shared<Predicate>(name, variables);
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

State::Key::Key(const std::string &predicate, const Assignment &assignment)
  : predicate(predicate), assignment(assignment)
{
}

bool State::Key::operator<(const State::Key &rhs) const
{
    return predicate < rhs.predicate and assignment < rhs.assignment;
}

State::State(const Problem *problem) : problem_(problem)
{
}

bool State::isTrue(const std::string &predicate, const Assignment &assignment) const
{
    Key key{.predicate = predicate, .assignment = assignment};
    auto it = assignments.find(key);
    return (it != assignments.end()) ? it->second : false;
}

// Assign 0-ary pred.
void State::assign0(const std::string &predicate, bool value)
{
    assignN(predicate, {{}}, value);
}

// Assign 1-ary pred.
void State::assign1(const std::string &predicate, const std::string &object, bool value)
{
    const auto &pred = problem_->domain->getPredicate(predicate);
    if (pred->arity() != 1)
        throw std::invalid_argument(log::format("Predicate %1% is not 1-ary!", predicate));

    const auto &var = *pred->variables.getNames().begin();
    Assignment assgn;
    assgn.assign(var, object);

    assignN(predicate, assgn, value);
}

// Assign n-ary pred.
void State::assignN(const std::string &predicate, const Assignment &assignment, bool value)
{
    const auto &pred = problem_->domain->getPredicate(predicate);

    auto [it, r] =
        assignments.emplace(std::piecewise_construct,  //
                            std::forward_as_tuple(predicate, assignment), std::forward_as_tuple(value));
    if (not r)
        it->second = value;
}

///
/// Actions
///

Action::Action(const std::string &name, const Variables &parameters, const ExpressionPtr &precondition,
               const ExpressionPtr &effect)
  : name(name), parameters(parameters), precondition(precondition), effect(effect)
{
    Variables in;
    precondition->getVariables(in);
    effect->getVariables(in);

    if (in != parameters)
        throw std::invalid_argument("Action parameter set does not match expressions!");
}

bool Action::preconditionsHold(const StatePtr &state, const Assignment &vars) const
{
    return precondition->evaluate(state, vars);
}

void Action::doAction(const StatePtr &in, const StatePtr &out, const Assignment &vars) const
{
    if (not preconditionsHold(in, vars))
        throw std::invalid_argument("Preconditions do not hold!");
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

VariadicExpression::VariadicExpression() = default;

VariadicExpression::VariadicExpression(const std::initializer_list<const ExpressionPtr> &terms)
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

Expression::Type VariadicExpression::type() const
{
    return Expression::VARIADIC;
}

void VariadicExpression::getVariables(Variables &vars) const
{
    for (const auto &e : terms_)
        e->getVariables(vars);
}

bool VariadicExpression::containsVariable(const std::string &variable) const
{
    for (const auto &e : terms_)
        if (e->containsVariable(variable))
            return true;

    return false;
}

bool VariadicExpression::containsProposition(const std::string &prop) const
{
    for (const auto &e : terms_)
        if (e->containsProposition(prop))
            return true;

    return false;
}

///
/// And
///

And::And(const std::initializer_list<const ExpressionPtr> &terms) : VariadicExpression(terms)
{
}

bool And::evaluate(const StatePtr &state, const Assignment &vars) const
{
    bool ret = true;
    for (unsigned int i = 0; i < terms_.size() and ret; ++i)
        ret &= terms_[i]->evaluate(state, vars);

    return ret;
}

Expression::Type And::type() const
{
    return Expression::AND;
}

AndPtr pddlboat::makeAnd(const std::initializer_list<const ExpressionPtr> &terms)
{
    return std::make_shared<And>(std::forward<const std::initializer_list<const ExpressionPtr>>(terms));
}

///
/// Or
///

Or::Or(const std::initializer_list<const ExpressionPtr> &terms) : VariadicExpression(terms)
{
}

bool Or::evaluate(const StatePtr &state, const Assignment &vars) const
{
    bool ret = false;
    for (unsigned int i = 0; i < terms_.size() and ret; ++i)
        ret |= terms_[i]->evaluate(state, vars);

    return ret;
}

Expression::Type Or::type() const
{
    return Expression::AND;
}

OrPtr pddlboat::makeOr(const std::initializer_list<const ExpressionPtr> &terms)
{
    return std::make_shared<Or>(std::forward<const std::initializer_list<const ExpressionPtr>>(terms));
}

///
/// Equals
///

Equals::Equals(const std::initializer_list<const ExpressionPtr> &terms) : VariadicExpression(terms)
{
}

bool Equals::evaluate(const StatePtr &state, const Assignment &vars) const
{
    bool allTrue = true;
    bool allFalse = true;

    for (unsigned int i = 0; i < terms_.size() and (allTrue or allFalse); ++i)
    {
        const bool eval = terms_[i]->evaluate(state, vars);
        allTrue &= eval;
        allFalse &= not eval;
    }

    return allTrue or allFalse;
}

Expression::Type Equals::type() const
{
    return Expression::AND;
}

EqualsPtr pddlboat::makeEquals(const std::initializer_list<const ExpressionPtr> &terms)
{
    return std::make_shared<Equals>(std::forward<const std::initializer_list<const ExpressionPtr>>(terms));
}

///
/// Not
///

Not::Not(const ExpressionPtr &term) : term_(term)
{
}

const ExpressionPtr Not::getTerm() const
{
    return term_;
}

bool Not::evaluate(const StatePtr &state, const Assignment &vars) const
{
    return not term_->evaluate(state, vars);
}

Expression::Type Not::type() const
{
    return Expression::NOT;
}

void Not::getVariables(Variables &vars) const
{
    term_->getVariables(vars);
}

bool Not::containsVariable(const std::string &variable) const
{
    return term_->containsVariable(variable);
}

bool Not::containsProposition(const std::string &proposition) const
{
    return term_->containsProposition(proposition);
}

NotPtr pddlboat::makeNot(const ExpressionPtr &term)
{
    return std::make_shared<Not>(std::forward<const ExpressionPtr>(term));
}

///
/// Imply
///

Imply::Imply(const ExpressionPtr &left, const ExpressionPtr &right) : left_(left), right_(right)
{
}

const ExpressionPtr Imply::getLeft() const
{
    return left_;
}

const ExpressionPtr Imply::getRight() const
{
    return right_;
}

bool Imply::evaluate(const StatePtr &state, const Assignment &vars) const
{
    return (not left_->evaluate(state, vars)) or right_->evaluate(state, vars);
}

Expression::Type Imply::type() const
{
    return Expression::IMPLY;
}

void Imply::getVariables(Variables &vars) const
{
    left_->getVariables(vars);
    right_->getVariables(vars);
}

bool Imply::containsVariable(const std::string &variable) const
{
    return left_->containsVariable(variable) or right_->containsVariable(variable);
}

bool Imply::containsProposition(const std::string &proposition) const
{
    return left_->containsProposition(proposition) or right_->containsProposition(proposition);
}

ImplyPtr pddlboat::makeImply(const ExpressionPtr &left, const ExpressionPtr &right)
{
    return std::make_shared<Imply>(std::forward<const ExpressionPtr>(left),
                                   std::forward<const ExpressionPtr>(right));
}
