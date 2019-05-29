/* Author: Zachary Kingston */

#include <pddlboat/parser/keywords.h>

using namespace pddlboat;

const std::string &Requirements::toString(Keyword req)
{
    return getMap().right.at(req);
}

Requirements::bimap &Requirements::getMap()
{
    static bimap map = Requirements::bimap();
    return map;
}

void Requirements::initialize()
{
    auto &map = getMap();
    map.insert(bimap::value_type(":strips", STRIPS));                                        //
    map.insert(bimap::value_type(":typing", TYPING));                                        //
    map.insert(bimap::value_type(":negative-preconditions", NEGATIVE_PRECONDITIONS));        //
    map.insert(bimap::value_type(":disjunctive-preconditions", DISJUNCTIVE_PRECONDITIONS));  //
    map.insert(bimap::value_type(":equality", EQUALITY));                                    //
    map.insert(bimap::value_type(":existential-preconditions", EXISTENTIAL_PRECONDITIONS));  //
    map.insert(bimap::value_type(":universal-preconditions", UNIVERSAL_PRECONDITIONS));      //
    map.insert(bimap::value_type(":quantified-preconditions", QUANTIFIED_PRECONDITIONS));    //
    map.insert(bimap::value_type(":conditional-effects", CONDITIONAL_EFFECTS));              //
    map.insert(bimap::value_type(":fluents", FLUENTS));                                      //
    map.insert(bimap::value_type(":adl", ADL));                                              //
    map.insert(bimap::value_type(":durative-actions", DURATIVE_ACTIONS));                    //
    map.insert(bimap::value_type(":derived-predicates", DERIVED_PREDICATES));                //
    map.insert(bimap::value_type(":timed-initial-literals", TIMED_INITIAL_LITERALS));        //
    map.insert(bimap::value_type(":preferences", PREFERENCES));                              //
    map.insert(bimap::value_type(":constraints", CONSTRAINTS));                              //
    map.insert(bimap::value_type(":domain-axioms", DOMAIN_AXIOMS));                          //
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
