#include <iostream>
#include <pddlboat/solver/expressions.h>
#include <pddlboat/solver/domain.h>

namespace pd = pddlboat;

int main(int argc, char **argv)
{
    std::cout << "Hello world" << std::endl;

    pd::Domain domain("simple");
    domain.addType("block");

    // Predicates
    auto p_on = pd::makePred("on", {{"x", "block"}, {"y", "block"}});
    domain.addPredicate(p_on);

    auto p_ontable = pd::makePred("ontable", {"x", "block"});
    domain.addPredicate(p_ontable);

    auto p_clear = pd::makePred("clear", {"x", "block"});
    domain.addPredicate(p_clear);

    auto p_empty = pd::makePred("handempty");
    domain.addPredicate(p_empty);

    auto p_holding = pd::makePred("holding", {"x", "block"});
    domain.addPredicate(p_holding);

    auto a_pickup = pd::makeAction("pick-up",
                                   // Parameters
                                   {"x", "block"},  //
                                   // Precondition
                                   pd::makeAnd({p_clear->bind("x"), p_ontable->bind("x"), p_empty->bind()}),
                                   // Effect
                                   pd::makeAnd({pd::makeNot(p_clear->bind("x")),    //
                                                pd::makeNot(p_ontable->bind("x")),  //
                                                pd::makeNot(p_empty->bind()),       //
                                                p_holding->bind("x")}));
    domain.addAction(a_pickup);

    auto a_putdown = pd::makeAction("put-down",
                                    // Parameters
                                    {"x", "block"},  //
                                    // Precondition
                                    pd::makeAnd({p_holding->bind("x")}),
                                    // Effect
                                    pd::makeAnd({p_clear->bind("x"),    //
                                                 p_ontable->bind("x"),  //
                                                 p_empty->bind(),       //
                                                 pd::makeNot(p_holding->bind("x"))}));
    domain.addAction(a_putdown);

    auto a_stack = pd::makeAction("stack",
                                  // Parameters
                                  {{"x", "block"}, {"y", "block"}},  //
                                  // Precondition
                                  pd::makeAnd({p_holding->bind("x"), p_clear->bind("y")}),
                                  // Effect
                                  pd::makeAnd({pd::makeNot(p_holding->bind("x")),  //
                                               pd::makeNot(p_clear->bind("y")),    //
                                               p_clear->bind("x"),                 //
                                               p_empty->bind(),                    //
                                               p_on->bind("x", "y")}));
    domain.addAction(a_stack);

    auto a_unstack = pd::makeAction("unstack",
                                    // Parameters
                                    {{"x", "block"}, {"y", "block"}},  //
                                    // Precondition
                                    pd::makeAnd({p_on->bind("x", "y"), p_clear->bind("x"), p_empty->bind()}),
                                    // Effect
                                    pd::makeAnd({p_holding->bind("x"),             //
                                                 p_clear->bind("y"),               //
                                                 pd::makeNot(p_clear->bind("x")),  //
                                                 pd::makeNot(p_empty->bind()),     //
                                                 pd::makeNot(p_on->bind("x", "y"))}));
    domain.addAction(a_unstack);
    // auto vptr = pd::makeAtom("atom_v");
    // auto wptr = pd::makeAtom("atom_w");

    // auto my_expr = pd::makeAnd({vptr, wptr});

    // auto my_state = std::make_shared<pd::State>();
    // my_state->assign("atom_v", true);
    // my_state->assign("atom_w", true);

    // std::cout << "The expression evaluates to: " << my_expr->evaluate(my_state) << std::endl;
}
