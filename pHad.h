#pragma once

#include <vector>
#include <array>
#include <functional>
#include <limits>
#include <cmath>

namespace pH {

namespace details {
    enum class operation {
        ADD,
        MULTIPLY,
        SIN,
        NONE
    };
}

class ad {
 public:
    class var {
     public:
      var(size_t index, ad* t) : ad_(t), index_(index) {}

      var operator+(const var& rhs) {
          return ad_->addNode(details::operation::ADD, {index_, rhs.index_}).variable_;
      }

      var operator*(const var& rhs) {
          return ad_->addNode(details::operation::MULTIPLY, {index_, rhs.index_}).variable_;
      }

      var sin() {
          return ad_->addNode(details::operation::SIN, {index_, std::numeric_limits<size_t>::max()}).variable_;
      }

      ad* ad_;
      size_t index_;
    };

    class node {
     public:
      node(size_t index, details::operation op, std::array<size_t, 2>&& parents, ad* t)
          : adjoint_values_({std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()}),
            parents_(std::move(parents)),
            operation_(op),
            value_(std::numeric_limits<double>::quiet_NaN()),
            variable_(index, t) {}

      std::array<double, 2> adjoint_values_;
      std::array<size_t, 2> parents_;
      details::operation operation_;
      double value_;

      var variable_;
    };

    ad(size_t num_independent_variables, std::function<var(std::vector<var>& variables)> generator) : num_independent_variables_(num_independent_variables) {
        std::vector<var> variables;
        for (size_t i = 0; i < num_independent_variables_; i++) {
          variables.push_back(addNode(details::operation::NONE, {0, 0}).variable_);
        }
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
          tape_[n].value_ = variables[n];
        }
        for (; n < tape_.size(); n++) {
            auto& node = tape_[n];
            switch (node.operation_) {
                case details::operation::MULTIPLY: {
                    const auto& parent0 = tape_[node.parents_[0]];
                    const auto& parent1 = tape_[node.parents_[1]];
                    node.value_ = parent0.value_ * parent1.value_;
                    node.adjoint_values_[0] = parent1.value_;
                    node.adjoint_values_[1] = parent0.value_;
                    break;
                }
                case details::operation::ADD: {
                    const auto& parent0 = tape_[node.parents_[0]];
                    const auto& parent1 = tape_[node.parents_[1]];
                    node.value_ = parent0.value_ + parent1.value_;
                    node.adjoint_values_[0] = 1;
                    node.adjoint_values_[1] = 1;
                    break;
                }
                case details::operation::SIN: {
                    const auto& parent0 = tape_[node.parents_[0]];
                    node.value_ = std::sin(parent0.value_);
                    node.adjoint_values_[0] = std::cos(parent0.value_);
                    break;
                }
                case details::operation::NONE:
                    break;
            }
        }
        return tape_.back().value_;
    }

    std::vector<double> gradient() const {
        std::vector<double> grad(tape_.size(), 0.0);
        grad.back() = 1.0;
        for (size_t n = grad.size() - 1; n >= num_independent_variables_; n--) {
            const auto& node = tape_[n];
            if (node.parents_[0] != std::numeric_limits<size_t>::max()) {
                auto& parent0 = tape_[node.parents_[0]];
                grad[parent0.variable_.index_] += grad[node.variable_.index_] * node.adjoint_values_[0];
            }
            if (node.parents_[1] != std::numeric_limits<size_t>::max()) {
                auto& parent1 = tape_[node.parents_[1]];
                grad[parent1.variable_.index_] += grad[node.variable_.index_] * node.adjoint_values_[1];
            }
        }
        grad.resize(num_independent_variables_);
        grad.shrink_to_fit();
        return grad;
    }

protected:

  node& addNode(details::operation op, std::array<size_t, 2>&& parents) {
    tape_.emplace_back(tape_.size(), op, std::move(parents), this);
    return tape_.back();
  }

  std::vector<node> tape_;
  size_t num_independent_variables_;
};

}  // namespace pH
