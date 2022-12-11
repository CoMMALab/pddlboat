#include <iostream>
#include <pddlboat/parser/parser.h>
#include <pddlboat/parser/translator.h>
#include <pddlboat/parser/ast.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/domain.h>
#include <pddlboat/solver/planner.h>
#include <pddlboat/solver/fdplanner.h>
#include <pddlboat/solver/z3planner.h>
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

void printSteps(pd::PlanPtr &plan)
{
    unsigned int i = 0;
    std::cout << "Step " << i++ << std::endl;
    plan->getProblem()->start->toPDDL(std::cout);
    std::cout << std::endl;

    for (const auto &step : plan->getSteps())
    {
        std::cout << "Step " << i++ << " " << step << std::endl;
        step.state->toPDDL(std::cout);
        std::cout << std::endl;
    }
}

int main(int argc, char **argv)
{
    const auto domain_file = std::string(PDDLBOAT_RESOURCE_DIRECTORY) + "/optimize_tamp/"
                                                                        "pick_place_reachability.pddl";
    const auto problem_file = std::string(PDDLBOAT_RESOURCE_DIRECTORY) + "/optimize_tamp/op4_problem.pddl";

    auto domain = parse_domain(domain_file);
    if (not domain)
        return 1;

    auto problem = parse_problem(domain, problem_file);
    if (not problem)
        return 1;

    //
    // Solve the initial problem
    //
    pd::Z3Planner::Options options;
    // options.dump_clauses = true;
    // options.horizon.max = 4;
    auto task_planner = std::make_shared<pd::Z3Planner>(problem, options);

    // Block put-down limited expression for all objects without goal.
    auto pd_action = domain->getAction("put-down-limited");

    // Need to loop over robots (not really), objects and limited locations (except for the one related to the
    // goal of that object)
    auto robots = {std::string("thing1")};
    auto limited_locations = {std::string("ta-eb"), std::string("ta-cb"), std::string("ta-bb"),
                              std::string("ta-ab"), std::string("ta-bc")};
    auto blocks = {std::string("bx"), std::string("by"), std::string("bz"), std::string("bw")};

    std::vector<std::pair<const pd::ActionPtr &, const pd::Assignment &>> actions;
    auto expr = pd::makeAnd({});
    std::vector<pd::Assignment> assignment_v;
    for (const auto &robot : robots)
    {
        for (const auto &block : blocks)
        {
            for (const auto &location : limited_locations)
            {
                if (not(block == "by" and location == "ta-eb"))
                {
                    std::cout << "Size of actions " << actions.size() << std::endl;
                    pd::Assignment amnt;
                    amnt.assign(*problem, "r", robot);
                    amnt.assign(*problem, "b", block);
                    amnt.assign(*problem, "l", location);

                    assignment_v.push_back(amnt);

                    //                      action;
                    //                     const auto &action = std::make_pair(pd_action,
                    //                     pd::Assignment(amnt)); actions.push_back(std::pair<const
                    //                     pd::ActionPtr&, const pd::Assignment&>(pd_action,
                    //                     pd::Assignment(amnt))); actions.emplace_back(action);

                    amnt.toString();
                    std::cout << std::endl;
                }
            }
        }
    }

    std::cout << "Outside" << std::endl;
    std::cout << "Size of actions " << actions.size() << std::endl;
    std::cout << std::endl;

    for (const auto &amnt : assignment_v)
    {
        actions.push_back(std::pair<const pd::ActionPtr &, const pd::Assignment &>(pd_action, amnt));
    }

    std::cout << "Out here " << std::endl;

    task_planner->blockExpressionActions(actions, expr, {});

    // auto task_planner = std::make_shared<pd::AStarPlanner>(problem);

    std::cout << "Plan:" << std::endl;
    auto plan = std::make_shared<pd::Plan>(problem);
    if (not task_planner->solve(*plan))
    {
        std::cerr << "Failed to solve!" << std::endl;
        return 1;
    }
    std::cout << *plan << std::endl;
    // printSteps(plan);

    //
    // Block first action
    //
    auto step1 = plan->getStep(0);

    std::cout << "Blocking: ";
    step1.toString(std::cout) << std::endl << std::endl;
    task_planner->blockStateAction(step1.action, step1.assignment, step1.prior);

    std::cout << "New plan after blocking:" << std::endl;
    plan = std::make_shared<pd::Plan>(problem);
    if (not task_planner->solve(*plan))
    {
        std::cerr << "Failed to solve!" << std::endl;
        return 1;
    }
    std::cout << *plan << std::endl;
    // printSteps(plan);

    //
    // Block second action
    //
    auto step2 = plan->getStep(1);

    std::cout << "Blocking: ";
    step2.toString(std::cout) << std::endl << std::endl;
    task_planner->blockStateAction(step2.action, step2.assignment, step2.prior);

    std::cout << "New plan after blocking:" << std::endl;
    plan = std::make_shared<pd::Plan>(problem);
    if (not task_planner->solve(*plan))
    {
        std::cerr << "Failed to solve!" << std::endl;
        return 1;
    }
    std::cout << *plan << std::endl;
    // printSteps(plan);

    step2 = plan->getStep(1);

    std::cout << "Blocking: ";
    step2.toString(std::cout) << std::endl << std::endl;
    task_planner->blockStateAction(step2.action, step2.assignment, step2.prior);

    std::cout << "New plan after blocking:" << std::endl;
    plan = std::make_shared<pd::Plan>(problem);
    if (not task_planner->solve(*plan))
    {
        std::cerr << "Failed to solve!" << std::endl;
        return 1;
    }
    std::cout << *plan << std::endl;

    step2 = plan->getStep(1);

    std::cout << "Blocking: ";
    step2.toString(std::cout) << std::endl << std::endl;
    task_planner->blockStateAction(step2.action, step2.assignment, step2.prior);

    std::cout << "New plan after blocking:" << std::endl;
    plan = std::make_shared<pd::Plan>(problem);
    if (not task_planner->solve(*plan))
    {
        std::cerr << "Failed to solve!" << std::endl;
        return 1;
    }
    std::cout << *plan << std::endl;
}
