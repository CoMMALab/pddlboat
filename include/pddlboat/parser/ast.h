/** Author: Zachary Kingston */

#ifndef PDDLBOAT_AST_
#define PDDLBOAT_AST_

#include <string>
#include <vector>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>

#include <pddlboat/parser/keywords.h>

namespace pddlboat
{
    namespace ast
    {
        namespace x3 = boost::spirit::x3;

        ///
        /// Reserved Keywords
        ///

        struct Keywords_ : x3::symbols<x3::unused_type>
        {
            Keywords_();
        } const Keywords;

        ///
        /// Requirements Flags
        ///

        struct RequirementsKeywords_ : x3::symbols<Requirements::Keyword>
        {
            RequirementsKeywords_();
        } const RequirementsKeywords;

        ///
        /// Operators Flags
        ///

        struct OperatorsKeywords_ : x3::symbols<Operators::Keyword>
        {
            OperatorsKeywords_();
        } const OperatorsKeywords;

        ///
        /// Terms
        ///
        class Term : x3::position_tagged
        {
        public:
            std::string symbol;
            std::vector<Term> terms;
        };

        ///
        /// TypedLists
        ///

        class TypedList : x3::position_tagged
        {
        public:
            std::vector<std::string> types;
            std::string parent{""};

            std::string toString(bool isVariable = true) const;
        };

        ///
        /// Atomic Formula
        ///

        class AtomicFormula : x3::position_tagged
        {
        public:
            std::string name;
            std::vector<TypedList> variables;

            std::string toString() const;
        };

        class AtomicFormulaList : x3::position_tagged
        {
        public:
            std::vector<AtomicFormula> formulas;
            std::string parent{""};

            std::string toString() const;
        };

        ///
        /// Actions
        ///

        struct ExpressionOperator : x3::variant<std::string, Operators::Keyword>
        {
            using base_type::base_type;
            using base_type::operator=;

            std::string toString() const;
            Operators::Keyword toOp() const;
        };

        class Expression : x3::position_tagged
        {
        public:
            Expression() : op(Operators::NONE)
            {
            }

            Expression(const std::string &string) : op(string)
            {
            }

            Expression(const ExpressionOperator &op, const std::vector<Expression> &terms)
              : op(op), terms(terms)
            {
            }

            Expression(const ExpressionOperator &op, const std::vector<TypedList> &parameters,
                       const std::vector<Expression> &terms)
              : op(op), parameters(parameters), terms(terms)
            {
            }

            ExpressionOperator op;
            std::vector<TypedList> parameters{};
            std::vector<Expression> terms{};

            std::string toString(bool literal = false) const;
        };

        class Action : x3::position_tagged
        {
        public:
            std::string name;
            std::vector<TypedList> parameters{};
            Expression precondition{};
            Expression effect{};

            std::string toString() const;
        };

        ///
        /// Domain
        ///

        class Domain : x3::position_tagged
        {
        public:
            std::string name;
            std::vector<Requirements::Keyword> requirements;
            std::vector<TypedList> types;
            std::vector<TypedList> constants;
            std::vector<AtomicFormula> predicates;
            std::vector<AtomicFormulaList> functions;
            std::vector<Action> actions;

            std::string toString() const;
        };

        ///
        /// Problem
        ///

        class Problem : x3::position_tagged
        {
        public:
            std::string name;
            std::string domain;
            std::vector<TypedList> objects{};
            std::vector<AtomicFormula> initial{};
            Expression goal{};

            std::string toString() const;
        };

        ///
        /// Plan
        ///

        class Plan : x3::position_tagged
        {
        public:
            std::vector<AtomicFormula> actions{};

            std::string toString() const;
        };

        using boost::fusion::operator<<;
    }  // namespace ast
}  // namespace pddlboat

#endif
