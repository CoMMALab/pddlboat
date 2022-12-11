#!/bin/bash
python3 generate_bindings.py \
        -m pddlboat \
        -o py/pddlboat.cpp \
        -c build \
        include/pddlboat/solver/variables.h \
        include/pddlboat/parser/translator.h \
        include/pddlboat/solver/astarplanner.h \
        include/pddlboat/solver/dependency.h \
        include/pddlboat/solver/domain.h \
        include/pddlboat/solver/expression.h \
        include/pddlboat/solver/expressions.h \
        include/pddlboat/solver/fdplanner.h \
        include/pddlboat/solver/ffplanner.h \
        include/pddlboat/solver/planner.h \
        include/pddlboat/solver/symkplanner.h \
        include/pddlboat/solver/z3planner.h

