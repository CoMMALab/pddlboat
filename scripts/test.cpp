#include <iostream>
#include "../include/pddlboat/expressions.h"

class SimpleState : public pddlboat::State
{
public:
    SimpleState(size_t id) : State()
    {
    }

    bool isTrue(const pddlboat::Predicate &predicate) const override
    {
        return false;
    }
    std::string evalFun(const pddlboat::Function &function) const override
    {
        return "";
    }
};

int main(int argc, char **argv)
{
    std::cout << "Hello world" << std::endl;

    auto vptr = pddlboat::makeAtom("atom_v");
    auto wptr = pddlboat::makeAtom("atom_w");

    auto my_expr = pddlboat::makeAnd({vptr, wptr});

    auto my_state = std::make_shared<SimpleState>(0);
    my_state->assignments.emplace("atom_v", true);
    my_state->assignments.emplace("atom_w", true);

    std::cout << "The expression evaluates to: " << my_expr->evaluate(my_state) << std::endl;
}
