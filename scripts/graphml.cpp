#include <iostream>
#include <pddlboat/parser/parser.h>
#include <pddlboat/parser/translator.h>
#include <pddlboat/parser/ast.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/domain.h>
#include <pddlboat/solver/planner.h>
#include <pddlboat/solver/astarplanner.h>

namespace pd = pddlboat;

auto parse_domain(const std::string &domain_file) -> pd::DomainPtr
{
    pd::DomainPtr domain;
    pd::ast::Domain domain_ast;
    if (not pd::parseFile(domain_file, domain_ast))
    {
        std::cerr << pd::log::format("Failed to parse domain file %1%.", domain_file) << std::endl;
        return nullptr;
    }

    try
    {
        domain = pd::toDomain(domain_ast);
    }
    catch (const std::exception &e)
    {
        std::cerr << pd::log::format("Caught exception translating domain: %1%", e.what()) << std::endl;
        return nullptr;
    }

    return domain;
}

auto parse_problem(const pd::DomainPtr &domain, const std::string &problem_file) -> pd::ProblemPtr
{
    pd::ProblemPtr problem;
    pd::ast::Problem problem_ast;
    if (not pd::parseFile(problem_file, problem_ast))
    {
        std::cerr << pd::log::format("Failed to parse problem file %1%.", problem_file) << std::endl;
        return nullptr;
    }

    try
    {
        problem = pd::toProblem(problem_ast, domain);
    }
    catch (const std::exception &e)
    {
        std::cerr << pd::log::format("Caught exception translating problem: %1%", e.what()) << std::endl;
        return nullptr;
    }

    return problem;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        std::cerr << pd::log::format("Invalid usage. ./pddlboat_graphml <domain> <problem> <output>") << std::endl;
        return 1;
    }

    auto domain = parse_domain(argv[1]);
    if (not domain)
        return 1;

    auto problem = parse_problem(domain, argv[2]);
    if (not problem)
        return 1;

    auto task_planner = std::make_shared<pd::AStarPlanner>(problem);
    task_planner->generateGraphML(argv[3]);

    return 0;
}
