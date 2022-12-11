import pddlboat as pd

domain = pd.parse_domain("../resources/blocks/blockworld.pddl")
problem = pd.parse_problem(domain, "../resources/blocks/p01.pddl")
planner = pd.FastDownwardPlanner(problem, pd.FastDownwardPlanner.Options())
plan = pd.Plan(problem)

if planner.solve(plan):
    print("yay!")
