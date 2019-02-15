#ifndef PDDLBOAT_ACTION_
#define PDDLBOAT_ACTION_

#include <memory>

#define PDDLBOAT_CLASS_FORWARD(C)               \
  class C;                                      \
  typedef std::shared_ptr<C> C##Ptr;            \
  typedef std::shared_ptr<const C> C##ConstPtr;

namespace pddlboat
{
  PDDLBOAT_CLASS_FORWARD(Action)
    class Action {
  }
}  // namespace pddlboat

#endif
