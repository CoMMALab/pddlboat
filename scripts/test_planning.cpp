#include <iostream>
#include <pddlboat/expressions.h>
#include <pddlboat/problem_builder.h>
#include <z3++.h>

int main(int argc, char **argv)
{
    // list all of the scene propositions
    auto aptr = pddlboat::makeAtom("atom_a");
    auto bptr = pddlboat::makeAtom("atom_b");
    auto cptr = pddlboat::makeAtom("atom_c");
    auto dptr = pddlboat::makeAtom("atom_d");

    // make a vector of the actions
    auto preconditions_ptr = pddlboat::makeAnd({aptr, bptr});
    auto effects_ptr = pddlboat::makeAnd({cptr, dptr});
    auto my_action = pddlboat::makeAction("my_action", preconditions_ptr, effects_ptr);

    // set the scene propositions for the start state
    auto my_state = std::make_shared<pddlboat::State>();
    my_state->assign("atom_a", true);
    my_state->assign("atom_b", true);
    my_state->assign("atom_c", false);
    my_state->assign("atom_d", true);

    auto goal = pddlboat::makeAnd({cptr, dptr});
    size_t planning_horizon = 1;

    pddlboat::Z3Planner solver(my_state, goal, {my_action}, planning_horizon);
    solver.solve();

    return 0;
}
