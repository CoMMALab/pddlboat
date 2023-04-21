import sys

sys.path.append('.')

import pddlboat as pd


def make_variable(name: str, pddl_type: str) -> pd.Value:
  return pd.Value(True, name, pddl_type)


def make_constant(name: str, pddl_type: str) -> pd.Value:
  return pd.Value(False, name, pddl_type)


def make_func_value(problem: pd.Problem, function: pd.ObjectFunctionDefinition, names: list[str]):
  return pd.Value(function.bind(names, problem))


domain = pd.Domain("blocksworld")
domain.add_type("abstract-location", "object")
domain.add_type("location", "abstract-location")
domain.add_type("fixed-location", "abstract-location")
domain.add_type("abstract-object", "abstract-location")
domain.add_type("block", "abstract-object")

domain.add_constant("no-object", "abstract-object")
position_func = pd.make_object_function(
    "position", "abstract-location", [pd.Variable("obj", "abstract-object")]
)
domain.add_function(position_func)

occupied_pred = pd.make_derived_pred(
    "occupied", [pd.Variable("loc", "abstract-location")],
    pd.make_exists(
        pd.VariableSet([pd.Variable("obj", "abstract-object")]),
        pd.make_equals(
            pd.Value(position_func.bind(["?obj"])), make_variable("loc", "abstract-location")
        )
    )
)

domain.add_derived_predicate(occupied_pred)

movable_pred = pd.make_derived_pred(
    "movable", [pd.Variable("obj", "block")], pd.make_not(occupied_pred.bind(["?obj"]))
)

domain.add_derived_predicate(movable_pred)

transfer_act = pd.make_action(
    "transfer", [pd.Variable("obj", "block"), pd.Variable("loc", "location")],
    pd.make_and([movable_pred.bind(["?obj"]), pd.make_not(occupied_pred.bind(["?loc"]))]),
    pd.make_equals(pd.Value(position_func.bind(["?obj"])), make_variable("loc", "location"))
)

domain.add_action(transfer_act)

stack_act = pd.make_action(
    "stack", [pd.Variable("obj", "block"), pd.Variable("loc", "block")],
    pd.make_and([movable_pred.bind(["?obj"]), pd.make_not(occupied_pred.bind(["?loc"]))]),
    pd.make_equals(pd.Value(position_func.bind(["?obj"])), make_variable("loc", "location"))
)

domain.add_action(stack_act)

print(domain)

problem = pd.Problem("example", domain)
problem.add_object("a", "block")
problem.add_object("b", "block")
problem.add_object("c", "block")
problem.add_object("d", "block")

problem.add_object("w", "location")
problem.add_object("x", "location")
problem.add_object("y", "location")
problem.add_object("z", "location")

problem.start.assign1("position", "a", "w")
problem.start.assign1("position", "b", "x")
problem.start.assign1("position", "c", "y")
problem.start.assign1("position", "d", "z")

problem.goal = pd.make_and([
    pd.make_equals(make_func_value(problem, position_func, ["d"]), make_constant("c", "block")),
    pd.make_equals(make_func_value(problem, position_func, ["c"]), make_constant("b", "block")),
    pd.make_equals(make_func_value(problem, position_func, ["b"]), make_constant("a", "block"))
])

print(problem)

options = pd.Z3Planner.Options()
planner = pd.Z3Planner(problem, options)
plan = pd.Plan(problem)

if planner.solve(plan):
  for step in plan.get_steps():
    print(step)
