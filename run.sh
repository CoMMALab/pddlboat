#!/bin/bash
python3 generate_bindings.py \
        -m pddlboat \
        -o pypddl \
        -c build \
        include/pddlboat/parser/parser.h \
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

# include/pddlboat/macros.h \
# include/pddlboat/solver/variables.h \
# include/pddlboat/solver/helper.h \
