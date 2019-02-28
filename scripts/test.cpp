#include <iostream>
#include "../include/pddlboat/expressions.h"

class SimpleState : public pddlboat::State {
public:
  SimpleState(size_t id):
    State() { }

  const bool isTrue(const pddlboat::Predicate &predicate) const override {
    return false;
  }
  const std::string evalFun(const pddlboat::Function &function) const override {
    return "";
  }
};

int main (int argc, char **argv) {
  std::cout<<"Hello world"<<std::endl;

  const pddlboat::Atom my_atom_v("atom_v");
  const pddlboat::Atom my_atom_w("atom_w");

  auto vptr = std::make_shared<pddlboat::Atom>("atom_v");
  auto wptr = std::make_shared<pddlboat::Atom>("atom_w");

  pddlboat::And my_expr({vptr, wptr});

  auto my_state = std::make_shared<SimpleState>(0);
  my_state->assignments.emplace(std::make_pair("atom_v", true));
  my_state->assignments.emplace(std::make_pair("atom_w", true));

  std::cout<<"The expression evaluates to: "<<my_expr.evaluate(my_state)<<std::endl;
}
