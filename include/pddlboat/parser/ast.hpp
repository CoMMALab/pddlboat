/** Author: Zachary Kingston */

#ifndef PDDLBOAT_AST_
#define PDDLBOAT_AST_

#include <string>
#include <vector>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

#include <pddlboat/parser/keywords.hpp>

namespace pddlboat
{
    namespace ast
    {
        ///
        /// Terms
        ///
        class Term
        {
        public:
            std::string symbol;
            std::vector<Term> terms;
        };

        ///
        /// TypedLists
        ///

        class TypedList
        {
        public:
            std::vector<std::string> types;
            std::string parent{""};

            std::ostream &toString(std::ostream &out, bool isVariable = true) const;
        };

        ///
        /// Atomic Formula
        ///

        class AtomicFormula
        {
        public:
            std::string name;
            std::vector<TypedList> variables;

            std::ostream &toString(std::ostream &out) const;
        };

        class AtomicFormulaList
        {
        public:
            std::vector<AtomicFormula> formulas;
            std::string parent{""};

            std::ostream &toString(std::ostream &out) const;
        };

        ///
        /// Actions
        ///

        class Expression
        {
        public:
            Expression() : op("")
            {
            }

            Expression(const std::string &string) : op(string)
            {
            }

            Expression(const std::string &op, const std::vector<Expression> &terms) : op(op), terms(terms)
            {
            }

            Expression(const std::string &op, const std::vector<TypedList> &parameters,
                       const std::vector<Expression> &terms)
              : op(op), parameters(parameters), terms(terms)
            {
            }

            std::string op;
            std::vector<TypedList> parameters{};
            std::vector<Expression> terms{};
            double number{0.};

            std::ostream &toString(std::ostream &out, bool literal = false) const;
            Operators::Keyword getKeyword() const;
        };

        class Action
        {
        public:
            bool derived;
            std::string name;
            std::vector<TypedList> parameters{};
            Expression precondition{};
            Expression effect{};

            std::ostream &toString(std::ostream &out) const;
        };

        ///
        /// Domain
        ///

        class Domain
        {
        public:
            std::string name;
            std::vector<Requirements::Keyword> requirements;
            std::vector<TypedList> types;
            std::vector<TypedList> constants;
            std::vector<AtomicFormula> predicates;
            std::vector<AtomicFormulaList> functions;
            std::vector<Action> actions;

            std::ostream &toString(std::ostream &out) const;
        };

        ///
        /// Problem
        ///

        class Initial
        {
        public:
            AtomicFormula formula;
            bool set{false};
            std::string object{""};

            std::ostream &toString(std::ostream &out) const;
        };

        class Metric
        {
        public:
            bool active{false};
            bool minimize;
            Expression function;
        };

        class Problem
        {
        public:
            std::string name;
            std::string domain;
            std::vector<TypedList> objects{};
            std::vector<Initial> initial{};
            Expression goal{};
            Metric metric{};

            std::ostream &toString(std::ostream &out) const;
        };

        ///
        /// Plan
        ///

        class Plan
        {
        public:
            std::vector<AtomicFormula> actions{};

            std::ostream &toString(std::ostream &out) const;
        };

        ///
        /// z3 Action
        ///

        class z3Action
        {
        public:
            std::string name;
            std::vector<std::string> args{};
            std::string time;
            unsigned int conv{0};

            std::ostream &toPDDL(std::ostream &out) const;
        };

        ///
        /// z3 Predicate
        ///

        class z3Predicate
        {
        public:
            std::string name;
            std::vector<std::string> args{};
            std::string time{""};
            unsigned int conv{0};

            std::ostream &toPDDL(std::ostream &out) const;
        };

        using boost::fusion::operator<<;
    }  // namespace ast
}  // namespace pddlboat

#endif
