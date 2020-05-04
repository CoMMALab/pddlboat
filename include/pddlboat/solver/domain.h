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
#include <pddlboat/solver/expression.h>

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

        /** \brief Conversion to a list.
         */
        operator VariableList() const;

        /** \brief Merges another set of variables with this set.
         *  \param[in] other Other set to merge.
         */
        void merge(const VariableSet &other);

        /** \brief Removes the contents of \a other from this variable set.
         *  \param[in] other Variable set to remove.
         */
        void remove(const VariableSet &other);

        /** \brief Removes the contents of \a other from this variable set.
         *  \param[in] other Variable list to remove.
         */
        void remove(const VariableList &other);

        /** \brief Adds a variable to the variable set.
         *  \param[in] var Variable to add.
         *  \throw std::invalid_argument if you try to add a variable that already exists AND that variable
         * has a different type.
         */
        void addVariable(const Variable &var);

        /** \brief Removes a variable from this variable set.
         *  \param[in] var Variable to remove.
         *  \return True if was actually removed, false if it did not exist.
         */
        bool removeVariable(const Variable &var);

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

        /** \brief Checks containment of another set.
         *  \param[in] other Set to compare against.
         *  \return True if other is contained in this set.
         */
        bool contains(const VariableSet &other) const;
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
        const Problem *problem;                       ///< The problem this assignment is from.
        std::map<std::string, ObjectPtr> varToObj{};  ///< Mapping of variable name to objects.

        /** \brief Constructor. Creates an empty assignment.
         *  \param[in] problem Problem to create assignment for.
         */
        Assignment(const Problem *problem);

        /** \brief Constructor. Creates an empty assignment.
         *  \param[in] problem Problem to create assignment for.
         */
        Assignment(const ProblemPtr &problem);

        /** \brief Copy constructor. Copies the assignment.
         *  \param[in] other The assignment to copy.
         */
        Assignment(const Assignment &other);

        /** \brief Take in and add another assignments contents to this assignment.
         *  \param[in] other The assignment to add.
         */
        void fold(const Assignment &other);

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

        /** \brief Unassign a variable in this assignment.
         *  \param[in] var Variable to unassign.
         */
        void unassign(const std::string &var);

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

    /** \brief A definition of a predicate in a domain. Used to check types and validity of assignments.
     * NOT an evaluatable expression. You must first bind a PredicateDefinition to be a Predicate. The binding
     * maps specific variable names that exist in an Expression to the arguments of a PredicateDefinition.
     */
    class PredicateDefinition
    {
    public:
        const std::string name;        ///< Name of predicate.
        const VariableList variables;  ///< Arguments to predicate.

        /** \brief Constructor.
         *  \param[in] name Name of predicate.
         *  \param[in] variables Argument list of predicate.
         */
        PredicateDefinition(const std::string &name, const VariableList &variables);

        /** \brief Bind this 0-ary predicate to an evaluatable Predicate.
         *  \return A bound version of a 0-ary predicate.
         *  \throw std::invalid_argument if this is not a 0-ary predicate.
         */
        PredicatePtr bind() const;

        /** \brief Bind this n-ary predicate to an evaluatable Predicate.
         *  Can be bound to either grounded variables or a named variable that exists in some expression.
         *  The bound list consists of variables, e.g. ?x, ?y --- and objects, e.g, a, b.
         *  If a `?` is in the first position, the label is bound as a variable.
         *  Otherwise, it is bound as an object, and will throw an error if not a valid object to bind.
         *  \param[in] bound List of bindings of variables and objects.
         *  \param[in] problem Optional, but if any objects exist in the bindings the problem is required.
         *  \param[in] domain Optional, but if any constants exist without a problem the domain is required.
         *                    Not needed if a problem is provided.
         *  \return A bound version of a n-ary predicate.
         *  \throw std::invalid_argument if this is not a valid binding.
         */
        PredicatePtr bind(const std::vector<std::string> &bound, const ProblemPtr &problem = nullptr,
                          const DomainPtr &domain = nullptr) const;

        /** \brief Checks if the set of object names is a valid assignment to this predicate. That is, do
         * their types match the argument types.
         *  \param[in] problem Problem to check with.
         *  \param[in] objects Objects to check against.
         *  \return True if this is a valid binding, false otherwise.
         */
        bool isValidAssignment(const Problem *problem, const std::vector<std::string> &objects) const;

        /** \brief Returns the arity of this predicate, i.e., how many arguments.
         *  \return The arity of the predicate.
         */
        unsigned int arity() const;

        /** \brief Outputs this predicate definition to PDDL.
         *  \param[in,out] out Output stream to use.
         *  \return The output stream.
         */
        std::ostream &toPDDL(std::ostream &out = std::cout) const;

        /** \brief Generates a unique ID for this predicate definition given a grounding.
         *  \param[in] vars A grounding of this predicate.
         *  \return ID of the grounded predicate.
         */
        std::string toID(const Assignment &vars) const;
    };

    /** \brief Helper function to create a predicate definition.
     *  \param[in] name Name of the predicate to create.
     *  \param[in] variables Arguments of the predicate.
     *  \return The created predicate definition.
     */
    PredicateDefinitionPtr makePred(const std::string &name, const VariableList &variables = {});

    /** \brief A predicate whose arguments are bound or grounded to a set of variable names or objects. This
     * predicate can be evaluated inside expressions, and all predicates within an expression should have
     * arguments bound to a shared set of variable names. Use a PredicateDefinition to create a new Predicate
     * without bound/grounded arguments.
     */
    class Predicate : public Expression
    {
    public:
        const std::vector<Value> bindings;  ///< Argument bindings for the predicate.

        /** \brief Constructor.
         *  \param[in] predicate Predicate definition that this predicate is a binding for.
         *  \param[in] bindings Argument bindings.
         */
        Predicate(const PredicateDefinition *predicate, const std::vector<Value> &bindings);

        /** \brief Return the list of unbound arguments to this predicate.
         *  \return The unbound variables for this predicate.
         */
        VariableList arguments() const;

        /** \brief Evaluate this predicate. Checks contents of state to see if the grounding is true.
         *  \param[in] state State of the world.
         *  \param[in] vars Grounding of the predicate.
         *  \return True if this predicate is true, false otherwise.
         */
        bool evaluate(const StatePtr &state, const Assignment &vars) const override;

        /** \brief Assigns this predicate to be true inside of a state given a grounding.
         *  \param[out] out State to set.
         *  \param[in] vars Grounding of this predicate.
         */
        void assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const override;

        /** \brief Returns the type of this expression.
         *  \return Expression type.
         */
        Expression::Type type() const override;

        /** \brief Get the set of all ungrounded variables in this expression.
         *  \param[out] vars Variables used in the expression.
         */
        void getVariables(VariableSet &vars) const override;

        /** \brief Add this predicate to the used predicate set.
         *  \param[out] predicates Set of all predicates.
         *  \param[in] vars Assignment.
         */
        void getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state, const Assignment &vars) const override;

        /** \brief Add this predicate to the assigned predicate set.
         *  \param[out] predicates Set of all predicates.
         *  \param[in] vars Assignment.
         */
        void getAssignedPredicates(std::set<std::string> &predicates, const StatePtr & /*state*/,
                                   const Assignment &vars) const override;

        /** \brief Returns true if this expression contains the variable name.
         *  \param[in] variable The variable name to search for.
         *  \return True if the variable is in the expression.
         */
        bool containsVariable(const std::string &variable) const override;

        /** \brief Returns true if this expression contains the predicate name.
         *  \param[in] predicate The predicate name to search for.
         *  \return True if the predicate is in the expression.
         */
        bool containsPredicate(const std::string &predicate) const override;

        /** \brief Serializes this predicate to PDDL.
         *  \param[in,out] out Output stream to use.
         *  \param[in] indent Amount of indentation per level.
         *  \param[in] level Level of indentation to output to.
         *  \return The output stream.
         */
        std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                             unsigned int level = 0) const override;

        /** \brief Convert this predicate to a grounded ID.
         *  \param[in] vars Grounding to use for ID.
         *  \return The ID of this grounded predicate.
         */
        std::string toID(const Assignment &vars) const;

        /** \brief Return the name of this predicate.
         *  \return The name of the predicate.
         */
        const std::string &name() const;

    private:
        /** \brief For a grounding, create a list of all object names that correspond to the arguments of the
         * predicate.
         *  \param[in] vars Grounding of the predicate.
         *  \return A vector of the grounded objects.
         */
        std::vector<std::string> getObjects(const Assignment &vars) const;

        const PredicateDefinition *predicate;  ///< Definition of this predicate.
    };

    ///
    /// Action
    ///

    /** \brief An action in a domain. Mirrors the concept in PDDL closely.
     */
    class Action
    {
    public:
        const std::string name;            ///< Name of the action.
        const VariableList parameters;     ///< Parameters this action has.
        const ExpressionPtr precondition;  ///< Precondition of this action.
        const ExpressionPtr effect;        ///< Effect of this action.

        /** \brief Constructor.
         *  \param[in] name Name of the action.
         *  \param[in] parameters Parameters of the action. Must match precondition and effect.
         *  \param[in] precondition Expression composed of predicates in domain that must hold before effect.
         *  \param[in] effect Expression composed of predicates in domain that change after acting.
         */
        Action(const std::string &name, const VariableList &parameters, const ExpressionPtr &precondition,
               const ExpressionPtr &effect);

        /** \brief Get the arity of this action, i.e., the number of parameters.
         *  \return The arity of the action.
         */
        unsigned int arity() const;

        /** \brief Checks if an action's preconditions hold at a world state and a grounding of the action.
         *  \param[in] state State of the problem.
         *  \param[in] vars Grounding of the action.
         *  \return True if the preconditions of this grounded action hold.
         */
        bool holds(const StatePtr &state, const Assignment &vars) const;

        /** \brief Checks if an action's effect's are true in a world state and a grounding of the action.
         *  \param[in] state State of the problem.
         *  \param[in] vars Grounding of the action.
         *  \return True if the effects of this grounded action are true.
         */
        bool effected(const StatePtr &state, const Assignment &vars) const;

        /** \brief Apply the effects of this action grounded to an assignment to a state.
         *  \param[out] out State that will have the effects of this action applied to \a in.
         *  \param[in] in Starting state. Preconditions must hold.
         *  \param[in] vars Grounding of this action.
         *  \return True on success, false on failure.
         */
        bool doAction(StatePtr &out, const StatePtr &in, const Assignment &vars) const;

        /** \brief Serializes this action to PDDL.
         *  \param[in,out] out Output stream to use.
         *  \param[in] indent Amount of indentation to use.
         *  \return The output stream.
         */
        std::ostream &toPDDL(std::ostream &out = std::cout, unsigned int indent = 2) const;

        /** \brief Generates a unique ID for this action given a grounding.
         *  \param[in] vars A grounding of this action.
         *  \return ID of the grounded action.
         */
        std::string toID(const Assignment &vars) const;
    };

    /** \brief Helper function to create an action.
     *  \param[in] name Name of the action.
     *  \param[in] parameters Parameters of the action. Must match precondition and effect.
     *  \param[in] precondition Expression composed of predicates in domain that must hold before effect.
     *  \param[in] effect Expression composed of predicates in domain that change after acting.
     *  \return Newly created action.
     */
    ActionPtr makeAction(const std::string &name, const VariableList &parameters,
                         const ExpressionPtr &precondition, const ExpressionPtr &effect);

    /** \brief Helper function to create a no-op action.
     *  \param[in] name Name of the action.
     *  \return Newly created action.
     */
    ActionPtr makeNoOpAction(const std::string &name = "no-op");

    ///
    /// Domain
    ///

    /** \brief A planning domain. Mirrors concepts in PDDL.
     */
    class Domain
    {
    public:
        const std::string name;                                    ///< Name of this domain.
        std::map<std::string, std::string> types;                  ///< Mapping of type to its parent type.
        std::vector<ObjectPtr> constants;                          ///< Constants in this domain.
        std::map<std::string, PredicateDefinitionPtr> predicates;  ///< Predicates defined in this domain.
        std::map<std::string, ActionPtr> actions;                  ///< Actions defined in this domain.

        /** \brief Constructor. Creates an empty domain.
         *  \param[in] name Name of this domain
         */
        Domain(const std::string &name);

        /* \name Types
           \{ */

        /** \brief Define a new type in the domain.
         *  \param[in] type Type name to add.
         *  \param[in] parent Parent of this type in the type hierarchy.
         */
        void addType(const std::string &type, const std::string &parent = PDDLBOAT_OBJECT_CLASS);

        /** \brief Get a list of all types in the domain.
         *  \return A list of all types in the domain.
         */
        std::vector<std::string> getTypes() const;

        /** \brief Check if a string is a type in the domain.
         *  \param[in] type Type to check.
         *  \return True if type, false otherwise.
         */
        bool isType(const std::string &type) const;

        /** \brief Checks if \a type is or is derived from \a other.
         *  \param[in] type Type to check.
         *  \param[in] other Type to check against.
         *  \return True if type is of type other or is derived from other.
         */
        bool isTypeOf(const std::string &type, const std::string &other) const;

        /** \} */

        /* \name Types
           \{ */

        /** \brief Get a constant in this domain.
         *  \param[in] name Name of constant.
         *  \return The constant if it exists, nullptr otherwise.
         */
        const ObjectPtr getConstant(const std::string &name) const;

        /** \brief Add a new constant to the domain.
         *  \param[in] object The constant to add.
         *  \throw std::invalid_argument If there already exists a constant with this name in the domain.
         */
        void addConstant(const ObjectPtr &object);

        /** \brief Add a new constant to the domain.
         *  \param[in] name Name of constant to add
         *  \param[in] type Type of constant to add
         *  \throw std::invalid_argument If there already exists a constant with this name in the domain.
         */
        void addConstant(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

        /** \} */

        /* \name Predicates
           \{ */

        /** \brief Define a new predicate in the domain.
         *  \param[in] predicate Predicate to add.
         */
        void addPredicate(const PredicateDefinitionPtr &predicate);

        /** \brief Retrieve a predicate defined in the domain.
         *  \param[in] name Name of predicate to retrieve.
         *  \return The named predicate definition.
         *  \throw std::invalid_argument if the predicate does not exist.
         */
        const PredicateDefinitionPtr getPredicate(const std::string &name) const;
        /** \} */

        /* \name Actions
           \{ */

        /** \brief Add an action to the domain.
         *  \param[in] action Action to add.
         */
        void addAction(const ActionPtr &action);

        /** \brief Retrieve a action defined in the domain.
         *  \param[in] name Name of action to retrieve.
         *  \return The named action.
         *  \throw std::invalid_argument if the action does not exist.
         */
        const ActionPtr getAction(const std::string &name) const;

        /** \} */

        /** \brief Serializes this domain to PDDL.
         *  \param[in,out] out Output stream to use.
         *  \return The output stream.
         */
        std::ostream &toPDDL(std::ostream &out = std::cout) const;
    };

    ///
    /// Problem
    ///

    /** \brief A planning problem in a domain. Mirrors the same concept in PDDL.
     */
    class Problem
    {
    public:
        const std::string name;          ///< Name of this planning problem.
        const DomainPtr domain;          ///< Domain this problem is foor.
        std::vector<ObjectPtr> objects;  ///< Objects defined in this problem.
        StatePtr start;                  ///< Starting state of the world.
        ExpressionPtr goal;              ///< Goal to reach.

        /** \brief Constructor.
         *  \param[in] name Name of the planning problem.
         *  \param[in] domain The domain of this problem.
         */
        Problem(const std::string &name, const DomainPtr &domain);

        /* \name Objects
           \{ */

        /** \brief Get object with a name. Will also grab domain constants if they exist.
         *  \param[in] name Name of object to find.
         *  \return The object if found, nullptr otherwise.
         */
        const ObjectPtr getObject(const std::string &name) const;

        /** \brief Add a new object to the problem.
         *  \param[in] object The object to add.
         *  \throw std::invalid_argument If there already exists a object with this name in the problem.
         */
        void addObject(const ObjectPtr &object);

        /** \brief Add a new object to the problem.
         *  \param[in] name Name of object to add
         *  \param[in] type Type of object to add
         *  \throw std::invalid_argument If there already exists a object with this name in the problem.
         */
        void addObject(const std::string &name, const std::string &type = PDDLBOAT_OBJECT_CLASS);

        /** \brief Return a list of all objects that are of a desired type, including those that are
         * derived from the type. \param[in] type Type to get objects for. \return A vector of all object
         * names that have the desired type.
         */
        std::vector<std::string> objectsOfType(const std::string &type) const;

        /** \} */

        /** \brief Get all possible groundings of a variable list as assignments.
         *  \param[in] vars The variables to find groundings for.
         *  \param[in] index Recursive index in the list.
         *  \return The groundings of all variables in vars, from index onward.
         */
        std::vector<Assignment> getGroundings(const VariableList &vars, unsigned int index = 0) const;

        /** \brief Allocate a new state for this problem.
         *  \return A new state.
         */
        StatePtr makeState() const;

        /** \brief Serializes this problem to PDDL.
         *  \param[in,out] out Output stream to use.
         *  \return The output stream.
         */
        std::ostream &toPDDL(std::ostream &out = std::cout) const;
    };

    ///
    /// State
    ///

    /** \brief A state of the planning world for a problem. Contains information on whether grounded
     * predicates are true or not.
     */
    class State
    {
    public:
        /** \brief The key into the assignment map. Contains the name of the predicate and its grounding
         * (a vector of objects)
         */
        using Key = std::pair<std::string, std::vector<std::string>>;

        std::map<Key, bool> assignments;  ///< Map from grounded predicate to truth value.

        /** \brief Constructor.
         *  \param[in] problem Problem this state is for.
         */
        State(const Problem *problem);

        /** \brief Clone this state.
         *  \return A clone of this state.
         */
        StatePtr clone() const;

        /* \name Assignment Manipulations
           \{ */

        /** \brief Clear all assignments.
         */
        void clear();

        /** \brief Take the `complement` of this state, i.e., flip truth value of all propositions
         * currently set. This is not a full complement as it only flips values that exist in the
         * assignment map.
         */
        void complement();

        /** \brief `Fold` values from another state into this state. Additional grounded predicate values
         * are only set if they do not already exist inside this state.
         */
        void fold(const StatePtr &other);

        /** \brief `Fold` values from another state into this state. Additional grounded predicate values
         * are only set if they do not already exist inside this state.
         */
        void fold(const State &other);

        /** \brief Assign the truth value of a 0-ary predicate.
         *  \param[in] predicate Name of predicate to assign.
         *  \param[in] value Value to assign.
         *  \throw std::invalid_argument If not a 0-ary predicate.
         */
        void assign0(const std::string &predicate, bool value = true);

        /** \brief Assign the truth value of a 1-ary predicate.
         *  \param[in] predicate Name of predicate to assign.
         *  \param[in] object Object to ground to predicate.
         *  \param[in] value Value to assign.
         *  \throw std::invalid_argument If not a 1-ary predicate or not a valid assignment.
         */
        void assign1(const std::string &predicate, const std::string &object, bool value = true);

        /** \brief Assign the truth value of a n-ary predicate.
         *  \param[in] predicate Name of predicate to assign.
         *  \param[in] objects Objects to ground to predicate.
         *  \param[in] value Value to assign.
         *  \throw std::invalid_argument If not a valid assignment.
         */
        void assignN(const std::string &predicate, const std::vector<std::string> &objects,
                     bool value = true);

        /** \} */

        /* \name Truth Evaluation
           \{ */

        /** \brief Checks if a predicate given an assignment is currently true in the state.
         *  \param[in] predicate Predicate to check.
         *  \param[in] assignment Grounding of predicate.
         *  \return True if predicate is true in the state, false otherwise.
         */
        bool isTrue(const PredicateDefinitionPtr &predicate, const Assignment &assignment) const;

        /** \brief Checks if a predicate given an assignment is currently true in the state.
         *  \param[in] predicate Name of predicate to check.
         *  \param[in] assignment Grounding of predicate.
         *  \return True if predicate is true in the state, false otherwise.
         */
        bool isTrue(const std::string &predicate, const Assignment &assignment) const;

        /** \brief Checks if a predicate given an assignment is currently true in the state.
         *  \param[in] predicate Name of predicate to check.
         *  \param[in] objects Grounding of predicate.
         *  \return True if predicate is true in the state, false otherwise.
         */
        bool isTrue(const std::string &predicate, const std::vector<std::string> &objects) const;

        /** \} */

        /** \brief Serializes this state to PDDL.
         *  \param[in,out] out Output stream to use.
         *  \param[in] indent Amount of indentation to use.
         *  \return The output stream.
         */
        std::ostream &toPDDL(std::ostream &out = std::cout, unsigned int indent = 0) const;

        /** \brief Get the problem this state is for.
         *  \return The problem this state is for.
         */
        const Problem *getProblem() const;

    private:
        const Problem *problem_;  ///< The problem this state is for.
    };
}  // namespace pddlboat

#endif
