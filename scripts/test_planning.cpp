#include <iostream>
#include <boost/timer/timer.hpp>
#include <pddlboat/parser/parser.h>
#include <pddlboat/parser/translator.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/planner.h>
#include <pddlboat/solver/z3planner.h>

namespace pd = pddlboat;

int main(int argc, char **argv)
{

    boost::timer::auto_cpu_timer t;

    if (argc < 3)
    {
        std::cerr << "Must provide file names!" << std::endl;
        return 1;
    }

    const std::string domainfile(argv[1]);
    pddlboat::ast::Domain domain;
    bool r = pddlboat::parseFile(domainfile, domain);
    std::cout << domainfile << ": " << (r ? "OK" : "FAILED") << std::endl;
    if (r)
    {
        auto loadedDomain = pddlboat::toDomain(domain);

        const std::string problemfile(argv[2]);
        pddlboat::ast::Problem problem;
        bool r = pddlboat::parseFile(problemfile, problem);
        std::cout << problemfile << ": " << (r ? "OK" : "FAILED") << std::endl;
        if (r)
        {
            auto loadedProblem = pddlboat::toProblem(problem, loadedDomain);

            size_t planning_horizon = 1;

            pddlboat::Z3Planner fd(loadedProblem, planning_horizon);
            pddlboat::Plan plan(loadedProblem);
            if (fd.solve(plan))
            {

            }
        }
    }

    return 0;
}
