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
#include <pddlboat/solver/expression.h>
#include <pddlboat/solver/domain.h>

namespace pddlboat
{
    ///
    /// Variadic Expression
    ///

    PDDLBOAT_CLASS_FORWARD(VariadicExpression)
    class VariadicExpression : public Expression
    {
    public:
        VariadicExpression();
        VariadicExpression(const std::vector<ExpressionPtr> &terms);

        void addTerm(const ExpressionPtr &expression);
        std::vector<ExpressionPtr> getTerms() const;

        Expression::Type type() const override;
        Expression::Type getTypes() const override;
        void getVariables(VariableSet &vars) const override;

        void getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state, const Assignment &vars) const override;
        void getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state, const Assignment &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;
        void renameVariable(const std::string &old, const std::string &name) override;

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
        And(const std::vector<ExpressionPtr> &terms = {});

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        void assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const override;
        Expression::Type type() const override;
        std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                             unsigned int level = 0) const override;
    };

    AndPtr makeAnd(const std::vector<ExpressionPtr> &terms);

    ///
    /// Or
    ///

    PDDLBOAT_CLASS_FORWARD(Or)
    class Or : public VariadicExpression
    {
    public:
        Or(const std::vector<ExpressionPtr> &terms = {});

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        Expression::Type type() const override;
        std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                             unsigned int level = 0) const override;
    };

    OrPtr makeOr(const std::vector<ExpressionPtr> &terms);

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
        void assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const override;
        Expression::Type type() const override;
        Expression::Type getTypes() const override;
        void getVariables(VariableSet &vars) const override;
        void getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state, const Assignment &vars) const override;
        void getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state, const Assignment &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;
        void renameVariable(const std::string &old, const std::string &name) override;

        std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                             unsigned int level = 0) const override;

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
        Expression::Type getTypes() const override;

        void getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state, const Assignment &vars) const override;
        void getVariables(VariableSet &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;
        void renameVariable(const std::string &old, const std::string &name) override;

        std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                             unsigned int level = 0) const override;

    private:
        const ExpressionPtr left_, right_;
    };

    ImplyPtr makeImply(const ExpressionPtr &left, const ExpressionPtr &right);

    ///
    /// When
    ///

    PDDLBOAT_CLASS_FORWARD(When)
    class When : public Expression
    {
    public:
        When(const ExpressionPtr &left, const ExpressionPtr &right);

        const ExpressionPtr getLeft() const;
        const ExpressionPtr getRight() const;

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        void assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const override;
        Expression::Type type() const override;
        Expression::Type getTypes() const override;

        void getVariables(VariableSet &vars) const override;
        void getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state, const Assignment &vars) const override;
        void getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state, const Assignment &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;
        void renameVariable(const std::string &old, const std::string &name) override;

        std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                             unsigned int level = 0) const override;

    private:
        const ExpressionPtr left_, right_;
    };

    WhenPtr makeWhen(const ExpressionPtr &left, const ExpressionPtr &right);

    ///
    /// QuantifierExpression
    ///

    PDDLBOAT_CLASS_FORWARD(QuantifierExpression)
    class QuantifierExpression : public Expression
    {
    public:
        QuantifierExpression(const VariableSet &domain, const ExpressionPtr &formula);

        const VariableList &getDomain() const;
        const ExpressionPtr &getFormula() const;

        Expression::Type type() const override;
        Expression::Type getTypes() const override;
        void getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state, const Assignment &vars) const override;
        void getVariables(VariableSet &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;
        void renameVariable(const std::string &old, const std::string &name) override;

    protected:
        VariableList domain_;
        VariableList internal_;
        ExpressionPtr formula_;
    };

    ///
    /// Exists
    ///

    PDDLBOAT_CLASS_FORWARD(Exists)
    class Exists : public QuantifierExpression
    {
    public:
        Exists(const VariableSet &domain, const ExpressionPtr &formula);

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        // void assign(StatePtr &out, const Assignment &vars) const override;
        Expression::Type type() const override;
        std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                             unsigned int level = 0) const override;
    };

    ExistsPtr makeExists(const VariableSet &domain, const ExpressionPtr &formula);

    ///
    /// Forall
    ///

    PDDLBOAT_CLASS_FORWARD(Forall)
    class Forall : public QuantifierExpression
    {
    public:
        Forall(const VariableSet &domain, const ExpressionPtr &formula);

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        void assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const override;
        void getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state, const Assignment &vars) const override;
        Expression::Type type() const override;
        std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                             unsigned int level = 0) const override;
    };

    ForallPtr makeForall(const VariableSet &domain, const ExpressionPtr &formula);

    ///
    /// Equals
    ///

    PDDLBOAT_CLASS_FORWARD(Equals)
    class Equals : public Expression
    {
    public:
        Equals(const Value &term1, const Value &term2);

        std::pair<Value, Value> getTerms() const;

        std::string getObject(const Value &term, const Assignment &vars) const;
        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        Expression::Type type() const override;
        Expression::Type getTypes() const override;
        void getVariables(VariableSet &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;
        void renameVariable(const std::string &old, const std::string &name) override;
        std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                             unsigned int level = 0) const override;

    private:
        Value term1_;
        Value term2_;
    };

    EqualsPtr makeEquals(const Value &term1, const Value &term2);

}  // namespace pddlboat

#endif
