#include <pddlboat/solver/helper.h>

int main(int argc, char **argv)
{
    pddlboat::ProblemSetup helper("PDDLBoat Planning Test");
    if (not helper.parseArguments(argc, argv))
        return 1;

    helper.initialize();
    helper.doPlanning();

    return 0;
}
