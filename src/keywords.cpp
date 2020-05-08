/* Author: Zachary Kingston */

#include <pddlboat/parser/keywords.h>

using namespace pddlboat;

const std::string &Requirements::toString(Keyword req)
{
    return getMap().right.at(req);
}

std::vector<std::string> Requirements::toStrings(Keyword req)
{
    std::vector<std::string> strings;

    auto addIf = [&](Keyword key) {
        if (isSubset(key, req))
        {
            strings.emplace_back(toString(key));
            return true;
        }
        return false;
    };

    // ADL
    if (not addIf(ADL))
    {
        addIf(STRIPS);
        addIf(TYPING);
        addIf(NEGATIVE_PRECONDITIONS);
        addIf(DISJUNCTIVE_PRECONDITIONS);
        addIf(EQUALITY);

        // quantifiers
        if (not addIf(QUANTIFIED_PRECONDITIONS))
        {
            addIf(EXISTENTIAL_PRECONDITIONS);
            addIf(UNIVERSAL_PRECONDITIONS);
        }

        addIf(CONDITIONAL_EFFECTS);
    }

    if (not addIf(FLUENTS))
    {
        addIf(OBJECT_FLUENTS);
        addIf(NUMERIC_FLUENTS);
    }

    addIf(DURATIVE_ACTIONS);

    addIf(DERIVED_PREDICATES);
    addIf(PREFERENCES);
    addIf(CONSTRAINTS);

    return strings;
}

Requirements::bimap &Requirements::getMap()
{
    static bimap map = Requirements::bimap();
    return map;
}

void Requirements::initialize()
{
    auto &map = getMap();
    map.insert(bimap::value_type("invalid", NONE));                                          //
    map.insert(bimap::value_type(":strips", STRIPS));                                        //
    map.insert(bimap::value_type(":typing", TYPING));                                        //
    map.insert(bimap::value_type(":negative-preconditions", NEGATIVE_PRECONDITIONS));        //
    map.insert(bimap::value_type(":disjunctive-preconditions", DISJUNCTIVE_PRECONDITIONS));  //
    map.insert(bimap::value_type(":equality", EQUALITY));                                    //
    map.insert(bimap::value_type(":existential-preconditions", EXISTENTIAL_PRECONDITIONS));  //
    map.insert(bimap::value_type(":universal-preconditions", UNIVERSAL_PRECONDITIONS));      //
    map.insert(bimap::value_type(":quantified-preconditions", QUANTIFIED_PRECONDITIONS));    //
    map.insert(bimap::value_type(":conditional-effects", CONDITIONAL_EFFECTS));              //
    map.insert(bimap::value_type(":numeric-fluents", NUMERIC_FLUENTS));                      //
    map.insert(bimap::value_type(":object-fluents", OBJECT_FLUENTS));                        //
    map.insert(bimap::value_type(":fluents", FLUENTS));                                      //
    map.insert(bimap::value_type(":adl", ADL));                                              //
    map.insert(bimap::value_type(":durative-actions", DURATIVE_ACTIONS));                    //
    map.insert(bimap::value_type(":derived-predicates", DERIVED_PREDICATES));                //
    map.insert(bimap::value_type(":preferences", PREFERENCES));                              //
    map.insert(bimap::value_type(":constraints", CONSTRAINTS));                              //
}

bool Requirements::isSubset(Keyword subset, Keyword set)
{
    return (subset & set) == subset;
}

Requirements::Keyword pddlboat::operator|(Requirements::Keyword lhs, Requirements::Keyword rhs)
{
    return static_cast<Requirements::Keyword>(                               //
        static_cast<std::underlying_type<Requirements::Keyword>::type>(lhs)  //
        | static_cast<std::underlying_type<Requirements::Keyword>::type>(rhs));
}

Requirements::Keyword pddlboat::operator&(Requirements::Keyword lhs, Requirements::Keyword rhs)
{
    return static_cast<Requirements::Keyword>(                               //
        static_cast<std::underlying_type<Requirements::Keyword>::type>(lhs)  //
        & static_cast<std::underlying_type<Requirements::Keyword>::type>(rhs));
}

const std::string &Operators::toString(Keyword op)
{
    return getMap().right.at(op);
}

Operators::bimap &Operators::getMap()
{
    static bimap map = Operators::bimap();
    return map;
}

void Operators::initialize()
{
    auto &map = getMap();
    map.insert(bimap::value_type("and", Operators::AND));                //
    map.insert(bimap::value_type("or", Operators::OR));                  //
    map.insert(bimap::value_type("not", Operators::NOT));                //
    map.insert(bimap::value_type("forall", Operators::FORALL));          //
    map.insert(bimap::value_type("imply", Operators::IMPLY));            //
    map.insert(bimap::value_type("exists", Operators::EXISTS));          //
    map.insert(bimap::value_type("preference", Operators::PREFERENCE));  //
    map.insert(bimap::value_type("-", Operators::MINUS));                //
    map.insert(bimap::value_type("/", Operators::DIVIDE));               //
    map.insert(bimap::value_type("*", Operators::MULTIPLY));             //
    map.insert(bimap::value_type("+", Operators::PLUS));                 //
    map.insert(bimap::value_type(">", Operators::GREATER));              //
    map.insert(bimap::value_type("<", Operators::LESS));                 //
    map.insert(bimap::value_type("=", Operators::EQUAL));                //
    map.insert(bimap::value_type(">=", Operators::GREATER_OR_EQUAL));    //
    map.insert(bimap::value_type("<=", Operators::LESS_OR_EQUAL));       //
    map.insert(bimap::value_type("when", Operators::WHEN));              //
    map.insert(bimap::value_type("assign", Operators::ASSIGN));          //
    map.insert(bimap::value_type("scale-up", Operators::SCALE_UP));      //
    map.insert(bimap::value_type("scale-down", Operators::SCALE_DOWN));  //
    map.insert(bimap::value_type("increase", Operators::INCREASE));      //
    map.insert(bimap::value_type("decrease", Operators::DECREASE));      //
}

Operators::Keyword Operators::getKeyword(const std::string &string)
{
    auto result = Operators::getMap().left.find(string);
    if (result == Operators::getMap().left.end())
        return Operators::LITERAL;

    return result->second;
}
