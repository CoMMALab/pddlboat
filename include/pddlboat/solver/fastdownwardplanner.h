#ifndef PDDLBOAT_SOLVER_FASTDOWNWARDPLANNER_
#define PDDLBOAT_SOLVER_FASTDOWNWARDPLANNER_

#include <pddlboat/utility.h>
#include <pddlboat/parser/parser.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/domain.h>

namespace pddlboat
{
    class FastDownwardPlanner : public Planner
    {
    public:
        FastDownwardPlanner(const ProblemPtr &problem)
          : Planner(problem)
          , uuid(io::generateUUID())
          , translator(io::resolvePath(log::format("%1%/translate/translate.py", directory)))
          , solver(io::resolvePath(log::format("%1%/downward", directory)))
          , domain_file(log::format(domain_file_template, uuid))
          , problem_file(log::format(problem_file_template, uuid))
          , sas_file(log::format(sas_file_template, uuid))
          , output_file(log::format(output_file_template, uuid))
          // , output_file_1(output_file + ".1")
          , output_file_1(output_file)
        {
            std::stringstream cs;
            cs << solver;

            // Output file
            cs << " --internal-plan-file " << output_file;

            // Lazy greedy best-first search with preferred operators and the queue alternation method
            cs << " --evaluator \"hff=ff()\"";
            cs << " --evaluator \"hcea=cea()\"";
            cs << " --search \"lazy_greedy([hff, hcea], preferred=[hff, hcea])\"";

            // Fast Downward Autotune 2
            // // Evaluators
            // cs << " --evaluator \"hcea=cea(transform=adapt_costs(plusone))\"";
            // cs << " --evaluator \"hcg=cg(transform=adapt_costs(one))\"";
            // cs << " --evaluator \"hgc=goalcount(transform=adapt_costs(plusone))\"";
            // cs << " --evaluator \"hff=ff()\"";

            // // Autotune Fast Downward Search
            // cs << " --search ";
            // cs << "\"";
            // cs << "iterated([";
            // cs << "ehc(hcea,preferred=[hcea],preferred_usage=0,cost_type=normal),";
            // cs << "lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),";
            // cs << "single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),";
            // cs << "single(sum([weight(g(),2),weight(hcg,3)])),";
            // cs << "single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),";
            // cs << "single(sum([weight(g(),2),weight(hcea,3)])),";
            // cs << "single(sum([weight(g(),2),weight(hcea,3)]),pref_only=true),";
            // cs << "single(sum([weight(g(),2),weight(hgc,3)])),";
            // cs << "single(sum([weight(g(),2),weight(hgc,3)]),pref_only=true)],";
            // cs << "boost=200),";
            // cs << "preferred=[hcea,hgc],reopen_closed=false,cost_type=one),";
            // cs << "lazy(alt([single(sum([g(),weight(hff,5)])),";
            // cs << "single(sum([g(),weight(hff,5)]),pref_only=true),";
            // cs << "single(sum([g(),weight(hcg,5)])),";
            // cs << "single(sum([g(),weight(hcg,5)]),pref_only=true),";
            // cs << "single(sum([g(),weight(hcea,5)])),";
            // cs << "single(sum([g(),weight(hcea,5)]),pref_only=true),";
            // cs << "single(sum([g(),weight(hgc,5)])),";
            // cs << "single(sum([g(),weight(hgc,5)]),pref_only=true)],";
            // cs << "boost=5000),";
            // cs << "preferred=[hcea,hgc],reopen_closed=true,cost_type=normal),";
            // cs << "lazy(alt([single(sum([g(),weight(hff,2)])),";
            // cs << "single(sum([g(),weight(hff,2)]),pref_only=true),";
            // cs << "single(sum([g(),weight(hcg,2)])),";
            // cs << "single(sum([g(),weight(hcg,2)]),pref_only=true),";
            // cs << "single(sum([g(),weight(hcea,2)])),";
            // cs << "single(sum([g(),weight(hcea,2)]),pref_only=true),";
            // cs << "single(sum([g(),weight(hgc,2)])),";
            // cs << "single(sum([g(),weight(hgc,2)]),pref_only=true)],";
            // cs << "boost=1000),";
            // cs << "preferred=[hcea,hgc],reopen_closed=true,cost_type=one)";
            // cs << "],repeat_last=true,continue_on_fail=true)";
            // cs << "\"";

            // Tell command what SAS file to use
            cs << " < " << sas_file;

            solver_command = cs.str();

            std::stringstream ts;
            ts << translator;
            ts << " --sas-file " << sas_file << " " << domain_file << " " << problem_file;
            translator_command = ts.str();

            update();
        }

        ~FastDownwardPlanner()
        {
            io::deleteFile(domain_file);
            io::deleteFile(problem_file);
            io::deleteFile(sas_file);
            io::deleteFile(output_file_1);
        }

        void update(bool domain = true, bool problem = true) const
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

        bool solve(Plan &plan) override
        {
            auto pr = io::runCommand(solver_command);
            if (not pr)
                return false;

            // Parse output file
            ast::Plan parsed;
            parseFile(output_file_1, parsed);

            for (const auto &action : parsed.actions)
            {
                std::vector<std::string> objects;
                for (const auto &tl : action.variables)
                    for (const auto &t : tl.types)
                        objects.emplace_back(t);

                plan.addStep(action.name, objects);
            }

            return true;
        }

    private:
        const std::string directory{PDDLBOAT_FASTDOWNWARD_LOCATION};
        const std::string domain_file_template{"/tmp/pddlboat_domain_%1%.pddl"};
        const std::string problem_file_template{"/tmp/pddlboat_problem_%1%.pddl"};
        const std::string sas_file_template{"/tmp/pddlboat_sas_%1%"};
        const std::string output_file_template{"/tmp/pddlboat_out_%1%"};

        const std::string uuid;
        const std::string translator;
        const std::string solver;
        const std::string domain_file;
        const std::string problem_file;
        const std::string sas_file;
        const std::string output_file;
        const std::string output_file_1;

        std::string solver_command;
        std::string translator_command;
    };

}  // namespace pddlboat

#endif
