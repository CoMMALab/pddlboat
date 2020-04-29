/* Author: Zachary Kingston */

#include <pddlboat/utility.h>
#include <pddlboat/solver/planner.h>

using namespace pddlboat;

static const bool debug = true;
// static const bool debug = false;

///
/// Plan::Step
///

Plan::Step::Step(const ActionPtr &action, const Assignment &assignment, const StatePtr &state)
  : action(action), assignment(assignment), state(state)
{
}

std::ostream &Plan::Step::toString(std::ostream &out) const
{
    out << "(" << action->name;
    for (const auto &parameter : action->parameters)
        out << " " << assignment.assigned(parameter.name)->name;

    out << ")";

    return out;
}

std::ostream &pddlboat::operator<<(std::ostream &out, const Plan::Step &step)
{
    return step.toString(out);
}

///
/// Plan
///

Plan::Plan(const ProblemPtr &problem) : problem_(problem)
{
}

void Plan::addStep(const std::string &action_name, const std::vector<std::string> &objects)
{
    // Get action corresponding to name
    ActionPtr action = problem_->domain->getAction(action_name);

    // Validate action parameters
    if (not action)
        throw std::invalid_argument(log::format("Action %1% does not exist!", action_name));

    if (action->arity() != objects.size())
        throw std::invalid_argument(
            log::format("Action %1% has arity %2%, not %3%!", action_name, action->arity(), objects.size()));

    // Validate that assignment objects match parameter types
    Assignment assignment(problem_.get());
    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        const auto &object_name = objects[i];
        ObjectPtr obj;

        // Check if object exists
        if (not(obj = problem_->getObject(object_name)))
        {
            // If not an object, may be a constant
            throw std::invalid_argument(
                log::format("Cannot do action with object %1%, does not exist!", object_name));
        }

        // Check type
        if (action->parameters[i].type != obj->type)
            throw std::invalid_argument(  //
                log::format("Cannot do action with object %1%, is type %2%, not %3%!", object_name, obj->type,
                            action->parameters[i].type));

        assignment.assign(action->parameters[i].name, objects[i]);
    }

    // Validate world state update from action
    // Use prior state in sequence, which may be the start
    StatePtr prev;
    if (steps.empty())
        prev = problem_->start;
    else
        prev = steps.back().state;

    StatePtr state = problem_->makeState();

    if (debug)
    {
        std::cout << "Attempting to do action " << action_name << " with: ";
        for (const auto &object : objects)
            std::cout << object << ", ";
        std::cout << std::endl;
        std::cout << "On State: " << std::endl;
        prev->toPDDL(std::cout);
        std::cout << std::endl;
    }

    if (not action->doAction(state, prev, assignment))
        throw std::invalid_argument(  //
            log::format("Action %1% failed to apply to previous state!", action_name));

    steps.emplace_back(action, assignment, state);
}

std::ostream &Plan::toString(std::ostream &out) const
{
    for (const auto &step : steps)
        step.toString(out) << std::endl;

    return out;
}

std::ostream &pddlboat::operator<<(std::ostream &out, const Plan &plan)
{
    return plan.toString(out);
}

///
/// Planner
///

Planner::Planner(const ProblemPtr &problem) : problem_(problem)
{
}
