#include <pddlboat/solver/helper.hpp>

int main(int argc, char **argv)
{
    pddlboat::ProblemSetup helper("PDDLBoat Planning Test");
    if (not helper.parseArguments(argc, argv))
        return 1;

    helper.initialize();
    if (helper.planner_type == pddlboat::SYMK)
    {
        helper.doMultiPlanning();
    }
    else
    {
        helper.doPlanning();
    }

    return 0;
}
