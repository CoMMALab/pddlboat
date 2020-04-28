/* Author: Zachary Kingston */

#include <boost/algorithm/string.hpp>

#include <pddlboat/utility.h>
#include <pddlboat/solver/expressions.h>

using namespace pddlboat;

///
/// Value
///

Value::Value(bool isVar, const std::string &name, const std::string &type)
  : isVar(isVar), name(boost::algorithm::to_lower_copy(name)), type(type)
{
}

Value pddlboat::getValue(const std::string &term, const Variable *variable, const ProblemPtr &problem,
                         const DomainPtr &domain)
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

///
/// Expression
///

Expression::Type Expression::type() const
{
    return EXPRESSION;
}

void Expression::assign(StatePtr &out, const Assignment &vars) const
{
    throw std::invalid_argument(log::format("Cannot assign with expression type %1%", type()));
}

void Expression::getVariables(VariableSet &vars) const
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

///
/// Variadic Expression
///

VariadicExpression::VariadicExpression() = default;

VariadicExpression::VariadicExpression(const std::vector<ExpressionPtr> &terms)
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

void VariadicExpression::getVariables(VariableSet &vars) const
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

bool VariadicExpression::containsPredicate(const std::string &prop) const
{
    for (const auto &e : terms_)
        if (e->containsPredicate(prop))
            return true;

    return false;
}

///
/// And
///

And::And(const std::vector<ExpressionPtr> &terms) : VariadicExpression(terms)
{
}

bool And::evaluate(const StatePtr &state, const Assignment &vars) const
{
    bool ret = true;
    for (unsigned int i = 0; i < terms_.size() and ret; ++i)
        ret &= terms_[i]->evaluate(state, vars);

    return ret;
}

void And::assign(StatePtr &out, const Assignment &vars) const
{
    for (unsigned int i = 0; i < terms_.size(); ++i)
        terms_[i]->assign(out, vars);
}

Expression::Type And::type() const
{
    return Expression::AND;
}

std::ostream &And::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(and " << std::endl;
    for (const auto &term : terms_)
    {
        term->toPDDL(out, indent, level + 1);
        out << std::endl;
    }
    out << SPACE << ")";

    return out;
}

AndPtr pddlboat::makeAnd(const std::vector<ExpressionPtr> &terms)
{
    return std::make_shared<And>(std::forward<const std::vector<ExpressionPtr>>(terms));
}

///
/// Or
///

Or::Or(const std::vector<ExpressionPtr> &terms) : VariadicExpression(terms)
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

std::ostream &Or::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(or " << std::endl;
    for (const auto &term : terms_)
    {
        term->toPDDL(out, indent, level + 1);
        out << std::endl;
    }

    out << SPACE << ")";

    return out;
}

OrPtr pddlboat::makeOr(const std::vector<ExpressionPtr> &terms)
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
    return not term_->evaluate(state, vars);
}

void Not::assign(StatePtr &out, const Assignment &vars) const
{
    auto temp = out->getProblem()->makeState();
    term_->assign(temp, vars);
    temp->complement();

    out->fold(temp);
}

Expression::Type Not::type() const
{
    return Expression::NOT;
}

void Not::getVariables(VariableSet &vars) const
{
    term_->getVariables(vars);
}

bool Not::containsVariable(const std::string &variable) const
{
    return term_->containsVariable(variable);
}

bool Not::containsPredicate(const std::string &predicate) const
{
    return term_->containsPredicate(predicate);
}

std::ostream &Not::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(not " << std::endl;
    term_->toPDDL(out, indent, level + 1);
    out << std::endl << SPACE << ")";

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
    return (not left_->evaluate(state, vars)) or right_->evaluate(state, vars);
}

Expression::Type Imply::type() const
{
    return Expression::IMPLY;
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

std::ostream &Imply::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(imply " << std::endl;
    left_->toPDDL(out, indent, level + 1);
    out << std::endl;
    right_->toPDDL(out, indent, level + 1);
    out << std::endl;
    out << SPACE << ")";

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
    return (not left_->evaluate(state, vars)) or right_->evaluate(state, vars);
}

Expression::Type When::type() const
{
    return Expression::WHEN;
}

void When::getVariables(VariableSet &vars) const
{
    left_->getVariables(vars);
    right_->getVariables(vars);
}

bool When::containsVariable(const std::string &variable) const
{
    return left_->containsVariable(variable) or right_->containsVariable(variable);
}

bool When::containsPredicate(const std::string &predicate) const
{
    return left_->containsPredicate(predicate) or right_->containsPredicate(predicate);
}

std::ostream &When::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(when " << std::endl;
    left_->toPDDL(out, indent, level + 1);
    out << std::endl;
    right_->toPDDL(out, indent, level + 1);
    out << std::endl;
    out << SPACE << ")";

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
}

Expression::Type QuantifierExpression::type() const
{
    return Expression::QUANTIFIER;
}

void QuantifierExpression::getVariables(VariableSet &vars) const
{
    formula_->getVariables(vars);
}

bool QuantifierExpression::containsVariable(const std::string &variable) const
{
    return formula_->containsVariable(variable);
}

bool QuantifierExpression::containsPredicate(const std::string &predicate) const
{
    return formula_->containsPredicate(predicate);
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
}

void Exists::assign(StatePtr &out, const Assignment &vars) const
{
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
    domain_.toPDDL(out) << std::endl;

    formula_->toPDDL(out, indent, level + 1);
    out << std::endl;

    out << SPACE << ")";

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
}

void Forall::assign(StatePtr &out, const Assignment &vars) const
{
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
    domain_.toPDDL(out) << std::endl;

    formula_->toPDDL(out, indent, level + 1);
    out << std::endl;

    out << SPACE << ")";

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

bool Equals::evaluate(const StatePtr &state, const Assignment &vars) const
{
    // return not term_->evaluate(state, vars);
}

void Equals::assign(StatePtr &out, const Assignment &vars) const
{
    // auto temp = out->getProblem()->makeState();
    // term_->assign(temp, vars);
    // temp->complement();

    // out->fold(temp);
}

Expression::Type Equals::type() const
{
    return Expression::EQUALS;
}

void Equals::getVariables(VariableSet &vars) const
{
    // term_->getVariables(vars);
}

bool Equals::containsVariable(const std::string &variable) const
{
    // return term_->containsVariable(variable);
    return false;
}

bool Equals::containsPredicate(const std::string &predicate) const
{
    // return term_->containsPredicate(predicate);
    return false;
}

std::ostream &Equals::toPDDL(std::ostream &out, unsigned int indent, unsigned int level) const
{
    const std::string SPACE = std::string(indent * level, ' ');
    out << SPACE;

    out << "(= ";
    out << (term1_.isVar ? "?" : "") << term1_.name << " ";
    out << (term2_.isVar ? "?" : "") << term2_.name;
    out << ")";

    return out;
}

EqualsPtr pddlboat::makeEquals(const Value &term1, const Value &term2)
{
    return std::make_shared<Equals>(term1, term2);
}
