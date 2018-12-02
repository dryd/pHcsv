#pragma once

#include <vector>
#include <array>
#include <functional>
#include <limits>
#include <cmath>

namespace pH {

namespace details {

static thread_local void* ad_ = nullptr;
struct ad_guard {
  ad_guard(void* ad) { ad_ = ad; }
  ~ad_guard() { ad_ = nullptr; }
};

static const size_t NO_INDEX = std::numeric_limits<size_t>::max();

enum class operation {
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  POW,
  EXP,
  LOG,
  SIN,
  COS,
  TAN,
  ROOT
};

}

class ad {
  class node;
 public:
  class var {
   public:
    var(double value) : index_(), value_(value) {
      if (getAD() == nullptr) {
        throw std::runtime_error("Initializing pH::ad::var outside of generator scope");
      }
      var& added = getAD()->addNode(details::operation::ROOT);
      added.value_ = value_;
      index_ = added.index_;
    }

    var operator+=(var rhs) { return modifyingOperation(rhs, details::operation::ADD); }
    var operator-=(var rhs) { return modifyingOperation(rhs, details::operation::SUBTRACT); }
    var operator*=(var rhs) { return modifyingOperation(rhs, details::operation::MULTIPLY); }
    var operator/=(var rhs) { return modifyingOperation(rhs, details::operation::DIVIDE); }

   private:
    var& modifyingOperation(const var& rhs, details::operation op) {
      index_ = getAD()->addNode(op, index_, rhs.index_).index_;
      return *this;
    }

    friend class node;
    friend class ad;
    explicit var(size_t index) : index_(index), value_(std::numeric_limits<double>::quiet_NaN()) {}

    size_t index_;
    double value_;
  };

  ad(size_t num_independent_variables, std::function<var(const std::vector<var>& variables)> generator)
    : num_independent_variables_(num_independent_variables), has_been_evaluated_(false) {
    std::vector<var> variables;
    for (size_t i = 0; i < num_independent_variables_; i++) {
      variables.push_back(addNode(details::operation::ROOT));
    }

    details::ad_guard adg(reinterpret_cast<void*>(this));
    var end_variable = generator(variables);

    long end_variable_index = static_cast<long>(end_variable.index_);
    for (auto it = tape_.cend() - 1; it != tape_.cbegin() + end_variable_index; it--) {
      tape_.erase(it);
    }
    tape_.shrink_to_fit();
  }

  double eval(const std::vector<double>& variables) {
    if (variables.size() != num_independent_variables_) {
      throw std::runtime_error("Invalid number of variables in pH::ad::eval, should be " + std::to_string(num_independent_variables_) + ", was " + std::to_string(variables.size()));
    }
    size_t n = 0;
    for (; n < num_independent_variables_; n++) {
      tape_[n].variable_.value_ = variables[n];
    }
    for (; n < tape_.size(); n++) {
      auto& node = tape_[n];
      switch (node.operation_) {
        case details::operation::ADD: {
          node.variable_.value_ = tape_[node.parents_[0]].variable_.value_ + tape_[node.parents_[1]].variable_.value_;
          node.adjoint_values_[0] = 1;
          node.adjoint_values_[1] = 1;
          break;
        }
        case details::operation::SUBTRACT: {
          node.variable_.value_ = tape_[node.parents_[0]].variable_.value_ - tape_[node.parents_[1]].variable_.value_;
          node.adjoint_values_[0] = 1;
          node.adjoint_values_[1] = -1;
          break;
        }
        case details::operation::MULTIPLY: {
          const auto& parent0 = tape_[node.parents_[0]];
          const auto& parent1 = tape_[node.parents_[1]];
          node.variable_.value_ = parent0.variable_.value_ * parent1.variable_.value_;
          node.adjoint_values_[0] = parent1.variable_.value_;
          node.adjoint_values_[1] = parent0.variable_.value_;
          break;
        }
        case details::operation::DIVIDE: {
          const auto& parent0 = tape_[node.parents_[0]];
          const auto& parent1 = tape_[node.parents_[1]];
          node.variable_.value_ = parent0.variable_.value_ / parent1.variable_.value_;
          node.adjoint_values_[0] = 1.0 / parent1.variable_.value_;
          node.adjoint_values_[1] = -1.0 / pow(parent0.variable_.value_, 2.0);
          break;
        }
        case details::operation::POW: {
          const auto& parent0 = tape_[node.parents_[0]];
          const auto& parent1 = tape_[node.parents_[1]];
          node.variable_.value_ = pow(parent0.variable_.value_, parent1.variable_.value_);
          node.adjoint_values_[0] = parent1.variable_.value_ * pow(parent0.variable_.value_, parent1.variable_.value_ - 1.0);
          node.adjoint_values_[1] = pow(parent0.variable_.value_, parent1.variable_.value_) * log(parent0.variable_.value_);
          break;
        }
        case details::operation::EXP: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = exp(parent0.variable_.value_);
          node.adjoint_values_[0] = exp(parent0.variable_.value_);
          break;
        }
        case details::operation::LOG: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = log(parent0.variable_.value_);
          node.adjoint_values_[0] = 1.0 / parent0.variable_.value_;
          break;
        }
        case details::operation::SIN: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = sin(parent0.variable_.value_);
          node.adjoint_values_[0] = cos(parent0.variable_.value_);
          break;
        }
        case details::operation::COS: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = cos(parent0.variable_.value_);
          node.adjoint_values_[0] = -sin(parent0.variable_.value_);
          break;
        }
        case details::operation::TAN: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = tan(parent0.variable_.value_);
          node.adjoint_values_[0] = 1.0 + pow(tan(parent0.variable_.value_), 2.0);
          break;
        }
        case details::operation::ROOT:
          break;
      }
    }
    has_been_evaluated_ = true;
    return tape_.back().variable_.value_;
  }

  std::vector<double> gradient() const {
    if (!has_been_evaluated_) {
      throw std::runtime_error("Unable to calculate gradient before eval");
    }
    std::vector<double> grad(tape_.size(), 0.0);
    grad.back() = 1.0;
    for (size_t n = grad.size() - 1; n >= num_independent_variables_; n--) {
      const auto& node = tape_[n];
      if (node.parents_[0] != details::NO_INDEX) {
        grad[tape_[node.parents_[0]].variable_.index_] += grad[node.variable_.index_] * node.adjoint_values_[0];
      }
      if (node.parents_[1] != details::NO_INDEX) {
        grad[tape_[node.parents_[1]].variable_.index_] += grad[node.variable_.index_] * node.adjoint_values_[1];
      }
    }
    grad.resize(num_independent_variables_);
    grad.shrink_to_fit();
    return grad;
  }

 private:
  std::vector<node> tape_;
  size_t num_independent_variables_;
  bool has_been_evaluated_;

  static ad* getAD() { return reinterpret_cast<ad*>(details::ad_); }

  class node {
   public:
    node(size_t index, details::operation op, size_t parent1, size_t parent2)
      : adjoint_values_({std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()}),
        parents_({parent1, parent2}),
        operation_(op),
        variable_(index) {}

    std::array<double, 2> adjoint_values_;
    std::array<size_t, 2> parents_;
    details::operation operation_;

    var variable_;
  };

  var& addNode(details::operation op, size_t parent1 = details::NO_INDEX, size_t parent2 = details::NO_INDEX) {
    tape_.emplace_back(tape_.size(), op, parent1, parent2);
    return tape_.back().variable_;
  }

  static size_t idx(const var& v) { return v.index_; }

  friend var unary(details::operation, const var);
  friend var binary(details::operation, const var, const var);
};

inline ad::var unary(details::operation op, ad::var lhs) { return ad::getAD()->addNode(op, ad::idx(lhs)); }
inline ad::var binary(details::operation op, ad::var lhs, ad::var rhs) { return ad::getAD()->addNode(op, ad::idx(lhs), ad::idx(rhs)); }

inline ad::var operator+(ad::var lhs, ad::var rhs) { return binary(details::operation::ADD, lhs, rhs); }
inline ad::var operator-(ad::var lhs, ad::var rhs) { return binary(details::operation::SUBTRACT, lhs, rhs); }
inline ad::var operator*(ad::var lhs, ad::var rhs) { return binary(details::operation::MULTIPLY, lhs, rhs); }
inline ad::var operator/(ad::var lhs, ad::var rhs) { return binary(details::operation::DIVIDE, lhs, rhs); }

inline ad::var pow(ad::var lhs, ad::var rhs) { return binary(details::operation::POW, lhs, rhs); }
inline double pow(double lhs, double rhs) { return std::pow(lhs, rhs); }

inline ad::var exp(ad::var lhs) { return unary(details::operation::EXP, lhs); }
inline double exp(double lhs) { return std::exp(lhs); }

inline ad::var log(ad::var lhs) { return unary(details::operation::LOG, lhs); }
inline double log(double lhs) { return std::log(lhs); }

inline ad::var sin(ad::var lhs) { return unary(details::operation::SIN, lhs); }
inline double sin(double lhs) { return std::sin(lhs); }

inline ad::var cos(ad::var lhs) { return unary(details::operation::COS, lhs); }
inline double cos(double lhs) { return std::cos(lhs); }

inline ad::var tan(ad::var lhs) { return unary(details::operation::TAN, lhs); }
inline double tan(double lhs) { return std::tan(lhs); }

}  // namespace pH
