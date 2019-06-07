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

    /** \brief An object in the problem. Has a name and type.
     */
    class Object
    {
    public:
        /** \brief Constructor.
         *  \param[in] name Name of object.
         *  \param[in] type Type of object.
         */
        Object(const std::string &name, const std::string &type);

        std::string name;                         ///< Name
        std::string type{PDDLBOAT_OBJECT_CLASS};  ///< Type
    };

    /** \brief Factory function to create a new object.
     *  \param[in] name Name of object.
     *  \param[in] type Type of object.
     */
    ObjectPtr makeObj(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

    ///
    /// Variables
    ///

    /** \brief A variable in a formula. Has a name and type.
     */
    class Variable
    {
    public:
        /** \brief Constructor.
         *  \param[in] both A pair consisting of the name and type of the variable.
         */
        Variable(const std::pair<std::string, std::string> &both);

        /** \brief Constructor.
         *  \param[in] name Name of the variable.
         *  \param[in] type Type of the variable.
         */
        Variable(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

        std::string name;                         ///< Name
        std::string type{PDDLBOAT_OBJECT_CLASS};  ///< Type

        /** \brief Less than operator for sorted storage classes.
         *  \param[in] other Other variable to compare to.
         *  \return Returns true if this is less than other, lexically by name.
         */
        bool operator<(const Variable &other) const;

        /** \brief Equality operator.
         *  \param[in] other Other variable to compare to.
         *  \return True if both name and type are the same, false otherwise.
         */
        bool operator==(const Variable &other) const;

        /** \brief Inquality operator.
         *  \param[in] other Other variable to compare to.
         *  \return True if either name and type are different, false otherwise.
         */
        bool operator!=(const Variable &other) const;
    };

    /** \brief A list of variables.
     *  Generally used to store argument lists to expressions, actions, or predicates.
     */
    using VariableList = std::vector<Variable>;

    /** \brief A set of variables. Does not have order.
     *  Generally used to check if the arguments for expressions match.
     */
    class VariableSet
    {
    public:
        std::set<Variable> variables;  ///< Variable set.

        /** \brief Constructor.
         */
        VariableSet();

        /** \brief Constructor.
         *  \param[in] variables A list of variables.
         */
        VariableSet(const VariableList &variables);

        /** \brief Merges another set of variables with this set.
         *  \param[in] other Other set to merge.
         */
        void merge(const VariableSet &other);

        /** \brief Adds a variable to the variable set.
         *  \param[in] var Variable to add.
         *  \throw std::invalid_argument if you try to add a variable that already exists AND that variable
         * has a different type.
         */
        void addVariable(const Variable &var);

        /** \brief Get the names of all the variables in the variable set.
         *  \return The names of all the variables in the set.
         */
        std::set<std::string> getNames() const;

        /** \brief Search the set of variables for a variable with a specific name.
         *  \param[in] name Name to search for.
         *  \return The found variable.
         *  \throw std::invalid_argument if the variable does not exist.
         */
        const Variable &getVariable(const std::string &name) const;

        /** \brief Serialize this set to PDDL.
         *  \param[in,out] out Output stream to use.
         *  \return The output stream.
         */
        std::ostream &toPDDL(std::ostream &out = std::cout) const;

        /** \brief Equality operator.
         *  \param[in] other Set to compare against.
         *  \return True if the sets are equal.
         */
        bool operator==(const VariableSet &other) const;

        /** \brief Inequality operator.
         *  \param[in] other Set to compare against.
         *  \return True if the sets are unequal.
         */
        bool operator!=(const VariableSet &other) const;
    };

    ///
    /// Assignment
    ///

    /** \brief An assignment of variables to objects.
     *  Used to ground expressions to a particular binding of variables to objects.
     */
    class Assignment
    {
    public:
        const Problem *problem; ///< The problem this assignment is from.
        std::map<std::string, ObjectPtr> varToObj{}; ///< Mapping of variable name to objects.

        /** \brief Constructor. Creates an empty assignment.
         *  \param[in] problem Problem to create assignment for.
         */
        Assignment(const Problem *problem);

        /** \brief Copy constructor. Copies the assignment.
         *  \param[in] other The assignment to copy.
         */
        Assignment(const Assignment &other);

        /** \brief Returns the arity of this assignment, i.e., the number of assignments.
         * \return The number of assignments.
         */
        unsigned int arity() const;

        /** \brief Assign a variable \a var to an object \a obj.
         *  \param[in] var Variable to assign.
         *  \param[in] obj Object to assign to variable.
         *  \throw std::invalid_argument if object \a obj does not exist.
         */
        void assign(const std::string &var, const std::string &obj);

        /** \brief Retrieve the object assigned to a variable.
         *  \param[in] var Variable to retrieve.
         *  \return The object assigned to a variable.
         *  \throw std::invalid_argument if variable \a var is not assigned.
         */
        const ObjectPtr &assigned(const std::string &var) const;

        /** \brief Get the set of all names of variables assigned.
         *  \return The set of all variables assigned.
         */
        std::set<std::string> getVariables() const;

        /** \brief Serialize this assignment.
         *  \param[in,out] out Output stream to use.
         *  \return The output stream.
         */
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
