#ifndef PDDLBOAT_EXPRESSIONS_
#define PDDLBOAT_EXPRESSIONS_

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <type_traits>

#include <pddlboat/macros.h>

namespace pddlboat
{
    PDDLBOAT_CLASS_FORWARD(Assignment)
    PDDLBOAT_CLASS_FORWARD(VariableSet)
    PDDLBOAT_CLASS_FORWARD(Problem)
    PDDLBOAT_CLASS_FORWARD(State)

    ///
    /// Expressions
    ///

    PDDLBOAT_CLASS_FORWARD(Expression)
    class Expression
    {
    public:
        enum Type
        {
            EXPRESSION,
            PREDICATE,
            VARIADIC,
            AND,
            OR,
            NOT,
            EQUALS,
            IMPLY
        };

        virtual bool evaluate(const StatePtr &state, const Assignment &vars) const = 0;
        virtual Type type() const;
        virtual void getVariables(VariableSet &vars) const;
        virtual bool containsVariable(const std::string &variable) const;
        virtual bool containsPredicate(const std::string &predicate) const;
        virtual void toPDDL(std::ostream &out, unsigned int indent = 2, unsigned int level = 0) const = 0;
    };

    ///
    /// Variadic Expression
    ///

    PDDLBOAT_CLASS_FORWARD(VariadicExpression)
    class VariadicExpression : public Expression
    {
    public:
        VariadicExpression();
        VariadicExpression(const std::initializer_list<const ExpressionPtr> &terms);

        void addTerm(const ExpressionPtr &expression);
        std::vector<ExpressionPtr> getTerms() const;

        Expression::Type type() const override;
        void getVariables(VariableSet &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;

    protected:
        std::vector<ExpressionPtr> terms_;
    };

    template <typename T>
    ExpressionPtr makeCAR(const std::shared_ptr<T> &expr)
    {
        static_assert(std::is_base_of<VariadicExpression, T>::value, "T must be a descendant of "
                                                               "VariadicExpression");

        return expr->getTerms().front();
    }

    template <typename T>
    std::shared_ptr<T> makeCDR(const std::shared_ptr<T> &expr)
    {
        static_assert(std::is_base_of<VariadicExpression, T>::value, "T must be a descendant of "
                                                               "VariadicExpression");

        const auto &terms = expr->getTerms();

        if (terms.size() > 1)
        {
            auto cdr = std::make_shared<T>();
            for (unsigned int i = 1; i < terms.size(); ++i)
                cdr->addTerm(terms[i]);

            return cdr;
        }

        return nullptr;
    }

    ///
    /// And
    ///

    PDDLBOAT_CLASS_FORWARD(And)
    class And : public VariadicExpression
    {
    public:
        And(const std::initializer_list<const ExpressionPtr> &terms = {});

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        Expression::Type type() const override;
        void toPDDL(std::ostream &out, unsigned int indent = 2, unsigned int level = 0) const override;
    };

    AndPtr makeAnd(const std::initializer_list<const ExpressionPtr> &terms);

    ///
    /// Or
    ///

    PDDLBOAT_CLASS_FORWARD(Or)
    class Or : public VariadicExpression
    {
    public:
        Or(const std::initializer_list<const ExpressionPtr> &terms = {});

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        Expression::Type type() const override;
        void toPDDL(std::ostream &out, unsigned int indent = 2, unsigned int level = 0) const override;
    };

    OrPtr makeOr(const std::initializer_list<const ExpressionPtr> &terms);

    ///
    /// Not
    ///

    PDDLBOAT_CLASS_FORWARD(Not)
    class Not : public Expression
    {
    public:
        Not(const ExpressionPtr &term);

        const ExpressionPtr getTerm() const;

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        Expression::Type type() const override;
        void getVariables(VariableSet &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;
        void toPDDL(std::ostream &out, unsigned int indent = 2, unsigned int level = 0) const override;

    private:
        const ExpressionPtr term_;
    };

    NotPtr makeNot(const ExpressionPtr &term);

    ///
    /// Imply
    ///

    PDDLBOAT_CLASS_FORWARD(Imply)
    class Imply : public Expression
    {
    public:
        Imply(const ExpressionPtr &left, const ExpressionPtr &right);

        const ExpressionPtr getLeft() const;
        const ExpressionPtr getRight() const;

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        Expression::Type type() const override;

        void getVariables(VariableSet &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;
        void toPDDL(std::ostream &out, unsigned int indent = 2, unsigned int level = 0) const override;

    private:
        const ExpressionPtr left_, right_;
    };

    ImplyPtr makeImply(const ExpressionPtr &left, const ExpressionPtr &right);

}  // namespace pddlboat

#endif
