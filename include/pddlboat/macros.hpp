#ifndef PDDLBOAT_MACROS_
#define PDDLBOAT_MACROS_

#include <memory>

#define PDDLBOAT_CLASS_FORWARD(C)                                                                            \
    class C;                                                                                                 \
    typedef std::shared_ptr<C> C##Ptr;                                                                       \
    typedef std::shared_ptr<const C> C##ConstPtr;

#define PDDLBOAT_OBJECT_CLASS "object"

#endif
