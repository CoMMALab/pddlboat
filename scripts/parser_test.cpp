/** Author: Zachary Kingston */

#include <iostream>

#include <boost/timer/timer.hpp>
#include <pddlboat/parser/parser.h>

void test(const std::string &file, bool result)
{
    boost::timer::auto_cpu_timer t;

    pddlboat::ast::Domain domain;
    bool r = pddlboat::parseFile(std::string(PDDLBOAT_RESOURCE_DIRECTORY "/") + file, domain);

    std::cout << file << ": " << ((r == result) ? "OK" : "FAILED") << std::endl;
    if (r)
        std::cout << domain.toString() << std::endl << std::endl;
}

int main(int argc, char **argv)
{
    test("domain_01.pddl", true);
    test("domain_02.pddl", true);
    test("domain_03.pddl", true);
    test("domain_04.pddl", true);
    test("domain_05.pddl", true);
    test("domain_06.pddl", true);
    test("domain_07.pddl", true);
    test("domain_08.pddl", true);
    test("domain_09.pddl", true);
    test("domain_10.pddl", true);
    test("domain_11.pddl", true);
    test("domain_12.pddl", true);
    test("domain_13.pddl", true);
    test("domain_14.pddl", true);
    test("domain_15.pddl", true);
    test("domain_16.pddl", true);
    test("domain_17.pddl", true);
    test("domain_18.pddl", true);
    test("domain_19.pddl", true);
    test("domain_20.pddl", true);
    test("domain_21.pddl", true);
    test("domain_22.pddl", true);
    test("domain_23.pddl", true);
    test("domain_24.pddl", true);
    test("domain_25.pddl", true);
    test("domain_26.pddl", true);
    test("blocks/blockworld.pddl", true);
    return 0;
}
