/** Author: Zachary Kingston */

#ifndef PDDLBOAT_PARSER_
#define PDDLBOAT_PARSER_

#include <string>
#include <pddlboat/parser/ast.h>

namespace pddlboat
{
    bool parseFile(const std::string &file, ast::Domain &domain);
    bool parseFile(const std::string &file, ast::Problem &problem);
    bool parseFile(const std::string &file, ast::Plan &plan);
    bool parseZ3Action(const std::string &string, ast::z3Action &action);
    bool parseFile(const std::string &file, ast::Expression &expression);
}  // namespace pddlboat

#endif
