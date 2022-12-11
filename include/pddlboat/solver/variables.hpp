#ifndef PDDLBOAT_VARIABLES_
#define PDDLBOAT_VARIABLES_

#include <iostream>
#include <string>
#include <set>
#include <vector>

#include <pddlboat/macros.hpp>

namespace pddlboat
{
    PDDLBOAT_CLASS_FORWARD(Object)
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

}  // namespace pddlboat

#endif
