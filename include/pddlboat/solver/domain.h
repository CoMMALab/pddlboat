#ifndef PDDLBOAT_DOMAIN_
#define PDDLBOAT_DOMAIN_

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <type_traits>

#include <pddlboat/utility.h>
#include <pddlboat/solver/expressions.h>

#define PDDLBOAT_OBJECT_CLASS "object"

namespace pddlboat
{
    PDDLBOAT_CLASS_FORWARD(Problem)
    PDDLBOAT_CLASS_FORWARD(State)

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

    ObjectPtr makeObj(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

    ///
    /// Variables
    ///

    PDDLBOAT_CLASS_FORWARD(Variable)
    class Variable
    {
    public:
        Variable(const std::pair<std::string, std::string> &both);
        Variable(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

        std::string name;
        std::string type{PDDLBOAT_OBJECT_CLASS};

        bool operator<(const Variable &other) const;
        bool operator==(const Variable &other) const;
        bool operator!=(const Variable &other) const;
    };

    using VariableList = std::vector<Variable>;

    class VariableSet
    {
    public:
        std::set<Variable> variables;

        VariableSet();
        VariableSet(const VariableList &variables);

        void merge(const VariableSet &other);
        void addVariable(const Variable &var);
        std::set<std::string> getNames() const;
        const Variable &getVariable(const std::string &name) const;

        bool operator==(const VariableSet &other) const;
        bool operator!=(const VariableSet &other) const;
    };

    ///
    /// Assignment
    ///

    class Assignment
    {
    public:
        std::map<std::string, ObjectPtr> varToObj{};

        Assignment(const Problem *problem);

        unsigned int arity() const;

        void assign(const std::string &var, const std::string &obj);
        const ObjectPtr &assigned(const std::string &var) const;

        std::set<std::string> getVariables() const;

    private:
        const Problem *problem;
    };

    ///
    /// Predicates
    ///

    PDDLBOAT_CLASS_FORWARD(Predicate)
    PDDLBOAT_CLASS_FORWARD(BoundPredicate)
    class Predicate
    {
    public:
        const std::string name;
        const VariableList variables;

        Predicate(const std::string &name, const VariableList &variables);

        BoundPredicatePtr bind() const;
        BoundPredicatePtr bind(const std::string &bound) const;
        BoundPredicatePtr bind(const std::string &boundA, const std::string &boundB) const;
        BoundPredicatePtr bind(const std::vector<std::string> &bound) const;

        unsigned int arity() const;
    };

    PredicatePtr makePred(const std::string &name, const VariableList &variables = {});

    class BoundPredicate : public Expression
    {
    public:
        const VariableList variables;

        BoundPredicate(const Predicate *predicate, const VariableList &variables);

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;

        Expression::Type type() const override;
        void getVariables(VariableSet &vars) const override;
        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;

    private:
        const Predicate *predicate;
    };

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
        using Key = std::pair<std::string, std::vector<std::string>>;

        State(const Problem *problem);

        bool isTrue(const std::string &predicate, const std::vector<std::string> &objects) const;

        // Assign 0-ary pred.
        void assign0(const std::string &predicate, bool value);

        // Assign 1-ary pred.
        void assign1(const std::string &predicate, const std::string &object, bool value);

        // Assign n-ary pred.
        void assignN(const std::string &predicate, const std::vector<std::string> &objects, bool value);

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
        const VariableSet parameters;
        const ExpressionPtr precondition;
        const ExpressionPtr effect;

        Action(const std::string &name, const VariableSet &parameters, const ExpressionPtr &precondition,
               const ExpressionPtr &effect);
    };

    ActionPtr makeAction(const std::string &name, const VariableSet &parameters,
                         const ExpressionPtr &precondition, const ExpressionPtr &effect);
}  // namespace pddlboat

#endif
