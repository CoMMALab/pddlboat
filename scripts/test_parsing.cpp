/** Author: Zachary Kingston */

#include <iostream>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>

#include <pddlboat/utility.hpp>
#include <pddlboat/parser/parser.hpp>
#include <pddlboat/parser/translator.hpp>
#include <pddlboat/solver/domain.hpp>

namespace po = boost::program_options;
namespace pd = pddlboat;

bool convert, print;

void testDomain(const std::string &file)
{
    boost::timer::auto_cpu_timer t;

    std::cout << "Parsing file: " << file << std::endl;

    pd::ast::Domain domain_ast;
    bool r = pd::parseFile(io::getResourceDirectory(PDDLBOAT_RESOURCE_DIRECTORY) "/" + file, domain_ast);
    if (not r)
        std::cout << "------------- FAILED! -------------" << std::endl << std::endl;

    if (print)
    {
        if (r)
            domain_ast.toString(std::cout) << std::endl << std::endl;
    }

    if (convert)
    {
        pd::DomainPtr domain;
        try
        {
            domain = pd::toDomain(domain_ast);
            domain->toPDDL(std::cout);
            std::cout << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << pd::log::format("Caught exception translating domain: %1%", e.what()) << std::endl;
        }
    }
}

void testProblem(const std::string &file)
{
    boost::timer::auto_cpu_timer t;

    pd::ast::Problem problem;
    bool r = pd::parseFile(std::string(PDDLBOAT_RESOURCE_DIRECTORY "/") + file, problem);
    if (r)
        problem.toString(std::cout) << std::endl << std::endl;

}

int main(int argc, char **argv)
{
    int filenum;
    po::options_description desc("PDDLBoat - Parser Testing Demo");
    desc.add_options()                                             //
        ("help,h", "Produces this help message")                   //
        ("print,p", po::bool_switch(&print),                       //
         "Print the AST of the intermediate parsed file.")         //
        ("convert,c", po::bool_switch(&convert),                   //
         "Print the PDDL of the translated parsed file.")          //
        ("number,n", po::value<int>(&filenum)->default_value(-1),  //
         "If set, does a specific numbered file.")                 //
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    po::notify(vm);

    if (filenum > 0)
        testDomain(pd::log::format("parsing/domain_%1$02u.pddl", filenum));
    else
    {
        for (unsigned int i = 1; i <= 29; ++i)
            testDomain(pd::log::format("parsing/domain_%1$02u.pddl", i));
    }

    return 0;
}
