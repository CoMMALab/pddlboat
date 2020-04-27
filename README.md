# PDDLboat

_PDDLboat_ (pronounced "paddle boat") is a library for parsing, constructing, and solving problems defined in the [_Planning Domain Definition Language (PDDL)_](http://users.cecs.anu.edu.au/~patrik/pddlman/writing.html).
Many PDDL-based solvers each roll their own parsing and modeling, providing only a command-line interface.
The goal of _PDDLboat_ is to make it easy to add PDDL-based problem modeling and planning to any project by providing a unified interface to parsing and solving with a flexible intermediate representation that can be accessed and modified programmatically.
_PDDLboat_ also plays nice with ROS, and can be used as a [catkin package](http://wiki.ros.org/catkin/conceptual_overview).
For more on PDDL's grammar, see the [BNF description](https://helios.hud.ac.uk/scommv/IPC-14/repository/kovacs-pddl-3.1-2011.pdf).

Currently, _PDDLboat_ supports `:strips`, `:typing`, `:negative-preconditions`, and `:disjunctive-preconditions`.
More are planned soon.

## Concepts

The main concepts in _PDDLboat_ attempt to match the intuition for their definition in the PDDL language as closely as possible.
To this end, the main object that describes a planning domain is `pddlboat::Domain`.
The domain has methods to add logical structures, such as `addType()` to add new object types, `addPredicate()` to add new predicates, and so on.
Many helper constructor functions are provided for creating types, such as `pddlboat::makePred()` to make a predicate, `pddlboat::makeAction()` to make an action, and so on.
Expressions, which can exist in actions, goals, and so on, are encapsulated by the abstract `pddlboat::Expression` type.
Predicates can be made into expressions by `bind()`, which binds their variables to specific names within an action.
A problem is defined with `pddlboat::Problem`, which contains the objects, start state, and goal expression for a problem.
Given a problem, a `pddlboat::Planner` can generate a `pddlboat::Plan`.

## Libraries

_PDDLboat_ uses:
- [Boost Spirit X3](https://www.boost.org/doc/libs/develop/libs/spirit/doc/x3/html/index.html) for parsing files. Currently, _PDDLboat_ supports parsing planning domains, planning problems, and plans.
- [Z3](https://github.com/Z3Prover/z3) for planning.
- [Fast Downward](http://www.fast-downward.org/) for planning.

## Scripts and Examples

Checkout the `scripts` directory for example programs.
- `test_building.cpp` shows how to make a blockworld domain with code using _PDDLboat_'s library.
- `test_parsing.cpp` tests _PDDLboat_'s parsing on a suite of test domains and blockworld problems that are found in the `resources` directory.
- `test_planning.cpp` is a script that will run either Z3 or Fast Downward planning to solve a parsed domain and problem.

## Compiling

For Ubuntu-based platforms:
```sh
sudo apt-get install libz3-dev libboost-all-dev

```

After cloning the repository, you must get the [Fast Downward](http://www.fast-downward.org/) code as a submodule.
We have provided a helper script to clone their repo, which is a Mercurial project.

First, install Mercurial:
```sh
sudo apt-get install mercurial
```

Then, run the script in the `submodules` directory:
```sh
cd submodules/
./getFD.sh
cd -
```

If you are building outside of a ROS environment, create a build directory and compile:
```sh
mkdir -p build/release
cd build/release
cmake ../..
make
```

Otherwise, simply run the catkin build command.
