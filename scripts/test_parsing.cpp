/** Author: Zachary Kingston */

#include <iostream>

#include <boost/timer/timer.hpp>
#include <pddlboat/parser/parser.h>
#include <pddlboat/parser/translator.h>
#include <pddlboat/solver/domain.h>

int main(int argc, char **argv)
{
    boost::timer::auto_cpu_timer t;

    if (argc < 2)
    {
        std::cerr << "Must provide file name!" << std::endl;
        return 1;
    }

    const std::string file(argv[1]);
    pddlboat::ast::Domain domain;
    bool r = pddlboat::parseFile(file, domain);
    std::cout << file << ": " << (r ? "OK" : "FAILED") << std::endl;
    if (r)
    {
        auto parsed = pddlboat::toDomain(domain);
        parsed->toPDDL();
    }

    return 0;
}
