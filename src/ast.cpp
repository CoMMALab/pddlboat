/* Author: Zachary Kingston */

#include <iostream>

#include <pddlboat/parser/ast.h>

using namespace pddlboat::ast;

Keywords_::Keywords_()
{
    add                         //
        ("define", x3::unused)  //
        ("domain", x3::unused)  //
        ;
}

RequirementsKeywords_::RequirementsKeywords_()
{
    Requirements::initialize();
    for (const auto &it : Requirements::getMap().left)
        add(it.first, it.second);
}

OperatorsKeywords_::OperatorsKeywords_()
{
    Operators::initialize();
    for (const auto &it : Operators::getMap().left)
        add(it.first, it.second);
}

std::string TypedList::toString(bool isVariable) const
{
    std::stringstream ss;
    ss << "[";
    for (auto it = types.begin(), end = types.end(); it != end;)
        ss << ((isVariable) ? "?" : "") << *it << ((++it != end) ? ", " : "");
    if (!parent.empty())
        ss << " - " << parent;
    ss << "]";

    return ss.str();
}

std::string AtomicFormula::toString() const
{
    std::stringstream ss;
    ss << "(";
    ss << name;
    if (!variables.empty())
    {
        ss << " - ";
        for (auto it = variables.begin(), end = variables.end(); it != end;)
            ss << it->toString() << ((++it != end) ? ", " : "");
    }
    ss << ")";

    return ss.str();
}

std::string AtomicFormulaList::toString() const
{
    std::stringstream ss;
    ss << "[";
    for (auto it = formulas.begin(), end = formulas.end(); it != end;)
        ss << it->toString() << ((++it != end) ? ", " : "");
    if (!parent.empty())
        ss << " - " << parent;
    ss << "]";

    return ss.str();
}

std::string ExpressionOperator::toString() const
{
    struct StringVisitor
    {
        std::string operator()(const Operators::Keyword &v) const
        {
            if (v == Operators::NONE)
                return "";
            return Operators::toString(v);
        }

        std::string operator()(const std::string &v) const
        {
            return v;
        }
    };

    StringVisitor v;
    return boost::apply_visitor(v, *this);
}

pddlboat::Operators::Keyword ExpressionOperator::toOp() const
{
    struct OperatorVisitor
    {
        Operators::Keyword operator()(const Operators::Keyword &v) const
        {
            return v;
        }

        Operators::Keyword operator()(const std::string &v) const
        {
            return Operators::LITERAL;
        }
    };

    OperatorVisitor v;
    return boost::apply_visitor(v, *this);
}

std::string Expression::toString(bool literal) const
{
    std::stringstream ss;

    bool brace = not literal;
    bool isLiteral = op.toOp() == Operators::LITERAL;
    if (brace)
        ss << "(";

    ss << ((literal && isLiteral) ? "?" : "") << op.toString();

    if (not parameters.empty())
    {
        ss << " - ";
        for (auto it = parameters.begin(), end = parameters.end(); it != end;)
            ss << it->toString() << ((++it != end) ? ", " : "");
    }

    if (not terms.empty())
    {
        ss << " ";
        for (auto it = terms.begin(), end = terms.end(); it != end;)
            ss << it->toString(isLiteral) << ((++it != end) ? " " : "");
    }

    if (brace)
        ss << ')';

    return ss.str();
}

std::string Action::toString() const
{
    std::stringstream ss;
    ss << "(";
    ss << name;
    if (not parameters.empty())
    {
        ss << " - ";
        for (auto it = parameters.begin(), end = parameters.end(); it != end;)
            ss << it->toString() << ((++it != end) ? ", " : "");
    }
    ss << " (precondition: " << precondition.toString() << ')';
    ss << " (effect: " << effect.toString() << ')';
    ss << ')';

    return ss.str();
}

std::string Domain::toString() const
{
    std::stringstream ss;
    ss << "domain(" << std::endl;

    // Name
    ss << "          name: " << name << std::endl;

    // Requirements
    if (!requirements.empty())
    {
        ss << "  requirements: (";
        for (auto it = requirements.begin(), end = requirements.end(); it != end;)
            ss << Requirements::toString(*it) << ((++it != end) ? ", " : "");
        ss << ")" << std::endl;
    }

    // Types
    if (!types.empty())
    {
        ss << "         types: (";
        for (auto it = types.begin(), end = types.end(); it != end;)
            ss << it->toString(false) << ((++it != end) ? ", " : "");
        ss << ")" << std::endl;
    }

    // Constants
    if (!constants.empty())
    {
        ss << "     constants: (";
        for (auto it = constants.begin(), end = constants.end(); it != end;)
            ss << it->toString() << ((++it != end) ? ", " : "");
        ss << ")" << std::endl;
    }

    // Predicates
    if (!predicates.empty())
    {
        ss << "    predicates: (";
        for (auto it = predicates.begin(), end = predicates.end(); it != end;)
            ss << it->toString() << ((++it != end) ? ", " : "");
        ss << ")" << std::endl;
    }

    // Functions
    if (!functions.empty())
    {
        ss << "     functions: (";
        for (auto it = functions.begin(), end = functions.end(); it != end;)
            ss << it->toString() << ((++it != end) ? ", " : "");
        ss << ")" << std::endl;
    }

    // Actions
    if (!actions.empty())
    {
        ss << "       actions: (" << std::endl;
        for (auto it = actions.begin(), end = actions.end(); it != end;)
            ss << "                  " << it++->toString() << std::endl;
        ss << "                )" << std::endl;
    }

    // End
    ss << ")";

    return ss.str();
}

std::string Problem::toString() const
{
    std::stringstream ss;
    ss << "problem(" << std::endl;
    ss << "          name: " << name << std::endl;
    ss << "        domain: " << domain << std::endl;

    if (!objects.empty())
    {
        ss << "     objects: (";
        for (auto it = objects.begin(), end = objects.end(); it != end;)
            ss << it->toString() << ((++it != end) ? ", " : "");
        ss << ")" << std::endl;
    }

    // Initial
    if (!initial.empty())
    {
        ss << "    initial: (";
        for (auto it = initial.begin(), end = initial.end(); it != end;)
            ss << it->toString() << ((++it != end) ? ", " : "");
        ss << ")" << std::endl;
    }

    ss << "           goal: " << goal.toString() << std::endl;
    ss << ")";

    return ss.str();
}

std::string Plan::toString() const
{
    std::stringstream ss;
    ss << "plan(" << std::endl;

    if (!actions.empty())
    {
        ss << "    actions: (";
        for (auto it = actions.begin(), end = actions.end(); it != end;)
            ss << it->toString() << ((++it != end) ? ", " : "");
        ss << ")" << std::endl;
    }
    return ss.str();
}
