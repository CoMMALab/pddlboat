/* Author: Zachary Kingston */

#include <pddlboat/utility.hpp>
#include <pddlboat/solver/dependency.hpp>
#include <pddlboat/solver/planner.hpp>
#include <sys/poll.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>

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
    Assignment assignment;
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

        assignment.assign(*problem_, action->parameters[i].name, objects[i]);
    }

    return {action, assignment};
}

Plan::Step &Plan::addStep(const std::string &action_name, const std::vector<std::string> &objects)
{
    return insertStep(action_name, objects, getMakespan());
}

Plan::Step &Plan::addStepAction(const ActionPtr &action, const Assignment &assignment)
{
    return insertStepAction(action, assignment, getMakespan());
}

Plan::Step &Plan::insertStep(const std::string &action_name, const std::vector<std::string> &objects,
                             std::size_t index)
{
    const auto &[action, assignment] = getAction(action_name, objects);
    return insertStepAction(action, assignment, index);
}

Plan::Step &Plan::insertStepAction(const ActionPtr &action, const Assignment &assignment, std::size_t index)
{
    if (index >= getMakespan())
        steps_.resize(index + 1);

    return steps_[index].emplace_back(action, assignment);
}

bool Plan::validate()
{
    auto previous = problem_->start;

    std::size_t i = 0;
    for (; i < getNumSteps(); ++i)
    {
        auto &step = getStep(i);

        step.prior = previous->cleanClone();
        step.state = problem_->makeState();

        if (not step.action->doAction(step.state, previous, step.assignment))
            return false;

        step.state->clean();
        previous = step.state;
    }

    if (i == 0)
        return problem_->goal->evaluate(problem_->start, {});
    else
        return problem_->goal->evaluate(getStepConst(i - 1).state, {});
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

double Plan::getTimeTaken(const std::string &key) const
{
    return times_.find(key)->second;
}

void Plan::setTimeTaken(const std::string &key, double time)
{
    times_[key] = time;
}

///
/// PlanSet
///

PlanSet::PlanSet(const ProblemPtr &problem) : problem_{problem}
{
}

PlanSet::PlanSet(const PlanSet &plans) : problem_{plans.problem_}, plans_(plans.plans_)
{
}

void PlanSet::addPlan(const Plan &plan)
{
    plans_.emplace_back(plan);
}

Plan &PlanSet::newPlan()
{
    return plans_.emplace_back(problem_);
}

void PlanSet::clear()
{
    plans_.clear();
}

Plan &PlanSet::getPlan(std::size_t index)
{
    return plans_.at(index);
}

const Plan &PlanSet::getPlanConst(std::size_t index) const
{
    return plans_.at(index);
}

std::vector<Plan>::iterator PlanSet::begin()
{
    return plans_.begin();
}

std::vector<Plan>::iterator PlanSet::end()
{
    return plans_.end();
}

std::vector<Plan>::const_iterator PlanSet::cbegin()
{
    return plans_.cbegin();
}

std::vector<Plan>::const_iterator PlanSet::cend()
{
    return plans_.cend();
}

std::size_t PlanSet::count() const
{
    return plans_.size();
}

const ProblemPtr &PlanSet::getProblem() const
{
    return problem_;
}

PlanWatcher::PlanWatcher(const std::filesystem::path &output_dir, const std::string &filename_prefix,
                         const ProblemPtr &problem, const std::string &cmd, bool start_immediately,
                         unsigned int timeout, unsigned int backlog_size)
  : plans(problem)
  , output_dir_{output_dir}
  , filename_prefix_{filename_prefix}
  , cmd_{cmd}
  , timeout_{timeout}

  // NOTE: Heuristic guess for size of events assuming that most closed written
  // files are plans. +4 for .X and a bit of slack
  , buf_len_{128 * (EVENT_SIZE + filename_prefix.size() + 4)}
  , backlog_max_{backlog_size}
{
    inot_fd_ = inotify_init();
    if (inot_fd_ < 0)
    {
        throw std::runtime_error("Failed to initialize inotify instance!");
    }

    pfd_.fd = inot_fd_;
    pfd_.events = POLLIN;
    pfd_.revents = 0;

    watch_fd_ = inotify_add_watch(inot_fd_, output_dir.c_str(), IN_CLOSE_WRITE);
    if (watch_fd_ < 0)
    {
        throw std::runtime_error("Failed to add inotify watch!");
    }

    event_buf_ = new char[buf_len_];
    if (start_immediately)
    {
        start();
    }
}

PlanWatcher::~PlanWatcher()
{
    stop();
    inotify_rm_watch(inot_fd_, watch_fd_);
    close(inot_fd_);
    delete[] event_buf_;
}

void PlanWatcher::start()
{
    if (planner_is_started_)
    {
        return;
    }

    std::tie(planner_proc_file_, planner_pid_) = startPlanner();
    if (planner_proc_file_ == nullptr)
    {
        throw std::runtime_error("Planner failed to start!");
    }

    auto planner_proc_fd = fileno(planner_proc_file_);
    auto flags = fcntl(planner_proc_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(planner_proc_fd, F_SETFL, flags);
    planner_is_started_ = true;
    planner_is_running_ = true;
}

void PlanWatcher::stop()
{
    if (not planner_is_started_ or planner_proc_file_ == nullptr)
    {
        return;
    }

    // NOTE: Right now, this doesn't wait for the planner to exit, which can result in a few extra
    // plan files being created after the FDPlanner dtor (which deletes plan files) gets called.
    // This shouldn't cause any real problems other than a bit of /tmp clutter, but we could also
    // try waiting here and see how much time it costs. I (Wil) chose this approach because I'd
    // rather have it be as cheap as possible to get rid of one planner and switch to another
    kill(planner_pid_, SIGTERM);
    close(fileno(planner_proc_file_));
}

void PlanWatcher::resume()
{
    if (planner_is_running_)
    {
        return;
    }

    if (not planner_is_started_)
    {
        start();
        return;
    }

    if (kill(planner_pid_, SIGCONT) == 0)
    {
        planner_is_running_ = true;
    }
}

void PlanWatcher::pause()
{
    if (not planner_is_started_ or not planner_is_running_)
    {
        return;
    }

    if (kill(planner_pid_, SIGTSTP) == 0)
    {
        planner_is_running_ = false;
    }
}

auto PlanWatcher::plannerRunning() -> bool
{
    if (not planner_is_started_)
    {
        return false;
    }

    if (feof(planner_proc_file_))
    {
        planner_is_started_ = false;
        planner_is_running_ = false;
        return false;
    }

    while (fgets(planner_buf_.data(), 128, planner_proc_file_) != nullptr)
    {
    }

    return true;
}

auto PlanWatcher::getNextPlanIdx() -> std::optional<std::size_t>
{
    if (not planner_is_started_)
    {
        start();
    }

    if (plan_idx >= plans.count())
    {
        if (not readPlans())
        {
            return std::nullopt;
        }
    }

    return plan_idx++;
}

auto PlanWatcher::readPlans() -> bool
{
    bool added_a_plan = false;
    bool keep_polling = true;
    int poll_ret = 0;
    // HACK: This whole loop is a bit of a hack - if we really wanted fine-grained as-needed plan
    // retrieval, we'd want to persist the list of plan filenames, keep the pause/resume logic, and
    // addPlan the next plan from the next plan filename as requested.
    // This version will pay a slight bit of overhead in reading more plans than strictly requested,
    // and may be sloppy with pausing at the right backlog size
    do
    {
        poll_ret = poll(&pfd_, 1, timeout_);
        if (poll_ret > 0)
        {
            keep_polling = false;
            const auto read_len = read(inot_fd_, event_buf_, buf_len_);
            if (read_len < 0)
            {
                if (errno != EINTR)
                {
                    throw std::runtime_error("Failed to read inotify events!");
                }
            }

            // NOTE: We take this two-pass approach because parsing may be slow and we don't want to
            // let the planner keep spewing out solutions while waiting for parsing
            unsigned int event_bytes = 0;
            unsigned int new_plans = 0;
            while (event_bytes < read_len)
            {
                const auto *event = reinterpret_cast<inotify_event *>(&event_buf_[event_bytes]);
                if (event->len > 0)
                {
                    std::string_view name(event->name);
                    if (name.compare(0, filename_prefix_.size(), filename_prefix_.c_str()) == 0)
                    {
                        plan_filename_backlog_[new_plans] = name;
                        ++new_plans;
                    }
                }

                event_bytes += EVENT_SIZE + event->len;
            }

            const auto new_total_plans = plans.count() + new_plans;
            if (planner_is_running_ and new_total_plans > plan_idx and
                new_total_plans - plan_idx >= backlog_max_)
            {
                pause();
            }

            for (unsigned int i = 0; i < new_plans; ++i)
            {
                if (not addPlan(plan_filename_backlog_[i]))
                {
                    return false;
                }
                else
                {
                    added_a_plan = true;
                }
            }
        }
        else if (poll_ret == 0 and keep_polling and not planner_is_running_)
        {
            resume();
        }
    } while (keep_polling or
             (poll_ret > 0 and (plans.count() > plan_idx and plans.count() - plan_idx < backlog_max_)));

    return added_a_plan;
}

auto PlanWatcher::addPlan(std::string_view filename) -> bool
{
    std::filesystem::path full_path = output_dir_ / filename;
    ast::Plan parsed;
    parseFile(full_path, parsed);
    auto &plan = plans.newPlan();
    for (const auto &action : parsed.actions)
    {
        std::vector<std::string> objects;
        for (const auto &tl : action.variables)
            for (const auto &t : tl.types)
                objects.emplace_back(t);

        plan.addStep(action.name, objects);
    }

    if (not plan.validate())
    {
        return false;
    }

    return true;
}

std::size_t PlanWatcher::getPlanIdx() const
{
    return plan_idx;
}

PlanSet &PlanWatcher::getPlans()
{
    return plans;
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

void Planner::blockExpressionAction(const ActionPtr &action, const Assignment &act_assignment,
                                    const ExpressionPtr &expression, const Assignment &exp_assignment)
{
    blockExpressionActions({{action, act_assignment}}, expression, exp_assignment);
}

void Planner::blockExpressionActions(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
    const ExpressionPtr &expression, const Assignment &exp_assignment)
{
    throw std::runtime_error("Not implemented!");
}

void Planner::addConstraint(Planner::ConstraintTime time, const ExpressionPtr &expression, bool value)
{
    throw std::runtime_error("Not implemented!");
}

auto pddlboat::PlanWatcher::startPlanner() -> std::pair<FILE *, pid_t>
{
    int pdes[2];
    pid_t ppid;
    pipe(pdes);
    switch (ppid = fork())
    {
        case -1:
            close(pdes[0]);
            close(pdes[1]);
            return {nullptr, -1};
        case 0:
            dup2(pdes[1], fileno(stdout));
            close(pdes[1]);
            close(pdes[0]);
            execl("/bin/sh", "sh", "-c", cmd_.c_str(), NULL);
            _exit(127);
    }

    FILE *pout = fdopen(pdes[0], "r");
    close(pdes[1]);

    return {pout, ppid};
}
