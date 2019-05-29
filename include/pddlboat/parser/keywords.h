/** Author: Zachary Kingston */

#ifndef PDDLBOAT_KEYWORDS_
#define PDDLBOAT_KEYWORDS_

#include <string>

#include <boost/bimap.hpp>

namespace pddlboat
{
    ///
    /// Requirements Flags
    ///

    struct Requirements
    {
        enum Keyword
        {
            STRIPS,
            TYPING,
            NEGATIVE_PRECONDITIONS,
            DISJUNCTIVE_PRECONDITIONS,
            EQUALITY,
            EXISTENTIAL_PRECONDITIONS,
            UNIVERSAL_PRECONDITIONS,
            QUANTIFIED_PRECONDITIONS,
            CONDITIONAL_EFFECTS,
            FLUENTS,
            ADL,
            DURATIVE_ACTIONS,
            DERIVED_PREDICATES,
            TIMED_INITIAL_LITERALS,
            PREFERENCES,
            CONSTRAINTS,
            DOMAIN_AXIOMS,
            ACTION_COSTS
        };

        typedef boost::bimap<std::string, Keyword> bimap;
        static bimap &getMap();
        static const std::string &toString(Keyword req);
        static void initialize();
    };

    ///
    /// Operators
    ///

    struct Operators
    {
        enum Keyword
        {
            NONE,
            LITERAL,
            AND,
            OR,
            NOT,
            FORALL,
            IMPLY,
            EXISTS,
            PREFERENCE,
            MINUS,
            DIVIDE,
            MULTIPLY,
            PLUS,
            GREATER,
            LESS,
            EQUAL,
            GREATER_OR_EQUAL,
            LESS_OR_EQUAL,
            WHEN,
            ASSIGN,
            SCALE_UP,
            SCALE_DOWN,
            INCREASE,
            DECREASE
        };

        typedef boost::bimap<std::string, Keyword> bimap;
        static bimap &getMap();
        static const std::string &toString(Keyword op);
        static void initialize();
    };
}  // namespace pddlboat

#endif
