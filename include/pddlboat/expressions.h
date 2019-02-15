#ifndef PDDLBOAT_EXPRESSIONS_
#define PDDLBOAT_EXPRESSIONS_

#include <memory>
#include <string>
#include <vector>

#define PDDLBOAT_CLASS_FORWARD(C)                                              \
  class C;                                                                     \
  typedef std::shared_ptr<C> C##Ptr;                                           \
  typedef std::shared_ptr<const C> C##ConstPtr;

namespace pddlboat {
PDDLBOAT_CLASS_FORWARD(Expression)
PDDLBOAT_CLASS_FORWARD(Predicate)
PDDLBOAT_CLASS_FORWARD(Function)
PDDLBOAT_CLASS_FORWARD(State)

class Expression {
public:
  virtual bool evaluate(const StatePtr &state) const = 0;
};

class State {
public:
  virtual const bool isTrue(const Predicate &predicate) const = 0;
  virtual const std::string evalFun(const Function &function) const = 0;
};

class Predicate : public Expression {
public:
  const std::string name;
  const std::vector<std::string> variables;
  const std::vector<std::string> types;

  bool evaluate(const StatePtr &state) const { return state->isTrue(*this); }
};

class Function {
public:
  const std::string name;
  const std::vector<std::string> variables;
  const std::vector<std::string> types;

  const std::string evaluate(const StatePtr &state) const {
    return state->evalFun(*this);
  }
};

class Action {
public:
  const std::string name;
  ExpressionPtr preconditions;
  ExpressionPtr effects;
};

PDDLBOAT_CLASS_FORWARD(VariadicExpression)
class VariadicExpression : public Expression {
public:
  VariadicExpression() {}

  VariadicExpression(const std::initializer_list<const ExpressionPtr> &terms) {
    for (auto &term : terms)
      addTerm(term);
  }

  void addTerm(const ExpressionPtr &expression) {
    terms_.emplace_back(expression);
  }

protected:
  std::vector<ExpressionPtr> terms_;
};

class And : public VariadicExpression {
public:
  And(const std::initializer_list<const ExpressionPtr> terms)
      : VariadicExpression(terms) {}

  bool evaluate(const StatePtr &state) const override {
    bool ret = true;
    for (unsigned int i = 0; i < terms_.size() && ret; ++i)
      ret &= terms_[i]->evaluate(state);

    return ret;
  }
};

class Or : public VariadicExpression {
public:
  Or(const std::initializer_list<const ExpressionPtr> terms)
      : VariadicExpression(terms) {}

  bool evaluate(const StatePtr &state) const override {
    bool ret = false;
    for (unsigned int i = 0; i < terms_.size() && !ret; ++i)
      ret |= terms_[i]->evaluate(state);

    return ret;
  }
};

class Not : public Expression {
public:
  Not(const ExpressionPtr &term) : term_(term) {}

  bool evaluate(const StatePtr &state) const override {
    return not term_->evaluate(state);
  }

private:
  const ExpressionPtr term_;
};

class Equals : public VariadicExpression {
public:
  Equals(const std::initializer_list<const ExpressionPtr> terms)
      : VariadicExpression(terms) {}

  bool evaluate(const StatePtr &state) const override {
    bool allTrue = true;
    bool allFalse = true;

    for (unsigned int i = 0; i < terms_.size() && (allTrue || allFalse); ++i) {
      const bool eval = terms_[i]->evaluate(state);
      allTrue &= eval;
      allFalse &= not eval;
    }

    return allTrue || allFalse;
  };
};

class Imply : public Expression {
public:
  Imply(const ExpressionPtr &left, const ExpressionPtr &right)
      : left_(left), right_(right) {}

  bool evaluate(const StatePtr &state) const override {
    return (not left_->evaluate(state)) or right_->evaluate(state);
  }

private:
  const ExpressionPtr left_, right_;
};

// class Forall : public Expression
// {
// public:
//     bool evaluate(const StatePtr &state) const override
//     {
//     }
// };

// class Exists : public Expression
// {
// public:
//     bool evaluate(const StatePtr &state) const override
//     {
//     }
// };

// class Is : public Expression
// {
// public:
//     bool evaluate(const StatePtr &state) const override
//     {
//     }
// };

} // namespace pddlboat

#endif
