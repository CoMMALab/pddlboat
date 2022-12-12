#include <pddlboat/solver/helper.hpp>
#include <iostream>

int main(int argc, char **argv)
{
    pddlboat::ProblemSetup helper("PDDLBoat Planning Test");
    if (not helper.parseArguments(argc, argv))
        return 1;

    if (helper.planner_type != pddlboat::SYMK)
    {
        std::cout << "Reconfiguring planner to sym-k\n";
        helper.planner_type = pddlboat::SYMK;
    }

    helper.initialize();
    helper.doAnytimePlanning();
    return 0;
}
