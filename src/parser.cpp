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
    /// Error Handler
    ///
    struct error_handler
    {
        template <typename Iterator, typename Exception, typename Context>
        x3::error_handler_result on_error(Iterator &first, Iterator const &last, Exception const &x,
                                          Context const &context)
        {
            auto &error_handler = x3::get<x3::error_handler_tag>(context).get();
            std::string message = "Error! Expecting: " + x.which() + " here:";
            error_handler(x.where(), message);
            return x3::error_handler_result::fail;
        }
    };

// #define PARSE(x)                                                \
//     BOOST_SPIRIT_DEFINE(x);                                     \
//     struct x##__class : error_handler, x3::annotate_on_success  \
//     {                                                           \
//     };
#define PARSE(x) BOOST_SPIRIT_DEFINE(x);

    ///
    /// Comments
    ///

    const auto comment = ";" >> *(x3::char_ - x3::eol) >> (x3::eol | x3::eoi);
    const auto skipper = ascii::space | comment;

    ///
    /// Identifier Rules
    ///

    /// Numbers
    struct number_class;
    const x3::rule<number_class, std::string> number = "number";
    const auto number_def = x3::lexeme[ascii::digit >> *ascii::digit >> -('.' >> *ascii::digit)];
    PARSE(number);

    /// Names
    struct name_class;
    struct namelist_class;
    const x3::rule<name_class, std::string> name = "name";
    const x3::rule<namelist_class, std::vector<std::string>> namelist = "namelist";
    const auto name_identifier = ascii::alpha >> *(ascii::alnum | x3::char_("_-"));
    const auto name_def = x3::lexeme[name_identifier];
    const auto namelist_def = +name;
    PARSE(name);
    PARSE(namelist);

    /// Variables
    struct variable_class;
    struct variablelist_class;
    const x3::rule<variable_class, std::string> variable = "variable";
    const x3::rule<variablelist_class, std::vector<std::string>> variablelist = "variablelist";
    const auto variable_identifier = '?' >> name_identifier;
    const auto variable_def = x3::lexeme[variable_identifier];
    const auto variablelist_def = +variable;
    PARSE(variable);
    PARSE(variablelist);

    struct nameorvarlist_class;
    const x3::rule<nameorvarlist_class, std::vector<std::string>> nameorvarlist = "nameorvarlist";
    const auto nameorvarlist_def = +(name | variable);
    PARSE(nameorvarlist);

    /// Terms
    struct term_class;
    struct termlist_class;
    const x3::rule<term_class, ast::Term> term = "term";
    const x3::rule<termlist_class, std::vector<ast::Term>> termlist = "termlist";
    const auto term_identifier = name_identifier | variable_identifier;
    const auto term_def = x3::lexeme[term_identifier] | (name >> +term);
    const auto termlist_def = +term;
    PARSE(term);
    PARSE(termlist);

    ///
    /// TypedLists
    ///

    struct typedlist_name_class;
    const x3::rule<typedlist_name_class, ast::TypedList> typedlist_name = "typedlist_name";
    const auto typedlist_name_def =  //
        namelist >> -('-' >> name)   //
        ;
    PARSE(typedlist_name);

    struct typedlist_variable_class;
    const x3::rule<typedlist_variable_class, ast::TypedList> typedlist_variable = "typedlist_variable";
    const auto typedlist_variable_def =  //
        variablelist >> -('-' >> name)   //
        ;
    PARSE(typedlist_variable);

    ///
    /// Atomic Formulas
    ///

    struct atomic_formula_class;
    const x3::rule<atomic_formula_class, ast::AtomicFormula> atomic_formula = "atomic_formula";
    const auto atomic_formula_def =                //
        '(' >> name >> *typedlist_variable >> ')'  //
        ;
    PARSE(atomic_formula);

    struct atomic_formula_name_class;
    const x3::rule<atomic_formula_name_class, ast::AtomicFormula> atomic_formula_name = "atomic_formula_name";
    const auto atomic_formula_name_def =       //
        '(' >> name >> *typedlist_name >> ')'  //
        ;
    PARSE(atomic_formula_name);

    ///
    /// Requirements Flags
    ///

    struct requirements_class;
    const x3::rule<requirements_class, std::vector<Requirements::Keyword>> requirements = "requirements";
    const auto requirements_def =                                             //
        '(' >> x3::lit(":requirements") >> *ast::RequirementsKeywords >> ')'  //
        ;
    PARSE(requirements);

    ///
    /// Types
    ///
    struct types_class;
    const x3::rule<types_class, std::vector<ast::TypedList>> types = "types";
    const auto types_def =                                  //
        '(' >> x3::lit(":types") >> *typedlist_name >> ')'  //
        ;
    PARSE(types);

    ///
    /// Constants
    ///

    struct constants_class;
    const x3::rule<constants_class, std::vector<ast::TypedList>> constants = "constants";
    const auto constants_def =                                  //
        '(' >> x3::lit(":constants") >> *typedlist_name >> ')'  //
        ;
    PARSE(constants);

    ///
    /// Predicates
    ///

    struct predicates_class;
    const x3::rule<predicates_class, std::vector<ast::AtomicFormula>> predicates = "predicates";
    const auto predicates_def =                                  //
        '(' >> x3::lit(":predicates") >> *atomic_formula >> ')'  //
        ;
    PARSE(predicates);

    ///
    /// Functions
    ///

    struct atomic_formula_list_class;
    const x3::rule<atomic_formula_list_class, ast::AtomicFormulaList> atomic_formula_list =  //
        "atomic_formula_list";
    const auto atomic_formula_list_def =   //
        +atomic_formula >> -('-' >> name)  //
        ;
    PARSE(atomic_formula_list);

    struct functions_class;
    const x3::rule<functions_class, std::vector<ast::AtomicFormulaList>> functions = "functions";
    const auto functions_def =                                       //
        '(' >> x3::lit(":functions") >> *atomic_formula_list >> ')'  //
        ;
    PARSE(functions);

    ///
    /// Actions
    ///

    struct expression_class;
    const x3::rule<expression_class, ast::Expression> expression = "expression";

    struct bound_predicate_class;
    const x3::rule<bound_predicate_class, ast::Expression> bound_predicate = "bound_predicate";
    const auto bound_predicate_def =                                                     //
        '(' >> name >> x3::attr(std::vector<ast::TypedList>{}) >> -nameorvarlist >> ')'  //
        ;
    PARSE(bound_predicate)

    struct boolean_operator_class;
    const x3::rule<boolean_operator_class, ast::Expression> boolean_operator = "boolean_operator";
    const auto boolean_operator_def =                                                                //
        '(' >> ast::OperatorsKeywords >> -('(' >> +typedlist_variable >> ')') >> +expression >> ')'  //
        ;
    PARSE(boolean_operator)

    const auto expression_def = boolean_operator | bound_predicate  //
        ;
    PARSE(expression)

    struct action_class;
    const x3::rule<action_class, ast::Action> action = "action";
    const auto action_def =                                               //
        '(' > x3::lit(":action") > name >                                 //
        -(x3::lit(":parameters") > '(' >> *typedlist_variable >> ')') >>  //
        -(x3::lit(":precondition") > expression) >>                       //
        -(x3::lit(":effect") > expression) >>                             //
        ')'                                                               //
        ;
    PARSE(action)
    struct action_class : error_handler
    {
    };

    ///
    /// Domain
    ///

    struct domain_class;
    const x3::rule<domain_class, ast::Domain> domain = "domain";
    const auto domain_def =                     //
        '(' > x3::lit("define") >               //
        '(' > x3::lit("domain") > name > ')' >  //
        -requirements >                         //
        -types >                                //
        -constants >                            //
        -predicates >                           //
        -functions >                            //
        *action >                               //
        ')'                                     //
        ;
    PARSE(domain);
    struct domain_class : error_handler
    {
    };

    ///
    /// Objects
    ///

    struct objects_class;
    const x3::rule<objects_class, std::vector<ast::TypedList>> objects = "objects";
    const auto objects_def =                               //
        '(' > x3::lit(":objects") > *typedlist_name > ')'  //
        ;
    PARSE(objects);

    ///
    /// Literal
    ///

    ///
    /// Initial
    ///

    struct initial_class;
    const x3::rule<initial_class, std::vector<ast::AtomicFormula>> initial = "initial";
    const auto initial_def =                                 //
        '(' > x3::lit(":init") > +atomic_formula_name > ')'  //
        ;
    PARSE(initial);

    ///
    /// Goal
    ///

    struct goal_class;
    const x3::rule<goal_class, ast::Expression> goal = "goal";
    const auto goal_def =                             //
        '(' >> x3::lit(":goal") >> expression >> ')'  //
        ;
    PARSE(goal);

    ///
    /// Problem
    ///

    struct problem_class;
    const x3::rule<problem_class, ast::Problem> problem = "problem";
    const auto problem_def =                     //
        '(' > x3::lit("define") >                //
        '(' > x3::lit("problem") > name > ')' >  //
        '(' > x3::lit(":domain") > name > ')' >  //
        -objects >                               //
        -initial >                               //
        -goal >                                  //
        ')'                                      //
        ;
    PARSE(problem);
    struct problem_class : error_handler
    {
    };
}  // namespace parser

bool pddlboat::parseFile(const std::string &file, ast::Domain &domain)
{
    typedef std::string::const_iterator iterator_type;

    std::cout << log::format("Reading file `%1%`", file) << std::endl;
    const std::string text = io::loadFileToString(file);
    // std::cout << text << std::endl;

    iterator_type iter = text.begin();
    const iterator_type end = text.end();

    using boost::spirit::x3::error_handler_tag;
    using boost::spirit::x3::with;
    using error_handler_type = boost::spirit::x3::error_handler<iterator_type>;
    error_handler_type error_handler(iter, end, std::cerr);

    domain = ast::Domain();  // Reset Domain

    bool r = false;

    // try
    // {
    r = x3::phrase_parse(iter, end, with<error_handler_tag>(std::ref(error_handler))[parser::domain],
                         parser::skipper, domain);
    // }
    // catch (const x3::expectation_failure<std::string::const_iterator> &e)
    // {
    //     const char needle[] = {'\n'};
    //     auto it = std::find_first_of(e.where(), end, needle, needle + 1);
    //     std::cout << "Expected: " << e.which() << " at '" << std::string(e.where(), it) << "'\n";
    // }

    return r && iter == end;
}

bool pddlboat::parseFile(const std::string &file, ast::Problem &problem)
{
    typedef std::string::const_iterator iterator_type;

    std::cout << log::format("Reading file `%1%`", file) << std::endl;
    const std::string text = io::loadFileToString(file);
    // std::cout << text << std::endl;

    iterator_type iter = text.begin();
    const iterator_type end = text.end();

    using boost::spirit::x3::error_handler_tag;
    using boost::spirit::x3::with;
    using error_handler_type = boost::spirit::x3::error_handler<iterator_type>;
    error_handler_type error_handler(iter, end, std::cerr);

    problem = ast::Problem();  // Reset Problem

    bool r = false;

    // try
    // {
    // std::vector<pddlboat::ast::Expression> test;
    // r = x3::phrase_parse(iter, end, with<error_handler_tag>(std::ref(error_handler))[parser::initial],
    //                      parser::skipper, test);
    r = x3::phrase_parse(iter, end, with<error_handler_tag>(std::ref(error_handler))[parser::problem],
                         parser::skipper, problem);
    // }
    // catch (const x3::expectation_failure<std::string::const_iterator> &e)
    // {
    //     const char needle[] = {'\n'};
    //     auto it = std::find_first_of(e.where(), end, needle, needle + 1);
    //     std::cout << "Expected: " << e.which() << " at '" << std::string(e.where(), it) << "'\n";
    // }

    return r && iter == end;
}
