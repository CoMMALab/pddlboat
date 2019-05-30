#ifndef PDDLBOAT_EXPRESSIONS_
#define PDDLBOAT_EXPRESSIONS_

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <type_traits>

#include <pddlboat/utility.h>

#define PDDLBOAT_OBJECT_CLASS "object"

#define PDDLBOAT_CLASS_FORWARD(C)                                                                            \
    class C;                                                                                                 \
    typedef std::shared_ptr<C> C##Ptr;                                                                       \
    typedef std::shared_ptr<const C> C##ConstPtr;

namespace pddlboat
{
    PDDLBOAT_CLASS_FORWARD(Problem)
    PDDLBOAT_CLASS_FORWARD(State)

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
        Object(const std::string &name, const std::string &type);

        std::string name;
        std::string type{PDDLBOAT_OBJECT_CLASS};
    };

    using Objects = std::set<ObjectPtr>;

    ObjectPtr makeObj(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

    ///
    /// Variables
    ///

    PDDLBOAT_CLASS_FORWARD(Variable)
    class Variable
    {
    public:
        Variable(const std::string &name, const std::string &type);

        std::string name;
        std::string type{PDDLBOAT_OBJECT_CLASS};
    };

    class Variables
    {
    public:
        std::map<std::string, VariablePtr> variables;

        Variables();
        Variables(const std::initializer_list<VariablePtr> &variables);

        void merge(const Variables &other);
        void addVariable(const VariablePtr &var);
        std::set<std::string> getNames() const;
        const VariablePtr getVariable(const std::string &name) const;

        bool operator==(const Variables &other) const;
        bool operator!=(const Variables &other) const;
    };

    VariablePtr makeVar(const std::string &variable, const std::string &type = PDDLBOAT_OBJECT_CLASS);

    ///
    /// Assignment
    ///

    class Assignment
    {
    public:
        std::set<std::string> variables{};
        std::set<std::string> objects{};

        std::map<std::string, std::string> varToObj{};

        Assignment();
        Assignment(const Assignment &other);
        Assignment(const std::map<std::string, std::string> &assgn);

        unsigned int arity() const;
        void assign(const std::string &var, const std::string &obj);
        const std::string &assigned(const std::string &var) const;
        std::set<std::string> getVariables() const;

        bool operator<(const Assignment &rhs) const;
    };

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
            ATOM,
            AND,
            OR,
            NOT,
            EQUALS,
            IMPLY
        };

        virtual bool evaluate(const StatePtr &state, const Assignment &vars) const = 0;
        virtual Type type() const;
        virtual void getVariables(Variables &vars) const;
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
        void getVariables(Variables &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsProposition(const std::string &proposition) const override;

        bool validAssignment(const Problem *problem, const Assignment &assgn) const;
    };

    PredicatePtr makePred(const std::string &name, const Variables &variables = {});

    ///
    /// Domain
    ///

    PDDLBOAT_CLASS_FORWARD(Domain)
    class Domain
    {
    public:
        const std::string name;
        std::map<std::string, std::string> types;  // type -> parent
        std::vector<ObjectPtr> constants;
        std::map<std::string, PredicatePtr> predicates;

        Domain(const std::string &name);

        void addType(const std::string &type, const std::string &parent = PDDLBOAT_OBJECT_CLASS);

        const ObjectPtr getConstant(const std::string &name) const;
        void addConstant(const ObjectPtr &object);
        void addConstant(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

        std::vector<std::string> getTypes() const;
        bool isType(const std::string &type) const;
        bool isTypeOf(const std::string &type, const std::string &other) const;

        void addPredicate(const PredicatePtr &predicate);
        const PredicatePtr getPredicate(const std::string &name) const;
    };

    ///
    /// Problem
    ///

    class Problem
    {
    public:
        DomainPtr domain;
        std::vector<ObjectPtr> objects;
        StatePtr start;
        ExpressionPtr goal;

        Problem(const DomainPtr &domain);

        const ObjectPtr getObject(const std::string &name) const;
        void addObject(const ObjectPtr &object);
        void addObject(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

        const std::vector<std::string> objectsOfType(const std::string &type) const;

        StatePtr makeState() const;
    };

    ///
    /// State
    ///

    class State
    {
    public:
        struct Key
        {
            Key(const std::string &predicate, const Assignment &assignment);

            std::string predicate;
            Assignment assignment;

            bool operator<(const Key &rhs) const;
        };

        State(const Problem *problem);

        bool isTrue(const std::string &predicate, const Assignment &assignment) const;

        // Assign 0-ary pred.
        void assign0(const std::string &predicate, bool value);

        // Assign 1-ary pred.
        void assign1(const std::string &predicate, const std::string &object, bool value);

        // Assign n-ary pred.
        void assignN(const std::string &predicate, const Assignment &assignment, bool value);

    private:
        std::map<Key, bool> assignments{};
        const Problem *problem_;
    };

    ///
    /// Action
    ///

    PDDLBOAT_CLASS_FORWARD(Action)
    class Action
    {
    public:
        const std::string name;
        const Variables parameters;
        const ExpressionPtr precondition;
        const ExpressionPtr effect;

        Action(const std::string &name, const Variables &parameters, const ExpressionPtr &precondition,
               const ExpressionPtr &effect);

        bool preconditionsHold(const StatePtr &state, const Assignment &vars) const;
        void doAction(const StatePtr &in, const StatePtr &out, const Assignment &vars) const;

        std::string atTime(Time step) const;
    };

    ActionPtr makeAction(const std::string &name, const Variables &parameters,
                         const ExpressionPtr &precondition, const ExpressionPtr &effect);

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
        void getVariables(Variables &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsProposition(const std::string &proposition) const override;

    protected:
        std::vector<ExpressionPtr> terms_;
    };

    template <typename T>
    ExpressionPtr makeCAR(const std::shared_ptr<T> &expr)
    {
        _assert(std::is_base_of<VariadicExpression, T>::value, "T must be a descendant of "
                                                               "VariadicExpression");

        return expr->getTerms().front();
    }

    template <typename T>
    std::shared_ptr<T> makeCDR(const std::shared_ptr<T> &expr)
    {
        _assert(std::is_base_of<VariadicExpression, T>::value, "T must be a descendant of "
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
    };

    OrPtr makeOr(const std::initializer_list<const ExpressionPtr> &terms);

    ///
    /// Equals
    ///

    PDDLBOAT_CLASS_FORWARD(Equals)
    class Equals : public VariadicExpression
    {
    public:
        Equals(const std::initializer_list<const ExpressionPtr> &terms = {});

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        Expression::Type type() const override;
    };

    EqualsPtr makeEquals(const std::initializer_list<const ExpressionPtr> &terms);

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
        void getVariables(Variables &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsProposition(const std::string &proposition) const override;

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
        void getVariables(Variables &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsProposition(const std::string &proposition) const override;

    private:
        const ExpressionPtr left_, right_;
    };

    ImplyPtr makeImply(const ExpressionPtr &left, const ExpressionPtr &right);

}  // namespace pddlboat

#endif
