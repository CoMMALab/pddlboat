/** Author: Zachary Kingston */

#ifndef PDDLBOAT_PARSER_
#define PDDLBOAT_PARSER_

#include <string>
#include <pddlboat/parser/ast.h>

namespace pddlboat
{
    bool parseFile(const std::string &file, ast::Domain &domain);

}  // namespace pddlboat

#endif
