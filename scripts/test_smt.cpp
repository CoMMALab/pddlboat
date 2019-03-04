#include <iostream>
#include <pddlboat/expressions.h>
#include <z3++.h>

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

    auto aptr = pddlboat::makeAtom("atom_a");
    auto bptr = pddlboat::makeAtom("atom_b");
    auto cptr = pddlboat::makeAtom("atom_c");
    auto dptr = pddlboat::makeAtom("atom_d");

    pddlboat::Not my_eff(cptr);

    auto preconditions_ptr = pddlboat::makeAnd({aptr, bptr});
    auto effects_ptr = pddlboat::makeNot(cptr);

    pddlboat::Action my_action("my_action", preconditions_ptr, effects_ptr);

    auto my_state = std::make_shared<SimpleState>(0);
    my_state->assignments.emplace("atom_a", true);
    my_state->assignments.emplace("atom_b", true);
    my_state->assignments.emplace("atom_c", false);
    my_state->assignments.emplace("atom_d", false);

    // std::cout<<"The expression evaluates to: "<<my_expr.evaluate(my_state)<<std::endl;

    std::cout << "de-Morgan example\n";
    z3::context c;

    z3::expr x = c.bool_const("x");
    z3::expr y = c.bool_const("y");
    z3::expr conjecture = (!(x && y)) == (!x || !y);

    z3::solver s(c);
    // adding the negation of the conjecture as a constraint.
    s.add(!conjecture);
    std::cout << s << "\n";
    std::cout << s.to_smt2() << "\n";
    switch (s.check())
    {
        case z3::unsat:
            std::cout << "de-Morgan is valid\n";
            break;
        case z3::sat:
            std::cout << "de-Morgan is not valid\n";
            break;
        case z3::unknown:
            std::cout << "unknown\n";
            break;
    }
}
