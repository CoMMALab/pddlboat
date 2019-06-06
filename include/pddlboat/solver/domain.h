#ifndef PDDLBOAT_DOMAIN_
#define PDDLBOAT_DOMAIN_

#include <iostream>
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
    /** \cond IGNORE */
    PDDLBOAT_CLASS_FORWARD(Object)
    PDDLBOAT_CLASS_FORWARD(PredicateDefinition)
    PDDLBOAT_CLASS_FORWARD(Predicate)
    PDDLBOAT_CLASS_FORWARD(Domain)
    PDDLBOAT_CLASS_FORWARD(Problem)
    PDDLBOAT_CLASS_FORWARD(State)
    PDDLBOAT_CLASS_FORWARD(Action)
    /** \endcond */

    /** \class pddlboat::ObjectPtr
        \brief A shared pointer wrapper for pddlboat::Object. */

    /** \class pddlboat::ObjectConstPtr
        \brief A const shared pointer wrapper for pddlboat::Object. */

    /** \class pddlboat::PredicateDefinitionPtr
        \brief A shared pointer wrapper for pddlboat::Predicate. */

    /** \class pddlboat::PredicateDefinitionConstPtr
        \brief A const shared pointer wrapper for pddlboat::Predicate. */

    /** \class pddlboat::PredicatePtr
        \brief A shared pointer wrapper for pddlboat::Predicate. */

    /** \class pddlboat::PredicateConstPtr
        \brief A const shared pointer wrapper for pddlboat::Predicate. */

    /** \class pddlboat::DomainPtr
        \brief A shared pointer wrapper for pddlboat::Domain. */

    /** \class pddlboat::DomainConstPtr
        \brief A const shared pointer wrapper for pddlboat::Domain. */

    /** \class pddlboat::ProblemPtr
        \brief A shared pointer wrapper for pddlboat::Problem. */

    /** \class pddlboat::ProblemConstPtr
        \brief A const shared pointer wrapper for pddlboat::Problem. */

    /** \class pddlboat::StatePtr
        \brief A shared pointer wrapper for pddlboat::State. */

    /** \class pddlboat::StateConstPtr
        \brief A const shared pointer wrapper for pddlboat::State. */

    /** \class pddlboat::ActionPtr
        \brief A shared pointer wrapper for pddlboat::Action. */

    /** \class pddlboat::ActionConstPtr
        \brief A const shared pointer wrapper for pddlboat::Action. */

    ///
    /// Objects
    ///

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

        std::ostream &toPDDL(std::ostream &out = std::cout) const;

        bool operator==(const VariableSet &other) const;
        bool operator!=(const VariableSet &other) const;
    };

    ///
    /// Assignment
    ///

    class Assignment
    {
    public:
        const Problem *problem;
        std::map<std::string, ObjectPtr> varToObj{};

        Assignment(const Problem *problem);
        Assignment(const Assignment &other);

        unsigned int arity() const;

        void assign(const std::string &var, const std::string &obj);
        const ObjectPtr &assigned(const std::string &var) const;

        std::set<std::string> getVariables() const;

        std::ostream &toString(std::ostream &out = std::cout) const;
    };

    ///
    /// Predicates
    ///

    class PredicateDefinition
    {
    public:
        const std::string name;
        const VariableList variables;

        PredicateDefinition(const std::string &name, const VariableList &variables);

        PredicatePtr bind() const;
        PredicatePtr bind(const std::vector<std::string> &bound, const ProblemPtr &problem = nullptr) const;

        bool isValidAssignment(const Problem *problem, const std::vector<std::string> &objects) const;

        unsigned int arity() const;

        std::ostream &toPDDL(std::ostream &out = std::cout) const;
        std::string toID(const Assignment &vars) const;
    };

    PredicateDefinitionPtr makePred(const std::string &name, const VariableList &variables = {});

    class Predicate : public Expression
    {
    public:
        struct Value
        {
            Value(bool isVar, const std::string &name, const std::string &type);
            bool isVar;
            std::string name;
            std::string type;
        };

        const std::vector<Value> bindings;

        Predicate(const PredicateDefinition *predicate, const std::vector<Value> &bindings);

        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        void assign(StatePtr &out, const Assignment &vars) const override;

        Expression::Type type() const override;
        void getVariables(VariableSet &vars) const override;
        VariableList arguments() const;

        bool containsVariable(const std::string &variable) const override;
        bool containsPredicate(const std::string &predicate) const override;

        std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                             unsigned int level = 0) const override;

        std::string toID(const Assignment &vars) const;
        const std::string &name() const;

    private:
        std::vector<std::string> getObjects(const Assignment &vars) const;
        const PredicateDefinition *predicate;
    };

    ///
    /// Domain
    ///

    class Domain
    {
    public:
        const std::string name;
        std::map<std::string, std::string> types;  // type -> parent
        std::vector<ObjectPtr> constants;
        std::map<std::string, PredicateDefinitionPtr> predicates;
        std::map<std::string, ActionPtr> actions;

        Domain(const std::string &name);

        void addType(const std::string &type, const std::string &parent = PDDLBOAT_OBJECT_CLASS);

        const ObjectPtr getConstant(const std::string &name) const;
        void addConstant(const ObjectPtr &object);
        void addConstant(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

        std::vector<std::string> getTypes() const;
        bool isType(const std::string &type) const;
        bool isTypeOf(const std::string &type, const std::string &other) const;

        void addPredicate(const PredicateDefinitionPtr &predicate);
        const PredicateDefinitionPtr getPredicate(const std::string &name) const;

        void addAction(const ActionPtr &action);
        const ActionPtr getAction(const std::string &name) const;

        std::ostream &toPDDL(std::ostream &out = std::cout) const;
    };

    ///
    /// Problem
    ///

    class Problem
    {
    public:
        const std::string name;
        const DomainPtr domain;
        std::vector<ObjectPtr> objects;
        StatePtr start;
        ExpressionPtr goal;

        Problem(const std::string &name, const DomainPtr &domain);

        const ObjectPtr getObject(const std::string &name) const;
        void addObject(const ObjectPtr &object);
        void addObject(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

        const std::vector<std::string> objectsOfType(const std::string &type) const;

        const std::vector<Assignment> getGroundings(const VariableList &vars, unsigned int index = 0) const;

        StatePtr makeState() const;

        std::ostream &toPDDL(std::ostream &out = std::cout) const;
    };

    ///
    /// State
    ///

    class State
    {
    public:
        using Key = std::pair<std::string, std::vector<std::string>>;

        State(const Problem *problem);

        const Problem *getProblem() const;
        void clear();
        void complement();
        void fold(const StatePtr &other);

        bool isTrue(const PredicateDefinitionPtr &predicate, const Assignment &assignment) const;
        bool isTrue(const std::string &predicate, const Assignment &assignment) const;
        bool isTrue(const std::string &predicate, const std::vector<std::string> &objects) const;

        // Assign 0-ary pred.
        void assign0(const std::string &predicate, bool value = true);

        // Assign 1-ary pred.
        void assign1(const std::string &predicate, const std::string &object, bool value = true);

        // Assign n-ary pred.
        void assignN(const std::string &predicate, const std::vector<std::string> &objects,
                     bool value = true);

        std::ostream &toPDDL(std::ostream &out = std::cout, unsigned int indent = 0) const;

        std::map<Key, bool> assignments{};

    private:
        const Problem *problem_;
    };

    ///
    /// Action
    ///

    class Action
    {
    public:
        const std::string name;
        const VariableList parameters;
        const ExpressionPtr precondition;
        const ExpressionPtr effect;

        Action(const std::string &name, const VariableList &parameters, const ExpressionPtr &precondition,
               const ExpressionPtr &effect);

        bool holds(const StatePtr &state, const Assignment &vars) const;
        bool doAction(StatePtr &out, const StatePtr &in, const Assignment &vars) const;

        std::ostream &toPDDL(std::ostream &out = std::cout, unsigned int indent = 2) const;

        std::string toID(const Assignment &vars) const;

        unsigned int arity() const;
    };

    ActionPtr makeAction(const std::string &name, const VariableList &parameters,
                         const ExpressionPtr &precondition, const ExpressionPtr &effect);
}  // namespace pddlboat

#endif
