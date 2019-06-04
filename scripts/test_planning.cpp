#include <iostream>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/planner.h>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/domain.h>
#include <z3++.h>

namespace pd = pddlboat;

int main(int argc, char **argv)
{
    size_t planning_horizon = 1;

    std::cout << "Hello world" << std::endl;

    auto domain = std::make_shared<pd::Domain>("block-world");
    domain->addType("block");

    // Predicates
    auto p_on = pd::makePred("on", {{"x", "block"}, {"y", "block"}});
    domain->addPredicate(p_on);

    auto p_ontable = pd::makePred("ontable", {{"x", "block"}});
    domain->addPredicate(p_ontable);

    auto p_clear = pd::makePred("clear", {{"x", "block"}});
    domain->addPredicate(p_clear);

    auto p_empty = pd::makePred("handempty");
    domain->addPredicate(p_empty);

    auto p_holding = pd::makePred("holding", {{"x", "block"}});
    domain->addPredicate(p_holding);

    auto a_pickup =  //
        pd::makeAction("pick-up",
                       // Parameters
                       {{"x", "block"}},  //
                       // Precondition
                       pd::makeAnd({p_clear->bind({"?x"}), p_ontable->bind({"?x"}), p_empty->bind()}),
                       // Effect
                       pd::makeAnd({pd::makeNot(p_clear->bind({"?x"})),    //
                                    pd::makeNot(p_ontable->bind({"?x"})),  //
                                    pd::makeNot(p_empty->bind()),          //
                                    p_holding->bind({"?x"})}));
    domain->addAction(a_pickup);

    auto a_putdown =  //
        pd::makeAction("put-down",
                       // Parameters
                       {{"x", "block"}},  //
                       // Precondition
                       pd::makeAnd({p_holding->bind({"?x"})}),
                       // Effect
                       pd::makeAnd({p_clear->bind({"?x"}),    //
                                    p_ontable->bind({"?x"}),  //
                                    p_empty->bind(),          //
                                    pd::makeNot(p_holding->bind({"?x"}))}));
    domain->addAction(a_putdown);

    auto a_stack =  //
        pd::makeAction("stack",
                       // Parameters
                       {{"x", "block"}, {"y", "block"}},  //
                       // Precondition
                       pd::makeAnd({p_holding->bind({"?x"}), p_clear->bind({"?y"})}),
                       // Effect
                       pd::makeAnd({pd::makeNot(p_holding->bind({"?x"})),  //
                                    pd::makeNot(p_clear->bind({"?y"})),    //
                                    p_clear->bind({"?x"}),                 //
                                    p_empty->bind(),                       //
                                    p_on->bind({"?x", "?y"})}));
    domain->addAction(a_stack);

    auto a_unstack =  //
        pd::makeAction("unstack",
                       // Parameters
                       {{"x", "block"}, {"y", "block"}},  //
                       // Precondition
                       pd::makeAnd({p_on->bind({"?x", "?y"}), p_clear->bind({"?x"}), p_empty->bind()}),
                       // Effect
                       pd::makeAnd({p_holding->bind({"?x"}),             //
                                    p_clear->bind({"?y"}),               //
                                    pd::makeNot(p_clear->bind({"?x"})),  //
                                    pd::makeNot(p_empty->bind()),        //
                                    pd::makeNot(p_on->bind({"?x", "?y"}))}));
    domain->addAction(a_unstack);
    //domain->toPDDL();

    std::cout << std::endl;

    auto problem = std::make_shared<pd::Problem>("simple", domain);
    problem->addObject("a", "block");
    problem->addObject("b", "block");
    problem->addObject("c", "block");
    problem->addObject("d", "block");

    problem->start->assign1("clear", "a");
    problem->start->assign1("clear", "b");
    problem->start->assign1("clear", "c");
    problem->start->assign1("clear", "d");
    problem->start->assign1("ontable", "a");
    problem->start->assign1("ontable", "b");
    problem->start->assign1("ontable", "c");
    problem->start->assign1("ontable", "d");
    problem->start->assign0("handempty");

    problem->goal = pd::makeAnd({
        p_on->bind({"d", "c"}, problem),
        p_on->bind({"c", "b"}, problem),
        p_on->bind({"b", "a"}, problem),
    });

    //problem->toPDDL();

    pddlboat::Z3Planner solver(problem, planning_horizon);
    solver.solve();


    return 0;
}
