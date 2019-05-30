/* Author: Zachary Kingston */

#include <pddlboat/utility.h>
#include <pddlboat/solver/expressions.h>

using namespace pddlboat;

///
/// Expression
///

Expression::Type Expression::type() const
{
    return EXPRESSION;
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

ImplyPtr pddlboat::makeImply(const ExpressionPtr &left, const ExpressionPtr &right)
{
    return std::make_shared<Imply>(std::forward<const ExpressionPtr>(left),
                                   std::forward<const ExpressionPtr>(right));
}
