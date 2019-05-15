#ifndef PDDLBOAT_EXPRESSIONS_
#define PDDLBOAT_EXPRESSIONS_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

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
    };

    class State
    {
    public:
        State() : assignments()
        {
        }

        virtual bool isTrue(const Predicate &predicate) const = 0;
        virtual std::string evalFun(const Function &function) const = 0;

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
        virtual ExpressionType getExprType() const
        {
            return ExpressionType::PREDICATE;
        }
    };

    class Function
    {
    public:
        const std::string name;
        const std::vector<std::string> variables;
        const std::vector<std::string> types;

        const std::string evaluate(const StatePtr &state) const
        {
            return state->evalFun(*this);
        }

        virtual ExpressionType getExprType() const
        {
            return ExpressionType::FUNCTION;
        }
    };

    class Action
    {
    public:
        Action(std::string name, ExpressionPtr preconditions, ExpressionPtr effects)
          : name(name), preconditions(preconditions), effects(effects)
        {
        }

        const std::string name;
        ExpressionPtr preconditions;
        ExpressionPtr effects;
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
            for (auto &term : terms)
                addTerm(term);
        }

        void addTerm(const ExpressionPtr &expression)
        {
            terms_.emplace_back(expression);
        }
        virtual ExpressionType getExprType() const
        {
            return ExpressionType::VARIADIC;
        }

        std::vector<ExpressionPtr> getTerms()
        {
            return terms_;
        }

    protected:
        std::vector<ExpressionPtr> terms_;
    };

    ///
    /// Atom
    ///

    PDDLBOAT_CLASS_FORWARD(Atom)
    class Atom : public Expression
    {
    public:
        Atom(const std::string &name) : name(name)
        {
        }
        const std::string name;
        bool evaluate(const StatePtr &state) const override
        {
            return state->assignments[name];
        }

        virtual ExpressionType getExprType() const
        {
            return ExpressionType::ATOM;
        }

        inline static const ExpressionType expr_type = ATOM;
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
        And(const std::initializer_list<const ExpressionPtr> &terms) : VariadicExpression(terms)
        {
        }

        bool evaluate(const StatePtr &state) const override
        {
            bool ret = true;
            for (unsigned int i = 0; i < terms_.size() && ret; ++i)
                ret &= terms_[i]->evaluate(state);

            return ret;
        }

        virtual ExpressionType getExprType() const
        {
            return ExpressionType::AND;
        }

        inline static const ExpressionType expr_type = AND;
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
        Or(const std::initializer_list<const ExpressionPtr> &terms) : VariadicExpression(terms)
        {
        }

        bool evaluate(const StatePtr &state) const override
        {
            bool ret = false;
            for (unsigned int i = 0; i < terms_.size() && !ret; ++i)
                ret |= terms_[i]->evaluate(state);

            return ret;
        }

        virtual ExpressionType getExprType() const
        {
            return ExpressionType::OR;
        }

        inline static const ExpressionType expr_type = OR;
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
        virtual ExpressionType getExprType() const
        {
            return ExpressionType::NOT;
        }

        const ExpressionPtr getTerm()
        {
            return term_;
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
        Equals(const std::initializer_list<const ExpressionPtr> terms) : VariadicExpression(terms)
        {
        }

        bool evaluate(const StatePtr &state) const override
        {
            bool allTrue = true;
            bool allFalse = true;

            for (unsigned int i = 0; i < terms_.size() && (allTrue || allFalse); ++i)
            {
                const bool eval = terms_[i]->evaluate(state);
                allTrue &= eval;
                allFalse &= not eval;
            }

            return allTrue || allFalse;
        }

        virtual ExpressionType getExprType() const
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

        virtual ExpressionType getExprType() const
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
