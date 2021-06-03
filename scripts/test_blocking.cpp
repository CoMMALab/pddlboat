#include <pddlboat/solver/helper.h>

int main(int argc, char **argv)
{
    pddlboat::ProblemSetup helper("PDDLBoat Planning Test");
    if (not helper.parseArguments(argc, argv))
        return 1;

    helper.initialize();

    unsigned int solve_attempts = 0;
    while (helper.doPlanning())
    {
        solve_attempts++;

        // Block action
        if (solve_attempts <= 10 and helper.plan->getNumSteps() > 0)
        {
            std::size_t index = rand() % helper.plan->getNumSteps();
            helper.planner->blockPlanStep(*helper.plan, index);
        }
        else
            break;
    }

    return 0;
}
