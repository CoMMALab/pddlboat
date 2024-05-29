from pathlib import Path
import pddlboat as pd

script_dir = Path(__file__).parent
block_dir = script_dir.parent / "resources" / "blocks"
domain_file = str(block_dir / "blockworld.pddl")
problem_file = str(block_dir / "p01.pddl")

domain = pd.parse_domain(domain_file)
problem = pd.parse_problem(domain, problem_file)

options = pd.Z3Planner.Options()
options.diagnostics = False


def plan_normal():
    planner = pd.Z3Planner(problem, options)
    plan = pd.Plan(problem)

    if planner.solve(plan):
        for step in plan.get_steps():
            print(step)


# How to block a specific step in the plan
def plan_block_step():
    planner = pd.Z3Planner(problem, options)
    plan = pd.Plan(problem)

    while planner.solve(plan):
        step = plan.get_step(0)
        print(f"Attempting to block action {step} from initial state...")

        planner.block_plan_step(plan, 0)
        previous_plan = plan
        plan = pd.Plan(problem)

    print("Failed! No other actions left. Returning plan with prior constraints...")

    for step in previous_plan.get_steps():
        print(step)


# How to block a specific state and action pair
def plan_block_state_action():
    planner = pd.Z3Planner(problem, options)
    plan = pd.Plan(problem)

    while planner.solve(plan):
        step = plan.get_step(0)
        print(f"Attempting to block action {step} from initial state...")

        action_to_block = step.action
        action_assignment = step.assignment
        state_to_block_action_in = step.prior

        planner.block_state_action(action_to_block, action_assignment, state_to_block_action_in)

        previous_plan = plan
        plan = pd.Plan(problem)

    print("Failed! No other actions left. Returning plan with prior constraints...")

    for step in previous_plan.get_steps():
        print(step)


def make_action_assignment(action_name, objects):
    action = domain.get_action(action_name)
    assignment = pd.Assignment()

    for obj, param in zip(objects, action.parameters):
        assignment.assign(problem, param.name, obj)

    return action, assignment


def make_grounded_predicate(predicate_name, objects):
    predicate = domain.get_predicate(predicate_name)
    return predicate.bind(objects, problem)


def plan_block_expression_actions():
    planner = pd.Z3Planner(problem, options)
    plan = pd.Plan(problem)

    blocked_action, blocked_assignment = make_action_assignment("pick-up", ["b"])

    # Create a condition where if both "d" and "c" are on the table, "b" cannot be picked
    ontable_d = make_grounded_predicate("ontable", ["d"])
    ontable_c = make_grounded_predicate("ontable", ["c"])

    condition = pd.make_and([ontable_d, ontable_c])

    planner.block_expression_action(blocked_action, blocked_assignment, condition, pd.Assignment())

    if planner.solve(plan):
        for step in plan.get_steps():
            print(step)


def plan_block_expression_actions_quantifier():
    planner = pd.Z3Planner(problem, options)
    plan = pd.Plan(problem)

    blocked_action, blocked_assignment = make_action_assignment("pick-up", ["b"])

    # Create a condition where if there is *not* a stacked block, "b" cannot be picked
    exist_variables = pd.VariableSet([pd.Variable("x", "block"), pd.Variable("y", "block")])

    on_x_y = make_grounded_predicate("on", ["?x", "?y"])  # ? indicates x and y are variable
    exists_on_x_y = pd.make_exists(exist_variables, on_x_y)
    not_exists_on_x_y = pd.make_not(exists_on_x_y)

    planner.block_expression_action(
        blocked_action, blocked_assignment, not_exists_on_x_y, pd.Assignment()
    )

    if planner.solve(plan):
        for step in plan.get_steps():
            print(step)


def plan_constraint():
    planner = pd.Z3Planner(problem, options)
    plan = pd.Plan(problem)

    # Constrain the problem so that "b" may never be on "a" if "c" is on the table
    on_b_a = make_grounded_predicate("on", ["b", "a"])
    ontable_c = make_grounded_predicate("ontable", ["c"])

    constraint = pd.make_and([on_b_a, ontable_c])

    planner.add_constraint(pd.Planner.ConstraintTime.OVER_ALL, constraint, False)

    if planner.solve(plan):
        for step in plan.get_steps():
            print(step)


# plan_normal()
# plan_block_step()
# plan_block_state_action()
# plan_block_expression_actions()
# plan_block_expression_actions_quantifier()
plan_constraint()
