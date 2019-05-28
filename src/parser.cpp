/** Author: Zachary Kingston */

#include <boost/config/warning_disable.hpp>

#include <pddlboat/utility.h>
#include <pddlboat/parser/adapted.h>
#include <pddlboat/parser/parser.h>

namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

using namespace pddlboat;

namespace parser
{
    ///
    /// Comments
    ///

    const auto comment = ";" >> *(x3::char_ - x3::eol) >> (x3::eol | x3::eoi);
    const auto skipper = ascii::space | comment;

    ///
    /// Identifier Rules
    ///

    /// Numbers
    const x3::rule<class number, std::string> number = "number";
    const auto number_def = x3::lexeme[ascii::digit >> *ascii::digit >> -('.' >> *ascii::digit)];

    BOOST_SPIRIT_DEFINE(number);

    /// Names
    const x3::rule<class name, std::string> name = "name";
    const x3::rule<class namelist, std::vector<std::string>> namelist = "namelist";
    const auto name_identifier = ascii::alpha >> *(ascii::alnum | x3::char_("_-"));
    const auto name_def = x3::lexeme[name_identifier];
    const auto namelist_def = +name;

    BOOST_SPIRIT_DEFINE(name);
    BOOST_SPIRIT_DEFINE(namelist);

    /// Variables
    const x3::rule<class variable, std::string> variable = "variable";
    const x3::rule<class variablelist, std::vector<std::string>> variablelist = "variablelist";
    const auto variable_identifier = '?' >> name_identifier;
    const auto variable_def = x3::lexeme[variable_identifier];
    const auto variablelist_def = +variable;

    BOOST_SPIRIT_DEFINE(variable);
    BOOST_SPIRIT_DEFINE(variablelist);

    /// Terms
    const x3::rule<class term, ast::Term> term = "term";
    const x3::rule<class termlist, std::vector<ast::Term>> termlist = "termlist";
    const auto term_identifier = name_identifier | variable_identifier;
    const auto term_def = x3::lexeme[term_identifier] | (name >> +term);
    const auto termlist_def = +term;

    BOOST_SPIRIT_DEFINE(term);
    BOOST_SPIRIT_DEFINE(termlist);

    ///
    /// TypedLists
    ///

    const x3::rule<class typedlist_name, ast::TypedList> typedlist_name = "typedlist_name";
    const auto typedlist_name_def =  //
        namelist >> -('-' >> name)   //
        ;

    BOOST_SPIRIT_DEFINE(typedlist_name);

    const x3::rule<class typedlist_variable, ast::TypedList> typedlist_variable = "typedlist_variable";
    const auto typedlist_variable_def =  //
        variablelist >> -('-' >> name)   //
        ;

    BOOST_SPIRIT_DEFINE(typedlist_variable);

    ///
    /// Atomic Formulas
    ///

    const x3::rule<class atomic_formula, ast::AtomicFormula> atomic_formula = "atomic_formula";
    const auto atomic_formula_def =                //
        '(' >> name >> *typedlist_variable >> ')'  //
        ;

    BOOST_SPIRIT_DEFINE(atomic_formula);

    ///
    /// Requirements Flags
    ///

    const x3::rule<class requirements, std::vector<Requirements::Keyword>> requirements = "requirements";
    const auto requirements_def =                                             //
        '(' >> x3::lit(":requirements") >> *ast::RequirementsKeywords >> ')'  //
        ;

    BOOST_SPIRIT_DEFINE(requirements);

    ///
    /// Types
    ///
    const x3::rule<class types, std::vector<ast::TypedList>> types = "types";
    const auto types_def =                                  //
        '(' >> x3::lit(":types") >> *typedlist_name >> ')'  //
        ;

    BOOST_SPIRIT_DEFINE(types);

    ///
    /// Constants
    ///
    const x3::rule<class constants, std::vector<ast::TypedList>> constants = "constants";
    const auto constants_def =                                  //
        '(' >> x3::lit(":constants") >> *typedlist_name >> ')'  //
        ;

    BOOST_SPIRIT_DEFINE(constants);

    ///
    /// Predicates
    ///
    const x3::rule<class predicates, std::vector<ast::AtomicFormula>> predicates = "predicates";
    const auto predicates_def =                                  //
        '(' >> x3::lit(":predicates") >> *atomic_formula >> ')'  //
        ;

    BOOST_SPIRIT_DEFINE(predicates);

    ///
    /// Functions
    ///

    const x3::rule<class atomic_formula_list, ast::AtomicFormulaList> atomic_formula_list =  //
        "atomic_formula_list";

    const auto atomic_formula_list_def =   //
        +atomic_formula >> -('-' >> name)  //
        ;

    BOOST_SPIRIT_DEFINE(atomic_formula_list);

    const x3::rule<class functions, std::vector<ast::AtomicFormulaList>> functions = "functions";
    const auto functions_def =                                       //
        '(' >> x3::lit(":functions") >> *atomic_formula_list >> ')'  //
        ;

    BOOST_SPIRIT_DEFINE(functions);

    ///
    /// Domain
    ///

    const x3::rule<class domain, ast::Domain> domain = "domain";
    const auto domain_def =                         //
        '(' >> x3::lit("define") >>                 //
        '(' >> x3::lit("domain") >> name >> ')' >>  //
        -requirements >>                            //
        -types >>                                   //
        -constants >>                               //
        -predicates >>                              //
        -functions >>                               //
        ')'  //
        ;

    BOOST_SPIRIT_DEFINE(domain);
}  // namespace parser

bool pddlboat::parseFile(const std::string &file, ast::Domain &domain)
{
    typedef std::string::const_iterator iterator_type;

    std::cout << log::format("Reading file `%1%`", file) << std::endl;
    const std::string text = io::loadFileToString(file);
    std::cout << text << std::endl;

    iterator_type iter = text.begin();
    const iterator_type end = text.end();

    domain = ast::Domain(); // Reset Domain

    bool r = x3::phrase_parse(iter, end, parser::domain, parser::skipper, domain);
    return r && iter == end;
}
