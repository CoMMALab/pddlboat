#ifndef PDDLBOAT_EXPRESSIONS_
#define PDDLBOAT_EXPRESSIONS_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <type_traits>

#include "utility.h"

#define PDDLBOAT_CLASS_FORWARD(C)                                                                            \
    class C;                                                                                                 \
    typedef std::shared_ptr<C> C##Ptr;                                                                       \
    typedef std::shared_ptr<const C> C##ConstPtr;

namespace pddlboat
{
    PDDLBOAT_CLASS_FORWARD(Expression)
    PDDLBOAT_CLASS_FORWARD(Predicate)
    PDDLBOAT_CLASS_FORWARD(Function)
    PDDLBOAT_CLASS_FORWARD(State)

    using Time = std::size_t;

    std::string propAtTime(const std::string &name, const Time step)
    {
        return log::format("t:%1$03u,p:%2%", step, name);
    }

    std::string actionAtTime(const std::string &name, const Time step)
    {
        return log::format("t:%1$03u,a:%2%", step, name);
    }

    enum ExpressionType
    {
        EXPRESSION,
        PREDICATE,
        FUNCTION,
        VARIADIC,
        ATOM,
        AND,
        OR,
        NOT,
        EQUALS,
        IMPLY
    };

    class Expression
    {
    public:
        virtual bool evaluate(const StatePtr &state) const = 0;
        virtual ExpressionType getExprType() const = 0;

        virtual bool containsProposition(const std::string &prop) const
        {
            std::cerr << log::format("Failed to find proposition \"%1%\" in expression.", prop) << std::endl;
            return false;
        }
    };

    class State
    {
    public:
        State() : assignments()
        {
        }

        virtual bool isTrue(const Predicate &predicate) const = 0;
        virtual ExpressionPtr function(const Function &function) const = 0;

        std::unordered_map<std::string, bool> assignments;
    };

    class Predicate : public Expression
    {
    public:
        const std::string name;
        const std::vector<std::string> variables;
        const std::vector<std::string> types;

        bool evaluate(const StatePtr &state) const
        {
            return state->isTrue(*this);
        }

        ExpressionType getExprType() const override
        {
            return ExpressionType::PREDICATE;
        }
    };

    class Function : public Expression
    {
    public:
        const std::string name;
        const std::vector<std::string> variables;
        const std::vector<std::string> types;

        // bool evaluate(const StatePtr &state) const override
        // {
        //     return state->function(*this)->evaluate(state);
        // }

        ExpressionType getExprType() const override
        {
            return ExpressionType::FUNCTION;
        }
    };

    class Action
    {
    public:
        const std::string name;
        ExpressionPtr preconditions;
        ExpressionPtr effects;

        Action(std::string name, ExpressionPtr preconditions, ExpressionPtr effects)
          : name(name), preconditions(preconditions), effects(effects)
        {
        }

        std::string atTime(Time step) const
        {
            return actionAtTime(name, step);
        }
    };

    PDDLBOAT_CLASS_FORWARD(VariadicExpression)
    class VariadicExpression : public Expression
    {
    public:
        VariadicExpression()
        {
        }

        VariadicExpression(const std::initializer_list<const ExpressionPtr> &terms)
        {
            for (const auto &term : terms)
                addTerm(term);
        }

        void addTerm(const ExpressionPtr &expression)
        {
            terms_.emplace_back(expression);
        }

        ExpressionType getExprType() const override
        {
            return ExpressionType::VARIADIC;
        }

        std::vector<ExpressionPtr> getTerms()
        {
            return terms_;
        }

        bool containsProposition(const std::string &prop) const override
        {
            for (const auto &e : terms_)
                if (e->containsProposition(prop))
                    return true;

            return false;
        }

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
    /// Atom
    ///

    PDDLBOAT_CLASS_FORWARD(Atom)
    class Atom : public Expression
    {
    public:
        const std::string name;

        Atom(const std::string &name) : name(name)
        {
        }

        bool evaluate(const StatePtr &state) const override
        {
            return state->assignments[name];
        }

        ExpressionType getExprType() const override
        {
            return ExpressionType::ATOM;
        }

        std::string atTime(Time step) const
        {
            return propAtTime(name, step);
        }

        bool containsProposition(const std::string &prop) const override
        {
            return prop == name;
        }
    };

    static AtomPtr makeAtom(const std::string &name)
    {
        return std::make_shared<Atom>(name);
    }

    ///
    /// And
    ///

    PDDLBOAT_CLASS_FORWARD(And)
    class And : public VariadicExpression
    {
    public:
        And(const std::initializer_list<const ExpressionPtr> &terms = {}) : VariadicExpression(terms)
        {
        }

        bool evaluate(const StatePtr &state) const override
        {
            bool ret = true;
            for (unsigned int i = 0; i < terms_.size() and ret; ++i)
                ret &= terms_[i]->evaluate(state);

            return ret;
        }

        ExpressionType getExprType() const override
        {
            return ExpressionType::AND;
        }
    };

    static AndPtr makeAnd(const std::initializer_list<const ExpressionPtr> &terms)
    {
        return std::make_shared<And>(std::forward<const std::initializer_list<const ExpressionPtr>>(terms));
    }

    ///
    /// Or
    ///

    PDDLBOAT_CLASS_FORWARD(Or)
    class Or : public VariadicExpression
    {
    public:
        Or(const std::initializer_list<const ExpressionPtr> &terms = {}) : VariadicExpression(terms)
        {
        }

        bool evaluate(const StatePtr &state) const override
        {
            bool ret = false;
            for (unsigned int i = 0; i < terms_.size() and not ret; ++i)
                ret |= terms_[i]->evaluate(state);

            return ret;
        }

        ExpressionType getExprType() const override
        {
            return ExpressionType::OR;
        }
    };

    static OrPtr makeOr(const std::initializer_list<const ExpressionPtr> &terms)
    {
        return std::make_shared<Or>(std::forward<const std::initializer_list<const ExpressionPtr>>(terms));
    }

    ///
    /// Not
    ///

    PDDLBOAT_CLASS_FORWARD(Not)
    class Not : public Expression
    {
    public:
        Not(const ExpressionPtr &term) : term_(term)
        {
        }

        bool evaluate(const StatePtr &state) const override
        {
            return not term_->evaluate(state);
        }

        ExpressionType getExprType() const override
        {
            return ExpressionType::NOT;
        }

        const ExpressionPtr getTerm()
        {
            return term_;
        }

        bool containsProposition(const std::string &prop) const override
        {
            return term_->containsProposition(prop);
        }

    private:
        const ExpressionPtr term_;
    };

    static NotPtr makeNot(const ExpressionPtr &term)
    {
        return std::make_shared<Not>(std::forward<const ExpressionPtr>(term));
    }

    ///
    /// Equals
    ///

    PDDLBOAT_CLASS_FORWARD(Equals)
    class Equals : public VariadicExpression
    {
    public:
        Equals(const std::initializer_list<const ExpressionPtr> terms = {}) : VariadicExpression(terms)
        {
        }

        bool evaluate(const StatePtr &state) const override
        {
            bool allTrue = true;
            bool allFalse = true;

            for (unsigned int i = 0; i < terms_.size() and (allTrue or allFalse); ++i)
            {
                const bool eval = terms_[i]->evaluate(state);
                allTrue &= eval;
                allFalse &= not eval;
            }

            return allTrue or allFalse;
        }

        ExpressionType getExprType() const override
        {
            return ExpressionType::EQUALS;
        }
    };

    static EqualsPtr makeEquals(const std::initializer_list<const ExpressionPtr> &terms)
    {
        return std::make_shared<Equals>(
            std::forward<const std::initializer_list<const ExpressionPtr>>(terms));
    }

    ///
    /// Imply
    ///

    PDDLBOAT_CLASS_FORWARD(Imply)
    class Imply : public Expression
    {
    public:
        Imply(const ExpressionPtr &left, const ExpressionPtr &right) : left_(left), right_(right)
        {
        }

        bool evaluate(const StatePtr &state) const override
        {
            return (not left_->evaluate(state)) or right_->evaluate(state);
        }

        ExpressionType getExprType() const override
        {
            return ExpressionType::IMPLY;
        }

        const ExpressionPtr getLeft()
        {
            return left_;
        }

        const ExpressionPtr getRight()
        {
            return right_;
        }

        bool containsProposition(const std::string &prop) const override
        {
            return left_->containsProposition(prop) or right_->containsProposition(prop);
        }

    private:
        const ExpressionPtr left_, right_;
    };

    static ImplyPtr makeImply(const ExpressionPtr &left, const ExpressionPtr &right)
    {
        return std::make_shared<Imply>(std::forward<const ExpressionPtr>(left),
                                       std::forward<const ExpressionPtr>(right));
    }

    // class Forall : public Expression
    // {
    // public:
    //     bool evaluate(const StatePtr &state) const override
    //     {
    //     }
    // };

    // class Exists : public Expression
    // {
    // public:
    //     bool evaluate(const StatePtr &state) const override
    //     {
    //     }
    // };

    // class Is : public Expmession
    // {
    // public:
    //     bool evaluate(const StatePtr &state) const override
    //     {
    //     }
    // };

}  // namespace pddlboat

#endif
