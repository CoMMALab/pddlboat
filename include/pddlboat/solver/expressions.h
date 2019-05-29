#ifndef PDDLBOAT_EXPRESSIONS_
#define PDDLBOAT_EXPRESSIONS_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <type_traits>

#include <pddlboat/utility.h>

#define PDDLBOAT_OBJECT_CLASS "object"

#define PDDLBOAT_CLASS_FORWARD(C)                                                                            \
    class C;                                                                                                 \
    typedef std::shared_ptr<C> C##Ptr;                                                                       \
    typedef std::shared_ptr<const C> C##ConstPtr;

namespace pddlboat
{
    using Time = std::size_t;

    std::string propAtTime(const std::string &name, const Time step);
    std::string actionAtTime(const std::string &name, const Time step);

    ///
    /// Objects
    ///

    PDDLBOAT_CLASS_FORWARD(Object)
    class Object
    {
    public:
        std::string name;
        std::string type{PDDLBOAT_OBJECT_CLASS};
    };

    using Objects = std::set<ObjectPtr>;

    static ObjectPtr makeObj(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

    ///
    /// Variables
    ///

    PDDLBOAT_CLASS_FORWARD(Variable)
    class Variable
    {
    public:
        std::string variable;
        std::string type{PDDLBOAT_OBJECT_CLASS};
    };

    class Variables : public std::set<VariablePtr>
    {
    public:
        std::set<std::string> getNames();
        VariablePtr getVariable(const std::string &name);
    };

    static VariablePtr makeVar(const std::string &variable, const std::string &type = PDDLBOAT_OBJECT_CLASS);

    ///
    /// Assignment
    ///

    class Assignment : public std::map<std::string, std::string>
    {
    public:
        std::set<std::string> getVariables();
    }

    ///
    /// Expressions
    ///

    PDDLBOAT_CLASS_FORWARD(Expression) class Expression
    {
    public:
        enum Type
        {
            EXPRESSION,
            PREDICATE,
            VARIADIC,
            ATOM,
            AND,
            OR,
            NOT,
            EQUALS,
            IMPLY
        };

        virtual bool evaluate(const StatePtr &state, const Assignment &vars) const = 0;
        virtual Type type() const;
        virtual void variables(Variables &vars) const;
        virtual bool containsVariable(const std::string &variable) const;
        virtual bool containsProposition(const std::string &proposition) const;
    };

    ///
    /// Predicates
    ///

    PDDLBOAT_CLASS_FORWARD(Predicate)
    class Predicate : public Expression
    {
    public:
        const std::string name;
        const Variables variables;

        Predicate(const std::string &name, const Variables &vars);

        unsigned int arity() const;

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        Expression::Type type() const override;
        void variables(Variables &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsProposition(const std::string &proposition) const override;
    };

    static PredicatePtr makePred(const std::string &name, const Variables &variables = {});

    ///
    /// Domain
    ///

    PDDLBOAT_CLASS_FORWARD(Domain)
    class Domain
    {
    public:
        const std::string name;
        std::map<std::string, std::string> types;  // type -> parent
        std::vector<ObjectPtr> objects;
        std::map<std::string, PredicatePtr> predicates;

        Domain(const std::string &name);

        void addType(const std::string &type, const std::string &parent = PDDLBOAT_OBJECT_CLASS);
        std::vector<std::string> getTypes() const;
        bool isType(const std::string &type) const;
        bool isTypeOf(const std::string &type, const std::string &other) const;
        const std::vector<ObjectPtr> objectsOfType(const std::string &type) const;

        void addPredicate(const PredicatePtr &predicate);
        const PredicatePtr getPredicate(const std::string &name);
    };

    ///
    /// State
    ///

    PDDLBOAT_CLASS_FORWARD(State)
    class State
    {
    public:
        struct Key
        {
            std::string predicate;
            Assignment assignment;
        };

        State(const DomainPtr &domain);

        virtual bool isTrue(const std::string &predicate, const Assignment &assignment) const;

        // Assign 0-ary pred.
        void assign(const std::string &predicate, bool value);

        // Assign 1-ary pred.
        void assign(const std::string &predicate, const std::string &object, bool value);

        // Assign n-ary pred.
        virtual void assign(const std::string &predicate, const Assignment &assignment, bool value);

        std::unordered_map<Key, bool> assignments;

    private:
        const DomainPtr domain_;
    };

    ///
    /// Action
    ///

    PDDLBOAT_CLASS_FORWARD(VariadicExpression)
    class Action
    {
    public:
        const std::string name;
        const ExpressionPtr preconditions;
        const ExpressionPtr effects;

        Action(const std::string &name, const ExpressionPtr &preconditions, const ExpressionPtr &effects);
        std::string atTime(Time step) const;
    };

    static ActionPtr makeAction(const std::string &name, const ExpressionPtr &preconditions,
                                const ExpressionPtr &effects);

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
        void variables(Variables &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsProposition(const std::string &proposition) const override;

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
