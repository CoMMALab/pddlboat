/* Author: Zachary Kingston */

#ifndef PDDLBOAT_ADAPTED_
#define PDDLBOAT_ADAPTED_

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/adapted.hpp>

#include <pddlboat/parser/ast.h>

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

BOOST_FUSION_ADAPT_STRUCT(                           //
    pddlboat::ast::Expression,                       //
    (pddlboat::ast::ExpressionOperator, op)          //
    (std::vector<pddlboat::ast::Expression>, terms)  //
);

BOOST_FUSION_ADAPT_STRUCT(                               //
    pddlboat::ast::Action,                               //
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

#endif
