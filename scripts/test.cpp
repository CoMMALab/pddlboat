#include <iostream>
#include <pddlboat/solver/expressions.h>

int main(int argc, char **argv)
{
    std::cout << "Hello world" << std::endl;

    auto vptr = pddlboat::makeAtom("atom_v");
    auto wptr = pddlboat::makeAtom("atom_w");

    auto my_expr = pddlboat::makeAnd({vptr, wptr});

    auto my_state = std::make_shared<pddlboat::State>();
    my_state->assign("atom_v", true);
    my_state->assign("atom_w", true);

    std::cout << "The expression evaluates to: " << my_expr->evaluate(my_state) << std::endl;
}
