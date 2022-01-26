/* Author: Zachary Kingston */

#include <pddlboat/utility.h>
#include <pddlboat/solver/dependency.h>
#include <pddlboat/solver/planner.h>

using namespace pddlboat;

// static const bool debug = true;
static const bool debug = false;

///
/// Plan::Step
///

Plan::Step::Step(const ActionPtr &action, const Assignment &assignment)
  : action(action), assignment(assignment)
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

bool Plan::Step::operator<(const Step &step_obj) const
{
    std::ostringstream ss1, ss2;
    toString(ss1);
    step_obj.toString(ss2);

    return ss1.str() < ss2.str();
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
    insertStep(action_name, objects, getMakespan());
}

void Plan::addStep(const ActionPtr &action, const Assignment &assignment)
{
    insertStep(action, assignment, getMakespan());
}

void Plan::insertStep(const std::string &action_name, const std::vector<std::string> &objects,
                      std::size_t index)
{
    const auto &[action, assignment] = getAction(action_name, objects);
    insertStep(action, assignment, index);
}

void Plan::insertStep(const ActionPtr &action, const Assignment &assignment, std::size_t index)
{
    if (index >= getMakespan())
        steps_.resize(index + 1);

    steps_[index].emplace_back(action, assignment);
}

bool Plan::validate()
{
    auto previous = problem_->start;

    std::size_t i = 0;
    for (; i < getNumSteps(); ++i)
    {
        auto &step = getStep(i);
        step.state = problem_->makeState();

        if (not step.action->doAction(step.state, previous, step.assignment))
            return false;

        previous = step.state;
    }

    if (i == 0)
        return problem_->goal->evaluate(problem_->start, Assignment(problem_));
    else
        return problem_->goal->evaluate(getStepConst(i - 1).state, Assignment(problem_));
}

std::ostream &Plan::toString(std::ostream &out, bool steps) const
{
    for (std::size_t i = 0; i < steps_.size(); ++i)
        for (const auto &step : steps_[i])
        {
            if (steps)
                out << i << " ";
            step.toString(out) << std::endl;
        }

    return out;
}

const ProblemPtr &Plan::getProblem() const
{
    return problem_;
}

std::vector<Plan::Step> Plan::getSteps() const
{
    std::vector<Plan::Step> steps;
    for (std::size_t i = 0; i < getNumSteps(); ++i)
        steps.emplace_back(getStepConst(i));

    return steps;
}

Plan::Step &Plan::getStep(std::size_t index)
{
    std::size_t total = 0;
    for (auto &set : steps_)
        for (auto &step : set)
        {
            if (index == total)
                return step;

            total++;
        }

    throw std::runtime_error(log::format("Invalid index `%1%` in plan.", index));
}

const Plan::Step &Plan::getStepConst(std::size_t index) const
{
    return const_cast<Plan *>(this)->getStep(index);
}

std::size_t Plan::getNumSteps() const
{
    std::size_t total = 0;
    for (const auto &set : steps_)
        total += set.size();

    return total;
}

std::size_t Plan::getMakespan() const
{
    return steps_.size();
}

const std::vector<Plan::Step> &Plan::getStepsAtMakespan(std::size_t makespan_depth) const
{
    if (makespan_depth >= getMakespan())
        throw std::runtime_error(log::format("Makespan depth %1% >= %2%", makespan_depth, getMakespan()));

    return steps_[makespan_depth];
}

const std::vector<std::vector<Plan::Step>> &Plan::getMakespanSteps() const
{
    return steps_;
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
    blockStateActions({{action, assignment}}, state);
}

void Planner::blockStateActions(const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
                                const StatePtr &state)
{
    throw std::runtime_error("Not implemented!");
}

void Planner::blockStateActionsSimultaneous(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions, const StatePtr &state)
{
    throw std::runtime_error("Not implemented!");
}
