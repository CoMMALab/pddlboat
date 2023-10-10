import sys
sys.path.append('.')

import pddlboat as pd

domain = pd.parse_domain("../resources/blocks/blockworld.pddl")
problem = pd.parse_problem(domain, "../resources/blocks/p11.pddl")

options = pd.FastDownwardPlanner.Options()

# options.search = pd.FastDownwardPlanner.Search.ASTAR
# options.search = pd.FastDownwardPlanner.Search.LAZYGREEDY
options.search = pd.FastDownwardPlanner.Search.FDAT
# options.search = pd.FastDownwardPlanner.Search.LAMA

planner = pd.FastDownwardPlanner(problem, options)
plan = pd.Plan(problem)

if planner.solve(plan):
    for step in plan.get_steps():
        print(step)
