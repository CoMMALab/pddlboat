
/* Author: Zachary Kingston */

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <sstream>
#include <iostream>

#include <pddlboat/utility.hpp>
#include <pddlboat/parser/parser.hpp>
#include <pddlboat/solver/ffplanner.hpp>

using namespace pddlboat;

const std::string FastForwardPlanner::FastForwardPlanner::ff_directory = io::getResourceDirectory(
    PDDLBOAT_FASTFORWARD_LOCATION);  ///< Directory where the fast forward binary resides

FastForwardPlanner::FastForwardPlanner(const ProblemPtr &problem, const Options &options)
  : Planner(problem)
  , uuid(io::generateUUID())
  , domain_file(temporary + "/" + log::format(domain_file_template, uuid))
  , problem_file(temporary + "/" + log::format(problem_file_template, uuid))
  , options(options)
{
    // checkRequirements();
}

FastForwardPlanner::~FastForwardPlanner()
{
    io::deleteFile(domain_file);
    io::deleteFile(problem_file);
}

Requirements::Keyword FastForwardPlanner::getSupportedRequirements() const
{
    auto reqs = Requirements::ADL;
    return reqs;
}

std::string FastForwardPlanner::generateSolverCommand() const
{
    std::stringstream cs;
    cs << *solver;

    // Output file
    cs << " -o " << domain_file;
    cs << " -f " << problem_file;
    cs << " -i 0";
    return cs.str();
}

void FastForwardPlanner::update(bool domain, bool problem)
{
    if (domain)
    {
        std::ofstream domain_out;
        io::createFile(domain_out, domain_file);
        problem_->domain->toPDDL(domain_out);
        domain_out.close();
    }

    if (problem)
    {
        std::ofstream problem_out;
        io::createFile(problem_out, problem_file);
        problem_->toPDDL(problem_out);
        problem_out.close();
    }
}

bool FastForwardPlanner::solve(Plan &plan)
{
    return solve(plan, 0);
}

bool FastForwardPlanner::solve(Plan &plan, unsigned int timeout)
{
    update();

    boost::timer::cpu_timer timer;

    auto output = io::runCommand(getSolverCommand());
    if (output == std::nullopt)
        return false;

    // Parse output file
    ast::Plan parsed;

    if (parseString(*output, parsed))
    {
        for (const auto &action : parsed.actions)
        {
            std::vector<std::string> objects;
            for (const auto &tl : action.variables)
                for (const auto &t : tl.types)
                    objects.emplace_back(boost::algorithm::to_lower_copy(t));

            plan.addStep(boost::algorithm::to_lower_copy(action.name), objects);
        }

        plan.setTimeTaken("solve", io::getElapsedMilliseconds(timer) / 1000.);
        return plan.validate();
    }
    else
    {
        std::cout << *output << std::endl;
    }

    return false;
}

const std::string &FastForwardPlanner::getDirectory() const
{
    return ff_directory;
}

std::string FastForwardPlanner::getSolver()
{
    return io::resolvePath(log::format("%1%/ff", getDirectory()));
}

const std::string &FastForwardPlanner::getSolverCommand()
{
    if (not solver_command)
    {
        if (not solver)
        {
            solver = getSolver();
        }

        solver_command = generateSolverCommand();
    }

    return *solver_command;
}
