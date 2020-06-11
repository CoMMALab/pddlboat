/* Author: Zachary Kingston */

#include <pddlboat/utility.h>
#include <pddlboat/solver/planner.h>

using namespace pddlboat;

// static const bool debug = true;
static const bool debug = false;

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

Plan::Plan(const Plan &plan) : steps_(plan.steps_), problem_(plan.problem_)
{
}

std::pair<ActionPtr, Assignment> Plan::getAction(const std::string &action_name,
                                                 const std::vector<std::string> &objects) const
{
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
        if (not problem_->domain->isTypeOf(obj->type, action->parameters[i].type))
            throw std::invalid_argument(  //
                log::format("Cannot do action with object %1%, is type %2%, not %3%!", object_name, obj->type,
                            action->parameters[i].type));

        assignment.assign(action->parameters[i].name, objects[i]);
    }

    return {action, assignment};
}

void Plan::addStep(const std::string &action_name, const std::vector<std::string> &objects)
{
    const auto &[action, assignment] = getAction(action_name, objects);
    auto state = propagateAction(action, assignment, steps_.size());
    if (not state)
        throw std::invalid_argument(  //
            log::format("Action %1% failed to apply to previous state!", action_name));

    steps_.emplace_back(action, assignment, state);
}

StatePtr Plan::propagateAction(const ActionPtr &action, const Assignment &assignment, std::size_t index) const
{
    StatePtr prev;
    if (steps_.empty() or index == 0)
        prev = problem_->start;
    else
        prev = steps_[index - 1].state;

    StatePtr state = problem_->makeState();

    if (debug)
    {
        std::cout << "Attempting to do action " << action->name << " with: ";
        assignment.toString(std::cout) << std::endl;
        std::cout << "On State: " << std::endl;
        prev->toPDDL(std::cout);
        std::cout << std::endl;
    }

    if (not action->doAction(state, prev, assignment))
        return nullptr;

    return state;
}

void Plan::insertStep(const std::string &action_name, const std::vector<std::string> &objects,
                      std::size_t index)
{
    const auto &[action, assignment] = getAction(action_name, objects);
    insertStep(action, assignment, index);
}

void Plan::insertStep(const ActionPtr &action, const Assignment &assignment, std::size_t index)
{
    auto state = propagateAction(action, assignment, index);
    if (not state)
        throw std::invalid_argument(  //
            log::format("Action %1% failed to apply to previous state!", action->name));

    Step step(action, assignment, state);
    steps_.insert(steps_.begin() + index, step);
}

void Plan::removeStep(std::size_t index)
{
    steps_.erase(steps_.begin() + index);
}

bool Plan::validate()
{
    for (std::size_t i = 0; i < steps_.size(); ++i)
    {
        auto state = propagateAction(steps_[i].action, steps_[i].assignment, i);
        if (not state)
            return false;

        steps_[i].state = state;
    }

    return problem_->goal->evaluate(steps_.back().state, Assignment(problem_));
}

std::ostream &Plan::toString(std::ostream &out) const
{
    for (const auto &step : steps_)
        step.toString(out) << std::endl;

    return out;
}

const ProblemPtr &Plan::getProblem() const
{
    return problem_;
}

const std::vector<Plan::Step> &Plan::getSteps() const
{
    return steps_;
}

std::size_t Plan::getNumSteps() const
{
    return steps_.size();
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

void Planner::reset()
{
}

void Planner::checkRequirements() const
{
    if (not problem_->domain->supportsRequirements(getSupportedRequirements()))
        throw std::runtime_error("Planner does not support domain requirements!");
}

void Planner::blockPlanStep(const Plan &plan, std::size_t index)
{
    const auto &steps = plan.getSteps();
    StatePtr prior = (index == 0) ? problem_->start : steps[index - 1].state;
    blockStateAction(steps[index].action, steps[index].assignment, prior);
}

void Planner::blockStateAction(const ActionPtr &action, const Assignment &assignment, const StatePtr &state)
{
    throw std::runtime_error("Not implemented!");
}
