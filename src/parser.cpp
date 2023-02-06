/** Author: Zachary Kingston */

#include <boost/config/warning_disable.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>

#include <pddlboat/utility.hpp>
#include <pddlboat/parser/adapted.hpp>
#include <pddlboat/parser/parser.hpp>

namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

namespace pddlboat::ast
{
    struct Keywords_ : x3::symbols<x3::unused_type>
    {
        Keywords_()
        {
            add                         //
                ("define", x3::unused)  //
                ("domain", x3::unused)  //
                ;
        }

    } const Keywords;

    ///
    /// Requirements Flags
    ///

    struct RequirementsKeywords_ : x3::symbols<Requirements::Keyword>
    {
        RequirementsKeywords_()
        {
            Requirements::initialize();
            for (const auto &it : Requirements::getMap().left)
                add(it.first, it.second);
        }

    } const RequirementsKeywords;

    ///
    /// Operators Flags
    ///

    struct OperatorsKeywords_ : x3::symbols<Operators::Keyword>
    {
        OperatorsKeywords_()
        {
            Operators::initialize();
            for (const auto &it : Operators::getMap().left)
                add(it.first, it.second);
        }

    } const OperatorsKeywords;
}  // namespace pddlboat::ast

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

    /*
#define PARSE(x)                                                \
    BOOST_SPIRIT_DEFINE(x);                                     \
    struct x##__class : error_handler, x3::annotate_on_success  \
    {                                                           \
    };
    */
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

    auto name_symbols1 = x3::char_("=");
    auto name_symbols2 = x3::char_("*+/><=_-");
    const auto name_identifier = (ascii::alpha | name_symbols1) >> *(ascii::alnum | name_symbols2);
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
        namelist >> -("- " >> name)  //
        ;
    PARSE(typedlist_name);

    struct typedlist_variable_class;
    const x3::rule<typedlist_variable_class, ast::TypedList> typedlist_variable = "typedlist_variable";
    const auto typedlist_variable_def =  //
        variablelist >> -("- " >> name)  //
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
    const auto requirements_def =                                           //
        '(' >> (x3::lit(":REQUIREMENTS") | x3::lit(":requirements")) >>     //
        *(ast::RequirementsKeywords |                                       //
          (x3::attr(Requirements::Keyword::NONE) >> ':' >> x3::omit[name])  //
          ) >>
        ')'  //
        ;
    PARSE(requirements);

    ///
    /// Types
    ///
    struct types_class;
    const x3::rule<types_class, std::vector<ast::TypedList>> types = "types";
    const auto types_def =                                                        //
        '(' >> (x3::lit(":TYPES") | x3::lit(":types")) >> *typedlist_name >> ')'  //
        ;
    PARSE(types);

    struct types_bare_class;
    const x3::rule<types_bare_class, std::vector<ast::TypedList>> types_bare = "types_bare";
    const auto types_bare_def =            //
        '(' >> *typedlist_variable >> ')'  //
        ;
    PARSE(types_bare);

    ///
    /// Constants
    ///

    struct constants_class;
    const x3::rule<constants_class, std::vector<ast::TypedList>> constants = "constants";
    const auto constants_def =                                                            //
        '(' >> (x3::lit(":CONSTANTS") | x3::lit(":constants")) >> *typedlist_name >> ')'  //
        ;
    PARSE(constants);

    ///
    /// Predicates
    ///

    struct predicates_class;
    const x3::rule<predicates_class, std::vector<ast::AtomicFormula>> predicates = "predicates";
    const auto predicates_def =                                                             //
        '(' >> (x3::lit(":PREDICATES") | x3::lit(":predicates")) >> *atomic_formula >> ')'  //
        ;
    PARSE(predicates);

    ///
    /// Functions
    ///

    struct atomic_formula_list_class;
    const x3::rule<atomic_formula_list_class, ast::AtomicFormulaList> atomic_formula_list =  //
        "atomic_formula_list";
    const auto atomic_formula_list_def =    //
        +atomic_formula >> -("- " >> name)  //
        ;
    PARSE(atomic_formula_list);

    struct functions_class;
    const x3::rule<functions_class, std::vector<ast::AtomicFormulaList>> functions = "functions";
    const auto functions_def =                                                                 //
        '(' >> (x3::lit(":FUNCTIONS") | x3::lit(":functions")) >> *atomic_formula_list >> ')'  //
        ;
    PARSE(functions);

    ///
    /// Actions
    ///

    struct expression_class;
    const x3::rule<expression_class, ast::Expression> expression = "expression";

    // struct nameorvarlist_class;
    // const x3::rule<nameorvarlist_class, std::vector<std::string>> nameorvarlist = "nameorvarlist";
    // const auto nameorvarlist_def = +(name | variable);
    // PARSE(nameorvarlist);

    struct name_expression_class;
    const x3::rule<name_expression_class, ast::Expression> name_expression = "name_expression";
    const auto name_expression_def =
        (name | variable) >>
        x3::attr(std::vector<ast::TypedList>{}) >> x3::attr(std::vector<ast::Expression>{});
    PARSE(name_expression)

    struct bound_predicate_class;
    const x3::rule<bound_predicate_class, ast::Expression> bound_predicate = "bound_predicate";
    const auto bound_predicate_def =                                                  //
        '(' >> name >> x3::attr(std::vector<ast::TypedList>{}) >> *expression >> ')'  //
        ;
    PARSE(bound_predicate)

    struct boolean_operator_class;
    const x3::rule<boolean_operator_class, ast::Expression> boolean_operator = "boolean_operator";
    const auto boolean_operator_def =                                              //
        '(' >> name >> -('(' >> +typedlist_variable >> ')') >> *expression >> ')'  //
        ;
    PARSE(boolean_operator)

    const auto expression_def = boolean_operator | name_expression | bound_predicate  //
        ;
    PARSE(expression)

    struct action_class;
    const x3::rule<action_class, ast::Action> action = "action";
    const auto action_def =                                                                           //
        '(' >> x3::attr(false) >> (x3::lit(":ACTION") | x3::lit(":action")) >> name >>                //
        -((x3::lit(":PARAMETERS") | x3::lit(":parameters")) >> '(' >> *typedlist_variable >> ')') >>  //
        -((x3::lit(":PRECONDITION") | x3::lit(":precondition")) >> expression) >>                     //
        -((x3::lit(":EFFECT") | x3::lit(":effect")) >> expression) >>                                 //
        ')'                                                                                           //
        ;
    PARSE(action)
    struct action_class : error_handler
    {
    };

    ///
    /// Derived Predicate
    ///

    struct derived_class;
    const x3::rule<derived_class, ast::Action> derived = "derived";
    const auto derived_def =                                                     //
        '(' >> x3::attr(true) >> (x3::lit(":DERIVED") | x3::lit(":derived")) >>  //
        '(' >> name >> *typedlist_variable >> ')' >>                             //
        -(expression) >>                                                         //
        x3::attr(ast::Expression{}) >>                                           //
        ')'                                                                      //
        ;
    PARSE(derived)
    struct derived_class : error_handler
    {
    };

    ///
    /// Axioms
    ///

    struct axiom_class;
    const x3::rule<axiom_class, ast::Action> axiom = "axiom";
    const auto axiom_def =  //
        '(' >> x3::attr(false) >>
        (x3::lit(":AXIOM") | x3::lit(":axiom")) >> x3::attr(std::string{"axiom"}) >>      //
        ((x3::lit(":VARS") | x3::lit(":vars")) >> '(' >> *typedlist_variable >> ')') >>  //
        ((x3::lit(":CONTEXT") | x3::lit(":context")) >> expression) >>                   //
        ((x3::lit(":IMPLIES") | x3::lit(":implies")) >> expression) >>                   //
        ')'  //
        ;
    PARSE(axiom)
    struct axiom_class : error_handler
    {
    };

    ///
    /// Domain
    ///

    struct domain_class;
    const x3::rule<domain_class, ast::Domain> domain = "domain";
    const auto domain_def =                                               //
        '(' >> (x3::lit("DEFINE") | x3::lit("define")) >>                 //
        '(' >> (x3::lit("DOMAIN") | x3::lit("domain")) >> name >> ')' >>  //
        -requirements >>                                                  //
        -types >>                                                         //
        -constants >>                                                     //
        -predicates >>                                                    //
        -functions >>                                                     //
        *(axiom | action | derived) >>                                    //
        ')'                                                               //
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
    const auto objects_def =                                                       //
        '(' > (x3::lit(":OBJECTS") | x3::lit(":objects")) > *typedlist_name > ')'  //
        ;
    PARSE(objects);

    ///
    /// Literal
    ///

    ///
    /// Initial
    ///

    struct initial_set_class;
    const x3::rule<initial_set_class, ast::Initial> initial_set = "initial_set";
    const auto initial_set_def =                                                 //
        "(=" >> atomic_formula_name >> x3::attr(true) >> (name | number) >> ')'  //
        ;
    PARSE(initial_set);

    struct initial_pred_class;
    const x3::rule<initial_pred_class, ast::Initial> initial_pred = "initial_pred";
    const auto initial_pred_def =                               //
        atomic_formula_name >> x3::attr(false) >> x3::attr("")  //
        ;
    PARSE(initial_pred);

    struct initial_class;
    const x3::rule<initial_class, std::vector<ast::Initial>> initial = "initial";
    const auto initial_def =                                                               //
        '(' > (x3::lit(":INIT") | x3::lit(":init")) > *(initial_set | initial_pred) > ')'  //
        ;
    PARSE(initial);

    ///
    /// Goal
    ///

    struct goal_class;
    const x3::rule<goal_class, ast::Expression> goal = "goal";
    const auto goal_def =                                                  //
        '(' >> (x3::lit(":GOAL") | x3::lit(":goal")) >> expression >> ')'  //
        ;
    PARSE(goal);

    ///
    /// Metric
    ///

    struct metric_class;
    const x3::rule<metric_class, ast::Metric> metric = "metric";
    const auto metric_def =                                                    //
        '(' >> (x3::lit(":METRIC") | x3::lit(":metric")) >> x3::attr(true) >>  //
        (((x3::lit("MINIMIZE") | x3::lit("minimize")) >> x3::attr(true)) |     //
         ((x3::lit("MAXIMIZE") | x3::lit("maximize")) >> x3::attr(false))) >>  //
        expression
        >>   //
        ')'  //
        ;
    PARSE(metric);

    ///
    /// Problem
    ///

    struct problem_class;
    const x3::rule<problem_class, ast::Problem> problem = "problem";
    const auto problem_def =                                                //
        '(' >> (x3::lit("DEFINE") | x3::lit("define")) >>                   //
        '(' >> (x3::lit("PROBLEM") | x3::lit("problem")) >> name >> ')' >>  //
        '(' >> (x3::lit(":DOMAIN") | x3::lit(":domain")) >> name >> ')' >>  //
        -objects >>                                                         //
        -initial >>                                                         //
        -goal >>                                                            //
        -metric >> ')'                                                      //
        ;
    PARSE(problem);
    struct problem_class : error_handler
    {
    };

    ///
    /// Plan
    ///

    struct plan_class;
    const x3::rule<plan_class, ast::Plan> plan = "plan";
    const auto plan_def =  //
        *atomic_formula_name;
    PARSE(plan);
    struct plan_class : error_handler
    {
    };

    ///
    /// Z3action
    ///

    struct z3action_class;
    const x3::rule<z3action_class, ast::z3Action> z3action = "z3action";
    const auto z3action_def =                                                                               //
        x3::lit("ACTION:") >> name >> x3::lit(":ARGS:") >> -(name % ',') >> -(x3::lit(":TIME:") >> number)  //
        ;
    PARSE(z3action);

    ///
    /// Z3predicate
    ///

    struct z3predicate_class;
    const x3::rule<z3predicate_class, ast::z3Predicate> z3predicate = "z3predicate";
    const auto z3predicate_def =                                                                          //
        x3::lit("PRED:") >> name >> x3::lit(":ARGS:") >> -(name % ',') >> -(x3::lit(":TIME:") >> number)  //
        ;
    PARSE(z3predicate);
}  // namespace parser

//
// Exposed parser functions
//

typedef std::string::const_iterator iterator_type;
using boost::spirit::x3::error_handler_tag;
using boost::spirit::x3::with;
using error_handler_type = boost::spirit::x3::error_handler<iterator_type>;

bool pddlboat::parseFile(const std::string &file, ast::Domain &domain)
{
    const std::string string = io::loadFileToString(file);
    return parseString(string, domain);
}

bool pddlboat::parseFile(const std::string &file, ast::Problem &problem)
{
    const std::string string = io::loadFileToString(file);
    return parseString(string, problem);
}

bool pddlboat::parseFile(const std::string &file, ast::Plan &plan)
{
    const std::string string = io::loadFileToString(file);
    return parseString(string, plan);
}

bool pddlboat::parseFile(const std::string &file, ast::Expression &expression)
{
    const std::string string = io::loadFileToString(file);
    return parseString(string, expression);
}

bool pddlboat::parseString(const std::string &string, ast::Domain &domain)
{
    iterator_type iter = string.begin();
    const iterator_type end = string.end();
    error_handler_type error_handler(iter, end, std::cerr);

    domain = ast::Domain();  // Reset Domain
    bool r = x3::phrase_parse(iter, end, with<error_handler_tag>(std::ref(error_handler))[parser::domain],
                              parser::skipper, domain);
    return r && iter == end;
}

bool pddlboat::parseString(const std::string &string, ast::Problem &problem)
{
    iterator_type iter = string.begin();
    const iterator_type end = string.end();
    error_handler_type error_handler(iter, end, std::cerr);

    problem = ast::Problem();  // Reset Problem
    bool r = x3::phrase_parse(iter, end, with<error_handler_tag>(std::ref(error_handler))[parser::problem],
                              parser::skipper, problem);
    return r && iter == end;
}

bool pddlboat::parseString(const std::string &string, ast::Plan &plan)
{
    iterator_type iter = string.begin();
    const iterator_type end = string.end();
    error_handler_type error_handler(iter, end, std::cerr);

    plan = ast::Plan();  // Reset Plan
    bool r = x3::phrase_parse(iter, end, with<error_handler_tag>(std::ref(error_handler))[parser::plan],
                              parser::skipper, plan);
    return r && iter == end;
}

bool pddlboat::parseString(const std::string &string, ast::Expression &expression)
{
    iterator_type iter = string.begin();
    const iterator_type end = string.end();
    error_handler_type error_handler(iter, end, std::cerr);

    expression = ast::Expression();  // Reset Problem
    bool r = x3::phrase_parse(iter, end, with<error_handler_tag>(std::ref(error_handler))[parser::expression],
                              parser::skipper, expression);
    return r && iter == end;
}

bool pddlboat::parseString(const std::string &string, std::vector<ast::TypedList> &parsed)
{
    iterator_type iter = string.begin();
    const iterator_type end = string.end();
    error_handler_type error_handler(iter, end, std::cerr);

    bool r = x3::phrase_parse(iter, end, with<error_handler_tag>(std::ref(error_handler))[parser::types_bare],
                              parser::skipper, parsed);
    return r && iter == end;
}

bool pddlboat::parseZ3Action(const std::string &string, ast::z3Action &action)
{
    iterator_type iter = string.begin();
    const iterator_type end = string.end();

    action = ast::z3Action();
    bool r = x3::phrase_parse(iter, end, parser::z3action, parser::skipper, action);
    return r && iter == end;
}

bool pddlboat::parseZ3Predicate(const std::string &string, ast::z3Predicate &predicate)
{
    iterator_type iter = string.begin();
    const iterator_type end = string.end();

    predicate = ast::z3Predicate();
    bool r = x3::phrase_parse(iter, end, parser::z3predicate, parser::skipper, predicate);
    return r && iter == end;
}
