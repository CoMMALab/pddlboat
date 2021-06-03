#ifndef PDDLBOAT_HELPER_
#define PDDLBOAT_HELPER_

#include <iostream>

#include <boost/program_options.hpp>

#include <pddlboat/utility.h>
#include <pddlboat/parser/parser.h>
#include <pddlboat/parser/translator.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/planner.h>
#include <pddlboat/solver/fdplanner.h>
#include <pddlboat/solver/z3planner.h>
#include <pddlboat/solver/astarplanner.h>
#include <pddlboat/solver/dependency.h>

namespace pddlboat
{
    namespace
    {
        namespace po = boost::program_options;
    }

    enum PlannerType
    {
        Z3,
        FD,
        AS,
    };

    std::istream &operator>>(std::istream &in, PlannerType &unit)
    {
        std::string token;
        in >> token;
        if (token == "z3")
            unit = PlannerType::Z3;
        else if (token == "fd")
            unit = PlannerType::FD;
        else if (token == "astar")
            unit = PlannerType::AS;
        else
            in.setstate(std::ios_base::failbit);
        return in;
    }

    std::istream &operator>>(std::istream &in, FastDownwardPlanner::Search &unit)
    {
        std::string token;
        in >> token;
        if (token == "astar")
            unit = FastDownwardPlanner::Search::ASTAR;
        else if (token == "greedy")
            unit = FastDownwardPlanner::Search::LAZYGREEDY;
        else if (token == "fd")
            unit = FastDownwardPlanner::Search::FDAT;
        else if (token == "lama")
            unit = FastDownwardPlanner::Search::LAMA;
        else
            in.setstate(std::ios_base::failbit);
        return in;
    }

    class ProblemSetup
    {
    public:
        std::string domain_file;
        ast::Domain domain_ast;
        DomainPtr domain;

        std::string problem_file;
        ast::Problem problem_ast;
        ProblemPtr problem;

        std::string graph_file;

        PlannerType planner_type;
        PlannerPtr planner;

        PlanPtr plan;
        PlanDependencyGraphPtr graph;

        unsigned int timeout;  // in ms

        struct
        {
            bool domain;
            bool problem;
            bool plan;
            bool steps;
            bool step_index;
            bool time;
            bool graph;
        } output;

        struct
        {
            bool leaf_dist;
            bool tooltips;
            bool start_goal;
        } dependency;

        struct
        {
            Z3Planner::Options options;

            struct
            {
                bool model;
                bool smt;
            } output;
        } z3;

        struct
        {
            FastDownwardPlanner::Search search;
        } fd;

        struct
        {
            AStarPlanner::Options options;

            bool planning_heuristic;
        } astar;

        std::string name;
        po::options_description description;

        ProblemSetup(const std::string &name) : name(name), description(name)
        {
            setupArguments();
        }

        /** \brief Setup common program arguments.
         */
        void setupArguments()
        {
            description.add_options()                                                                 //
                ("help,h", "Produces this help message")                                              //
                ("domain,d", po::value<std::string>(&domain_file)->required(),                        //
                 "PDDL file that describes the problem domain")                                       //
                ("problem,p", po::value<std::string>(&problem_file)->required(),                      //
                 "PDDL file that describes the problem")                                              //
                ("planner,s", po::value<PlannerType>(&planner_type)->default_value(PlannerType::FD),  //
                 "Planner to use. Either `z3` or `fd`")                                               //
                ("od", po::bool_switch(&output.domain),                                               //
                 "Print the domain as PDDL")                                                          //
                ("op", po::bool_switch(&output.problem),                                              //
                 "Print the problem as PDDL")                                                         //
                ("oa", po::bool_switch(&output.plan),                                                 //
                 "Print the steps of a plan as PDDL")                                                 //
                ("os", po::bool_switch(&output.steps),                                                //
                 "Print the steps of a plan as a sequence of states")                                 //
                ("ot", po::bool_switch(&output.time),                                                 //
                 "Print the planning time")                                                           //
                ("og", po::bool_switch(&output.graph),                                                //
                 "Print the dependency graph of the plan")                                            //
                ("ogf", po::value<std::string>(&graph_file)->default_value(""),                       //
                 "Output file for dependency graph")                                                  //
                ("step_index", po::bool_switch(&output.step_index),                                   //
                 "Output index of step for simultaneous action plans")                                //
                ("timeout", po::value<unsigned int>(&timeout)->default_value(0),                      //
                 "Internal planning timeout in milliseconds.")                                        //
                ("leafdist", po::bool_switch(&dependency.leaf_dist),                                  //
                 "Print the dependency graph out based on leaf distance and not makespan")            //
                ("tooltips", po::bool_switch(&dependency.tooltips),                                   //
                 "Add tooltip information to dependency graph output")                                //
                ("startgoal", po::bool_switch(&dependency.start_goal),                                //
                 "Add start/goal to dependency graph output")                                         //
                ;

            po::options_description z3opt("Z3 Planner Options");
            z3opt.add_options()                                                                        //
                ("max_horizon,z", po::value<std::size_t>(&z3.options.horizon.max)->default_value(20),  //
                 "Maximum planning horizon to use")                                                    //
                ("min_horizon,l", po::value<std::size_t>(&z3.options.horizon.min)->default_value(0),   //
                 "Minimum planning horizon to use")                                                    //
                ("diagnostics,w", po::bool_switch(&z3.options.diagnostics),                            //
                 "Print debugging statistics")                                                         //
                ("parallel", po::bool_switch(&z3.options.parallel),                                    //
                 "Parallel solving through Z3")                                                        //
                ("optimize", po::bool_switch(&z3.options.optimize),                                    //
                 "Optimize problem metric if available")                                               //
                ("dump_clauses,v", po::bool_switch(&z3.options.dump_clauses),                          //
                 "Print all constituent clauses in plan")                                              //
                ("simultaneous", po::bool_switch(&z3.options.simultaneous),                            //
                 "Simultaneous action execution in Z3 encoding")                                       //
                ("dump_model", po::bool_switch(&z3.output.model),                                      //
                 "Print the checked Z3 model")                                                         //
                ("dump_smt", po::bool_switch(&z3.output.smt),                                          //
                 "Print the model as a smt2 file")                                                     //
                ;
            description.add(z3opt);

            po::options_description fdopt("Fast Downward Planner Options");
            fdopt.add_options()  //
                ("search,y",
                 po::value<FastDownwardPlanner::Search>(&fd.search)
                     ->default_value(FastDownwardPlanner::Search::LAZYGREEDY),          //
                 "Search algorithm for FD. Can be `astar`, `greedy`, `fd`, or `lama`")  //
                ;
            description.add(fdopt);

            po::options_description asopt("A* Planner Options");
            asopt.add_options()                                                     //
                ("planning_heuristic", po::bool_switch(&astar.planning_heuristic),  //
                 "Use the planner heuristic for A* search")                         //
                ("astar_verbose", po::bool_switch(&astar.options.verbose),          //
                 "Verbose output for A* planner")                                   //
                ;
            description.add(asopt);
        }

        /** \brief Parse arguments from command line.
         */
        bool parseArguments(int argc, char **argv)
        {
            po::variables_map vm;
            po::store(po::parse_command_line(argc, argv, description), vm);

            if (vm.count("help"))
            {
                std::cout << description << std::endl;
                return false;
            }

            po::notify(vm);
            return true;
        }

        /** \brief Initialize internal structures after options have been set.
         */
        bool initialize()
        {
            // parse domain
            if (not parseFile(domain_file, domain_ast))
            {
                std::cerr << log::format("Failed to parse domain file %1%.", domain_file) << std::endl;
                return false;
            }

            // translate domain
            try
            {
                domain = toDomain(domain_ast);
            }
            catch (const std::exception &e)
            {
                std::cerr << log::format("Caught exception translating domain: %1%", e.what()) << std::endl;
                return false;
            }

            if (output.domain)
            {
                domain->toPDDL(std::cout);
                std::cout << std::endl;
            }

            // parse problem
            if (not parseFile(problem_file, problem_ast))
            {
                std::cerr << log::format("Failed to parse problem file %1%.", problem_file) << std::endl;
                return false;
            }

            // translate problem
            try
            {
                problem = toProblem(problem_ast, domain);
            }
            catch (const std::exception &e)
            {
                std::cerr << log::format("Caught exception translating problem: %1%", e.what()) << std::endl;
                return false;
            }

            if (output.problem)
            {
                problem->toPDDL(std::cout);
                std::cout << std::endl;
            }

            // Use special heuristic if requested
            if (astar.planning_heuristic)
            {
                auto copy = fd.search;
                astar.options.heuristic =
                    PlannerHeuristicFactory::make(problem, [copy](const ProblemPtr &problem) {
                        return std::make_shared<FastDownwardPlanner>(problem, copy);
                    });
            }

            // setup planner
            switch (planner_type)
            {
                case Z3:
                    planner = std::make_shared<Z3Planner>(problem, z3.options);
                    break;
                case FD:
                    planner = std::make_shared<FastDownwardPlanner>(problem, fd.search);
                    break;
                case AS:
                    planner = std::make_shared<AStarPlanner>(problem, astar.options);
                    break;
                default:
                    std::cerr << "Unknown planner type. Exiting." << std::endl;
                    return false;
            }

            return true;
        }

        bool doPlanning()
        {
            bool r;
            plan = std::make_shared<Plan>(problem);

            {
                boost::timer::cpu_timer t;
                r = planner->solve(*plan, timeout);

                if (output.time)
                {
                    t.stop();
                    std::cout << io::getTiming(t) << std::endl;
                }
            }

            if (r)
            {
                std::cerr << "Planning succeeded!" << std::endl;

                if (output.plan)
                    plan->toString(std::cout, output.step_index) << std::endl;

                if (output.steps)
                {
                    unsigned int i = 0;
                    std::cout << "Step " << i++ << std::endl;
                    problem->start->toPDDL(std::cout);
                    std::cout << std::endl;

                    for (const auto &step : plan->getSteps())
                    {
                        std::cout << "Step " << i++ << " " << step << std::endl;
                        step.state->toPDDL(std::cout);
                        std::cout << std::endl;
                    }
                }

                if (output.graph)
                {
                    graph = std::make_shared<PlanDependencyGraph>(plan);

                    if (dependency.tooltips)
                        graph->computeNodeStates();

                    PlanDependencyGraph::DotFileOptions dot;
                    if (dependency.leaf_dist)
                        dot.ranking = PlanDependencyGraph::DotFileOptions::Rank::LEAFDIST;

                    dot.edgeToolTips = dependency.tooltips;
                    dot.stateToolTips = dependency.tooltips;
                    dot.drawStart = dependency.start_goal;
                    dot.drawEnd = dependency.start_goal;

                    if (graph_file.empty())
                        graph->toDotFile(std::cout, dot);

                    else
                    {
                        std::ofstream out;
                        io::createFile(out, graph_file);
                        graph->toDotFile(out, dot);
                        out.close();
                    }
                }
            }
            else
                std::cerr << "Planning failed!" << std::endl;

            if (planner_type == Z3 and z3.output.model)
                std::dynamic_pointer_cast<Z3Planner>(planner)->printModel(std::cout);

            if (planner_type == Z3 and z3.output.smt)
                std::dynamic_pointer_cast<Z3Planner>(planner)->printSMT2(std::cout);

            return r;
        }
    };  // namespace pddlboat
}  // namespace pddlboat

#endif
