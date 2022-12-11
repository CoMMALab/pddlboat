#include <iostream>
#include <pddlboat/solver/expressions.hpp>
#include <z3++.h>

int main(int argc, char **argv)
{
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
    return 0;
}
