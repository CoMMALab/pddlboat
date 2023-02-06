import sys

sys.path.append('.')

import pddlboat as pd

domain = pd.Domain("block-world")
domain.add_type("block", "object")

p_on = pd.make_pred("on", [pd.Variable("x", "block"), pd.Variable("y", "block")])
domain.add_predicate(p_on)

p_ontable = pd.make_pred("ontable", [pd.Variable("x", "block")])
domain.add_predicate(p_ontable)

p_clear = pd.make_pred("clear", [pd.Variable("x", "block")])
domain.add_predicate(p_clear)

p_empty = pd.make_pred("handempty", [])
domain.add_predicate(p_empty)

p_holding = pd.make_pred("holding", [pd.Variable("x", "block")])
domain.add_predicate(p_holding)

a_pickup = pd.make_action(
    "pick-up",
    # Parameters
    [pd.Variable("x", "block")],
    # Precondition
    pd.make_and([p_clear.bind(["?x"]), p_ontable.bind(["?x"]), p_empty.bind([])]),
    # Effect
    pd.make_and([
        pd.make_not(p_clear.bind(["?x"])),
        pd.make_not(p_ontable.bind(["?x"])),
        pd.make_not(p_empty.bind([])),
        p_holding.bind(["?x"])
    ])
)
domain.add_action(a_pickup)

a_putdown = pd.make_action(
    "put-down",
    # Parameters
    [pd.Variable("x", "block")],
    # Precondition
    pd.make_and([p_holding.bind(["?x"])]),
    # Effect
    pd.make_and([
        p_clear.bind(["?x"]),
        p_ontable.bind(["?x"]),
        p_empty.bind([]),
        pd.make_not(p_holding.bind(["?x"]))
    ])
)
domain.add_action(a_putdown)

a_stack = pd.make_action(
    "stack",
    # Parameters
    [pd.Variable("x", "block"), pd.Variable("y", "block")],
    # Precondition
    pd.make_and([p_holding.bind(["?x"]), p_clear.bind(["?y"])]),
    # Effect
    pd.make_and([
        pd.make_not(p_holding.bind(["?x"])),
        pd.make_not(p_clear.bind(["?y"])),
        p_clear.bind(["?x"]),
        p_empty.bind([]),
        p_on.bind(["?x", "?y"])
    ])
)
domain.add_action(a_stack)

a_unstack = pd.make_action(
    "unstack",
    # Parameters
    [pd.Variable("x", "block"), pd.Variable("y", "block")],
    # Precondition
    pd.make_and([p_on.bind(["?x", "?y"]), p_clear.bind(["?x"]), p_empty.bind([])]),
    # Effect
    pd.make_and([
        p_holding.bind(["?x"]),
        p_clear.bind(["?y"]),
        pd.make_not(p_clear.bind(["?x"])),
        pd.make_not(p_empty.bind([])),
        pd.make_not(p_on.bind(["?x", "?y"]))
    ])
)
domain.add_action(a_unstack)

print(domain)
print()

problem = pd.Problem("simple", domain)
problem.add_object("a", "block")
problem.add_object("b", "block")
problem.add_object("c", "block")
problem.add_object("d", "block")

problem.start.assign1("clear", "a", True)
problem.start.assign1("clear", "b", True)
problem.start.assign1("clear", "c", True)
problem.start.assign1("clear", "d", True)
problem.start.assign1("ontable", "a", True)
problem.start.assign1("ontable", "b", True)
problem.start.assign1("ontable", "c", True)
problem.start.assign1("ontable", "d", True)
problem.start.assign0("handempty", True)

problem.goal = pd.make_and([
    p_on.bind(["d", "c"], problem),
    p_on.bind(["c", "b"], problem),
    p_on.bind(["b", "a"], problem),
])

print(problem)
print()

options = pd.Z3Planner.Options()
planner = pd.Z3Planner(problem, options)
plan = pd.Plan(problem)

if planner.solve(plan):
  for step in plan.get_steps():
    print(step)
