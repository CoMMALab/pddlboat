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
        };

        virtual bool evaluate(const StatePtr &state, const Assignment &vars) const = 0;
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

    class NumericExpression : public Expression
    {
    public:
        bool evaluate(const StatePtr &state, const Assignment &vars) const override;
        virtual double getValue(const StatePtr &state, const Assignment &vars) = 0;
    };

    /** \brief A bound value.
     */
    struct Value
    {
        /** \brief Constructor.
         *  \param[in] isVar Is this a variable name?
         *  \param[in] name Name of variable or object.
         *  \param[in] type Type of argument.
         */
        Value(bool isVar, const std::string &name, const std::string &type);

        bool isVar;        ///< Is this value a variable to be substituted?
        std::string name;  ///< Name of this value.
        std::string type;  ///< Type of this value.

        /** \brief Print this value out.
         *  \param[in,out] out Output stream.
         *  \return The output stream.
         */
        std::ostream &toString(std::ostream &out) const;
    };

    Value getValue(const std::string &term, const Variable *variable, const ProblemPtr &problem,
                   const DomainPtr &domain);
}  // namespace pddlboat

#endif
