/* Author: Zachary Kingston */

#ifndef PDDLBOAT_SOLVER_FDPLANNER_
#define PDDLBOAT_SOLVER_FDPLANNER_

#include <pddlboat/utility.h>
#include <pddlboat/solver/planner.h>

namespace pddlboat
{
    class FastDownwardPlanner : public Planner
    {
    public:
        enum Search
        {
            ASTAR,       // A*
            LAZYGREEDY,  // Lazy greedy best-first search with preferred operators and queue alternation
            FDAT         // Fast Downward Autotune 2
        };

        FastDownwardPlanner(const ProblemPtr &problem, const Search &search = LAZYGREEDY);
        ~FastDownwardPlanner();

        void update(bool domain = true, bool problem = true) const;
        bool solve(Plan &plan) override;

    private:
        const Search search{LAZYGREEDY};

        const std::string directory{PDDLBOAT_FASTDOWNWARD_LOCATION};
        const std::string temporary{"/tmp"};
        const std::string domain_file_template{"pddlboat_domain_%1%.pddl"};
        const std::string problem_file_template{"pddlboat_problem_%1%.pddl"};
        const std::string sas_file_template{"pddlboat_sas_%1%"};
        const std::string output_file_template{"pddlboat_out_%1%"};

        const std::string uuid;
        const std::string translator;
        const std::string solver;
        const std::string domain_file;
        const std::string problem_file;
        const std::string sas_file;
        const std::string output_filename;
        const std::string output_file;

        std::string solver_command;
        std::string translator_command;
    };

}  // namespace pddlboat

#endif
