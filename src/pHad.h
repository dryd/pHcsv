#pragma once

#include <vector>
#include <map>
#include <array>
#include <functional>
#include <limits>
#include <cmath>

namespace pH {

namespace details {

static thread_local void* ad_ = nullptr;
struct adGuard {
  adGuard(void* ad) { ad_ = ad; }
  ~adGuard() { ad_ = nullptr; }
};

static const size_t NO_INDEX = std::numeric_limits<size_t>::max();

enum class operation {
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  POW,
  MAX,
  MIN,
  EXP,
  LOG,
  SIN,
  COS,
  TAN,
  ABS,
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
      index_ = getAD()->addRoot(value_).index_;
    }

    var operator+=(var rhs) { return modifyingOperation(rhs, details::operation::ADD); }
    var operator-=(var rhs) { return modifyingOperation(rhs, details::operation::SUBTRACT); }
    var operator*=(var rhs) { return modifyingOperation(rhs, details::operation::MULTIPLY); }
    var operator/=(var rhs) { return modifyingOperation(rhs, details::operation::DIVIDE); }

   private:
    var& modifyingOperation(const var& rhs, details::operation op) {
      index_ = getAD()->addBinary(op, *this, rhs).index_;
      return *this;
    }

    friend class node;
    friend class ad;
    explicit var(size_t index) : index_(index), value_(std::numeric_limits<double>::quiet_NaN()) {}

    size_t index_;
    double value_;
  };

  ad(size_t num_independent_variables, std::function<var(const std::vector<var>& variables)> generator)
    : num_independent_variables_(num_independent_variables), num_roots_(num_independent_variables), has_been_evaluated_(false) {
    std::vector<var> variables;
    for (size_t i = 0; i < num_independent_variables_; i++) {
      variables.push_back(addRoot());
    }

    details::adGuard adg(this);
    var end_variable = generator(variables);
    root_cache_.clear();

    long end_variable_index = static_cast<long>(end_variable.index_);
    for (auto it = tape_.cend() - 1; it != tape_.cbegin() + end_variable_index; it--) {
      tape_.erase(it);
    }
    tape_.shrink_to_fit();

    optimizeRoots();
  }

  double eval(const std::vector<double>& variables) {
    if (variables.size() != num_independent_variables_) {
      throw std::runtime_error("Invalid number of variables in pH::ad::eval, should be " + std::to_string(num_independent_variables_) + ", was " + std::to_string(variables.size()));
    }
    size_t n = 0;
    for (; n < num_independent_variables_; n++) {
      tape_[n].variable_.value_ = variables[n];
    }
    for (n = num_roots_; n < tape_.size(); n++) {
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
          node.adjoint_values_[1] = -1.0 / std::pow(parent0.variable_.value_, 2.0);
          break;
        }
        case details::operation::POW: {
          const auto& parent0 = tape_[node.parents_[0]];
          const auto& parent1 = tape_[node.parents_[1]];
          node.variable_.value_ = std::pow(parent0.variable_.value_, parent1.variable_.value_);
          node.adjoint_values_[0] = parent1.variable_.value_ * std::pow(parent0.variable_.value_, parent1.variable_.value_ - 1.0);
          node.adjoint_values_[1] = std::pow(parent0.variable_.value_, parent1.variable_.value_) * std::log(parent0.variable_.value_);
          break;
        }
        case details::operation::MAX: {
          const auto& parent0 = tape_[node.parents_[0]];
          const auto& parent1 = tape_[node.parents_[1]];
          node.variable_.value_ = std::max(parent0.variable_.value_, parent1.variable_.value_);
          if (parent0.variable_.value_ > parent1.variable_.value_) {
            node.adjoint_values_[0] = 1.0;
            node.adjoint_values_[1] = 0.0;
          } else {
            node.adjoint_values_[0] = 0.0;
            node.adjoint_values_[1] = 1.0;
          }
          break;
        }
        case details::operation::MIN: {
          const auto& parent0 = tape_[node.parents_[0]];
          const auto& parent1 = tape_[node.parents_[1]];
          node.variable_.value_ = std::min(parent0.variable_.value_, parent1.variable_.value_);
          if (parent0.variable_.value_ < parent1.variable_.value_) {
            node.adjoint_values_[0] = 1.0;
            node.adjoint_values_[1] = 0.0;
          } else {
            node.adjoint_values_[0] = 0.0;
            node.adjoint_values_[1] = 1.0;
          }
          break;
        }
        case details::operation::EXP: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = std::exp(parent0.variable_.value_);
          node.adjoint_values_[0] = std::exp(parent0.variable_.value_);
          break;
        }
        case details::operation::LOG: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = std::log(parent0.variable_.value_);
          node.adjoint_values_[0] = 1.0 / parent0.variable_.value_;
          break;
        }
        case details::operation::SIN: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = std::sin(parent0.variable_.value_);
          node.adjoint_values_[0] = std::cos(parent0.variable_.value_);
          break;
        }
        case details::operation::COS: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = std::cos(parent0.variable_.value_);
          node.adjoint_values_[0] = -std::sin(parent0.variable_.value_);
          break;
        }
        case details::operation::TAN: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = std::tan(parent0.variable_.value_);
          node.adjoint_values_[0] = 1.0 + std::pow(std::tan(parent0.variable_.value_), 2.0);
          break;
        }
        case details::operation::ABS: {
          const auto& parent0 = tape_[node.parents_[0]];
          node.variable_.value_ = std::abs(parent0.variable_.value_);
          node.adjoint_values_[0] = parent0.variable_.value_ < 0.0 ? -1.0 : 1.0;
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
    for (size_t n = grad.size() - 1; n >= num_roots_; n--) {
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
  size_t num_roots_;
  bool has_been_evaluated_;

  std::map<double, size_t> root_cache_;

  void move(size_t to, size_t from) {
    node tmp = tape_.at(from);
    tape_.erase(tape_.begin() + from);
    tape_.insert(tape_.begin() + (from > to ? to : to - 1), tmp);

    auto move_idx = [from, to] (size_t& idx) {
      if (idx == from) idx = to;
      else if (from < idx && idx <= to) idx--;
      else if (to <= idx && idx < from) idx++;
    };
    for (size_t i = 0; i < tape_.size(); i++) {
      move_idx(tape_[i].variable_.index_);
      move_idx(tape_[i].parents_[0]);
      move_idx(tape_[i].parents_[1]);
    }
  }

  void optimizeRoots() {
    for (size_t i = num_roots_; i < tape_.size(); i++) {
      if (tape_[i].operation_ == details::operation::ROOT) {
        if (i == num_roots_) {
          num_roots_++;
        } else {
          move(num_roots_, i);
          num_roots_++;
        }
      }
    }
  }

  static ad* getAD() { return reinterpret_cast<ad*>(details::ad_); }

  class node {
   public:
    node(size_t index, details::operation op, size_t parent1, size_t parent2)
      : adjoint_values_({std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()}),
        parents_({parent1, parent2}),
        operation_(op),
        variable_(index) {}

    node(const node& other) : adjoint_values_(other.adjoint_values_), parents_(other.parents_), operation_(other.operation_), variable_(other.variable_) {}

    std::array<double, 2> adjoint_values_;
    std::array<size_t, 2> parents_;
    details::operation operation_;

    var variable_;
  };


  var& addRoot(double value = std::numeric_limits<double>::quiet_NaN()) {
    auto it = root_cache_.find(value);
    if (it != root_cache_.end()) {
      return tape_.at(it->second).variable_;
    }

    tape_.emplace_back(tape_.size(), details::operation::ROOT, details::NO_INDEX, details::NO_INDEX);
    var& v = tape_.back().variable_;
    v.value_ = value;

    if (!(value != value)) root_cache_[value] = v.index_; // is not NaN
    return v;
  }

  var& addUnary(details::operation op, var parent1) {
    tape_.emplace_back(tape_.size(), op, parent1.index_, details::NO_INDEX);
    return tape_.back().variable_;
  }

  var& addBinary(details::operation op, var parent1, var parent2) {
    tape_.emplace_back(tape_.size(), op, parent1.index_, parent2.index_);
    return tape_.back().variable_;
  }

  friend var unary(details::operation, const var);
  friend var binary(details::operation, const var, const var);
};

inline ad::var unary(details::operation op, ad::var lhs) { return ad::getAD()->addUnary(op, lhs); }
inline ad::var binary(details::operation op, ad::var lhs, ad::var rhs) { return ad::getAD()->addBinary(op, lhs, rhs); }

inline ad::var operator+(ad::var lhs, ad::var rhs) { return binary(details::operation::ADD, lhs, rhs); }
inline ad::var operator-(ad::var lhs, ad::var rhs) { return binary(details::operation::SUBTRACT, lhs, rhs); }
inline ad::var operator*(ad::var lhs, ad::var rhs) { return binary(details::operation::MULTIPLY, lhs, rhs); }
inline ad::var operator/(ad::var lhs, ad::var rhs) { return binary(details::operation::DIVIDE, lhs, rhs); }

inline ad::var pow(ad::var lhs, ad::var rhs) { return binary(details::operation::POW, lhs, rhs); }
inline double pow(double lhs, double rhs) { return std::pow(lhs, rhs); }

inline ad::var max(ad::var lhs, ad::var rhs) { return binary(details::operation::MAX, lhs, rhs); }
inline double max(double lhs, double rhs) { return std::max(lhs, rhs); }

inline ad::var min(ad::var lhs, ad::var rhs) { return binary(details::operation::MIN, lhs, rhs); }
inline double min(double lhs, double rhs) { return std::min(lhs, rhs); }

inline ad::var operator-(ad::var lhs) { return binary(details::operation::MULTIPLY, -1.0, lhs); }

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

inline ad::var abs(ad::var lhs) { return unary(details::operation::ABS, lhs); }
inline double abs(double lhs) { return std::abs(lhs); }

}  // namespace pH
