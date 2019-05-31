/** Author: Zachary Kingston */

#include <iostream>

#include <boost/timer/timer.hpp>
#include <pddlboat/parser/parser.h>

void testDomain(const std::string &file, bool result)
{
    boost::timer::auto_cpu_timer t;

    pddlboat::ast::Domain domain;
    bool r = pddlboat::parseFile(std::string(PDDLBOAT_RESOURCE_DIRECTORY "/") + file, domain);

    std::cout << file << ": " << ((r == result) ? "OK" : "FAILED") << std::endl;
    if (r)
        std::cout << domain.toString() << std::endl << std::endl;
}

void testProblem(const std::string &file, bool result)
{
    boost::timer::auto_cpu_timer t;

    pddlboat::ast::Problem problem;
    bool r = pddlboat::parseFile(std::string(PDDLBOAT_RESOURCE_DIRECTORY "/") + file, problem);

    std::cout << file << ": " << ((r == result) ? "OK" : "FAILED") << std::endl;
    if (r)
        std::cout << problem.toString() << std::endl << std::endl;
}

int main(int argc, char **argv)
{
    testDomain("domain_01.pddl", true);
    testDomain("domain_02.pddl", true);
    testDomain("domain_03.pddl", true);
    testDomain("domain_04.pddl", true);
    testDomain("domain_05.pddl", true);
    testDomain("domain_06.pddl", true);
    testDomain("domain_07.pddl", true);
    testDomain("domain_08.pddl", true);
    testDomain("domain_09.pddl", true);
    testDomain("domain_10.pddl", true);
    testDomain("domain_11.pddl", true);
    testDomain("domain_12.pddl", true);
    testDomain("domain_13.pddl", true);
    testDomain("domain_14.pddl", true);
    testDomain("domain_15.pddl", true);
    testDomain("domain_16.pddl", true);
    testDomain("domain_17.pddl", true);
    testDomain("domain_18.pddl", true);
    testDomain("domain_19.pddl", true);
    testDomain("domain_20.pddl", true);
    testDomain("domain_21.pddl", true);
    testDomain("domain_22.pddl", true);
    testDomain("domain_23.pddl", true);
    testDomain("domain_24.pddl", true);
    testDomain("domain_25.pddl", true);
    testDomain("domain_26.pddl", true);
    testDomain("blocks/blockworld.pddl", true);
    testProblem("blocks/problem_01.pddl", true);
    return 0;
}
