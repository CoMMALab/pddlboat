#include <iostream>
#include <boost/timer/timer.hpp>
#include <boost/program_options.hpp>

#include <pddlboat/parser/parser.h>
#include <pddlboat/parser/translator.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/planner.h>
#include <pddlboat/solver/fastdownwardplanner.h>
#include <pddlboat/solver/z3planner.h>

namespace po = boost::program_options;
namespace pd = pddlboat;

enum Planner
{
    Z3,
    FD
};

std::istream &operator>>(std::istream &in, Planner &unit)
{
    std::string token;
    in >> token;
    if (token == "z3")
        unit = Planner::Z3;
    else if (token == "fd")
        unit = Planner::FD;
    else
        in.setstate(std::ios_base::failbit);
    return in;
}

int main(int argc, char **argv)
{
    std::string domainfile, problemfile;
    Planner planner;
    po::options_description desc("PDDLBoat - Planner Testing Demo");
    desc.add_options()  //
        ("help,h", "Produces this help message")
        ("domain,d", po::value<std::string>(&domainfile)->required(),
         "PDDL file that describes the problem domain")  //
        ("problem,p", po::value<std::string>(&problemfile)->required(),
         "PDDL file that describes the problem")  //
        ("planner,s", po::value<Planner>(&planner)->default_value(Planner::FD),
         "Planner to use. Either `z3` or `fd`");

    unsigned int horizon;
    po::options_description z3opt("z3 Planner Options");
    z3opt.add_options()  //
        ("horizon,h", po::value<unsigned int>(&horizon)->default_value(1), "Maximum planning horizon to use");

    desc.add(z3opt);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    po::notify(vm);

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
