#include <iostream>

#include <boost/program_options.hpp>

#include <pddlboat/utility.h>
#include <pddlboat/parser/parser.h>
#include <pddlboat/parser/translator.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/planner.h>
#include <pddlboat/solver/fdplanner.h>
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

namespace pddlboat
{
    std::istream &operator>>(std::istream &in, FastDownwardPlanner::Search &unit)
    {
        std::string token;
        in >> token;
        if (token == "astar")
            unit = pd::FastDownwardPlanner::Search::ASTAR;
        else if (token == "greedy")
            unit = pd::FastDownwardPlanner::Search::LAZYGREEDY;
        else if (token == "fd")
            unit = pd::FastDownwardPlanner::Search::FDAT;
        else
            in.setstate(std::ios_base::failbit);
        return in;
    }
}  // namespace pddlboat

int main(int argc, char **argv)
{
    //
    // Program Options
    //

    std::string domainfile, problemfile;
    bool domainout, problemout, planout, stepsout, timeout;
    Planner plannertype;
    po::options_description desc("PDDLBoat - Planner Testing Demo");
    desc.add_options()                                                               //
        ("help,h", "Produces this help message")                                     //
        ("domain,d", po::value<std::string>(&domainfile)->required(),                //
         "PDDL file that describes the problem domain")                              //
        ("problem,p", po::value<std::string>(&problemfile)->required(),              //
         "PDDL file that describes the problem")                                     //
        ("planner,s", po::value<Planner>(&plannertype)->default_value(Planner::FD),  //
         "Planner to use. Either `z3` or `fd`")                                      //
        ("ot", po::bool_switch(&timeout),                                            //
         "Print the planning time")                                                  //
        ("od", po::bool_switch(&domainout),                                          //
         "Print the domain as PDDL")                                                 //
        ("op", po::bool_switch(&problemout),                                         //
         "Print the problem as PDDL")                                                //
        ("oa", po::bool_switch(&planout),                                            //
         "Print the steps of a plan as PDDL")                                        //
        ("os", po::bool_switch(&stepsout),                                           //
         "Print the steps of a plan as a sequence of states")                        //
        ;

    unsigned int horizon;
    bool parallel, diagnostics, dump_clauses;
    po::options_description z3opt("z3 Planner Options");
    z3opt.add_options()                                                     //
        ("horizon,z", po::value<unsigned int>(&horizon)->default_value(6),  //
         "Maximum planning horizon to use")                                 //
        ("parallel,x", po::bool_switch(&parallel),                          //
         "Use Z3's cube-and-conquer parallelism")                           //
        ("diagnostics,w", po::bool_switch(&diagnostics),                    //
         "Print debugging statistics")                                      //
        ("dump_clauses,v", po::bool_switch(&dump_clauses),                  //
         "Print all constituent clauses in plan")                           //
        ;
    desc.add(z3opt);

    pd::FastDownwardPlanner::Search search;
    po::options_description fdopt("Fast Downward Planner Options");
    fdopt.add_options()  //
        ("search,y",
         po::value<pd::FastDownwardPlanner::Search>(&search)->default_value(
             pd::FastDownwardPlanner::Search::LAZYGREEDY),              //
         "Search algorithm for FD. Can be `astar`, `greedy`, or `fd`")  //
        ;
    desc.add(fdopt);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    po::notify(vm);

    //
    // Parse the domain
    //

    pd::ast::Domain domain_ast;
    if (not pd::parseFile(domainfile, domain_ast))
    {
        std::cerr << pd::log::format("Failed to parse domain file %1%.", domainfile) << std::endl;
        return 1;
    }

    // Translate domain
    pd::DomainPtr domain;
    try
    {
        domain = pd::toDomain(domain_ast);
    }
    catch (const std::exception &e)
    {
        std::cerr << pd::log::format("Caught exception translating domain: %1%", e.what()) << std::endl;
        return 1;
    }

    if (domainout)
    {
        domain->toPDDL(std::cout);
        std::cout << std::endl;
    }

    //
    // Parse the problem
    //

    pd::ast::Problem problem_ast;
    if (not pd::parseFile(problemfile, problem_ast))
    {
        std::cerr << pd::log::format("Failed to parse problem file %1%.", problemfile) << std::endl;
        return 1;
    }

    pd::ProblemPtr problem;
    try
    {
        problem = pd::toProblem(problem_ast, domain);
    }
    catch (const std::exception &e)
    {
        std::cerr << pd::log::format("Caught exception translating problem: %1%", e.what()) << std::endl;
        return 1;
    }

    if (problemout)
    {
        problem->toPDDL(std::cout);
        std::cout << std::endl;
    }

    //
    // Create the planner
    //

    pd::PlannerPtr planner;
    switch (plannertype)
    {
        case Z3:
            planner = std::make_shared<pd::Z3Planner>(problem, horizon, parallel, diagnostics, dump_clauses);
            break;
        case FD:
            planner = std::make_shared<pd::FastDownwardPlanner>(problem, search);
            break;
        default:
            std::cerr << "Unknown planner type. Exiting." << std::endl;
            return 1;
    }

    //
    // Do some planning
    //

    bool r;
    pd::Plan plan(problem);

    {
        boost::timer::cpu_timer t;
        r = planner->solve(plan);

        if (timeout)
        {
            t.stop();
            std::cout << pd::io::getTiming(t) << std::endl;
        }
    }

    if (r)
    {
        std::cerr << "Planning succeeded!" << std::endl;

        if (planout)
            std::cout << plan << std::endl;

        if (stepsout)
        {
            unsigned int i = 0;
            std::cout << "Step " << i++ << std::endl;
            problem->start->toPDDL(std::cout);
            std::cout << std::endl;

            for (const auto &step : plan.steps)
            {
                std::cout << "Step " << i++ << " " << step << std::endl;
                step.state->toPDDL(std::cout);
                std::cout << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "Planning failed!" << std::endl;
        return 1;
    }

    return 0;
}
