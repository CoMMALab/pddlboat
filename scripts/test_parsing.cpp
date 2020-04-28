/** Author: Zachary Kingston */

#include <iostream>

#include <boost/timer/timer.hpp>

#include <pddlboat/utility.h>
#include <pddlboat/parser/parser.h>

void testDomain(const std::string &file)
{
    boost::timer::auto_cpu_timer t;

    pddlboat::ast::Domain domain;
    bool r = pddlboat::parseFile(std::string(PDDLBOAT_RESOURCE_DIRECTORY "/") + file, domain);
    if (r)
        domain.toString(std::cout) << std::endl << std::endl;
}

void testProblem(const std::string &file)
{
    boost::timer::auto_cpu_timer t;

    pddlboat::ast::Problem problem;
    bool r = pddlboat::parseFile(std::string(PDDLBOAT_RESOURCE_DIRECTORY "/") + file, problem);
    if (r)
        problem.toString(std::cout) << std::endl << std::endl;
}

int main(int argc, char **argv)
{
    for (unsigned int i = 1; i <= 27; ++i)
        testDomain(pddlboat::log::format("parsing/domain_%1$02u.pddl", i));

    // testDomain("blocks/blockworld.pddl");
    // for (unsigned int i = 1; i <= 21; ++i)
    //     testProblem(pddlboat::log::format("blocks/p%1$02u.pddl", i));

    return 0;
}
