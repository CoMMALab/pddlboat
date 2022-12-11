/** Author: Zachary Kingston */

#ifndef PDDLBOAT_PARSER_
#define PDDLBOAT_PARSER_

#include <string>
#include <pddlboat/parser/ast.hpp>

namespace pddlboat
{
    bool parseFile(const std::string &file, ast::Domain &domain);
    bool parseFile(const std::string &file, ast::Problem &problem);
    bool parseFile(const std::string &file, ast::Plan &plan);
    bool parseFile(const std::string &file, ast::Expression &expression);

    bool parseString(const std::string &string, ast::Domain &domain);
    bool parseString(const std::string &string, ast::Problem &problem);
    bool parseString(const std::string &string, ast::Plan &plan);
    bool parseString(const std::string &string, ast::Expression &expression);
    bool parseString(const std::string &string, std::vector<ast::TypedList> &parsed);

    bool parseZ3Action(const std::string &string, ast::z3Action &action);
    bool parseZ3Predicate(const std::string &string, ast::z3Predicate &predicate);
}  // namespace pddlboat

#endif
