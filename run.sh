#!/bin/bash
python3 generate_bindings.py \
        -m pddlboat \
        -o py/pddlboat.cpp \
        -c build \
        include/pddlboat/parser/ast.hpp \
        include/pddlboat/parser/parser.hpp \
        include/pddlboat/parser/translator.hpp \
        include/pddlboat/solver/astarplanner.hpp \
        include/pddlboat/solver/dependency.hpp \
        include/pddlboat/solver/domain.hpp \
        include/pddlboat/solver/expression.hpp \
        include/pddlboat/solver/expressions.hpp \
        include/pddlboat/solver/fdplanner.hpp \
        include/pddlboat/solver/ffplanner.hpp \
        include/pddlboat/solver/planner.hpp \
        include/pddlboat/solver/symkplanner.hpp \
        include/pddlboat/solver/variables.hpp \
        include/pddlboat/solver/z3planner.hpp

