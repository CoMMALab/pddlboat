/* Author: Zachary Kingston */

#include <sstream>
#include <iostream>

#include <pddlboat/utility.h>
#include <pddlboat/parser/parser.h>
#include <pddlboat/solver/fdplanner.h>

using namespace pddlboat;

const std::string FastDownwardPlanner::FastDownwardPlanner::directory =
    PDDLBOAT_FASTDOWNWARD_LOCATION;  ///< Directory where the fast downward

FastDownwardPlanner::FastDownwardPlanner(const ProblemPtr &problem, const Search &search)
  : Planner(problem)
  , search(search)
  , uuid(io::generateUUID())
  , translator(io::resolvePath(log::format("%1%/translate/translate.py", directory)))
  , solver(io::resolvePath(log::format("%1%/downward", directory)))
  , domain_file(temporary + "/" + log::format(domain_file_template, uuid))
  , problem_file(temporary + "/" + log::format(problem_file_template, uuid))
  , sas_file(temporary + "/" + log::format(sas_file_template, uuid))
  , output_filename(log::format(output_file_template, uuid))
  , output_file(temporary + "/" + output_filename)
  , solver_command(generateSolverCommand())
  , translator_command(generateTranslatorCommand())
{
    checkRequirements();
    // Fast Downward uses an intermediate format that needs to be generated
    update();
}

FastDownwardPlanner::~FastDownwardPlanner()
{
    io::deleteFile(domain_file);
    io::deleteFile(problem_file);
    io::deleteFile(sas_file);

    // Fast downward sometimes generates multiple output files as it refines its search
    auto matches = io::glob("/tmp", output_filename + ".*");
    if (matches)
    {
        for (const auto &match : *matches)
            io::deleteFile(match);
    }
}

Requirements::Keyword FastDownwardPlanner::getSupportedRequirements() const
{
    auto reqs = Requirements::ADL | Requirements::ACTION_COSTS;
    if (search != ASTAR)
        reqs = reqs | Requirements::DERIVED_PREDICATES;
    return reqs;
}

std::string FastDownwardPlanner::generateSolverCommand() const
{
    std::stringstream cs;
    cs << solver;

    // Output file
    cs << " --internal-plan-file " << output_file;

    // See http://www.fast-downward.org/PlannerUsage for more
    // Also, the aliases.py file in the driver provided with fast downward
    switch (search)
    {
        case ASTAR:
            // A* with landmark cut heuristic
            cs << " --search \"astar(lmcut())\"";
            break;

        case LAZYGREEDY:
            // Lazy greedy best-first search with preferred operators and the queue alternation method
            cs << " --evaluator \"hff=ff()\"";
            cs << " --evaluator \"hcea=cea()\"";
            cs << " --search \"lazy_greedy([hff, hcea], preferred=[hff, hcea])\"";
            break;

        case FDAT:
            // Fast Downward Autotune 2
            cs << " --evaluator \"hcea=cea(transform=adapt_costs(plusone))\"";
            cs << " --evaluator \"hcg=cg(transform=adapt_costs(one))\"";
            cs << " --evaluator \"hgc=goalcount(transform=adapt_costs(plusone))\"";
            cs << " --evaluator \"hff=ff()\"";

            cs << " --search ";
            cs << "\"";
            cs << "iterated([";
            cs << "ehc(hcea,preferred=[hcea],preferred_usage=0,cost_type=normal),";
            cs << "lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),";
            cs << "single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),";
            cs << "single(sum([weight(g(),2),weight(hcg,3)])),";
            cs << "single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),";
            cs << "single(sum([weight(g(),2),weight(hcea,3)])),";
            cs << "single(sum([weight(g(),2),weight(hcea,3)]),pref_only=true),";
            cs << "single(sum([weight(g(),2),weight(hgc,3)])),";
            cs << "single(sum([weight(g(),2),weight(hgc,3)]),pref_only=true)],";
            cs << "boost=200),";
            cs << "preferred=[hcea,hgc],reopen_closed=false,cost_type=one),";
            cs << "lazy(alt([single(sum([g(),weight(hff,5)])),";
            cs << "single(sum([g(),weight(hff,5)]),pref_only=true),";
            cs << "single(sum([g(),weight(hcg,5)])),";
            cs << "single(sum([g(),weight(hcg,5)]),pref_only=true),";
            cs << "single(sum([g(),weight(hcea,5)])),";
            cs << "single(sum([g(),weight(hcea,5)]),pref_only=true),";
            cs << "single(sum([g(),weight(hgc,5)])),";
            cs << "single(sum([g(),weight(hgc,5)]),pref_only=true)],";
            cs << "boost=5000),";
            cs << "preferred=[hcea,hgc],reopen_closed=true,cost_type=normal),";
            cs << "lazy(alt([single(sum([g(),weight(hff,2)])),";
            cs << "single(sum([g(),weight(hff,2)]),pref_only=true),";
            cs << "single(sum([g(),weight(hcg,2)])),";
            cs << "single(sum([g(),weight(hcg,2)]),pref_only=true),";
            cs << "single(sum([g(),weight(hcea,2)])),";
            cs << "single(sum([g(),weight(hcea,2)]),pref_only=true),";
            cs << "single(sum([g(),weight(hgc,2)])),";
            cs << "single(sum([g(),weight(hgc,2)]),pref_only=true)],";
            cs << "boost=1000),";
            cs << "preferred=[hcea,hgc],reopen_closed=true,cost_type=one)";
            cs << "],repeat_last=true,continue_on_fail=true)";
            cs << "\"";
            break;

        case LAMA:
            cs << " --if-unit-cost";
            cs << " --evaluator \"hlm=lmcount(lm_rhw(reasonable_orders=true),pref=true)\"";
            cs << " --evaluator \"hff=ff()\"";
            cs << " --search ";
            cs << "\"";
            cs << "iterated([";
            cs << "lazy_greedy([hff,hlm],preferred=[hff,hlm]),";
            cs << "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5),";
            cs << "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),";
            cs << "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),";
            cs << "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1)";
            cs << "],repeat_last=true,continue_on_fail=true)";
            cs << "\"";

            cs << " --if-non-unit-cost";
            cs << " --evaluator "
                  "\"hlm1=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(one),pref=true)\"";
            cs << " --evaluator \"hff1=ff(transform=adapt_costs(one))\"";
            cs << " --evaluator "
                  "\"hlm2=lmcount(lm_rhw(reasonable_orders=true),transform=adapt_costs(plusone),pref=true)\"";
            cs << " --evaluator \"hff2=ff(transform=adapt_costs(plusone))\"";
            cs << " --search ";
            cs << "\"";
            cs << "iterated([";
            cs << "lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],cost_type=one,reopen_closed=false),";
            cs << "lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],reopen_closed=false),";
            cs << "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),";
            cs << "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),";
            cs << "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),";
            cs << "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)";
            cs << "],repeat_last=true,continue_on_fail=true)";
            cs << "\"";
            cs << " --always";
            break;
    }

    // Tell command what SAS file to use
    cs << " < " << sas_file;

    return cs.str();
}

std::string FastDownwardPlanner::generateTranslatorCommand() const
{
    std::stringstream ts;
    ts << translator;
    ts << " --sas-file " << sas_file << " " << domain_file << " " << problem_file;

    return ts.str();
}

void FastDownwardPlanner::update(bool domain, bool problem) const
{
    // Serialize problem and domain

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

    if (domain or problem)
    {
        // Call fast downward
        auto tr = io::runCommand(translator_command);
        if (not tr)
            throw std::runtime_error("Failed to translate problem and domain!");
    }
}

bool FastDownwardPlanner::solve(Plan &plan) {
    return solve(plan);
}

bool FastDownwardPlanner::solve(Plan &plan, unsigned int timeout)
{
    auto pr = io::runCommand(solver_command);
    if (not pr)
        return false;

    // Parse output file
    ast::Plan parsed;

    auto matches = io::glob("/tmp", output_filename + ".*");
    if (matches)
    {
        auto files = *matches;
        std::sort(files.begin(), files.end());

        if (files.empty())
            return false;

        auto file = files.back();

        parseFile(file, parsed);

        for (const auto &action : parsed.actions)
        {
            std::vector<std::string> objects;
            for (const auto &tl : action.variables)
                for (const auto &t : tl.types)
                    objects.emplace_back(t);

            plan.addStep(action.name, objects);
        }
    }

    return true;
}
