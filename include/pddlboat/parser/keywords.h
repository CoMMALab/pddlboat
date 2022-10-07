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
            NONE = 0,
            STRIPS = 1 << 0,
            TYPING = 1 << 1,
            NEGATIVE_PRECONDITIONS = 1 << 2,
            DISJUNCTIVE_PRECONDITIONS = 1 << 3,
            EQUALITY = 1 << 4,
            EXISTENTIAL_PRECONDITIONS = 1 << 5,
            UNIVERSAL_PRECONDITIONS = 1 << 6,
            QUANTIFIED_PRECONDITIONS = EXISTENTIAL_PRECONDITIONS |  //
                                       UNIVERSAL_PRECONDITIONS,
            CONDITIONAL_EFFECTS = 1 << 7,
            ADL = CONDITIONAL_EFFECTS |        //
                  QUANTIFIED_PRECONDITIONS |   //
                  EQUALITY |                   //
                  DISJUNCTIVE_PRECONDITIONS |  //
                  NEGATIVE_PRECONDITIONS |     //
                  TYPING |                     //
                  STRIPS,
            DURATIVE_ACTIONS = 1 << 8,
            DERIVED_PREDICATES = 1 << 9,
            PREFERENCES = 1 << 10,
            CONSTRAINTS = 1 << 11,
            ACTION_COSTS = 1 << 12,
            OBJECT_FLUENTS = 1 << 13,
            NUMERIC_FLUENTS = 1 << 14 | ACTION_COSTS,
            FLUENTS = OBJECT_FLUENTS | NUMERIC_FLUENTS,
            TOPK = 1 << 15,
            TOPQ = 1 << 16,
        };

        typedef boost::bimap<std::string, Keyword> bimap;
        static bimap &getMap();
        static const std::string &toString(Keyword req);
        static std::vector<std::string> toStrings(Keyword req);
        static void initialize();

        static bool isSubset(Keyword subset, Keyword set);
    };

    Requirements::Keyword operator|(Requirements::Keyword lhs, Requirements::Keyword rhs);
    Requirements::Keyword operator&(Requirements::Keyword lhs, Requirements::Keyword rhs);

    ///
    /// Operators
    ///

    struct Operators
    {
        enum Keyword
        {
            NONE,
            LITERAL,
            NUMBER,
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

        static Operators::Keyword getKeyword(const std::string &string);
    };
}  // namespace pddlboat

#endif
