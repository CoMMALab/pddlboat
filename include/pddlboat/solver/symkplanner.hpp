/* Author: Wil Thomason */

#ifndef PDDLBOAT_SOLVER_SYMKPLANNER_
#define PDDLBOAT_SOLVER_SYMKPLANNER_

#include <pddlboat/utility.hpp>
#include <pddlboat/solver/planner.hpp>
#include <pddlboat/solver/fdplanner.hpp>
#include <limits>

namespace pddlboat
{
    /** \brief A planner that uses the sym-k planning system (based on Fast Downward) for top-k and top-q
     * planning (https://github.com/speckdavid/symk) Calls sym-k with the recommended option combinations, as
     * well as forwarding Fast Downward arguments. Calls the Fast Downward executable from the symk submodule.
     */
    class SymKPlanner : public FastDownwardPlanner
    {
    public:
        /** \brief Search types for sym-k planning.
         */
        enum SolutionSelector
        {
            TOPK,  ///< top-k search
            TOPQ,  ///< top-q search
                   // TODO (Wil): Could look at supporting unordered plan generation or a custom TMP selector
        };

        /** \brief Constructor.
         *  \param[in] problem Problem to solve.
         *  \param[in] search Search type to perform
         *  \param[in] k unsigned int number of solutions to find
         *  \param[in] q float quality bound on solutions. Ignored for top-k
         *  \param[in] run_preprocessor bool switch to use the h2 preprocessor before search or not
         */
        SymKPlanner(const ProblemPtr &problem, SolutionSelector selector = SolutionSelector::TOPK,
                    unsigned int k = 5, float q = std::numeric_limits<float>::infinity(),
                    bool run_preprocessor = false);

        /** \brief Returns the supported domain requirements of the sym-k planner's configuration.
         *  \return The supported requirements.
         */
        Requirements::Keyword getSupportedRequirements() const override;

        /** \brief Solve the planning problem for k solutions
         *  \param[out] plans Found plans
         *  \return True on success, false on failure.
         */
        bool multiSolve(PlanSet &plans);

        /** \brief Enumerate solutions in an anytime manner
         *  \param timeout Desired timeout, in ms, for each check for new plans
         *  \param start_immediately Flag for whether the planner should be started immediately
         *  \return PlanWatcher instance which will retrieve plans as sym-k outputs them
         */
        PlanWatcher anytimeSolve(unsigned int timeout = 50, bool start_immediately = true);

    protected:
        /** \brief Generate the command for calling sym-k's search
         *  \return The command to use.
         */
        std::string generateSolverCommand() const override;

        /** \brief Generate the command for running sym-k directly for anytime search
         * \return The command to use
         */
        std::string generateAnytimeSolverCommand();

        /** \brief Generate the command for calling sym-k's preprocessor
         *  \return The command to use.
         */
        std::string generatePreprocessorCommand() const;

        /** \brief Return the correct path to the solver executable
         * \return The path to the executable
         */
        std::string getSolver() override;

        /** \brief Get the directory where the binary resides
         *  \return Path to the binary directory
         */
        const std::string &getDirectory() const override;

        /** \brief Run the preprocessor command, if any
         * \return True if the preprocessor succeeded, false otherwise
         */
        bool preprocess() const override;

    private:
        unsigned int k{1};
        float q{std::numeric_limits<float>::infinity()};
        const SolutionSelector selector{SolutionSelector::TOPK};  ///< Search method to use.
        const static std::string symk_directory;
        // NOTE: Maybe this should be static?
        const std::string preprocessor;
        const bool run_preprocessor;
    };

}  // namespace pddlboat

#endif
