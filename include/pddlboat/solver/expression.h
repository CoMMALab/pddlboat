#ifndef PDDLBOAT_EXPRESSION_
#define PDDLBOAT_EXPRESSION_

#include <pddlboat/macros.h>

namespace pddlboat
{
    PDDLBOAT_CLASS_FORWARD(Assignment)
    PDDLBOAT_CLASS_FORWARD(Variable)
    PDDLBOAT_CLASS_FORWARD(VariableSet)
    PDDLBOAT_CLASS_FORWARD(Domain)
    PDDLBOAT_CLASS_FORWARD(Problem)
    PDDLBOAT_CLASS_FORWARD(State)
    PDDLBOAT_CLASS_FORWARD(ObjectFunction)

    ///
    /// Expression
    ///

    PDDLBOAT_CLASS_FORWARD(Expression)
    class Expression
    {
    public:
        enum Type
        {
            EXPRESSION = 0,
            PREDICATE = 1 << 1,
            DERIVED = 1 << 2,
            AND = 1 << 3,
            OR = 1 << 4,
            VARIADIC = AND | OR,
            NOT = 1 << 5,
            EQUALS = 1 << 6,
            IMPLY = 1 << 7,
            FORALL = 1 << 8,
            EXISTS = 1 << 9,
            QUANTIFIER = EXISTS | FORALL,
            WHEN = 1 << 10,

            OBJECT_FUNCTION = 1 << 11,
            NUMERIC_FUNCTION = 1 << 12,

            GREATER_THAN = 1 << 13,
            LESS_THAN = 1 << 14,
            GREATER_OR_EQUAL = 1 << 15,
            LESS_OR_EQUAL = 1 << 16,

            MINUS = 1 << 17,
            DIVIDE = 1 << 18,
            MULTIPLY = 1 << 19,
            PLUS = 1 << 20,

            ASSIGN = 1 << 21,
            SCALE_UP = 1 << 22,
            SCALE_DOWN = 1 << 23,
            INCREASE = 1 << 24,
            DECREASE = 1 << 25,
        };

        virtual bool evaluate(const StatePtr &state, const Assignment &vars) const;
        virtual void assign(StatePtr &out, const StatePtr &in, const Assignment &vars) const;
        virtual Type type() const;
        virtual Type getTypes() const;
        virtual void getVariables(VariableSet &vars) const;
        virtual void getUsedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                       const Assignment &vars) const;
        virtual void getAssignedPredicates(std::set<std::string> &predicates, const StatePtr &state,
                                           const Assignment &vars) const;
        virtual bool containsVariable(const std::string &variable) const;
        virtual bool containsPredicate(const std::string &predicate) const;
        virtual void renameVariable(const std::string &old, const std::string &name);
        virtual std::ostream &toPDDL(std::ostream &out, unsigned int indent = 2,
                                     unsigned int level = 0) const = 0;
    };

    Expression::Type operator|(Expression::Type lhs, Expression::Type rhs);
    Expression::Type operator&(Expression::Type lhs, Expression::Type rhs);
    bool isIn(Expression::Type type, Expression::Type set);

    PDDLBOAT_CLASS_FORWARD(NumericExpression)
    class NumericExpression : public Expression
    {
    public:
        virtual double getValue(const StatePtr &state, const Assignment &vars) const;
    };

    /** \brief A bound value.
     */
    struct Value
    {
        /** \brief Constructor.
         *  \param[in] isVar Is this a variable name?
         *  \param[in] name Name of variable or object.
         *  \param[in] type Type of argument
         */
        Value(bool isVar, const std::string &name, const std::string &type);

        /** \brief Constructor.
         *  \param[in] function Function that returns an object value.
         */
        Value(const ObjectFunctionPtr &function);

        bool isVar{false};                        ///< Is this value a variable to be substituted?
        std::string name;                         ///< Name of this value.
        std::string type{PDDLBOAT_OBJECT_CLASS};  ///< Type of this value.
        ObjectFunctionPtr function{nullptr};      ///< Object returning function.

        /** \brief Retrieve the object this value is bound to.
         *  \param[in] state State of the world.
         *  \param[in] vars The assignment to variables.
         *  \return The object this value is currently ground to.
         */
        std::string getObject(const StatePtr &state, const Assignment &vars) const;

        void getVariables(VariableSet &vars) const;
        bool containsVariable(const std::string &variable) const;
        void renameVariable(const std::string &old, const std::string &name);

        /** \brief Print this value out.
         *  \param[in,out] out Output stream.
         *  \return The output stream.
         */
        std::ostream &toPDDL(std::ostream &out) const;

        /** \brief Print this value out.
         *  \param[in,out] out Output stream.
         *  \return The output stream.
         */
        std::ostream &toString(std::ostream &out) const;
    };

    std::vector<std::string> getValueObjects(const std::vector<Value> &values, const StatePtr &state,
                                             const Assignment &assignment);

    Value getValue(const std::string &term, const Variable *variable, const ProblemPtr &problem,
                   const DomainPtr &domain);

    std::vector<Value> getValues(const std::vector<std::string> &terms,
                                 const std::vector<Variable> &variables, const ProblemPtr &problem,
                                 const DomainPtr &domain);
}  // namespace pddlboat

#endif
