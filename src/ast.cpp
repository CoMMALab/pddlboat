/* Author: Zachary Kingston */

#include <iostream>

#include <pddlboat/parser/ast.hpp>

using namespace pddlboat::ast;

std::ostream &TypedList::toString(std::ostream &out, bool isVariable) const
{
    out << "[";
    for (auto it = types.begin(), end = types.end(); it != end;)
        out << ((isVariable) ? "?" : "") << *it << ((++it != end) ? ", " : "");
    if (!parent.empty())
        out << " - " << parent;
    out << "]";

    return out;
}

std::ostream &AtomicFormula::toString(std::ostream &out) const
{
    out << "(";
    out << name;
    if (!variables.empty())
    {
        out << " - ";
        for (auto it = variables.begin(), end = variables.end(); it != end;)
            it->toString(out) << ((++it != end) ? ", " : "");
    }
    out << ")";

    return out;
}

std::ostream &AtomicFormulaList::toString(std::ostream &out) const
{
    out << "[";
    for (auto it = formulas.begin(), end = formulas.end(); it != end;)
        it->toString(out) << ((++it != end) ? ", " : "");
    if (!parent.empty())
        out << " - " << parent;
    out << "]";

    return out;
}

// std::string ExpressionOperator::toString() const
// {
//     struct StringVisitor
//     {
//         std::string operator()(const Operators::Keyword &v) const
//         {
//             if (v == Operators::NONE)
//                 return "";
//             return Operators::toString(v);
//         }

//         std::string operator()(const std::string &v) const
//         {
//             return v;
//         }
//     };

//     StringVisitor v;
//     return boost::apply_visitor(v, *this);
// }

// pddlboat::Operators::Keyword ExpressionOperator::toOp() const
// {
//     struct OperatorVisitor
//     {
//         Operators::Keyword operator()(const Operators::Keyword &v) const
//         {
//             return v;
//         }

//         Operators::Keyword operator()(const std::string &v) const
//         {
//             return Operators::getKeyword(v);
//         }
//     };

//     OperatorVisitor v;
//     return boost::apply_visitor(v, *this);
// }


pddlboat::Operators::Keyword Expression::getKeyword() const
{
    return Operators::getKeyword(op);
}

std::ostream &Expression::toString(std::ostream &out, bool literal) const
{
    bool brace = not literal;
    bool isLiteral = getKeyword() == Operators::LITERAL;
    if (brace)
        out << "(";

    out << ((literal && isLiteral) ? "?" : "") << op;

    if (not parameters.empty())
    {
        out << " - ";
        for (auto it = parameters.begin(), end = parameters.end(); it != end;)
            it->toString(out) << ((++it != end) ? ", " : "");
    }

    if (not terms.empty())
    {
        out << " ";
        for (auto it = terms.begin(), end = terms.end(); it != end;)
            it->toString(out, isLiteral) << ((++it != end) ? " " : "");
    }

    if (brace)
        out << ')';

    return out;
}

std::ostream &Action::toString(std::ostream &out) const
{
    out << "(";
    out << name;
    if (not parameters.empty())
    {
        out << " - ";
        for (auto it = parameters.begin(), end = parameters.end(); it != end;)
            it->toString(out) << ((++it != end) ? ", " : "");
    }
    out << " (precondition: ";
    precondition.toString(out) << ')';
    out << " (effect: ";
    effect.toString(out) << ')';
    out << ')';

    return out;
}

std::ostream &Domain::toString(std::ostream &out) const
{
    out << "domain(" << std::endl;

    // Name
    out << "          name: " << name << std::endl;

    // Requirements
    if (!requirements.empty())
    {
        out << "  requirements: (";
        for (auto it = requirements.begin(), end = requirements.end(); it != end;)
            out << Requirements::toString(*it) << ((++it != end) ? ", " : "");
        out << ")" << std::endl;
    }

    // Types
    if (!types.empty())
    {
        out << "         types: (";
        for (auto it = types.begin(), end = types.end(); it != end;)
            it->toString(out, false) << ((++it != end) ? ", " : "");
        out << ")" << std::endl;
    }

    // Constants
    if (!constants.empty())
    {
        out << "     constants: (";
        for (auto it = constants.begin(), end = constants.end(); it != end;)
            it->toString(out) << ((++it != end) ? ", " : "");
        out << ")" << std::endl;
    }

    // Predicates
    if (!predicates.empty())
    {
        out << "    predicates: (";
        for (auto it = predicates.begin(), end = predicates.end(); it != end;)
            it->toString(out) << ((++it != end) ? ", " : "");
        out << ")" << std::endl;
    }

    // Functions
    if (!functions.empty())
    {
        out << "     functions: (";
        for (auto it = functions.begin(), end = functions.end(); it != end;)
            it->toString(out) << ((++it != end) ? ", " : "");
        out << ")" << std::endl;
    }

    // Actions
    if (!actions.empty())
    {
        out << "       actions: (" << std::endl;
        for (auto it = actions.begin(), end = actions.end(); it != end;)
        {
            out << "                  ";
            it++->toString(out) << std::endl;
        }
        out << "                )" << std::endl;
    }

    // End
    out << ")";

    return out;
}

std::ostream &Initial::toString(std::ostream &out) const
{
    out << "[";
    formula.toString(out);
    if (set)
        out << " `" << object << "`";
    out << "]";

    return out;
}

std::ostream &Problem::toString(std::ostream &out) const
{
    out << "problem(" << std::endl;
    out << "          name: " << name << std::endl;
    out << "        domain: " << domain << std::endl;

    if (!objects.empty())
    {
        out << "     objects: (";
        for (auto it = objects.begin(), end = objects.end(); it != end;)
            it->toString(out) << ((++it != end) ? ", " : "");
        out << ")" << std::endl;
    }

    // Initial
    if (!initial.empty())
    {
        out << "    initial: (";
        for (auto it = initial.begin(), end = initial.end(); it != end;)
            it->toString(out) << ((++it != end) ? ", " : "");
        out << ")" << std::endl;
    }

    out << "           goal: ";
    goal.toString(out) << std::endl;
    out << ")";

    return out;
}

std::ostream &Plan::toString(std::ostream &out) const
{
    out << "plan(" << std::endl;

    if (!actions.empty())
    {
        out << "    actions: (";
        for (auto it = actions.begin(), end = actions.end(); it != end;)
            it->toString(out) << ((++it != end) ? ", " : "");
        out << ")" << std::endl;
    }
    return out;
}

std::ostream &z3Action::toPDDL(std::ostream &out) const
{
    out << "(" << name;
    for (const auto &arg : args)
        out << " " << arg;
    out << ")";
    return out;
}

std::ostream &z3Predicate::toPDDL(std::ostream &out) const
{
    out << "(" << name;
    for (const auto &arg : args)
        out << " " << arg;
    out << ")";
    return out;
}
