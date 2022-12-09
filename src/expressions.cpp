/* Author: Zachary Kingston */

#include <boost/algorithm/string.hpp>

#include <pddlboat/utility.h>
#include <pddlboat/solver/expressions.h>

using namespace pddlboat;

static const bool debug = false;
// static const bool debug = true;

///
/// Value
///

Value::Value(bool isVar, const std::string &name, const std::string &type)
  : isVar(isVar), name(boost::algorithm::to_lower_copy(name)), type(type)
{
}

Value::Value(const ObjectFunctionPtr &function)
  : name(function->getName()), type(function->getReturnType()), function(function)
{
}

std::string Value::getObject(const StatePtr &state, const Assignment &vars) const
{
    if (isVar)
        return vars.assigned(name)->name;
    else if (function)
        return function->getObject(state, vars);
    else
        return name;
}

std::vector<std::string> pddlboat::getValueObjects(const std::vector<pddlboat::Value> &values,
                                                   const StatePtr &state, const Assignment &assignment)
{
    std::vector<std::string> objects;
    for (const auto &value : values)
        objects.emplace_back(value.getObject(state, assignment));

    return objects;
}

Value pddlboat::getValue(const std::string &term, const Variable *variable, const Problem *problem,
                         const Domain *domain)
{
    std::string b = term;
    std::string type = PDDLBOAT_OBJECT_CLASS;
    bool isVar = io::isPrefix("?", b);
    if (isVar)
    {
        b.erase(0, 1);

        if (variable)
            type = variable->type;
    }
    else if (problem)
    {
        auto obj = problem->getObject(b);
        if (not obj)
            throw std::invalid_argument(log::format("Cannot bind object %1%, does not exist!", b));

        type = obj->type;
    }
    else if (domain)
    {
        auto obj = domain->getConstant(b);
        if (not obj)
            throw std::invalid_argument(log::format("Cannot bind object %1%, does not exist!", b));

        type = obj->type;
    }
    else
    {
        if (not problem)
            throw std::invalid_argument(log::format("Cannot bind object %1% without problem or domain!", b));
    }

    return Value(isVar, b, type);
}

std::vector<pddlboat::Value> pddlboat::getValues(const std::vector<std::string> &terms,
                                                 const std::vector<pddlboat::Variable> &variables,
                                                 const ProblemPtr &problem, const DomainPtr &domain)
{
    return getValues(terms, variables, (problem) ? problem.get() : nullptr,
                     (domain) ? domain.get() : nullptr);
}

std::vector<pddlboat::Value> pddlboat::getValues(const std::vector<std::string> &terms,
                                                 const std::vector<pddlboat::Variable> &variables,
                                                 const Problem *problem, const Domain *domain)
{
    std::vector<pddlboat::Value> toUse;
    for (unsigned int i = 0; i < terms.size(); ++i)
        toUse.emplace_back(getValue(terms[i], &variables[i], problem, domain));

    return toUse;
}

void Value::getVariables(VariableSet &vars) const
{
    if (isVar)
        vars.addVariable({name, type});
    if (function)
        function->getVariables(vars);
}

bool Value::containsVariable(const std::string &variable) const
{
    if (isVar and name == variable)
        return true;

    if (function and function->containsVariable(variable))
        return true;

    return false;
}

void Value::renameVariable(const std::string &old, const std::string &newname)
{
    if (isVar and name == old)
        name = newname;
    if (function)
        function->renameVariable(old, newname);
}

void Value::getUsedFunctions(std::set<std::string> &functions, const StatePtr &state,
                             const Assignment &vars) const
{
    if (function)
        function->getUsedPredicates(functions, state, vars);
}

std::ostream &Value::toPDDL(std::ostream &out) const
{
    if (isVar)
        out << "?";
    if (function)
        function->toPDDL(out);
    else
        out << name;

    return out;
}

std::ostream &Value::toString(std::ostream &out) const
{
    if (isVar)
        out << "?";
    out << name << " - " << type;
    return out;
}

///
/// Expression
///

void Expression::Effect::flip()
{
    auto temp = remove;
    remove = add;
    add = temp;
}

void Expression::Effect::absorb(const Effect &other)
{
    add.insert(other.add.begin(), other.add.end());
    remove.insert(other.remove.begin(), other.remove.end());
    assigned.insert(other.assigned.begin(), other.assigned.end());
}

std::ostream &Expression::Effect::toPDDL(std::ostream &out, std::size_t indent) const
{
    const std::string SPACE = std::string(indent, ' ');

    if (not add.empty())
    {
        out << SPACE << "+";
        for (const auto &predicate : add)
            out << " " << predicate;
        out << std::endl;
    }

    if (not remove.empty())
    {
        out << SPACE << "-";
        for (const auto &predicate : remove)
            out << " " << predicate;
        out << std::endl;
    }

    if (not assigned.empty())
    {
        out << SPACE << "=";
        for (const auto &fluent : assigned)
            out << " " << fluent;
        out << std::endl;
    }

    return out;
}

Expression::Type Expression::type() const
{
    return EXPRESSION;
}

Expression::Type Expression::getTypes() const
{
    return type();
}

bool Expression::evaluate(const StatePtr &state, const Assignment &vars) const
{
    throw std::invalid_argument(log::format("Cannot evaluate with expression type %1%", type()));
}

void Expression::assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    throw std::invalid_argument(log::format("Cannot assign with expression type %1%", type()));
}

void Expression::getVariables(VariableSet &vars) const
{
}

void Expression::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                   const Assignment &vars) const
{
}

void Expression::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                       const Assignment &vars) const
{
}

void Expression::getEffects(Effect &effect, const StatePtr &state, const Assignment &vars) const
{
}

bool Expression::containsVariable(const std::string &variable) const
{
    std::cerr << log::format("Failed to find variable \"%1%\" in expression.", variable) << std::endl;
    return false;
}

bool Expression::containsPredicate(const std::string &predicate) const
{
    std::cerr << log::format("Failed to find predicate \"%1%\" in expression.", predicate) << std::endl;
    return false;
}

void Expression::renameVariable(const std::string &old, const std::string &name)
{
}

Expression::Type pddlboat::operator|(Expression::Type lhs, Expression::Type rhs)
{
    return static_cast<Expression::Type>(                               //
        static_cast<std::underlying_type<Expression::Type>::type>(lhs)  //
        | static_cast<std::underlying_type<Expression::Type>::type>(rhs));
}

Expression::Type pddlboat::operator&(Expression::Type lhs, Expression::Type rhs)
{
    return static_cast<Expression::Type>(                               //
        static_cast<std::underlying_type<Expression::Type>::type>(lhs)  //
        & static_cast<std::underlying_type<Expression::Type>::type>(rhs));
}

bool pddlboat::isIn(Expression::Type type, Expression::Type set)
{
    return (type & set) == type;
}

///
/// Numeric Expression
///

double NumericExpression::getValue(const StatePtr &state, const Assignment &vars) const
{
    throw std::invalid_argument(log::format("Cannot getValue with numeric expression type %1%", type()));
}

///
/// Variadic Expression
///

VariadicExpression::VariadicExpression() = default;

VariadicExpression::VariadicExpression(const std::vector<pddlboat::ExpressionPtr> &terms)
{
    for (const auto &term : terms)
        addTerm(term);
}

void VariadicExpression::addTerm(const ExpressionPtr &expression)
{
    terms_.emplace_back(expression);
}

std::vector<pddlboat::ExpressionPtr> VariadicExpression::getTerms() const
{
    return terms_;
}

Expression::Type VariadicExpression::type() const
{
    return Expression::VARIADIC;
}

Expression::Type VariadicExpression::getTypes() const
{
    Expression::Type r = type();
    for (const auto &e : terms_)
        r = r | e->getTypes();

    return r;
}

void VariadicExpression::getVariables(VariableSet &vars) const
{
    for (const auto &e : terms_)
        e->getVariables(vars);
}

void VariadicExpression::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                           const Assignment &vars) const
{
    for (const auto &e : terms_)
        e->getUsedPredicates(predicates, state, vars);
}

void VariadicExpression::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                               const Assignment &vars) const
{
    for (const auto &e : terms_)
        e->getAssignedPredicates(predicates, state, vars);
}

void VariadicExpression::getEffects(Effect &effect, const StatePtr &state, const Assignment &vars) const
{
    for (const auto &e : terms_)
        e->getEffects(effect, state, vars);
}

bool VariadicExpression::containsVariable(const std::string &variable) const
{
    for (const auto &e : terms_)
        if (e->containsVariable(variable))
            return true;

    return false;
}

bool VariadicExpression::containsPredicate(const std::string &prop) const
{
    for (const auto &e : terms_)
        if (e->containsPredicate(prop))
            return true;

    return false;
}

void VariadicExpression::renameVariable(const std::string &old, const std::string &name)
{
    for (const auto &e : terms_)
        e->renameVariable(old, name);
}

///
/// And
///

And::And(const std::vector<pddlboat::ExpressionPtr> &terms) : VariadicExpression(terms)
{
}

bool And::evaluate(const StatePtr &state, const Assignment &vars) const
{
    bool ret = true;
    for (unsigned int i = 0; i < terms_.size() and ret; ++i)
        ret &= terms_[i]->evaluate(state, vars);

    if (debug)
    {
        std::cout << "Expression is " << ret << std::endl;
        toPDDL(std::cout);
        std::cout << std::endl;
        vars.toString(std::cout);
        std::cout << std::endl;
    }

    return ret;
}

void And::assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    for (unsigned int i = 0; i < terms_.size(); ++i)
        terms_[i]->assign(out, in, vars);
}

Expression::Type And::type() const
{
    return Expression::AND;
}

std::ostream &And::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(and";
    if (not terms_.empty())
        out << std::endl;
    for (std::size_t i = 0; i < terms_.size(); ++i)
    {
        terms_[i]->toPDDL(out, indent, level + 1);
        if (i != terms_.size() - 1)
            out << std::endl;
    }
    out << ")";

    return out;
}

AndPtr pddlboat::makeAnd(const std::vector<pddlboat::ExpressionPtr> &terms)
{
    return std::make_shared<And>(std::forward<const std::vector<ExpressionPtr>>(terms));
}

///
/// Or
///

Or::Or(const std::vector<pddlboat::ExpressionPtr> &terms) : VariadicExpression(terms)
{
}

bool Or::evaluate(const StatePtr &state, const Assignment &vars) const
{
    bool ret = false;
    for (unsigned int i = 0; i < terms_.size() and not ret; ++i)
        ret |= terms_[i]->evaluate(state, vars);

    if (debug)
    {
        std::cout << "Expression is " << ret << std::endl;
        toPDDL(std::cout);
        std::cout << std::endl;
        vars.toString(std::cout);
        std::cout << std::endl;
    }

    return ret;
}

Expression::Type Or::type() const
{
    return Expression::OR;
}

std::ostream &Or::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(or";
    if (not terms_.empty())
        out << std::endl;

    for (std::size_t i = 0; i < terms_.size(); ++i)
    {
        terms_[i]->toPDDL(out, indent, level + 1);
        if (i != terms_.size() - 1)
            out << std::endl;
    }
    out << ")";

    return out;
}

OrPtr pddlboat::makeOr(const std::vector<pddlboat::ExpressionPtr> &terms)
{
    return std::make_shared<Or>(std::forward<const std::vector<ExpressionPtr>>(terms));
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
    bool ret = not term_->evaluate(state, vars);

    if (debug)
    {
        std::cout << "Expression is " << ret << std::endl;
        toPDDL(std::cout);
        std::cout << std::endl;
        vars.toString(std::cout);
        std::cout << std::endl;
    }

    return ret;
}

void Not::assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    auto temp = out->getProblem()->makeState();
    term_->assign(temp, in, vars);
    temp->complement();

    out->fold(temp);
}

Expression::Type Not::type() const
{
    return Expression::NOT;
}

Expression::Type Not::getTypes() const
{
    return type() | term_->getTypes();
}

void Not::getVariables(VariableSet &vars) const
{
    term_->getVariables(vars);
}

void Not::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                            const Assignment &vars) const
{
    term_->getUsedPredicates(predicates, state, vars);
}

void Not::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                const Assignment &vars) const
{
    term_->getAssignedPredicates(predicates, state, vars);
}

void Not::getEffects(Effect &effect, const StatePtr &state, const Assignment &vars) const
{
    Effect temp;
    term_->getEffects(temp, state, vars);
    temp.flip();
    effect.absorb(temp);
}

bool Not::containsVariable(const std::string &variable) const
{
    return term_->containsVariable(variable);
}

bool Not::containsPredicate(const std::string &predicate) const
{
    return term_->containsPredicate(predicate);
}

void Not::renameVariable(const std::string &old, const std::string &name)
{
    term_->renameVariable(old, name);
}

std::ostream &Not::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(not " << std::endl;
    term_->toPDDL(out, indent, level + 1);
    out << ")";

    return out;
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
    bool ret = (not left_->evaluate(state, vars)) or right_->evaluate(state, vars);

    if (debug)
    {
        std::cout << "Expression is " << ret << std::endl;
        toPDDL(std::cout);
        std::cout << std::endl;
        vars.toString(std::cout);
        std::cout << std::endl;
    }

    return ret;
}

Expression::Type Imply::type() const
{
    return Expression::IMPLY;
}

Expression::Type Imply::getTypes() const
{
    return type() | left_->getTypes() | right_->getTypes();
}

void Imply::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                              const Assignment &vars) const
{
    left_->getUsedPredicates(predicates, state, vars);
    right_->getUsedPredicates(predicates, state, vars);
}

void Imply::getVariables(VariableSet &vars) const
{
    left_->getVariables(vars);
    right_->getVariables(vars);
}

bool Imply::containsVariable(const std::string &variable) const
{
    return left_->containsVariable(variable) or right_->containsVariable(variable);
}

bool Imply::containsPredicate(const std::string &predicate) const
{
    return left_->containsPredicate(predicate) or right_->containsPredicate(predicate);
}

void Imply::renameVariable(const std::string &old, const std::string &name)
{
    left_->renameVariable(old, name);
    right_->renameVariable(old, name);
}

std::ostream &Imply::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(imply " << std::endl;
    left_->toPDDL(out, indent, level + 1);
    out << std::endl;
    right_->toPDDL(out, indent, level + 1);
    out << ")";

    return out;
}

ImplyPtr pddlboat::makeImply(const ExpressionPtr &left, const ExpressionPtr &right)
{
    return std::make_shared<Imply>(std::forward<const ExpressionPtr>(left),
                                   std::forward<const ExpressionPtr>(right));
}

///
/// When
///

When::When(const ExpressionPtr &left, const ExpressionPtr &right) : left_(left), right_(right)
{
}

const ExpressionPtr When::getLeft() const
{
    return left_;
}

const ExpressionPtr When::getRight() const
{
    return right_;
}

bool When::evaluate(const StatePtr &state, const Assignment &vars) const
{
    bool r = (not left_->evaluate(state, vars)) or right_->evaluate(state, vars);

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

void When::assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    if (left_->evaluate(in, vars))
        right_->assign(out, in, vars);
}

Expression::Type When::type() const
{
    return Expression::WHEN;
}

Expression::Type When::getTypes() const
{
    return type() | left_->getTypes() | right_->getTypes();
}

void When::getVariables(VariableSet &vars) const
{
    left_->getVariables(vars);
    right_->getVariables(vars);
}

void When::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                             const Assignment &vars) const
{
    left_->getUsedPredicates(predicates, state, vars);
    right_->getUsedPredicates(predicates, state, vars);
}

void When::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                 const Assignment &vars) const
{
    right_->getAssignedPredicates(predicates, state, vars);
}

void When::getEffects(Effect &effect, const StatePtr &state, const Assignment &vars) const
{
    right_->getEffects(effect, state, vars);
}

bool When::containsVariable(const std::string &variable) const
{
    return left_->containsVariable(variable) or right_->containsVariable(variable);
}

bool When::containsPredicate(const std::string &predicate) const
{
    return left_->containsPredicate(predicate) or right_->containsPredicate(predicate);
}

void When::renameVariable(const std::string &old, const std::string &name)
{
    left_->renameVariable(old, name);
    right_->renameVariable(old, name);
}

std::ostream &When::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(when " << std::endl;
    left_->toPDDL(out, indent, level + 1);
    out << std::endl;
    right_->toPDDL(out, indent, level + 1);
    out << ")";

    return out;
}

WhenPtr pddlboat::makeWhen(const ExpressionPtr &left, const ExpressionPtr &right)
{
    return std::make_shared<When>(std::forward<const ExpressionPtr>(left),
                                  std::forward<const ExpressionPtr>(right));
}

///
/// QuantifierExpression
///

QuantifierExpression::QuantifierExpression(const VariableSet &domain, const ExpressionPtr &formula)
  : domain_(domain), formula_(formula)
{
    if (domain_.empty())
        throw std::runtime_error("Domain for quantifier must be non-empty!");
}

const VariableList &QuantifierExpression::getDomain() const
{
    return domain_;
}

const ExpressionPtr &QuantifierExpression::getFormula() const
{
    return formula_;
}

Expression::Type QuantifierExpression::type() const
{
    return Expression::QUANTIFIER;
}

Expression::Type QuantifierExpression::getTypes() const
{
    return type() | formula_->getTypes();
}

void QuantifierExpression::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                             const Assignment &vars) const
{
    auto copy = vars;
    const auto &groundings = state->getProblem()->getGroundings(domain_);

    for (const auto &grounding : groundings)
    {
        copy.fold(grounding);
        formula_->getUsedPredicates(predicates, state, copy);
    }
}

void QuantifierExpression::getVariables(VariableSet &vars) const
{
    formula_->getVariables(vars);
    vars.remove(domain_);
}

bool QuantifierExpression::containsVariable(const std::string &variable) const
{
    return formula_->containsVariable(variable);
}

bool QuantifierExpression::containsPredicate(const std::string &predicate) const
{
    return formula_->containsPredicate(predicate);
}

void QuantifierExpression::renameVariable(const std::string &old, const std::string &name)
{
    formula_->renameVariable(old, name);
}

///
/// Exists
///

Exists::Exists(const VariableSet &domain, const ExpressionPtr &formula)
  : QuantifierExpression(domain, formula)
{
}

bool Exists::evaluate(const StatePtr &state, const Assignment &vars) const
{
    auto copy = vars;
    const auto &groundings = state->getProblem()->getGroundings(domain_);

    bool r = false;
    for (const auto &grounding : groundings)
    {
        copy.fold(grounding);
        if (formula_->evaluate(state, copy))
        {
            r = true;
            break;
        }
    }

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

Expression::Type Exists::type() const
{
    return Expression::EXISTS;
}

std::ostream &Exists::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(exists ";
    VariableSet(domain_).toPDDL(out) << std::endl;

    formula_->toPDDL(out, indent, level + 1);
    out << ")";

    return out;
}

ExistsPtr pddlboat::makeExists(const VariableSet &domain, const ExpressionPtr &formula)
{
    return std::make_shared<Exists>(domain, formula);
}

///
/// Forall
///

Forall::Forall(const VariableSet &domain, const ExpressionPtr &formula)
  : QuantifierExpression(domain, formula)
{
}

bool Forall::evaluate(const StatePtr &state, const Assignment &vars) const
{
    auto copy = vars;
    const auto &groundings = state->getProblem()->getGroundings(domain_);

    bool r = true;
    for (const auto &grounding : groundings)
    {
        copy.fold(grounding);
        if (not formula_->evaluate(state, copy))
        {
            r = false;
            break;
        }
    }

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

void Forall::assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    auto copy = vars;
    const auto &groundings = in->getProblem()->getGroundings(domain_);

    for (const auto &grounding : groundings)
    {
        copy.fold(grounding);
        formula_->assign(out, in, copy);
    }
}

void Forall::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                   const Assignment &vars) const
{
    auto copy = vars;
    const auto &groundings = state->getProblem()->getGroundings(domain_);

    for (const auto &grounding : groundings)
    {
        copy.fold(grounding);
        formula_->getAssignedPredicates(predicates, state, copy);
    }
}

void Forall::getEffects(Effect &effect, const StatePtr &state, const Assignment &vars) const
{
    formula_->getEffects(effect, state, vars);
}

Expression::Type Forall::type() const
{
    return Expression::FORALL;
}

std::ostream &Forall::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(forall ";
    VariableSet(domain_).toPDDL(out) << std::endl;

    formula_->toPDDL(out, indent, level + 1);
    out << ")";

    return out;
}

ForallPtr pddlboat::makeForall(const VariableSet &domain, const ExpressionPtr &formula)
{
    return std::make_shared<Forall>(domain, formula);
}

///
/// Equals
///

Equals::Equals(const Value &term1, const Value &term2) : term1_(term1), term2_(term2)
{
}

std::pair<Value, Value> Equals::getTerms() const
{
    return {term1_, term2_};
}

void Equals::assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    if (term1_.function)
        out->assignN(term1_.name, getValueObjects(term1_.function->bindings, in, vars),
                     term2_.getObject(in, vars));
    else
        throw std::invalid_argument(log::format("Cannot assign with = with non-function first term."));
}

bool Equals::evaluate(const StatePtr &state, const Assignment &vars) const
{
    return term1_.getObject(state, vars) == term2_.getObject(state, vars);
}

Expression::Type Equals::type() const
{
    return Expression::EQUALS;
}

Expression::Type Equals::getTypes() const
{
    return type();
}

void Equals::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                               const Assignment &vars) const
{
    if (term1_.function)
        term1_.function->getUsedPredicates(predicates, state, vars);
    if (term2_.function)
        term2_.function->getUsedPredicates(predicates, state, vars);
}

void Equals::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                   const Assignment &vars) const
{
    if (term1_.function)
        term1_.function->getAssignedPredicates(predicates, state, vars);
}

void Equals::getEffects(Effect &effect, const StatePtr &state, const Assignment &vars) const
{
    if (term1_.function)
        term1_.function->getEffects(effect, state, vars);
}

void Equals::getVariables(VariableSet &vars) const
{
    term1_.getVariables(vars);
    term2_.getVariables(vars);
}

bool Equals::containsVariable(const std::string &variable) const
{
    return term1_.containsVariable(variable) or term2_.containsVariable(variable);
}

bool Equals::containsPredicate(const std::string &predicate) const
{
    return false;
}

void Equals::renameVariable(const std::string &old, const std::string &name)
{
    term1_.renameVariable(old, name);
    term2_.renameVariable(old, name);
}

std::ostream &Equals::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(= ";
    term1_.toPDDL(out) << " ";
    term2_.toPDDL(out);
    out << ")";

    return out;
}

EqualsPtr pddlboat::makeEquals(const Value &term1, const Value &term2)
{
    return std::make_shared<Equals>(term1, term2);
}

///
/// Binary
///

BinaryExpression::BinaryExpression(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
  : lhs_(lhs), rhs_(rhs)
{
}

const NumericExpressionPtr &BinaryExpression::getLHS() const
{
    return lhs_;
}

const NumericExpressionPtr &BinaryExpression::getRHS() const
{
    return rhs_;
}

Expression::Type BinaryExpression::getTypes() const
{
    return type() | lhs_->getTypes() | rhs_->getTypes();
}

void BinaryExpression::getVariables(VariableSet &vars) const
{
    lhs_->getVariables(vars);
    rhs_->getVariables(vars);
}

void BinaryExpression::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                         const Assignment &vars) const
{
    lhs_->getUsedPredicates(predicates, state, vars);
    rhs_->getUsedPredicates(predicates, state, vars);
}

bool BinaryExpression::containsVariable(const std::string &variable) const
{
    return lhs_->containsVariable(variable) or rhs_->containsVariable(variable);
}

bool BinaryExpression::containsPredicate(const std::string &predicate) const
{
    return lhs_->containsPredicate(predicate) or rhs_->containsPredicate(predicate);
}

void BinaryExpression::renameVariable(const std::string &old, const std::string &name)
{
    lhs_->renameVariable(old, name);
    rhs_->renameVariable(old, name);
}

std::ostream &BinaryExpression::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(";
    switch (type())
    {
        case Expression::MINUS:
            out << "-";
            break;
        case Expression::DIVIDE:
            out << "/";
            break;
        case Expression::MULTIPLY:
            out << "*";
            break;
        case Expression::PLUS:
            out << "+";
            break;
        case Expression::GREATER_THAN:
            out << ">";
            break;
        case Expression::LESS_THAN:
            out << "<";
            break;
        case Expression::GREATER_OR_EQUAL:
            out << ">=";
            break;
        case Expression::LESS_OR_EQUAL:
            out << "<=";
            break;
        default:
            throw std::runtime_error("Invalid type for binary expression!");
    };

    out << " " << std::endl;
    lhs_->toPDDL(out, indent, level + 1);
    out << std::endl;
    rhs_->toPDDL(out, indent, level + 1);
    out << ")";

    return out;
}

///
/// Greater Than
///

GreaterThan::GreaterThan(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
  : BinaryExpression(lhs, rhs)
{
}

Expression::Type GreaterThan::type() const
{
    return Expression::GREATER_THAN;
}

bool GreaterThan::evaluate(const StatePtr &state, const Assignment &vars) const
{
    return lhs_->getValue(state, vars) > rhs_->getValue(state, vars);
}

GreaterThanPtr pddlboat::makeGreaterThan(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<GreaterThan>(lhs, rhs);
}

///
/// Less Than
///

LessThan::LessThan(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
  : BinaryExpression(lhs, rhs)
{
}

Expression::Type LessThan::type() const
{
    return Expression::LESS_THAN;
}

bool LessThan::evaluate(const StatePtr &state, const Assignment &vars) const
{
    return lhs_->getValue(state, vars) < rhs_->getValue(state, vars);
}

LessThanPtr pddlboat::makeLessThan(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<LessThan>(lhs, rhs);
}

///
/// Greater OrEqual
///

GreaterOrEqual::GreaterOrEqual(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
  : BinaryExpression(lhs, rhs)
{
}

Expression::Type GreaterOrEqual::type() const
{
    return Expression::GREATER_OR_EQUAL;
}

bool GreaterOrEqual::evaluate(const StatePtr &state, const Assignment &vars) const
{
    return lhs_->getValue(state, vars) >= rhs_->getValue(state, vars);
}

GreaterOrEqualPtr pddlboat::makeGreaterOrEqual(const NumericExpressionPtr &lhs,
                                               const NumericExpressionPtr &rhs)
{
    return std::make_shared<GreaterOrEqual>(lhs, rhs);
}

///
/// Less Or Equal
///

LessOrEqual::LessOrEqual(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
  : BinaryExpression(lhs, rhs)
{
}

Expression::Type LessOrEqual::type() const
{
    return Expression::LESS_OR_EQUAL;
}

bool LessOrEqual::evaluate(const StatePtr &state, const Assignment &vars) const
{
    return lhs_->getValue(state, vars) <= rhs_->getValue(state, vars);
}

LessOrEqualPtr pddlboat::makeLessOrEqual(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<LessOrEqual>(lhs, rhs);
}

///
/// Minus
///

Minus::Minus(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs) : BinaryExpression(lhs, rhs)
{
}

Expression::Type Minus::type() const
{
    return Expression::MINUS;
}

double Minus::getValue(const StatePtr &state, const Assignment &vars) const
{
    return lhs_->getValue(state, vars) - rhs_->getValue(state, vars);
}

MinusPtr pddlboat::makeMinus(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<Minus>(lhs, rhs);
}

///
/// Divide
///

Divide::Divide(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs) : BinaryExpression(lhs, rhs)
{
}

Expression::Type Divide::type() const
{
    return Expression::DIVIDE;
}

double Divide::getValue(const StatePtr &state, const Assignment &vars) const
{
    return lhs_->getValue(state, vars) / rhs_->getValue(state, vars);
}

DividePtr pddlboat::makeDivide(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<Divide>(lhs, rhs);
}

///
/// Multiply
///

Multiply::Multiply(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
  : BinaryExpression(lhs, rhs)
{
}

Expression::Type Multiply::type() const
{
    return Expression::MULTIPLY;
}

double Multiply::getValue(const StatePtr &state, const Assignment &vars) const
{
    return lhs_->getValue(state, vars) * rhs_->getValue(state, vars);
}

MultiplyPtr pddlboat::makeMultiply(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<Multiply>(lhs, rhs);
}

///
/// Plus
///

Plus::Plus(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs) : BinaryExpression(lhs, rhs)
{
}

Expression::Type Plus::type() const
{
    return Expression::PLUS;
}

double Plus::getValue(const StatePtr &state, const Assignment &vars) const
{
    return lhs_->getValue(state, vars) + rhs_->getValue(state, vars);
}

PlusPtr pddlboat::makePlus(const NumericExpressionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<Plus>(lhs, rhs);
}

///
/// Assign Object
///

AssignObject::AssignObject(const ObjectFunctionPtr &lhs, const Value &rhs) : lhs_(lhs), rhs_(rhs)
{
}

void AssignObject::assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    out->assignN(lhs_->getName(), getValueObjects(lhs_->bindings, in, vars), rhs_.getObject(in, vars));
}

Expression::Type AssignObject::type() const
{
    return Expression::ASSIGN;
}

void AssignObject::getVariables(VariableSet &vars) const
{
    lhs_->getVariables(vars);
    rhs_.getVariables(vars);
}

void AssignObject::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                     const Assignment &vars) const
{
    lhs_->getUsedPredicates(predicates, state, vars);
    rhs_.getUsedFunctions(predicates, state, vars);
}

void AssignObject::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                         const Assignment &vars) const
{
    lhs_->getAssignedPredicates(predicates, state, vars);
}

bool AssignObject::containsVariable(const std::string &variable) const
{
    return lhs_->containsVariable(variable) or rhs_.containsVariable(variable);
}

void AssignObject::renameVariable(const std::string &old, const std::string &name)
{
    lhs_->renameVariable(old, name);
    rhs_.renameVariable(old, name);
}

std::ostream &AssignObject::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;
    out << "(assign ";
    lhs_->toPDDL(out) << " ";
    rhs_.toPDDL(out);
    out << ")";

    return out;
}

AssignObjectPtr pddlboat::makeAssign(const ObjectFunctionPtr &lhs, const Value &rhs)
{
    return std::make_shared<AssignObject>(lhs, rhs);
}

///
/// Assign Number
///

AssignNumber::AssignNumber(const NumericFunctionPtr &lhs, const NumericExpressionPtr &rhs)
  : lhs_(lhs), rhs_(rhs)
{
}

AssignNumber::AssignNumber(const NumericFunctionPtr &lhs, double rhs) : lhs_(lhs), constant_(rhs)
{
}

double AssignNumber::getRHSValue(const StatePtr &in, const Assignment &vars) const
{
    if (rhs_)
        return rhs_->getValue(in, vars);
    else
        return constant_;
}

double AssignNumber::getLHSValue(const StatePtr &in, const Assignment &vars) const
{
    return in->getValue(lhs_->getName(), getValueObjects(lhs_->bindings, in, vars));
}

const NumericFunctionPtr &AssignNumber::getLHS() const
{
    return lhs_;
}

const NumericExpressionPtr &AssignNumber::getRHS() const
{
    return rhs_;
}

void AssignNumber::assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const
{
    out->assignN(lhs_->getName(), getValueObjects(lhs_->bindings, in, vars), getRHSValue(in, vars));
}

Expression::Type AssignNumber::type() const
{
    return Expression::ASSIGN;
}

void AssignNumber::getVariables(VariableSet &vars) const
{
    lhs_->getVariables(vars);
    if (rhs_)
        rhs_->getVariables(vars);
}

void AssignNumber::getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                     const Assignment &vars) const
{
    lhs_->getUsedPredicates(predicates, state, vars);
    if (rhs_)
        rhs_->getUsedPredicates(predicates, state, vars);
}

void AssignNumber::getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                         const Assignment &vars) const
{
    lhs_->getAssignedPredicates(predicates, state, vars);
}

bool AssignNumber::containsVariable(const std::string &variable) const
{
    return lhs_->containsVariable(variable) or (rhs_ and rhs_->containsVariable(variable));
}

void AssignNumber::renameVariable(const std::string &old, const std::string &name)
{
    lhs_->renameVariable(old, name);
    if (rhs_)
        rhs_->renameVariable(old, name);
}

std::ostream &AssignNumber::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(";
    switch (type())
    {
        case Expression::ASSIGN:
            out << "assign";
            break;
        case Expression::SCALE_UP:
            out << "scale-up";
            break;
        case Expression::SCALE_DOWN:
            out << "scale-down";
            break;
        case Expression::INCREASE:
            out << "increase";
            break;
        case Expression::DECREASE:
            out << "decrease";
            break;
        default:
            throw std::runtime_error("Invalid type for assign expression!");
    };

    out << " ";
    lhs_->toPDDL(out) << " ";
    if (rhs_)
        rhs_->toPDDL(out);
    else
    {
        double intpart;
        if (std::modf(constant_, &intpart) == 0.0)
            out << log::format("%1$d", (int)constant_);
        else
            out << log::format("%1$f", constant_);
    }
    out << ")";

    return out;
}

AssignNumberPtr pddlboat::makeAssign(const NumericFunctionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<AssignNumber>(lhs, rhs);
}

AssignNumberPtr pddlboat::makeAssign(const NumericFunctionPtr &lhs, double rhs)
{
    return std::make_shared<AssignNumber>(lhs, rhs);
}

///
/// Scale Up
///

ScaleUp::ScaleUp(const NumericFunctionPtr &lhs, const NumericExpressionPtr &rhs) : AssignNumber(lhs, rhs)
{
}

ScaleUp::ScaleUp(const NumericFunctionPtr &lhs, double rhs) : AssignNumber(lhs, rhs)
{
}

Expression::Type ScaleUp::type() const
{
    return Expression::SCALE_UP;
}

double ScaleUp::getRHSValue(const StatePtr &in, const Assignment &vars) const
{
    return getLHSValue(in, vars) * AssignNumber::getRHSValue(in, vars);
}

ScaleUpPtr pddlboat::makeScaleUp(const NumericFunctionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<ScaleUp>(lhs, rhs);
}

ScaleUpPtr pddlboat::makeScaleUp(const NumericFunctionPtr &lhs, double rhs)
{
    return std::make_shared<ScaleUp>(lhs, rhs);
}

///
/// Scale Down
///

ScaleDown::ScaleDown(const NumericFunctionPtr &lhs, const NumericExpressionPtr &rhs) : AssignNumber(lhs, rhs)
{
}

ScaleDown::ScaleDown(const NumericFunctionPtr &lhs, double rhs) : AssignNumber(lhs, rhs)
{
}

Expression::Type ScaleDown::type() const
{
    return Expression::SCALE_DOWN;
}

double ScaleDown::getRHSValue(const StatePtr &in, const Assignment &vars) const
{
    return getLHSValue(in, vars) / AssignNumber::getRHSValue(in, vars);
}

ScaleDownPtr pddlboat::makeScaleDown(const NumericFunctionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<ScaleDown>(lhs, rhs);
}

ScaleDownPtr pddlboat::makeScaleDown(const NumericFunctionPtr &lhs, double rhs)
{
    return std::make_shared<ScaleDown>(lhs, rhs);
}

///
/// Increase
///

Increase::Increase(const NumericFunctionPtr &lhs, const NumericExpressionPtr &rhs) : AssignNumber(lhs, rhs)
{
}

Increase::Increase(const NumericFunctionPtr &lhs, double rhs) : AssignNumber(lhs, rhs)
{
}

Expression::Type Increase::type() const
{
    return Expression::INCREASE;
}

double Increase::getRHSValue(const StatePtr &in, const Assignment &vars) const
{
    return getLHSValue(in, vars) + AssignNumber::getRHSValue(in, vars);
}

IncreasePtr pddlboat::makeIncrease(const NumericFunctionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<Increase>(lhs, rhs);
}

IncreasePtr pddlboat::makeIncrease(const NumericFunctionPtr &lhs, double rhs)
{
    return std::make_shared<Increase>(lhs, rhs);
}

///
/// Decrease
///

Decrease::Decrease(const NumericFunctionPtr &lhs, const NumericExpressionPtr &rhs) : AssignNumber(lhs, rhs)
{
}
Decrease::Decrease(const NumericFunctionPtr &lhs, double rhs) : AssignNumber(lhs, rhs)
{
}

Expression::Type Decrease::type() const
{
    return Expression::DECREASE;
}

double Decrease::getRHSValue(const StatePtr &in, const Assignment &vars) const
{
    return getLHSValue(in, vars) - AssignNumber::getRHSValue(in, vars);
}

DecreasePtr pddlboat::makeDecrease(const NumericFunctionPtr &lhs, const NumericExpressionPtr &rhs)
{
    return std::make_shared<Decrease>(lhs, rhs);
}

DecreasePtr pddlboat::makeDecrease(const NumericFunctionPtr &lhs, double rhs)
{
    return std::make_shared<Decrease>(lhs, rhs);
}
