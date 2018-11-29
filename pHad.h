#pragma once

#include <vector>
#include <array>
#include <functional>
#include <limits>
#include <cmath>

namespace pH {

namespace details {
    static thread_local void* ad_ = nullptr;

    enum class operation {
        ADD,
        SUBTRACT,
        MULTIPLY,
        POW,
        EXP,
        SIN,
        COS,
        NONE
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
          var& added = getAD()->addNode(details::operation::NONE, nullptr, nullptr).variable_;
          added.value_ = value_;
          index_ = added.index_;
      }

     protected:
      friend class node;
      friend class ad;
      var(double value, size_t index) : index_(index), value_(value) {}

      size_t index_;
      double value_;
    };

    ad(size_t num_independent_variables, std::function<var(std::vector<var>& variables)> generator) : num_independent_variables_(num_independent_variables) {
        std::vector<var> variables;
        for (size_t i = 0; i < num_independent_variables_; i++) {
          variables.push_back(addNode(details::operation::NONE, nullptr, nullptr).variable_);
        }
        details::ad_ = reinterpret_cast<void*>(this);
        var end_variable = generator(variables);
        details::ad_ = nullptr;
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
                case details::operation::POW: {
                    const auto& parent0 = tape_[node.parents_[0]];
                    const auto& parent1 = tape_[node.parents_[1]];
                    node.variable_.value_ = std::pow(parent0.variable_.value_, parent1.variable_.value_);
                    node.adjoint_values_[0] = parent1.variable_.value_ * std::pow(parent0.variable_.value_, parent1.variable_.value_ - 1.0);
                    node.adjoint_values_[1] = std::pow(parent0.variable_.value_, parent1.variable_.value_) * std::log(parent0.variable_.value_);
                    break;
                }
                case details::operation::EXP: {
                    const auto& parent0 = tape_[node.parents_[0]];
                    node.variable_.value_ = std::exp(parent0.variable_.value_);
                    node.adjoint_values_[0] = std::exp(parent0.variable_.value_);
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
                    node.adjoint_values_[0] = std::sin(parent0.variable_.value_);
                    break;
                }
                case details::operation::NONE:
                    break;
            }
        }
        return tape_.back().variable_.value_;
    }

    std::vector<double> gradient() const {
        std::vector<double> grad(tape_.size(), 0.0);
        grad.back() = 1.0;
        for (size_t n = grad.size() - 1; n >= num_independent_variables_; n--) {
            const auto& node = tape_[n];
            if (node.parents_[0] != std::numeric_limits<size_t>::max()) {
                grad[tape_[node.parents_[0]].variable_.index_] += grad[node.variable_.index_] * node.adjoint_values_[0];
            }
            if (node.parents_[1] != std::numeric_limits<size_t>::max()) {
                grad[tape_[node.parents_[1]].variable_.index_] += grad[node.variable_.index_] * node.adjoint_values_[1];
            }
        }
        grad.resize(num_independent_variables_);
        grad.shrink_to_fit();
        return grad;
    }

 private:
    static ad* getAD() { return reinterpret_cast<ad*>(details::ad_); }

    class node {
     public:
      node(size_t index, details::operation op, const var* parent1, const var* parent2)
          : adjoint_values_({std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()}),
            parents_({getIndex(parent1), getIndex(parent2)}),
            operation_(op),
            variable_(std::numeric_limits<double>::quiet_NaN(), index) {}

      std::array<double, 2> adjoint_values_;
      std::array<size_t, 2> parents_;
      details::operation operation_;

      var variable_;

     private:
      size_t getIndex(const var* parent) const {
          return parent == nullptr ? std::numeric_limits<size_t>::max() : parent->index_;
      }
    };

    node& addNode(details::operation op, const var* parent1, const var* parent2) {
      tape_.emplace_back(tape_.size(), op, parent1, parent2);
      return tape_.back();
    }


    std::vector<node> tape_;
    size_t num_independent_variables_;

    friend ad::var& operator+(const ad::var& lhs, const ad::var& rhs);
    friend ad::var& operator-(const ad::var& lhs, const ad::var& rhs);
    friend ad::var& operator*(const ad::var& lhs, const ad::var& rhs);
    friend ad::var& pow(const ad::var& lhs, const ad::var& rhs);
    friend ad::var& exp(const ad::var& lhs);
    friend ad::var& sin(const ad::var& lhs);
    friend ad::var& cos(const ad::var& lhs);
};

ad::var& operator+(const ad::var& lhs, const ad::var& rhs) { return ad::getAD()->addNode(details::operation::ADD, &lhs, &rhs).variable_; }
ad::var& operator-(const ad::var& lhs, const ad::var& rhs) { return ad::getAD()->addNode(details::operation::SUBTRACT, &lhs, &rhs).variable_; }
ad::var& operator*(const ad::var& lhs, const ad::var& rhs) { return ad::getAD()->addNode(details::operation::MULTIPLY, &lhs, &rhs).variable_; }
ad::var& pow(const ad::var& lhs, const ad::var& rhs) { return ad::getAD()->addNode(details::operation::POW, &lhs, &rhs).variable_; }
ad::var& exp(const ad::var& lhs) { return ad::getAD()->addNode(details::operation::EXP, &lhs, nullptr).variable_; }
ad::var& sin(const ad::var& lhs) { return ad::getAD()->addNode(details::operation::SIN, &lhs, nullptr).variable_; }
ad::var& cos(const ad::var& lhs) { return ad::getAD()->addNode(details::operation::COS, &lhs, nullptr).variable_; }

}  // namespace pH
