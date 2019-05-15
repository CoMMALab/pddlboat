#include <iostream>
#include <pddlboat/expressions.h>
#include <pddlboat/problem_builder.h>
#include <z3++.h>

int main(int argc, char **argv)
{
    std::cout << "Hello world" << std::endl;
    // list all of the scene propositions
    auto aptr = pddlboat::makeAtom("atom_a");
    auto bptr = pddlboat::makeAtom("atom_b");
    auto cptr = pddlboat::makeAtom("atom_c");
    auto dptr = pddlboat::makeAtom("atom_d");

    // make a vector of the actions
    std::vector<pddlboat::Action> actions;
    pddlboat::Not my_eff(cptr);
    auto preconditions_ptr = pddlboat::makeAnd({aptr, bptr});
    auto effects_ptr = pddlboat::makeAnd({cptr, dptr});
    pddlboat::Action my_action("my_action", preconditions_ptr, effects_ptr);

    actions.push_back(my_action);

    // set the scene propositions for the start state
    auto my_state = std::make_shared<pddlboat::SimpleState>(0);
    my_state->assignments.emplace("atom_a", true);
    my_state->assignments.emplace("atom_b", true);
    my_state->assignments.emplace("atom_c", false);
    my_state->assignments.emplace("atom_d", true);

    auto g = pddlboat::And({cptr, dptr});

    auto goal = std::make_shared<pddlboat::And>(g);
    size_t planning_horizon = 1;

    pddlboat::build_instance(my_state, goal, actions, planning_horizon);

    return 0;
}
