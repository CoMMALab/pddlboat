#ifndef PDDLBOAT_FUNCTION_
#define PDDLBOAT_FUNCTION_

#include <memory>

#define PDDLBOAT_CLASS_FORWARD(C)               \
  class C;                                      \
  typedef std::shared_ptr<C> C##Ptr;            \
  typedef std::shared_ptr<const C> C##ConstPtr;

namespace pddlboat
{
  PDDLBOAT_CLASS_FORWARD(Function)
    class Function {
  }
}  // namespace pddlboat

#endif
