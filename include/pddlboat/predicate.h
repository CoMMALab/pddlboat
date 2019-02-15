#ifndef PDDLBOAT_PREDICATE_
#define PDDLBOAT_PREDICATE_

#include <memory>

#define PDDLBOAT_CLASS_FORWARD(C)    \
  class C;                           \
  typedef std::shared_ptr<C> C##Ptr; \
  typedef std::shared_ptr<const C> C##ConstPtr;

namespace pddlboat {
PDDLBOAT_CLASS_FORWARD(Predicate)
class Predicate {
 public:
  virtual bool evaluate(consnt StatePtr &state) const = 0;
};
}  // namespace pddlboat

#endif
