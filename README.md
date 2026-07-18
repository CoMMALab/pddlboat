# PDDLboat

_PDDLboat_ (pronounced "paddle boat") is a library for parsing, constructing, and solving problems defined in the [_Planning Domain Definition Language (PDDL)_](http://users.cecs.anu.edu.au/~patrik/pddlman/writing.html).
Many PDDL-based solvers each roll their own parsing and modeling, providing only a command-line interface.
The goal of _PDDLboat_ is to make it easy to add PDDL-based problem modeling and planning to any project by providing a unified interface to parsing and solving with a flexible intermediate representation that can be accessed and modified programmatically.
_PDDLboat_ also plays nice with ROS, and can be used as a [catkin package](http://wiki.ros.org/catkin/conceptual_overview).
For more on PDDL's grammar, see the [BNF description](https://helios.hud.ac.uk/scommv/IPC-14/repository/kovacs-pddl-3.1-2011.pdf). Additionally, _PDDLboat_ supports the modeling of problems in library code, so domains and problems can be constructed and dynamically changed.

Currently, _PDDLboat_ supports the following PDDL `:requirements`:

- `:strips`: Basic STRIPS style planning.
- `:typing`: Object and variable types.
- `:negative-preconditions`: `not` is allowed in goals and preconditions.
- `:disjunctive-preconditions`: `or` is allowed in goals and preconditions.
- `:equality`: `(= ?a ?b)` is supported as a built-in predicate.
- `:existential-preconditions`: `exists` is allowed in goals and action preconditions.
- `:universal-preconditions`: `forall` is allowed in goals, action preconditions, and action effects.
- `:quantified-preconditions`: Both `:existential-preconditions` and `:universal-preconditions` are supported.
- `:conditional-effects`: `when` is allowed in action effects.
- `:adl`: All of the above are supported.
- `:derived-predicates`: Predicates with values defined by expressions are allowed. Note that these definitions can be recursive.
- `:actions-costs`: Allows for the `total-cost` numerical function and actions that assign to this total cost.
- `:numeric-fluents`: Allows numerical function definitions and numerical operators (e.g., `-`, `+`, `assign`, etc.)
- `:object-fluents`: Allows function definitions that return objects of a certain type.
- `:fluents`: Both `:numeric-fluents` and `:object-fluents` are supported.

Note that not all solver support all requirements.

## Concepts

The main concepts in _PDDLboat_ attempt to match the intuition for their definition in the PDDL language as closely as possible.

### Overview

The main object that describes a planning domain is `pddlboat::Domain`.
The domain has methods to add logical structures, such as `addType()` to add new object types, `addPredicate()` to add new predicates, and so on.
Many helper constructor functions are provided for creating types, such as `pddlboat::makePred()` to make a predicate, `pddlboat::makeAction()` to make an action, and so on.
Expressions, which can exist in actions, goals, and so on, are encapsulated by the abstract `pddlboat::Expression` type.
Expressions can be evaluated for their truth value with `evaluate()`, which returns true or false given a current world state `pddlboat::State`.
Expressions can also be used to assign the value of predicates in a new state with `assign()`.
Predicates can be made into expressions by `bind()`, which binds their variables to specific names within an action.
A problem is defined with `pddlboat::Problem`, which contains the objects, start state, and goal expression for a problem.
Given a problem, a `pddlboat::Planner` can generate a `pddlboat::Plan`.

### Parsers

Parsers for common PDDL structures are provided in:
```cpp
#include <pddlboat/parser/parser.hpp>
```

These parsers will load the contents of a PDDL definition file (for a domain, problem, or plan) into an intermediate representation in the `pddlboat::ast` (for abstract syntax tree) namespace.
These intermediate structures can be converted into the actual _PDDLboat_ types easily, for example:

```cpp
pddlboat::ast::Domain ast;
pddlboat::parseFile("domain.pddl", ast);
pddlboat::DomainPtr domain = pddlboat::toDomain(ast);
```

Also, note that `pddlboat::*` classes typically have a `toPDDL()` function to output them back to a file.

### Planners

There are three planners provided in _PDDLboat_, which implement a simple abstract interface: `plan()`, which returns a `pddlboat::Plan` given a `pddlboat::Problem`.
The planners are:
- A Z3-based SAT plan approach.
- An implementation that uses Fast Forward.
- An implementation that uses Fast Downward.
- An implementation that uses SymK.
- A simple implementation of A*/Dijkstra for search.

Both of these implement different options, but the defaults generally work well.
Some notes about each planner:
- Z3 requires providing a minimum and maximum planning horizon, which is the number of steps a plan can take. If a plan is not found, this number should be increased. Moreover, Z3 tries to incrementally solve the problem so the optimal plan (given the minimum number of steps) is found. Z3 can also solve the problem "simultaneously", by enabling actions to be executed in tandem if they do not conflict.
- Fast Downward has many different kinds of heuristics, a few defaults of which can be selected via the `pddlboat::FastDownwardPlanner::Search` enumerated type. If you want to add your own configurations, check [their website](http://www.fast-downward.org/PlannerUsage) for more configurations. Currently, the following are supported:
  - `pddlboat::FastDownwardPlanner::Search::ASTAR`: A* with landmark cut heuristic.
  - `pddlboat::FastDownwardPlanner::Search::LAZYGREEDY`:  Lazy greedy best-first search with preferred operators and the queue alternation method.
  - `pddlboat::FastDownwardPlanner::Search::FDAT`: Fast Downward Autotune 2.
  - `pddlboat::FastDownwardPlanner::Search::LAMA`: LAMA 2011 configuration from IPC 2011.
  
An example of planning once a problem has been defined:
```cpp
#include <pddlboat/solver/planner.hpp>
#include <pddlboat/solver/fdplanner.hpp>
...
pddlboat::ProblemPtr problem;
...
pddlboat::FastDownwardPlanner planner(problem, search);
pddlboat::Plan plan(problem);
bool r = planner.solve(plan);
```

Take a look at the file `helper.h` for an example on how to configure, load, and solve a planning problem.

### Constructing Block World Example

Let's construct a simple blockworld domain using the _PDDLboat_ API!
First, we must create our domain:
```cpp
auto domain = std::make_shared<pd::Domain>("block-world");
```
Then, we add an object type to our domain:
```cpp
domain->addType("block");
```
This would look like the following in a PDDL file:
```lisp
(:types block)
```

Let's add some predicates to this domain that allow us to describe the state of the world.
We want to be able to describe where every block is, either on the table or on another block.
```cpp
auto p_on = pd::makePred("on", {{"x", "block"}, {"y", "block"}});
domain->addPredicate(p_on);

auto p_ontable = pd::makePred("ontable", {{"x", "block"}});
domain->addPredicate(p_ontable);
```
Here, we make the `on` and `ontable` predicates, and give them some variables with types.
The `pd::makePred()` makes this easy.
We finish up declaring our predicates to describe when blocks do not have something stacked on top of them, when the robot's gripper is empty, and if the robot is grasping a block:
```cpp
auto p_clear = pd::makePred("clear", {{"x", "block"}});
domain->addPredicate(p_clear);

auto p_empty = pd::makePred("handempty");
domain->addPredicate(p_empty);

auto p_holding = pd::makePred("holding", {{"x", "block"}});
domain->addPredicate(p_holding);
```
Within an actual PDDL file, these predicates would be defined as follows:
```lisp
(:predicates (on ?x - block ?y - block)
             (ontable ?x - block)
             (clear ?x - block)
             (handempty)
             (holding ?x - block))
```

Now, let's add an action to our domain.
Here we create an action to describe picking up a block.
```cpp
auto a_pickup =  //
    pd::makeAction("pick-up",
                   // Parameters
                   {{"x", "block"}},  //
                   // Precondition
                   pd::makeAnd({p_clear->bind({"?x"}), p_ontable->bind({"?x"}), p_empty->bind()}),
                   // Effect
                   pd::makeAnd({pd::makeNot(p_clear->bind({"?x"})),    //
                                pd::makeNot(p_ontable->bind({"?x"})),  //
                                pd::makeNot(p_empty->bind()),          //
                                p_holding->bind({"?x"})}));
domain->addAction(a_pickup);
```
There is a lot going on in this call.
First, the high level structure is based in the `pd::makeAction()` function, which takes in the function name, action parameters (a typed list), action preconditions (an expression), and the action effect (another expression).
See how all predicate objects called are used with `bind()`?
`bind()` creates an executable expression from a predicate, and binds a specific variable name (in this case, `?x`) to the predicate.
What this means is, whatever `?x` is when the action is grounded is what will be used to check that predicate's truth value.
For example, if this action `pick-up` is grounded as `(pick-up a)`, all the `?x` bound predicates will check their truth value using `?x = a`.

This action would be defined as follows in a PDDL file:
```lisp
(:action pick-up
         :parameters (?x - block)
         :precondition (and (clear ?x) (ontable ?x) (handempty))
         :effect
         (and (not (ontable ?x))
              (not (clear ?x))
              (not (handempty))
              (holding ?x)))
```

As you can see, the in line syntax for _PDDLboat_ is very similar to how PDDL files are structured.
This example can be seen in its complete form in `scripts/test_building.cpp`, and its complimentary PDDL domain can be found in `resources/blocks/blockworld.pddl`.

## Libraries

_PDDLboat_ uses:
- [Boost Spirit X3](https://www.boost.org/doc/libs/develop/libs/spirit/doc/x3/html/index.html) for parsing files. Currently, _PDDLboat_ supports parsing planning domains, planning problems, and plans.
- [Z3](https://github.com/Z3Prover/z3) for planning.
- [Fast Downward](http://www.fast-downward.org/) for planning.

## Scripts and Examples

Checkout the `scripts` directory for example programs.
- `test_building.cpp` shows how to make a blockworld domain with code using _PDDLboat_'s library.
- `test_parsing.cpp` tests _PDDLboat_'s parsing on a suite of test domains and blockworld problems that are found in the `resources` directory.
- `test_planning.cpp` is a script that has fully configurable options for planning to solve a parsed domain and problem.
- `test_blocking.cpp` solves problems like in `test_planning.cpp`, but also blocks randomly chosen state/action pairs from the output plan, and then replans.

## Compiling

For Ubuntu-based platforms:
```sh
sudo apt-get install libboost-all-dev
sudo apt-get install bison
sudo apt-get install autoconf
```

Install Z3
```sh
sudo apt-get install libz3-dev
```

You will need to clone the submodules (Fast Downward, FF, Symk).
Either clone this repo with:
```
git clone --recursive git@github.com:KavrakiLab/pddlboat.git
```

Or initialize after the fact:
```
git submodule init
git submodule update
```

If you are building outside of a ROS environment, create a build directory and compile:
```sh
mkdir -p build/release
cd build/release
cmake ../..
make
```

Otherwise, simply run the catkin build command.

### Troubleshooting the Boost version problem
In Ubuntu 22.04, the default released version of `Boost` is 1.74. If you encounter a problem of requiring `Boost` 1.82, download `boost_1_82_0.tar.gz` from the [Boost website](https://www.boost.org/users/history/version_1_82_0.html). Untar it and enter its directory. Then run the bootstrap script, assuming the installation prefix is `/usr`:
```sh
./bootstrap.sh --prefix=/usr/
sudo ./b2 install
```

### Faster compilation
You can compile `pddlboat` using Ninja. Note: if you installed `Boost` using the above command, replace `$PREFIX` with `/usr/`, if it is empty.
```
cmake -Bbuild -GNinja -DBOOST_ROOT=$PREFIX .
cmake --build build
```


### Installing the Python bindings
Before you compile `pddlboat`, make sure you have `pybind11` installed. Assuming you are using a conda environment, activate it and install:
```sh
conda install -c conda-forge pybind11
```
The CMake build compiles the `pddlboat` Python extension and copies it into
`py/pddlboat/`, so **build the project first** (see above), then install the
package from the repository root:
```sh
pip install -e .
```
This is an editable install of the already-built extension: because the CMake
build re-copies the compiled `.so` into `py/pddlboat/` on every rebuild, later
rebuilds are picked up automatically without reinstalling.

# TODO

More features in _PDDLboat_ are planned soon.
On the horizon:
- Basic uncertainty (e.g., `oneof`, etc.).
- Durative actions.
- Support for additional planners.
