/* Author: Zachary Kingston */

#ifndef PDDLBOAT_ADAPTED_
#define PDDLBOAT_ADAPTED_

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/adapted.hpp>

#include <pddlboat/parser/ast.hpp>

///
/// Fusion Adaptation
///

BOOST_FUSION_ADAPT_STRUCT(                     //
    pddlboat::ast::Term,                       //
    (std::string, symbol)                      //
    (std::vector<pddlboat::ast::Term>, terms)  //
);

BOOST_FUSION_ADAPT_STRUCT(             //
    pddlboat::ast::TypedList,          //
    (std::vector<std::string>, types)  //
    (std::string, parent)              //
);

BOOST_FUSION_ADAPT_STRUCT(                              //
    pddlboat::ast::AtomicFormula,                       //
    (std::string, name)                                 //
    (std::vector<pddlboat::ast::TypedList>, variables)  //
);

BOOST_FUSION_ADAPT_STRUCT(                                 //
    pddlboat::ast::AtomicFormulaList,                      //
    (std::vector<pddlboat::ast::AtomicFormula>, formulas)  //
    (std::string, parent)                                  //
);

BOOST_FUSION_ADAPT_STRUCT(                               //
    pddlboat::ast::Expression,                           //
    (std::string, op)              //
    (std::vector<pddlboat::ast::TypedList>, parameters)  //
    (std::vector<pddlboat::ast::Expression>, terms)      //
);

BOOST_FUSION_ADAPT_STRUCT(                               //
    pddlboat::ast::Action,                               //
    (bool, derived)                                      //
    (std::string, name)                                  //
    (std::vector<pddlboat::ast::TypedList>, parameters)  //
    (pddlboat::ast::Expression, precondition)            //
    (pddlboat::ast::Expression, effect)                  //
);

BOOST_FUSION_ADAPT_STRUCT(                                        //
    pddlboat::ast::Domain,                                        //
    (std::string, name)                                           //
    (std::vector<pddlboat::Requirements::Keyword>, requirements)  //
    (std::vector<pddlboat::ast::TypedList>, types)                //
    (std::vector<pddlboat::ast::TypedList>, constants)            //
    (std::vector<pddlboat::ast::AtomicFormula>, predicates)       //
    (std::vector<pddlboat::ast::AtomicFormulaList>, functions)    //
    (std::vector<pddlboat::ast::Action>, actions)                 //
);

BOOST_FUSION_ADAPT_STRUCT(                   //
    pddlboat::ast::Initial,                  //
    (pddlboat::ast::AtomicFormula, formula)  //
    (bool, set)                              //
    (std::string, object)                    //
);

BOOST_FUSION_ADAPT_STRUCT(                 //
    pddlboat::ast::Metric,                 //
    (bool, active)                         //
    (bool, minimize)                       //
    (pddlboat::ast::Expression, function)  //
);

BOOST_FUSION_ADAPT_STRUCT(                            //
    pddlboat::ast::Problem,                           //
    (std::string, name)                               //
    (std::string, domain)                             //
    (std::vector<pddlboat::ast::TypedList>, objects)  //
    (std::vector<pddlboat::ast::Initial>, initial)    //
    (pddlboat::ast::Expression, goal)                 //
    (pddlboat::ast::Metric, metric)                   //
);

BOOST_FUSION_ADAPT_STRUCT(                                //
    pddlboat::ast::Plan,                                  //
    (std::vector<pddlboat::ast::AtomicFormula>, actions)  //
);

BOOST_FUSION_ADAPT_STRUCT(            //
    pddlboat::ast::z3Action,          //
    (std::string, name)               //
    (std::vector<std::string>, args)  //
    (std::string, time)               //
);

BOOST_FUSION_ADAPT_STRUCT(            //
    pddlboat::ast::z3Predicate,       //
    (std::string, name)               //
    (std::vector<std::string>, args)  //
    (std::string, time)               //
);

#endif
