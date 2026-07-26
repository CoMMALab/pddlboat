/* Madagascar-style SAT planner. See madagascarplanner.hpp for an overview. */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <z3++.h>
#include <pddlboat/solver/madagascarplanner.hpp>

namespace
{
    bool isTrueExpr(const z3::expr &expr)
    {
        return expr.is_app() and Z3_OP_TRUE == expr.decl().decl_kind();
    }

    // Literal over ground atoms: +(atom + 1) for positive, -(atom + 1) for negative.
    using Lit = std::int32_t;

    Lit posLit(std::size_t atom)
    {
        return static_cast<Lit>(atom) + 1;
    }

    Lit negLit(std::size_t atom)
    {
        return -(static_cast<Lit>(atom) + 1);
    }

    std::size_t litAtom(Lit l)
    {
        return static_cast<std::size_t>((l > 0 ? l : -l) - 1);
    }

    bool sortedContains(const std::vector<int> &v, int x)
    {
        return std::binary_search(v.begin(), v.end(), x);
    }
}  // namespace

namespace pddlboat
{
    class MadagascarPlannerInternal
    {
    public:
        MadagascarPlannerInternal(const ProblemPtr &problem, const MadagascarPlanner::Options &options)
          : problem_(problem), options_(options)
        {
            checkSupported();
            ground();

            if (options_.invariants)
                computeInvariants();

            if (options_.semantics == MadagascarPlanner::Semantics::EXISTS_STEP)
                buildDisablingGraphChains();
            else
                buildForallMutexes();

            reset();
        }

        bool solve(Plan &plan, unsigned int timeout);
        void reset();

        void blockStateActions(const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
                               const StatePtr &state);
        void blockStateActionsSimultaneous(
            const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
            const StatePtr &state);
        void
        blockExpressionActions(const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
                               const ExpressionPtr &expression, const Assignment &exp_assignment);
        void addConstraint(Planner::ConstraintTime time, const ExpressionPtr &expression, bool value);

    private:
        /** \brief One conditional effect of a ground action: the literals asserted when every
         *  antecedent in `conditions` holds. Nested `when`s stack their antecedents; an enclosing
         *  `forall` yields one of these per grounding, with `assignment` binding its variables.
         */
        struct CondEffect
        {
            std::vector<ExpressionPtr> conditions;  ///< Antecedents, conjoined.
            Assignment assignment;                  ///< Binding for translating `conditions`.
            std::vector<int> add, del;              ///< Asserted atom indices (sorted).
            std::vector<int> cond_atoms;            ///< Atoms occurring in `conditions` (sorted).
            std::size_t id{0};                      ///< Index into the flat conditional effect table.
        };

        /** \brief Static analysis of a single ground action.
         *
         *  Effects split into the unconditional part and the conditional effects. Analyses that
         *  must not miss a change (interference, invariant threats) use the `may_*` sets, which
         *  add every conditional literal; analyses that need a guarantee (invariant rescues) use
         *  `add`/`del` alone, since a conditional effect need not fire.
         */
        struct GroundActionInfo
        {
            GroundedActionPtr ga;
            std::vector<int> add, del;          ///< Unconditional effect atom indices (sorted).
            std::vector<CondEffect> ces;        ///< Conditional effects.
            std::vector<int> may_add, may_del;  ///< Unconditional plus conditional (sorted).
            std::vector<int> pre_pos, pre_neg;  ///< Over-approximate precondition literals (sorted).
            std::vector<int> must_pos, must_neg;  ///< Guaranteed precondition literals (sorted).
        };

        /** \brief A reference to the encoding variable that carries an effect: the action variable
         *  for unconditional effects, or the conditional effect's own variable.
         */
        struct EffectRef
        {
            int action;
            int ce;  ///< Flat conditional effect index, or -1 for the unconditional effect.
        };

        /** \brief Chain constraint specification for a literal within an SCC: falsifiers and
         *  requirers of the literal, each sorted by global action index.
         */
        struct ChainSpec
        {
            Lit lit;
            std::vector<int> falsifiers;
            std::vector<int> requirers;
        };

        /** \brief An independent solver for one fixed horizon.
         */
        struct HorizonSolver
        {
            Time horizon;
            std::unique_ptr<z3::context> ctx;
            std::unique_ptr<z3::solver> solver;
            std::vector<std::vector<z3::expr>> atom_vars;  ///< [t][atom], t in [0, horizon].
            std::vector<std::vector<z3::expr>> act_vars;   ///< [t][action], t in [0, horizon).
            std::vector<std::vector<z3::expr>> ce_vars;    ///< [t][conditional effect].
            std::size_t aux_counter{0};                    ///< Fresh auxiliary variable counter.
            bool built{false};

            HorizonSolver(Time h) : horizon(h)
            {
            }
        };

        void checkSupported() const;
        void ground();
        void collectPrecLiterals(const ExpressionPtr &expr, const Assignment &assignment, bool negated,
                                 bool definite, GroundActionInfo &info) const;
        void collectEffects(const ExpressionPtr &expr, const Assignment &assignment, bool negated,
                            std::vector<ExpressionPtr> &conditions, int target,
                            GroundActionInfo &info) const;
        void collectCondAtoms(const ExpressionPtr &expr, const Assignment &assignment,
                              std::vector<int> &atoms) const;
        void computeInvariants();
        void buildDisablingGraphChains();
        void buildForallMutexes();

        std::size_t getAtomIndex(const PredicatePtr &predicate, const Assignment &assignment) const;
        std::size_t getGroundActionIndex(const ActionPtr &action, const Assignment &assignment) const;

        void encode(HorizonSolver &hs);
        void addStep(HorizonSolver &hs, Time t);
        z3::expr translate(HorizonSolver &hs, const ExpressionPtr &expr, const Assignment &assignment,
                           Time step) const;
        z3::expr stateExpr(HorizonSolver &hs, const StatePtr &state, Time step) const;
        z3::expr litExpr(HorizonSolver &hs, Lit l, Time step) const;
        void addClause(HorizonSolver &hs, const z3::expr &clause) const;

        // Constraint replay: emit recorded constraints into a horizon solver.
        void emitStateActionConstraint(HorizonSolver &hs, const StatePtr &state,
                                       const std::set<std::size_t> &indices, Time step,
                                       bool simultaneous) const;
        void emitExpressionConstraint(HorizonSolver &hs, const GroundedExpression &exp,
                                      const std::set<std::size_t> &indices, Time step) const;
        void emitExpStateConstraint(HorizonSolver &hs, const ExpressionPtr &exp, bool value,
                                    Time step) const;
        void replayConstraints(HorizonSolver &hs) const;

        bool buildPlan(Plan &plan, HorizonSolver &hs) const;

        ProblemPtr problem_;
        MadagascarPlanner::Options options_;

        /** \brief A ground atom. Enumerated over all type-consistent argument tuples (predicate
         *  declarations may repeat a variable name, so groundings of the declared variable list
         *  would miss atoms).
         */
        struct AtomInfo
        {
            PredicateDefinitionPtr predicate;
            std::vector<std::string> objects;
            std::string string;
        };

        // Ground atom table.
        std::vector<AtomInfo> atoms_;
        std::map<std::string, std::size_t> atom_index_;
        std::vector<bool> init_true_;  ///< Closed-world initial value per atom.

        // Ground action table.
        std::vector<GroundActionInfo> actions_;
        std::map<std::string, std::size_t> action_index_;

        // Per-atom occurrence lists (indices into actions_, ascending).
        std::vector<std::vector<int>> pos_req_, neg_req_;

        // Per-atom lists of the effects that may change it, and of the actions whose conditional
        // effect antecedents mention it (either polarity, since both firing and not firing matter).
        std::vector<std::vector<EffectRef>> adders_, deleters_;
        std::vector<std::vector<int>> cond_req_;
        std::size_t ce_count_{0};  ///< Total conditional effects over all ground actions.

        // Invariants: unit literals and 2-literal clauses, all true in every reachable state.
        std::vector<Lit> invariant_units_;
        std::vector<std::pair<Lit, Lit>> invariant_pairs_;

        // ∃-step machinery: chains per SCC, and the guaranteed-executable order of actions.
        std::vector<std::vector<ChainSpec>> scc_chains_;  ///< Chains for SCCs with >= 2 members.
        std::vector<std::pair<std::size_t, std::size_t>> exec_rank_;  ///< action -> (scc rank, index).

        // ∀-step machinery: unordered mutex action pairs from disabling edges.
        std::vector<std::pair<int, int>> mutex_pairs_;

        // Recorded constraints (persist across reset, replayed into every solver).
        StateMap<std::set<std::size_t>> constraints_;
        std::vector<std::pair<StatePtr, std::set<std::size_t>>> simultaneous_constraints_;
        std::vector<std::pair<GroundedExpression, std::set<std::size_t>>> expression_constraints_;
        std::vector<std::pair<ExpressionPtr, bool>> end_state_constraints_;
        std::vector<std::pair<ExpressionPtr, bool>> all_state_constraints_;

        // Search state.
        std::vector<std::unique_ptr<HorizonSolver>> active_;
        Time next_horizon_{0};
        HorizonSolver *winner_{nullptr};
        bool solved_{false};
    };
}  // namespace pddlboat

using namespace pddlboat;

///
/// MadagascarPlanner
///

MadagascarPlanner::MadagascarPlanner(const ProblemPtr &problem, const Options &options)
  : Planner(problem), internal_(std::make_shared<MadagascarPlannerInternal>(problem, options))
{
    checkRequirements();
}

void MadagascarPlanner::reset()
{
    internal_->reset();
}

Requirements::Keyword MadagascarPlanner::getSupportedRequirements() const
{
    return Requirements::STRIPS                     //
           | Requirements::TYPING                   //
           | Requirements::NEGATIVE_PRECONDITIONS   //
           | Requirements::DISJUNCTIVE_PRECONDITIONS  //
           | Requirements::EQUALITY                 //
           | Requirements::QUANTIFIED_PRECONDITIONS  //
           | Requirements::CONDITIONAL_EFFECTS;
}

bool MadagascarPlanner::solve(Plan &plan)
{
    return solve(plan, 0);
}

bool MadagascarPlanner::solve(Plan &plan, unsigned int timeout)
{
    return internal_->solve(plan, timeout);
}

void MadagascarPlanner::blockStateActions(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions, const StatePtr &state)
{
    internal_->blockStateActions(actions, state);
}

void MadagascarPlanner::blockStateActionsSimultaneous(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions, const StatePtr &state)
{
    internal_->blockStateActionsSimultaneous(actions, state);
}

void MadagascarPlanner::blockExpressionActions(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
    const ExpressionPtr &expression, const Assignment &exp_assignment)
{
    internal_->blockExpressionActions(actions, expression, exp_assignment);
}

void MadagascarPlanner::addConstraint(Planner::ConstraintTime time, const ExpressionPtr &expression,
                                      bool value)
{
    internal_->addConstraint(time, expression, value);
}

///
/// MadagascarPlannerInternal: static analysis
///

void MadagascarPlannerInternal::checkSupported() const
{
    const auto &domain = problem_->domain;

    if (not domain->functions.empty())
        throw std::runtime_error("MadagascarPlanner does not support numeric or object fluents.");

    if (not domain->derived.empty())
        throw std::runtime_error("MadagascarPlanner does not support derived predicates.");

    if (not domain->axioms.empty())
        throw std::runtime_error("MadagascarPlanner does not support domain axioms.");
}

void MadagascarPlannerInternal::ground()
{
    // Ground atom table: all type-consistent argument tuples per predicate. IDs are built in
    // the same format as Predicate::toID / PredicateDefinition::toID.
    for (const auto &[name, predicate] : problem_->domain->predicates)
    {
        std::vector<std::vector<std::string>> candidates;
        for (const auto &variable : predicate->variables)
            candidates.emplace_back(problem_->objectsOfType(variable.type));

        std::vector<std::size_t> odometer(candidates.size(), 0);
        while (true)
        {
            std::vector<std::string> objects;
            for (std::size_t i = 0; i < candidates.size(); ++i)
                objects.emplace_back(candidates[i][odometer[i]]);

            std::stringstream id;
            id << "PRED:" << name << ":ARGS:";
            for (std::size_t i = 0; i < objects.size(); ++i)
                id << objects[i] << ((i < objects.size() - 1) ? "," : "");

            atom_index_.emplace(id.str(), atoms_.size());
            init_true_.push_back(problem_->start->isTrue(name, objects));
            atoms_.push_back({predicate, std::move(objects), id.str()});

            // Advance the odometer over argument tuples.
            std::size_t position = 0;
            for (; position < candidates.size(); ++position)
            {
                if (++odometer[position] < candidates[position].size())
                    break;
                odometer[position] = 0;
            }
            if (position == candidates.size())
                break;
        }
    }

    adders_.resize(atoms_.size());
    deleters_.resize(atoms_.size());
    pos_req_.resize(atoms_.size());
    neg_req_.resize(atoms_.size());
    cond_req_.resize(atoms_.size());

    // Ground action table.
    for (const auto &[name, action] : problem_->domain->actions)
    {
        const auto &assignments = problem_->getGroundings(action->parameters);
        for (const auto &assignment : assignments)
        {
            GroundActionInfo info;
            info.ga = std::make_shared<GroundedAction>(action, assignment);

            std::vector<ExpressionPtr> conditions;
            collectEffects(action->effect, assignment, false, conditions, -1, info);
            collectPrecLiterals(action->precondition, assignment, false, true, info);

            auto tidy = [](std::vector<int> &v) {
                std::sort(v.begin(), v.end());
                v.erase(std::unique(v.begin(), v.end()), v.end());
            };

            tidy(info.add);
            tidy(info.del);
            for (auto *v : {&info.pre_pos, &info.pre_neg, &info.must_pos, &info.must_neg})
                tidy(*v);

            info.may_add = info.add;
            info.may_del = info.del;

            // Conditional effects that assert nothing constrain nothing.
            info.ces.erase(std::remove_if(info.ces.begin(), info.ces.end(),
                                          [](const CondEffect &ce) {
                                              return ce.add.empty() and ce.del.empty();
                                          }),
                           info.ces.end());

            for (auto &ce : info.ces)
            {
                tidy(ce.add);
                tidy(ce.del);
                tidy(ce.cond_atoms);

                ce.id = ce_count_++;
                info.may_add.insert(info.may_add.end(), ce.add.begin(), ce.add.end());
                info.may_del.insert(info.may_del.end(), ce.del.begin(), ce.del.end());
            }

            tidy(info.may_add);
            tidy(info.may_del);

            const int index = static_cast<int>(actions_.size());
            for (int p : info.add)
                adders_[p].push_back({index, -1});
            for (int p : info.del)
                deleters_[p].push_back({index, -1});
            for (const auto &ce : info.ces)
            {
                for (int p : ce.add)
                    adders_[p].push_back({index, static_cast<int>(ce.id)});
                for (int p : ce.del)
                    deleters_[p].push_back({index, static_cast<int>(ce.id)});
                for (int p : ce.cond_atoms)
                    cond_req_[p].push_back(index);
            }
            for (int p : info.pre_pos)
                pos_req_[p].push_back(index);
            for (int p : info.pre_neg)
                neg_req_[p].push_back(index);

            action_index_.emplace(action->toID(assignment), actions_.size());
            actions_.emplace_back(std::move(info));
        }
    }

    for (auto &v : cond_req_)
        v.erase(std::unique(v.begin(), v.end()), v.end());  // Built in ascending action order.

    if (options_.diagnostics)
        std::cout << log::format("Madagascar: %1% ground atoms, %2% ground actions, "
                                 "%3% conditional effects",
                                 atoms_.size(), actions_.size(), ce_count_)
                  << std::endl;
}

/**
 * Collects the effect of an action into its unconditional literals and its conditional effects.
 * `negated` tracks polarity through NOT; `conditions` is the stack of enclosing `when` antecedents.
 *
 * A `when` starts a new conditional effect holding the literals directly beneath it, and recursion
 * continues so that nested `when`s produce their own effects with the antecedents conjoined. A
 * `forall` is expanded over its groundings, so a `when` beneath one yields a separate conditional
 * effect per grounding, while a `forall` beneath a `when` merely adds literals to it. This mirrors
 * the reference implementation's NEWlocateconditionals/NEWinserteffectlits pair.
 */
void MadagascarPlannerInternal::collectEffects(const ExpressionPtr &expr, const Assignment &assignment,
                                               bool negated, std::vector<ExpressionPtr> &conditions,
                                               int target, GroundActionInfo &info) const
{
    const auto &type = expr->type();
    switch (type)
    {
        case Expression::Type::PREDICATE:
        {
            const auto &predicate = std::static_pointer_cast<Predicate>(expr);
            const int atom = static_cast<int>(getAtomIndex(predicate, assignment));

            // Literals land in the innermost enclosing `when`, or in the unconditional effect.
            auto &add = (target < 0) ? info.add : info.ces[target].add;
            auto &del = (target < 0) ? info.del : info.ces[target].del;
            (negated ? del : add).push_back(atom);
            return;
        }
        case Expression::Type::NOT:
        {
            const auto &not_expr = std::static_pointer_cast<Not>(expr);
            collectEffects(not_expr->getTerm(), assignment, not negated, conditions, target, info);
            return;
        }
        case Expression::Type::AND:
        {
            const auto &variadic = std::static_pointer_cast<VariadicExpression>(expr);
            for (const auto &term : variadic->getTerms())
                collectEffects(term, assignment, negated, conditions, target, info);
            return;
        }
        case Expression::Type::WHEN:
        {
            const auto &when_expr = std::static_pointer_cast<When>(expr);
            if (negated)
                throw std::runtime_error("MadagascarPlanner: negated conditional effect.");

            // An antecedent mentioning no predicate is decided at grounding time: a false one
            // makes the effect dead, a true one leaves the consequent as conditional as it was.
            if (not isIn(Expression::Type::PREDICATE, when_expr->getLeft()->getTypes()))
            {
                if (when_expr->getLeft()->evaluate(problem_->start, assignment))
                    collectEffects(when_expr->getRight(), assignment, negated, conditions, target, info);
                return;
            }

            conditions.push_back(when_expr->getLeft());

            CondEffect ce;
            ce.conditions = conditions;
            ce.assignment = assignment;
            for (const auto &condition : conditions)
                collectCondAtoms(condition, assignment, ce.cond_atoms);

            const int nested = static_cast<int>(info.ces.size());
            info.ces.emplace_back(std::move(ce));

            collectEffects(when_expr->getRight(), assignment, negated, conditions, nested, info);
            conditions.pop_back();
            return;
        }
        case Expression::Type::FORALL:
        {
            const auto &quantifier = std::static_pointer_cast<QuantifierExpression>(expr);

            auto copy = assignment;
            const auto &groundings = problem_->getGroundings(quantifier->getDomain());
            for (const auto &grounding : groundings)
            {
                copy.fold(grounding);
                collectEffects(quantifier->getFormula(), copy, negated, conditions, target, info);
            }
            return;
        }
        default:
            throw std::runtime_error(
                log::format("MadagascarPlanner: unsupported expression type %1% in effect.", type));
    }
}

/**
 * Collects the atoms occurring in a conditional effect antecedent, ignoring polarity: an action
 * that changes such an atom may flip whether the effect fires, either way around.
 */
void MadagascarPlannerInternal::collectCondAtoms(const ExpressionPtr &expr, const Assignment &assignment,
                                                 std::vector<int> &atoms) const
{
    const auto &type = expr->type();
    switch (type)
    {
        case Expression::Type::PREDICATE:
        {
            const auto &predicate = std::static_pointer_cast<Predicate>(expr);
            atoms.push_back(static_cast<int>(getAtomIndex(predicate, assignment)));
            return;
        }
        case Expression::Type::NOT:
            collectCondAtoms(std::static_pointer_cast<Not>(expr)->getTerm(), assignment, atoms);
            return;
        case Expression::Type::AND:
        case Expression::Type::OR:
        {
            const auto &variadic = std::static_pointer_cast<VariadicExpression>(expr);
            for (const auto &term : variadic->getTerms())
                collectCondAtoms(term, assignment, atoms);
            return;
        }
        case Expression::Type::IMPLY:
        {
            const auto &imply_expr = std::static_pointer_cast<Imply>(expr);
            collectCondAtoms(imply_expr->getLeft(), assignment, atoms);
            collectCondAtoms(imply_expr->getRight(), assignment, atoms);
            return;
        }
        case Expression::Type::FORALL:
        case Expression::Type::EXISTS:
        {
            const auto &quantifier = std::static_pointer_cast<QuantifierExpression>(expr);

            auto copy = assignment;
            for (const auto &grounding : problem_->getGroundings(quantifier->getDomain()))
            {
                copy.fold(grounding);
                collectCondAtoms(quantifier->getFormula(), copy, atoms);
            }
            return;
        }
        case Expression::Type::EQUALS:
            return;  // Compile-time constant, contributes no atoms.
        default:
            throw std::runtime_error(log::format(
                "MadagascarPlanner: unsupported expression type %1% in effect condition.", type));
    }
}

/**
 * Collects the atom literals of a precondition. `negated` tracks polarity through NOT.
 * `definite` tracks whether the literal is implied by the precondition (conjunctive context)
 * or only possibly relevant (under a disjunction). The `pre_*` sets over-approximate (used
 * for the disabling graph, where extra edges are sound); the `must_*` sets under-approximate
 * (used for invariant threat checks, where assuming less is sound).
 */
void MadagascarPlannerInternal::collectPrecLiterals(const ExpressionPtr &expr, const Assignment &assignment,
                                                    bool negated, bool definite,
                                                    GroundActionInfo &info) const
{
    const auto &type = expr->type();
    switch (type)
    {
        case Expression::Type::PREDICATE:
        {
            const auto &predicate = std::static_pointer_cast<Predicate>(expr);
            const int atom = static_cast<int>(getAtomIndex(predicate, assignment));

            if (negated)
            {
                info.pre_neg.push_back(atom);
                if (definite)
                    info.must_neg.push_back(atom);
            }
            else
            {
                info.pre_pos.push_back(atom);
                if (definite)
                    info.must_pos.push_back(atom);
            }
            return;
        }
        case Expression::Type::NOT:
        {
            const auto &not_expr = std::static_pointer_cast<Not>(expr);
            collectPrecLiterals(not_expr->getTerm(), assignment, not negated, definite, info);
            return;
        }
        case Expression::Type::AND:
        case Expression::Type::OR:
        {
            const auto &variadic = std::static_pointer_cast<VariadicExpression>(expr);

            // The effective connective after negation: AND stays conjunctive when not negated,
            // OR becomes conjunctive when negated (De Morgan).
            const bool conjunctive = (type == Expression::Type::AND) != negated;
            for (const auto &term : variadic->getTerms())
                collectPrecLiterals(term, assignment, negated, definite and conjunctive, info);
            return;
        }
        case Expression::Type::IMPLY:
        {
            // (imply l r) == (or (not l) r): disjunctive in both branches.
            const auto &imply_expr = std::static_pointer_cast<Imply>(expr);
            collectPrecLiterals(imply_expr->getLeft(), assignment, not negated, false, info);
            collectPrecLiterals(imply_expr->getRight(), assignment, negated, false, info);
            return;
        }
        case Expression::Type::FORALL:
        case Expression::Type::EXISTS:
        {
            const auto &quantifier = std::static_pointer_cast<QuantifierExpression>(expr);

            // forall is conjunctive over groundings unless negated; exists the reverse.
            const bool conjunctive = (type == Expression::Type::FORALL) != negated;

            auto copy = assignment;
            const auto &groundings = problem_->getGroundings(quantifier->getDomain());
            for (const auto &grounding : groundings)
            {
                copy.fold(grounding);
                collectPrecLiterals(quantifier->getFormula(), copy, negated, definite and conjunctive,
                                    info);
            }
            return;
        }
        case Expression::Type::EQUALS:
            return;  // Compile-time constant, contributes no atoms.
        default:
            throw std::runtime_error(
                log::format("MadagascarPlanner: unsupported expression type %1% in precondition.", type));
    }
}

std::size_t MadagascarPlannerInternal::getAtomIndex(const PredicatePtr &predicate,
                                                    const Assignment &assignment) const
{
    const auto &id = predicate->toID(problem_->start, assignment);
    auto it = atom_index_.find(id);
    if (it == atom_index_.end())
        throw std::runtime_error(log::format("MadagascarPlanner: unknown ground atom `%1%`.", id));

    return it->second;
}

std::size_t MadagascarPlannerInternal::getGroundActionIndex(const ActionPtr &action,
                                                            const Assignment &assignment) const
{
    const auto &id = action->toID(assignment);
    auto it = action_index_.find(id);
    if (it == action_index_.end())
        throw std::runtime_error(log::format("MadagascarPlanner: unknown ground action `%1%`.", id));

    return it->second;
}

///
/// Invariant synthesis: Rintanen's iterative weakening at the ground level, clauses of size <= 2.
///

void MadagascarPlannerInternal::computeInvariants()
{
    const auto start_time = std::chrono::steady_clock::now();
    const double budget_seconds = 5.0;
    const std::size_t pair_cap = 200000;

    // Threat tests ask whether a *may* assert a literal, so they range over conditional effects
    // too; rescue tests ask whether a *must* assert one, so they range over unconditional effects
    // only. Conflating the two would keep invariants that a conditional effect can break.
    auto mayAssert = [&](const GroundActionInfo &a, Lit l) {
        return (l > 0) ? sortedContains(a.may_add, static_cast<int>(litAtom(l))) :
                         sortedContains(a.may_del, static_cast<int>(litAtom(l)));
    };
    auto mustAssert = [&](const GroundActionInfo &a, Lit l) {
        return (l > 0) ? sortedContains(a.add, static_cast<int>(litAtom(l))) :
                         sortedContains(a.del, static_cast<int>(litAtom(l)));
    };
    auto inMustPre = [&](const GroundActionInfo &a, Lit l) {
        return (l > 0) ? sortedContains(a.must_pos, static_cast<int>(litAtom(l))) :
                         sortedContains(a.must_neg, static_cast<int>(litAtom(l)));
    };

    // Candidates: units seeded from the (closed-world) initial state; pairs arise by weakening.
    std::unordered_set<Lit> units;
    for (std::size_t i = 0; i < atoms_.size(); ++i)
        units.insert(init_true_[i] ? posLit(i) : negLit(i));

    std::set<std::pair<Lit, Lit>> pairs;  // normalized (min, max)
    auto normalizePair = [](Lit a, Lit b) { return std::make_pair(std::min(a, b), std::max(a, b)); };

    // Is a pre-state satisfying the current candidates, a's guaranteed precondition literals,
    // and `extra` possible? Unit propagation over the candidate clauses until fixpoint; a
    // contradiction proves no such state exists. Conservative: no contradiction found means
    // "possible".
    auto preStateConsistent = [&](const GroundActionInfo &a, Lit extra) {
        std::unordered_set<Lit> assumed;
        std::vector<Lit> queue;

        auto assume = [&](Lit l) {
            if (assumed.count(-l))
                return false;
            if (assumed.insert(l).second)
                queue.push_back(l);
            return true;
        };

        for (int p : a.must_pos)
            if (not assume(posLit(p)))
                return false;
        for (int p : a.must_neg)
            if (not assume(negLit(p)))
                return false;
        if (extra != 0 and not assume(extra))
            return false;

        while (not queue.empty())
        {
            const Lit l = queue.back();
            queue.pop_back();

            if (units.count(-l))
                return false;

            // Pairs containing -l force their other literal.
            for (const auto &[p1, p2] : pairs)
            {
                if (p1 == -l)
                {
                    if (not assume(p2))
                        return false;
                }
                else if (p2 == -l)
                {
                    if (not assume(p1))
                        return false;
                }
            }
        }

        return true;
    };

    bool aborted = false;
    bool changed = true;
    while (changed and not aborted)
    {
        changed = false;

        for (const auto &a : actions_)
        {
            const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() -  //
                                                               start_time)
                                     .count();
            if (elapsed > budget_seconds or pairs.size() > pair_cap)
            {
                aborted = true;
                break;
            }

            // Actions asserting both an atom and its negation can never fire in the encoding;
            // actions inapplicable in every candidate-invariant state threaten nothing either.
            bool dead = false;
            for (int p : a.add)
                if (sortedContains(a.del, p))
                {
                    dead = true;
                    break;
                }
            if (dead or not preStateConsistent(a, 0))
                continue;

            // Literals a may assert (threats), and those it is guaranteed to (rescues).
            std::vector<Lit> effects, forced;
            for (int p : a.may_add)
                effects.push_back(posLit(p));
            for (int p : a.may_del)
                effects.push_back(negLit(p));
            for (int p : a.add)
                forced.push_back(posLit(p));
            for (int p : a.del)
                forced.push_back(negLit(p));

            // A candidate is only threatened if a falsifies one of its literals.
            for (Lit e : effects)
            {
                const Lit falsified = -e;  // a asserts e, so the literal -e is falsified

                // Unit candidate {falsified}: always threatened; weaken to pairs.
                if (units.count(falsified))
                {
                    units.erase(falsified);
                    changed = true;

                    // Only a guaranteed effect can stand in for the lost literal.
                    for (Lit rescue : forced)
                        if (rescue != e and rescue != falsified)
                            pairs.insert(normalizePair(falsified, rescue));
                }

                // Pair candidates containing `falsified`: threatened if the other literal can be
                // false after a: either a also falsifies it, or it persists false from a pre-state
                // where it may be false.
                for (auto it = pairs.begin(); it != pairs.end();)
                {
                    const auto &[l1, l2] = *it;
                    if (l1 != falsified and l2 != falsified)
                    {
                        ++it;
                        continue;
                    }

                    const Lit other = (l1 == falsified) ? l2 : l1;

                    const bool other_false_post =
                        mayAssert(a, -other)  //
                        or (not mustAssert(a, other) and preStateConsistent(a, -other));
                    if (other_false_post)
                    {
                        it = pairs.erase(it);
                        changed = true;
                    }
                    else
                        ++it;
                }
            }
        }
    }

    if (aborted)
    {
        std::cerr << "MadagascarPlanner: invariant synthesis exceeded budget, disabling invariants."
                  << std::endl;
        return;
    }

    invariant_units_.assign(units.begin(), units.end());
    invariant_pairs_.assign(pairs.begin(), pairs.end());

    if (options_.diagnostics)
    {
        const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();
        std::cout << log::format("Madagascar: %1% unit and %2% binary invariants (%3% s)",
                                 invariant_units_.size(), invariant_pairs_.size(), elapsed)
                  << std::endl;
    }
}

///
/// Disabling graph, SCCs (iterative Tarjan), and ∃-step chain specifications.
///

void MadagascarPlannerInternal::buildDisablingGraphChains()
{
    const std::size_t n = actions_.size();

    // Successors of a in the disabling graph: actions whose precondition literals a may falsify,
    // plus actions with a conditional effect antecedent over an atom a may change -- changing it
    // decides whether that effect fires, so the two are order dependent either way around.
    auto successors = [&](std::size_t a) {
        std::vector<int> succ;
        for (int p : actions_[a].may_del)
        {
            succ.insert(succ.end(), pos_req_[p].begin(), pos_req_[p].end());
            succ.insert(succ.end(), cond_req_[p].begin(), cond_req_[p].end());
        }
        for (int p : actions_[a].may_add)
        {
            succ.insert(succ.end(), neg_req_[p].begin(), neg_req_[p].end());
            succ.insert(succ.end(), cond_req_[p].begin(), cond_req_[p].end());
        }

        std::sort(succ.begin(), succ.end());
        succ.erase(std::unique(succ.begin(), succ.end()), succ.end());
        succ.erase(std::remove(succ.begin(), succ.end(), static_cast<int>(a)), succ.end());
        return succ;
    };

    // Iterative Tarjan. SCCs are emitted in reverse topological order of the condensation:
    // an emitted SCC only has edges into previously emitted SCCs, so executing SCCs in
    // emission order guarantees every disabled action runs before its disabler.
    std::vector<int> scc_of(n, -1);
    std::vector<int> low(n, 0), disc(n, -1);
    std::vector<bool> on_stack(n, false);
    std::vector<int> stack;
    std::vector<std::vector<int>> sccs;
    int timer = 0;

    struct Frame
    {
        std::size_t node;
        std::vector<int> succ;
        std::size_t next{0};
    };

    for (std::size_t root = 0; root < n; ++root)
    {
        if (disc[root] != -1)
            continue;

        std::vector<Frame> frames;
        frames.push_back({root, successors(root)});
        disc[root] = low[root] = timer++;
        stack.push_back(static_cast<int>(root));
        on_stack[root] = true;

        while (not frames.empty())
        {
            auto &frame = frames.back();

            if (frame.next < frame.succ.size())
            {
                const std::size_t child = static_cast<std::size_t>(frame.succ[frame.next++]);
                if (disc[child] == -1)
                {
                    disc[child] = low[child] = timer++;
                    stack.push_back(static_cast<int>(child));
                    on_stack[child] = true;
                    frames.push_back({child, successors(child)});
                }
                else if (on_stack[child])
                    low[frame.node] = std::min(low[frame.node], disc[child]);
            }
            else
            {
                if (low[frame.node] == disc[frame.node])
                {
                    std::vector<int> members;
                    int member;
                    do
                    {
                        member = stack.back();
                        stack.pop_back();
                        on_stack[member] = false;
                        scc_of[member] = static_cast<int>(sccs.size());
                        members.push_back(member);
                    } while (member != static_cast<int>(frame.node));

                    std::sort(members.begin(), members.end());
                    sccs.emplace_back(std::move(members));
                }

                const std::size_t node = frame.node;
                frames.pop_back();
                if (not frames.empty())
                    low[frames.back().node] = std::min(low[frames.back().node], low[node]);
            }
        }
    }

    // Execution order: SCC emission rank, then global action index within the SCC.
    exec_rank_.resize(n);
    for (std::size_t a = 0; a < n; ++a)
        exec_rank_[a] = {static_cast<std::size_t>(scc_of[a]), a};

    // Chain specifications per SCC with >= 2 members: for each literal, the members that
    // falsify it and the members that require it, sorted by global index.
    std::size_t total_chains = 0, nontrivial_sccs = 0;
    for (const auto &members : sccs)
    {
        if (members.size() < 2)
            continue;

        nontrivial_sccs++;

        std::map<Lit, ChainSpec> specs;
        auto spec = [&](Lit l) -> ChainSpec & {
            auto it = specs.find(l);
            if (it == specs.end())
                it = specs.emplace(l, ChainSpec{l, {}, {}}).first;
            return it->second;
        };

        for (int member : members)  // ascending: falsifier/requirer lists stay sorted
        {
            const auto &info = actions_[member];
            for (int p : info.may_del)
                spec(posLit(p)).falsifiers.push_back(member);
            for (int p : info.pre_pos)
                spec(posLit(p)).requirers.push_back(member);
            for (int p : info.may_add)
                spec(negLit(p)).falsifiers.push_back(member);
            for (int p : info.pre_neg)
                spec(negLit(p)).requirers.push_back(member);

            // A conditional effect antecedent depends on its atoms at both polarities.
            for (const auto &ce : info.ces)
                for (int p : ce.cond_atoms)
                {
                    spec(posLit(p)).requirers.push_back(member);
                    spec(negLit(p)).requirers.push_back(member);
                }
        }

        // Members were visited in ascending order, but an atom can enter a spec from several of
        // the lists above, so the chain builder's ordering assumption needs restoring.
        std::vector<ChainSpec> chains;
        for (auto &[l, s] : specs)
        {
            for (auto *v : {&s.falsifiers, &s.requirers})
            {
                std::sort(v->begin(), v->end());
                v->erase(std::unique(v->begin(), v->end()), v->end());
            }

            if (not s.falsifiers.empty() and not s.requirers.empty())
                chains.emplace_back(std::move(s));
        }

        if (not chains.empty())
        {
            total_chains += chains.size();
            scc_chains_.emplace_back(std::move(chains));
        }
    }

    if (options_.diagnostics)
        std::cout << log::format("Madagascar: %1% SCCs (%2% nontrivial), %3% chain literals",
                                 sccs.size(), nontrivial_sccs, total_chains)
                  << std::endl;
}

void MadagascarPlannerInternal::buildForallMutexes()
{
    // ∀-step semantics: any disabling edge in either direction forbids co-occurrence.
    std::set<std::pair<int, int>> pairs;
    for (std::size_t a = 0; a < actions_.size(); ++a)
    {
        auto addEdges = [&](const std::vector<int> &requirers) {
            for (int b : requirers)
                if (b != static_cast<int>(a))
                    pairs.emplace(std::min(static_cast<int>(a), b), std::max(static_cast<int>(a), b));
        };

        for (int p : actions_[a].may_del)
        {
            addEdges(pos_req_[p]);
            addEdges(cond_req_[p]);
        }
        for (int p : actions_[a].may_add)
        {
            addEdges(neg_req_[p]);
            addEdges(cond_req_[p]);
        }
    }

    mutex_pairs_.assign(pairs.begin(), pairs.end());

    // In ∀-step mode every action executes independently, so any order works.
    exec_rank_.resize(actions_.size());
    for (std::size_t a = 0; a < actions_.size(); ++a)
        exec_rank_[a] = {0, a};

    if (options_.diagnostics)
        std::cout << log::format("Madagascar: %1% ∀-step mutex pairs", mutex_pairs_.size()) << std::endl;
}

///
/// Encoding
///

void MadagascarPlannerInternal::addClause(HorizonSolver &hs, const z3::expr &clause) const
{
    hs.solver->add(clause);
    if (options_.dump_clauses)
        std::cout << clause << std::endl;
}

z3::expr MadagascarPlannerInternal::litExpr(HorizonSolver &hs, Lit l, Time step) const
{
    const auto &var = hs.atom_vars[step][litAtom(l)];
    return (l > 0) ? var : not var;
}

void MadagascarPlannerInternal::encode(HorizonSolver &hs)
{
    const auto start_time = std::chrono::steady_clock::now();

    hs.ctx = std::make_unique<z3::context>();
    hs.solver = std::make_unique<z3::solver>(*hs.ctx);

    // Variables.
    for (Time t = 0; t <= hs.horizon; ++t)
    {
        std::vector<z3::expr> vars;
        vars.reserve(atoms_.size());
        for (std::size_t i = 0; i < atoms_.size(); ++i)
            vars.push_back(hs.ctx->bool_const(log::format("%1%@%2%", atoms_[i].string, t).c_str()));
        hs.atom_vars.emplace_back(std::move(vars));
    }

    for (Time t = 0; t < hs.horizon; ++t)
    {
        std::vector<z3::expr> vars;
        vars.reserve(actions_.size());
        for (std::size_t a = 0; a < actions_.size(); ++a)
            vars.push_back(hs.ctx->bool_const(log::format("%1%@%2%", actions_[a].ga->string, t).c_str()));
        hs.act_vars.emplace_back(std::move(vars));
    }

    // One variable per conditional effect per step, standing for "this effect fires here".
    for (Time t = 0; t < hs.horizon; ++t)
    {
        std::vector<z3::expr> vars;
        vars.reserve(ce_count_);
        for (const auto &info : actions_)
            for (const auto &ce : info.ces)
                vars.push_back(hs.ctx->bool_const(
                    log::format("%1%:ce%2%@%3%", info.ga->string, ce.id, t).c_str()));
        hs.ce_vars.emplace_back(std::move(vars));
    }

    // Initial state (closed world).
    if (options_.dump_clauses)
        std::cout << ";; Initial state @ horizon " << hs.horizon << ":" << std::endl;
    for (std::size_t i = 0; i < atoms_.size(); ++i)
        addClause(hs, init_true_[i] ? hs.atom_vars[0][i] : not hs.atom_vars[0][i]);

    // Steps.
    for (Time t = 0; t < hs.horizon; ++t)
        addStep(hs, t);

    // Invariants at every non-initial step (step 0 is fully determined).
    if (not invariant_units_.empty() or not invariant_pairs_.empty())
    {
        if (options_.dump_clauses)
            std::cout << ";; Invariants:" << std::endl;
        for (Time t = 1; t <= hs.horizon; ++t)
        {
            for (Lit l : invariant_units_)
                addClause(hs, litExpr(hs, l, t));
            for (const auto &[l1, l2] : invariant_pairs_)
                addClause(hs, litExpr(hs, l1, t) or litExpr(hs, l2, t));
        }
    }

    // Goal at the final step.
    if (options_.dump_clauses)
        std::cout << ";; Goal @ " << hs.horizon << ":" << std::endl;
    addClause(hs, translate(hs, problem_->goal, {}, hs.horizon));

    // Recorded constraints.
    replayConstraints(hs);

    hs.built = true;

    if (options_.diagnostics)
    {
        const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();
        std::cout << log::format("Madagascar: encoded horizon %1% (%2% s)", hs.horizon, elapsed)
                  << std::endl;
    }
}

void MadagascarPlannerInternal::addStep(HorizonSolver &hs, Time t)
{
    // (1) Actions imply their preconditions and effects.
    if (options_.dump_clauses)
        std::cout << ";; Action implications @ " << t << ":" << std::endl;

    for (std::size_t a = 0; a < actions_.size(); ++a)
    {
        const auto &info = actions_[a];
        const auto &act = hs.act_vars[t][a];

        z3::expr body = translate(hs, info.ga->action->precondition, info.ga->assignment, t);
        for (int p : info.add)
            body = body and hs.atom_vars[t + 1][p];
        for (int p : info.del)
            body = body and not hs.atom_vars[t + 1][p];

        addClause(hs, z3::implies(act, body));

        // A conditional effect variable is equivalent to its action taken with its antecedent
        // holding; the frame axioms below then use it as the explanation for its literals.
        for (const auto &ce : info.ces)
        {
            const auto &fires = hs.ce_vars[t][ce.id];

            z3::expr condition = hs.ctx->bool_val(true);
            for (const auto &term : ce.conditions)
                condition = condition and translate(hs, term, ce.assignment, t);

            addClause(hs, z3::implies(act and condition, fires));
            addClause(hs, z3::implies(fires, act));
            addClause(hs, z3::implies(fires, condition));

            z3::expr consequent = hs.ctx->bool_val(true);
            for (int p : ce.add)
                consequent = consequent and hs.atom_vars[t + 1][p];
            for (int p : ce.del)
                consequent = consequent and not hs.atom_vars[t + 1][p];

            addClause(hs, z3::implies(fires, consequent));
        }
    }

    // (2) Explanatory frame axioms.
    if (options_.dump_clauses)
        std::cout << ";; Frame axioms @ " << t << ":" << std::endl;

    for (std::size_t p = 0; p < atoms_.size(); ++p)
    {
        const auto &curr = hs.atom_vars[t][p];
        const auto &next = hs.atom_vars[t + 1][p];

        if (adders_[p].empty() and deleters_[p].empty())
        {
            addClause(hs, curr == next);
            continue;
        }

        auto effectVar = [&](const EffectRef &ref) {
            return (ref.ce < 0) ? hs.act_vars[t][ref.action] : hs.ce_vars[t][ref.ce];
        };

        z3::expr_vector dels(*hs.ctx), adds(*hs.ctx);
        for (const auto &ref : deleters_[p])
            dels.push_back(effectVar(ref));
        for (const auto &ref : adders_[p])
            adds.push_back(effectVar(ref));

        addClause(hs, z3::implies(curr and not next,
                                  dels.empty() ? hs.ctx->bool_val(false) : z3::mk_or(dels)));
        addClause(hs, z3::implies(not curr and next,
                                  adds.empty() ? hs.ctx->bool_val(false) : z3::mk_or(adds)));
    }

    // (3) Step semantics.
    if (options_.dump_clauses)
        std::cout << ";; Step semantics @ " << t << ":" << std::endl;

    if (options_.semantics == MadagascarPlanner::Semantics::EXISTS_STEP)
    {
        // Chain constraints: within an SCC, forbid a falsifier of a literal at a smaller index
        // from co-occurring with a requirer of that literal at a larger index. Any remaining
        // active disabling edges then point backwards in index order, so ascending index order
        // within the SCC is a valid execution order.
        for (const auto &chains : scc_chains_)
            for (const auto &chain : chains)
            {
                std::size_t i = 0, j = 0;
                unsigned int m = 0;
                z3::expr d(*hs.ctx);

                while (j < chain.requirers.size())
                {
                    if (i < chain.falsifiers.size() and chain.falsifiers[i] < chain.requirers[j])
                    {
                        // Falsifier: extend the chain. (A tie means the action both requires and
                        // falsifies the literal; the requirer is processed first so an action
                        // never blocks itself.)
                        z3::expr dn = hs.ctx->bool_const(
                            log::format("mdg-chain-%1%@%2%", hs.aux_counter++, t).c_str());
                        addClause(hs, z3::implies(hs.act_vars[t][chain.falsifiers[i]], dn));
                        if (m > 0)
                            addClause(hs, z3::implies(d, dn));
                        d = dn;
                        ++m;
                        ++i;
                    }
                    else
                    {
                        if (m > 0)
                            addClause(hs, z3::implies(d, not hs.act_vars[t][chain.requirers[j]]));
                        ++j;
                    }
                }
            }
    }
    else
    {
        for (const auto &[a, b] : mutex_pairs_)
            addClause(hs, not(hs.act_vars[t][a] and hs.act_vars[t][b]));
    }
}

z3::expr MadagascarPlannerInternal::translate(HorizonSolver &hs, const ExpressionPtr &expr,
                                              const Assignment &assignment, Time step) const
{
    const auto &type = expr->type();
    switch (type)
    {
        case Expression::Type::PREDICATE:
        {
            const auto &predicate = std::static_pointer_cast<Predicate>(expr);
            return hs.atom_vars[step][getAtomIndex(predicate, assignment)];
        }
        case Expression::Type::AND:
        {
            const auto &and_expr = std::static_pointer_cast<And>(expr);
            z3::expr result = hs.ctx->bool_val(true);
            for (const auto &term : and_expr->getTerms())
                result = result and translate(hs, term, assignment, step);
            return result;
        }
        case Expression::Type::OR:
        {
            const auto &or_expr = std::static_pointer_cast<Or>(expr);
            z3::expr result = hs.ctx->bool_val(false);
            for (const auto &term : or_expr->getTerms())
                result = result or translate(hs, term, assignment, step);
            return result;
        }
        case Expression::Type::NOT:
        {
            const auto &not_expr = std::static_pointer_cast<Not>(expr);
            return not translate(hs, not_expr->getTerm(), assignment, step);
        }
        case Expression::Type::IMPLY:
        {
            const auto &imply_expr = std::static_pointer_cast<Imply>(expr);
            return z3::implies(translate(hs, imply_expr->getLeft(), assignment, step),
                               translate(hs, imply_expr->getRight(), assignment, step));
        }
        case Expression::Type::EXISTS:
        case Expression::Type::FORALL:
        {
            const auto &quantifier = std::static_pointer_cast<QuantifierExpression>(expr);

            auto copy = assignment;
            const auto &groundings = problem_->getGroundings(quantifier->getDomain());

            const bool conjunctive = type == Expression::Type::FORALL;
            z3::expr result = hs.ctx->bool_val(conjunctive);
            for (const auto &grounding : groundings)
            {
                copy.fold(grounding);
                const auto &term = translate(hs, quantifier->getFormula(), copy, step);
                result = conjunctive ? (result and term) : (result or term);
            }
            return result;
        }
        case Expression::Type::EQUALS:
        {
            const auto &equals_expr = std::static_pointer_cast<Equals>(expr);
            const auto &[t1, t2] = equals_expr->getTerms();
            return hs.ctx->bool_val(t1.getObject(problem_->start, assignment) ==
                                    t2.getObject(problem_->start, assignment));
        }
        default:
            throw std::runtime_error(
                log::format("MadagascarPlanner: unsupported expression type %1%.", type));
    }
}

z3::expr MadagascarPlannerInternal::stateExpr(HorizonSolver &hs, const StatePtr &state, Time step) const
{
    // Anything not true in the (clean) state is false.
    z3::expr result = hs.ctx->bool_val(true);
    for (std::size_t i = 0; i < atoms_.size(); ++i)
    {
        const auto &var = hs.atom_vars[step][i];
        result = result and
                 (state->isTrue(atoms_[i].predicate->name, atoms_[i].objects) ? var : not var);
    }

    return result;
}

///
/// Constraint / blocking interface
///

void MadagascarPlannerInternal::emitStateActionConstraint(HorizonSolver &hs, const StatePtr &state,
                                                          const std::set<std::size_t> &indices, Time step,
                                                          bool simultaneous) const
{
    if (indices.empty())
        return;

    z3::expr acts(*hs.ctx);
    bool first = true;
    for (std::size_t index : indices)
    {
        const auto &act = hs.act_vars[step][index];
        if (first)
        {
            acts = act;
            first = false;
        }
        else
            acts = simultaneous ? (acts and act) : (acts or act);
    }

    addClause(hs, z3::implies(stateExpr(hs, state, step), not acts));
}

void MadagascarPlannerInternal::emitExpressionConstraint(HorizonSolver &hs, const GroundedExpression &exp,
                                                         const std::set<std::size_t> &indices,
                                                         Time step) const
{
    if (indices.empty())
        return;

    z3::expr acts = hs.ctx->bool_val(false);
    for (std::size_t index : indices)
        acts = acts or hs.act_vars[step][index];

    addClause(hs, z3::implies(translate(hs, exp.expression, exp.assignment, step), not acts));
}

void MadagascarPlannerInternal::emitExpStateConstraint(HorizonSolver &hs, const ExpressionPtr &exp,
                                                       bool value, Time step) const
{
    addClause(hs, translate(hs, exp, {}, step) == hs.ctx->bool_val(value));
}

void MadagascarPlannerInternal::replayConstraints(HorizonSolver &hs) const
{
    if (options_.dump_clauses)
        std::cout << ";; Recorded constraints:" << std::endl;

    for (Time t = 0; t < hs.horizon; ++t)
    {
        for (const auto &[state, indices] : constraints_)
            emitStateActionConstraint(hs, state, indices, t, false);

        for (const auto &[state, indices] : simultaneous_constraints_)
            emitStateActionConstraint(hs, state, indices, t, true);

        for (const auto &[exp, indices] : expression_constraints_)
            emitExpressionConstraint(hs, exp, indices, t);
    }

    for (const auto &[exp, value] : end_state_constraints_)
        emitExpStateConstraint(hs, exp, value, hs.horizon);

    for (const auto &[exp, value] : all_state_constraints_)
        for (Time t = 0; t <= hs.horizon; ++t)
            emitExpStateConstraint(hs, exp, value, t);
}

void MadagascarPlannerInternal::blockStateActions(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions, const StatePtr &state)
{
    std::set<std::size_t> toBlock;
    for (const auto &[action, assignment] : actions)
        toBlock.emplace(getGroundActionIndex(action, assignment));

    const auto &copy = state->cleanClone();
    auto [it, inserted] = constraints_.emplace(copy, toBlock);
    if (not inserted)
        it->second.insert(toBlock.begin(), toBlock.end());

    for (const auto &hs : active_)
        if (hs->built)
            for (Time t = 0; t < hs->horizon; ++t)
                emitStateActionConstraint(*hs, it->first, toBlock, t, false);

    solved_ = false;
    winner_ = nullptr;
}

void MadagascarPlannerInternal::blockStateActionsSimultaneous(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions, const StatePtr &state)
{
    std::set<std::size_t> toBlock;
    for (const auto &[action, assignment] : actions)
        toBlock.emplace(getGroundActionIndex(action, assignment));

    const auto &copy = state->cleanClone();
    simultaneous_constraints_.emplace_back(copy, toBlock);

    for (const auto &hs : active_)
        if (hs->built)
            for (Time t = 0; t < hs->horizon; ++t)
                emitStateActionConstraint(*hs, copy, toBlock, t, true);

    solved_ = false;
    winner_ = nullptr;
}

void MadagascarPlannerInternal::blockExpressionActions(
    const std::vector<std::pair<const ActionPtr &, const Assignment &>> &actions,
    const ExpressionPtr &expression, const Assignment &exp_assignment)
{
    std::set<std::size_t> toBlock;
    for (const auto &[action, assignment] : actions)
        toBlock.emplace(getGroundActionIndex(action, assignment));

    const auto &record = expression_constraints_.emplace_back(
        std::piecewise_construct, std::forward_as_tuple(expression, exp_assignment),
        std::forward_as_tuple(toBlock));

    for (const auto &hs : active_)
        if (hs->built)
            for (Time t = 0; t < hs->horizon; ++t)
                emitExpressionConstraint(*hs, record.first, record.second, t);

    solved_ = false;
    winner_ = nullptr;
}

void MadagascarPlannerInternal::addConstraint(Planner::ConstraintTime time, const ExpressionPtr &expression,
                                              bool value)
{
    if (time == Planner::AT_END)
    {
        end_state_constraints_.emplace_back(expression, value);

        for (const auto &hs : active_)
            if (hs->built)
                emitExpStateConstraint(*hs, expression, value, hs->horizon);
    }
    else if (time == Planner::OVER_ALL)
    {
        all_state_constraints_.emplace_back(expression, value);

        for (const auto &hs : active_)
            if (hs->built)
                for (Time t = 0; t <= hs->horizon; ++t)
                    emitExpStateConstraint(*hs, expression, value, t);
    }

    solved_ = false;
    winner_ = nullptr;
}

///
/// Search
///

void MadagascarPlannerInternal::reset()
{
    z3::set_param("parallel.enable", options_.parallel);

    active_.clear();
    next_horizon_ = options_.horizon_min;
    winner_ = nullptr;
    solved_ = false;
}

bool MadagascarPlannerInternal::solve(Plan &plan, unsigned int timeout)
{
    if (solved_ and winner_)
        return buildPlan(plan, *winner_);

    const auto start_time = std::chrono::steady_clock::now();
    auto remainingMs = [&]() -> long {
        if (timeout == 0)
            return -1;
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start_time)
                                 .count();
        return static_cast<long>(timeout) - static_cast<long>(elapsed);
    };

    const unsigned int window = options_.algorithm_b ? std::max(1u, options_.window) : 1;
    const Time step = std::max<Time>(1, options_.horizon_step);
    const unsigned int rlimit_min = 10000;

    // Per-round budget scale. Doubling each round makes the cumulative budget of every active
    // horizon unbounded, so each check eventually resolves (a fixed per-round budget could
    // starve a hard horizon forever).
    double round_scale = 1.0;

    while (true)
    {
        // Schedule new horizons into the window.
        while (active_.size() < window and next_horizon_ <= options_.horizon_max)
        {
            active_.emplace_back(std::make_unique<HorizonSolver>(next_horizon_));
            next_horizon_ += step;
        }

        if (active_.empty())
            return false;  // Every horizon up to horizon_max is unsatisfiable.

        // Round-robin over active horizons, shortest first, with geometrically decreasing effort.
        for (std::size_t rank = 0; rank < active_.size(); /* increment below */)
        {
            auto &hs = *active_[rank];

            const long remaining = remainingMs();
            if (timeout > 0 and remaining <= 0)
                return false;

            if (not hs.built)
                encode(hs);

            // Scoped so the params object never outlives the context (a z3 object destructed
            // after its context corrupts the heap; the context dies below on unsat).
            {
                z3::params params(*hs.ctx);
                if (options_.algorithm_b)
                {
                    const double budget = round_scale * static_cast<double>(options_.rlimit_base) *
                                          std::pow(options_.gamma, static_cast<double>(rank));
                    const double clamped =
                        std::min(budget, static_cast<double>(std::numeric_limits<unsigned int>::max()));
                    params.set("rlimit", std::max(rlimit_min, static_cast<unsigned int>(clamped)));
                }
                if (timeout > 0)
                    params.set("timeout", static_cast<unsigned int>(remaining));
                hs.solver->set(params);
            }

            boost::timer::cpu_timer round_timer;
            const auto result = hs.solver->check();
            round_timer.stop();

            if (options_.diagnostics)
                std::cout << log::format("Madagascar: horizon %1% check: %2% (%3%)", hs.horizon,
                                         (result == z3::sat)     ? "sat" :
                                         (result == z3::unsat)   ? "unsat" :
                                                                   "unknown",
                                         io::getTiming(round_timer))
                          << std::endl;

            if (result == z3::sat)
            {
                solved_ = true;
                winner_ = &hs;

                const auto elapsed =
                    std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();
                plan.setTimeTaken("solve", elapsed);

                return buildPlan(plan, hs);
            }

            if (result == z3::unsat)
                active_.erase(active_.begin() + rank);  // This horizon is settled forever.
            else
            {
                if (timeout > 0 and remainingMs() <= 0)
                    return false;
                ++rank;  // Resource limit hit; retry next round with a doubled budget.
            }
        }

        if (options_.algorithm_b)
            round_scale = std::min(round_scale * 2.0, 1e9);
    }
}

bool MadagascarPlannerInternal::buildPlan(Plan &plan, HorizonSolver &hs) const
{
    auto model = hs.solver->get_model();

    for (Time t = 0; t < hs.horizon; ++t)
    {
        // Order actions within the step by the guaranteed-executable order: SCC emission rank,
        // then global index. Plan::validate() executes actions in insertion order.
        std::vector<std::size_t> chosen;
        for (std::size_t a = 0; a < actions_.size(); ++a)
            if (isTrueExpr(model.eval(hs.act_vars[t][a])))
                chosen.push_back(a);

        std::sort(chosen.begin(), chosen.end(),
                  [&](std::size_t a, std::size_t b) { return exec_rank_[a] < exec_rank_[b]; });

        for (std::size_t a : chosen)
        {
            const auto &info = actions_[a];

            std::vector<std::string> args;
            for (const auto &parameter : info.ga->action->parameters)
                args.emplace_back(info.ga->assignment.assigned(parameter.name)->name);

            try
            {
                plan.insertStep(info.ga->action->name, args, t);
            }
            catch (std::exception &e)
            {
                std::cerr << e.what() << std::endl;
                return false;
            }
        }
    }

    return plan.validate();
}
