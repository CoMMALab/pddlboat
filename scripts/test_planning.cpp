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
        loadedDomain->toPDDL(std::cout);

        const std::string problemfile(argv[2]);
        pddlboat::ast::Problem problem;
        bool r = pddlboat::parseFile(problemfile, problem);
        std::cout << problemfile << ": " << (r ? "OK" : "FAILED") << std::endl;
        if (r)
        {
            auto loadedProblem = pddlboat::toProblem(problem, loadedDomain);
            loadedProblem->toPDDL(std::cout);

            // size_t planning_horizon = 1;
            // pddlboat::Z3Planner fd(loadedProblem, planning_horizon);

            bool r;
            pddlboat::Plan plan(loadedProblem);

            {
                boost::timer::auto_cpu_timer t;
                pddlboat::FastDownwardPlanner fd(loadedProblem);
                r = fd.solve(plan);
            }

            if (r)
            {
                std::cout << std::endl;
                std::cout << plan.toString();
                std::cout << std::endl;

                auto state = loadedProblem->makeState();
                state->fold(loadedProblem->start);

                auto prevState = loadedProblem->makeState();
                prevState->fold(state);

                std::cout << "Start State" << std::endl;
                state->toPDDL(std::cout);
                std::cout << std::endl;

                unsigned int i = 1;
                for (const auto &step : plan.plan)
                {
                    if (not step.action->doAction(state, prevState, step.assignment))
                    {
                        std::cout << "Invalid action!" << std::endl;
                        return 1;
                    }

                    std::cout << "Step " << i++ << std::endl;
                    std::cout << step.toString() << std::endl;

                    state->toPDDL(std::cout);
                    std::cout << std::endl;

                    prevState->clear();
                    prevState->fold(state);
                }
            }
        }
    }

    return 0;
}
