/* Author: Wil Thomason */

#include <limits>
#include <sstream>
#include <iostream>
#include "pddlboat/solver/fdplanner.h"

#include <pddlboat/utility.h>
#include <pddlboat/parser/parser.h>
#include <pddlboat/solver/symkplanner.h>

using namespace pddlboat;

const std::string SymKPlanner::SymKPlanner::symk_directory =
    PDDLBOAT_SYMK_LOCATION;  ///< Directory where the sym-k fast downward binary resides

SymKPlanner::SymKPlanner(const ProblemPtr &problem, SolutionSelector selector, unsigned int k, float q,
                         bool run_preprocessor)
  : FastDownwardPlanner(problem)
  , k{k}
  , q{q}
  , selector{selector}
  , preprocessor{io::resolvePath(log::format("%1%/preprocess", getDirectory()))}
  , run_preprocessor{run_preprocessor}
{
}

std::string SymKPlanner::generateSolverCommand() const
{
    std::stringstream cs;
    cs << *solver;

    // Output file
    cs << " --internal-plan-file " << output_file;
    switch (selector)
    {
        case SolutionSelector::TOPK:
            cs << " --search \"symk-bd(plan_selection=top_k(num_plans=" << k << "))\"";
            break;
        case SolutionSelector::TOPQ:
            if (q == std::numeric_limits<float>::infinity())
            {
                cs << " --search \"symq-bd(plan_selection=top_k(num_plans=" << k << "),quality=infinity)\"";
            }
            else
            {
                cs << " --search \"symq-bd(plan_selection=top_k(num_plans=" << k << "),quality=" << q
                   << ")\"";
            }

            break;
    }

    // Tell command what SAS file to use
    cs << " < " << sas_file;
    return cs.str();
}

std::string SymKPlanner::generateAnytimeSolverCommand()
{
    if (not solver)
    {
        solver = getSolver();
    }

    std::stringstream cmd;
    cmd << *solver << " --internal-plan-file " << output_file
        << " --search \"symk-bd(plan_selection=top_k(num_plans=infinity))\""
        << " < " << sas_file;
    return cmd.str();
}

bool SymKPlanner::multiSolve(PlanSet &plans)
{
    if (not runSolver())
    {
        return false;
    }

    // Parse output files
    auto matches = io::glob("/tmp", output_filename + ".*");
    if (matches)
    {
        auto files = *matches;
        if (files.empty())
        {
            return false;
        }

        std::sort(files.begin(), files.end());
        ast::Plan parsed;
        for (const auto &file : files)
        {
            parseFile(file, parsed);
            auto &plan = plans.newPlan();
            for (const auto &action : parsed.actions)
            {
                std::vector<std::string> objects;
                for (const auto &tl : action.variables)
                    for (const auto &t : tl.types)
                        objects.emplace_back(t);

                plan.addStep(action.name, objects);
            }

            if (not plan.validate())
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}

PlanWatcher SymKPlanner::anytimeSolve(unsigned int timeout, bool start_immediately)
{
    if (not translator_command)
    {
        // This signifies that we haven't run update() ever
        update();
    }

    return PlanWatcher("/tmp", output_filename, problem_, generateAnytimeSolverCommand(), timeout,
                       start_immediately);
}

const std::string &SymKPlanner::getDirectory() const
{
    return symk_directory;
}

std::string SymKPlanner::getSolver()
{
    return io::resolvePath(log::format("%1%/downward", getDirectory()));
}

Requirements::Keyword SymKPlanner::getSupportedRequirements() const
{
    return FastDownwardPlanner::getSupportedRequirements() | Requirements::TOPK | Requirements::TOPQ;
}

bool SymKPlanner::preprocess() const
{
    if (run_preprocessor)
    {
        auto tr = io::runCommand(generatePreprocessorCommand());
        return tr != std::nullopt;
    }

    return true;
}

std::string SymKPlanner::generatePreprocessorCommand() const
{
    std::stringstream cs;
    cs << preprocessor << " < " << sas_file;
    return cs.str();
}
