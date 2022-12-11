/* Author: Zachary Kingston */

#ifndef PDDLBOAT_SOLVER_FFPLANNER_
#define PDDLBOAT_SOLVER_FFPLANNER_

#include <pddlboat/utility.hpp>
#include <pddlboat/solver/planner.hpp>
#include <optional>

namespace pddlboat
{
    /** \brief A planner that uses the fast forward planning system,
     * https://fai.cs.uni-saarland.de/hoffmann/ff.html.
     * Calls the fast forward executable from the fast forward submodule.
     */
    class FastForwardPlanner : public Planner
    {
    public:
        struct Options
        {
            Options(){};
        };

        /** \brief Constructor.
         *  \param[in] problem Problem to solve.
         *  \param[in] search Search method to use.
         */
        FastForwardPlanner(const ProblemPtr &problem, const Options &options = Options());

        /** \brief Destructor. Cleans up generated files.
         */
        ~FastForwardPlanner();

        /** \brief Returns the supported domain requirements.
         *  \return The supported requirements.
         */
        Requirements::Keyword getSupportedRequirements() const override;

        /** \brief Update domain or problem PDDL files that the fast forward executable reads. Call this
         * method whenever the problem or domain is updated.
         *  \param[in] domain Update the domain file.
         *  \param[in] problem Update the problem file.
         */
        void update(bool domain = true, bool problem = true);

        /** \brief Solve the planning problem.
         *  \param[out] plan Output plan.
         *  \return True on success, false on failure.
         */
        bool solve(Plan &plan) override;

        /** \brief Solve the planning problem.
         *  \param[out] plan Found plan.
         *  \return True on success, false on failure.
         */
        bool solve(Plan &plan, unsigned int timeout) override;

    private:
        /** \brief Retrieve or initialize the command for calling fast downward.
         * \return The command to use
         */
        const std::string &getSolverCommand();

        /** \brief Return the correct path to the solver executable
         * \return The path to the executable
         */
        std::string getSolver();

        /** \brief Generate the command for calling fast forward.
         *  \return The command to use.
         */
        std::string generateSolverCommand() const;

        /** \brief Get the directory where the binary resides
         *  \return Path to the binary directory
         */
        const std::string &getDirectory() const;

        const std::string temporary{"/tmp"};  ///< Scratch directory to generate intermediate files.

        const std::string domain_file_template{"pddlboat_domain_%1%.pddl"};    ///< Template domain name.
        const std::string problem_file_template{"pddlboat_problem_%1%.pddl"};  ///< Template problem name.

        const std::string uuid;          ///< Unique ID for this solver.
        const std::string domain_file;   ///< Domain file for this solver.
        const std::string problem_file;  ///< Problem file for this solver.

        std::optional<std::string> solver{std::nullopt};          ///< Solver executable.
        std::optional<std::string> solver_command{std::nullopt};  ///< Command to use for solving.

        const Options options;                  ///< Options.
        const static std::string ff_directory;  ///< Directory where FD executable lives.
    };
}  // namespace pddlboat

#endif
