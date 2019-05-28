#include <pddlboat/parser/keywords.h>

using namespace pddlboat;

const std::string &Requirements::toString(Keyword req)
{
    return map.right.at(req);
}

const Requirements::bimap Requirements::map = []() {
    Requirements::bimap bm;
    bm.insert(bimap::value_type(":strips", STRIPS));                                        //
    bm.insert(bimap::value_type(":typing", TYPING));                                        //
    bm.insert(bimap::value_type(":negative-preconditions", NEGATIVE_PRECONDITIONS));        //
    bm.insert(bimap::value_type(":disjunctive-preconditions", DISJUNCTIVE_PRECONDITIONS));  //
    bm.insert(bimap::value_type(":equality", EQUALITY));                                    //
    bm.insert(bimap::value_type(":existential-preconditions", EXISTENTIAL_PRECONDITIONS));  //
    bm.insert(bimap::value_type(":universal-preconditions", UNIVERSAL_PRECONDITIONS));      //
    bm.insert(bimap::value_type(":quantified-preconditions", QUANTIFIED_PRECONDITIONS));    //
    bm.insert(bimap::value_type(":conditional-effects", CONDITIONAL_EFFECTS));              //
    bm.insert(bimap::value_type(":fluents", FLUENTS));                                      //
    bm.insert(bimap::value_type(":adl", ADL));                                              //
    bm.insert(bimap::value_type(":durative-actions", DURATIVE_ACTIONS));                    //
    bm.insert(bimap::value_type(":derived-predicates", DERIVED_PREDICATES));                //
    bm.insert(bimap::value_type(":timed-initial-literals", TIMED_INITIAL_LITERALS));        //
    bm.insert(bimap::value_type(":preferences", PREFERENCES));                              //
    bm.insert(bimap::value_type(":constraints", CONSTRAINTS));                              //
    bm.insert(bimap::value_type(":domain-axioms", DOMAIN_AXIOMS));                          //
    return bm;
}();

const std::string &Operators::toString(Keyword op)
{
    return map.right.at(op);
}

const Operators::bimap Operators::map = []() {
    Operators::bimap bm;
    // bm.insert(bimap::value_type
    bm.insert(bimap::value_type("and", Operators::AND));                //
    bm.insert(bimap::value_type("or", Operators::OR));                  //
    bm.insert(bimap::value_type("not", Operators::NOT));                //
    bm.insert(bimap::value_type("forall", Operators::FORALL));          //
    bm.insert(bimap::value_type("imply", Operators::IMPLY));            //
    bm.insert(bimap::value_type("exists", Operators::EXISTS));          //
    bm.insert(bimap::value_type("preference", Operators::PREFERENCE));  //
    bm.insert(bimap::value_type("-", Operators::MINUS));                //
    bm.insert(bimap::value_type("/", Operators::DIVIDE));               //
    bm.insert(bimap::value_type("*", Operators::MULTIPLY));             //
    bm.insert(bimap::value_type("+", Operators::PLUS));                 //
    bm.insert(bimap::value_type(">", Operators::GREATER));              //
    bm.insert(bimap::value_type("<", Operators::LESS));                 //
    bm.insert(bimap::value_type("=", Operators::EQUAL));                //
    bm.insert(bimap::value_type(">=", Operators::GREATER_OR_EQUAL));    //
    bm.insert(bimap::value_type("<=", Operators::LESS_OR_EQUAL));       //
    bm.insert(bimap::value_type("when", Operators::WHEN));              //
    bm.insert(bimap::value_type("assign", Operators::ASSIGN));          //
    bm.insert(bimap::value_type("scale-up", Operators::SCALE_UP));      //
    bm.insert(bimap::value_type("scale-down", Operators::SCALE_DOWN));  //
    bm.insert(bimap::value_type("increase", Operators::INCREASE));      //
    bm.insert(bimap::value_type("decrease", Operators::DECREASE));      //
    return bm;
}();
