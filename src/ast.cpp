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
    for (const auto &it : Requirements::map.left)
        add(it.first, it.second);
}

OperatorsKeywords_::OperatorsKeywords_()
{
    for (const auto &it : Operators::map.left)
        add(it.first, it.second);
}

std::string TypedList::toString() const
{
    std::stringstream ss;
    ss << "[";
    for (auto it = types.begin(), end = types.end(); it != end;)
        ss << *it << ((++it != end) ? ", " : "");
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
            ss << it->toString() << ((++it != end) ? ", " : "");
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

    // End
    ss << ")";

    return ss.str();
}
