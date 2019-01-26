#pragma once

#include <cstdlib>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <limits>

namespace pH {

enum class constraint_type {
  LEQ,
  EQ,
  GEQ
};

struct solution {
  std::vector<double> x;
  double obj;

  std::string toString() const {
    std::string result = "{" + std::to_string(obj) + ", [";
    for (size_t i = 0; i < x.size(); i++) {
      if (i != 0) result += ", ";
      result += std::to_string(x[i]);
    }
    return result + "]}";
  }
};

namespace details {

struct constraint {
  constraint(std::map<size_t, double> a, constraint_type t, double rhs) : a(std::move(a)), t(t), rhs(rhs) {
    if (t == constraint_type::EQ) {
      throw std::runtime_error("Unsupported constraint type");
    }
  }

  std::map<size_t, double> a;
  constraint_type t;
  double rhs;
};

struct variable {
  variable(double obj, double lb, double ub) : obj(obj), lb(lb), ub(ub) {}
  double obj;
  double lb;
  double ub;

  bool hasLowerBound() const {
    return lb > -std::numeric_limits<double>::max();
  }

  bool hasUpperBound() const {
    return ub < std::numeric_limits<double>::max();
  }
};

struct mapped {
  mapped(double factor = 1.0, int index = -1, double term = 0.0) : factor(factor), index(index), term(term) {}
  double factor;
  int index;
  double term;
};

struct stdModel {
  stdModel(std::vector<variable> variables, std::vector<constraint> constraints)
    : original_variable_size_(variables.size()),
      variables_(std::move(variables)),
      constraints_(std::move(constraints)),
      mapped_variables_(original_variable_size_),
      objective_term_(0.0) {
    size_t original_constraints_size = constraints_.size();
    for (size_t var = 0; var < original_variable_size_; var++) {
      auto& current = variables_[var];
      if (!current.hasLowerBound() && !current.hasUpperBound()) {
        current.lb = 0.0;
        variables_.emplace_back(-current.obj, 0.0, std::numeric_limits<double>::max());
        mapped_variables_.emplace_back(-1.0, var);
        for (auto& constraint : constraints_) {
          auto it = constraint.a.find(var);
          if (it != constraint.a.end()) {
            constraint.a.emplace(variables_.size() - 1, -it->second);
          }
        }
      } else if (current.hasLowerBound()) {
        if (current.lb != 0.0) {
          for (auto& constraint : constraints_) {
            auto it = constraint.a.find(var);
            if (it != constraint.a.end()) {
              constraint.rhs -= current.lb * it->second;
            }
          }
          mapped_variables_[var].term = current.lb;
          objective_term_ += current.lb * current.obj;
        }
        if (current.hasUpperBound()) {
          constraints_.emplace_back(std::map<size_t, double>{{var, 1.0}}, constraint_type::LEQ, current.ub - current.lb);
        }
      } else if (!current.hasLowerBound() && current.hasUpperBound()) {
        for (auto& constraint : constraints_) {
          auto it = constraint.a.find(var);
          if (it != constraint.a.end()) {
            constraint.rhs -= current.ub * it->second;
            it->second *= -1.0;
          }
        }
        mapped_variables_[var].factor = -1.0;
        mapped_variables_[var].term = current.ub;
        objective_term_ += current.ub * current.obj;
        current.obj *= -1.0;
      }
    }
  }

  solution convertSolution(solution solution) {
    solution.obj += objective_term_;
    for (size_t var = 0; var < mapped_variables_.size(); var++) {
      auto& mapped = mapped_variables_[var];
      auto& x = solution.x[var];
      if (mapped.index < 0) {
        x *= mapped.factor;
        x += mapped.term;
      } else {
        solution.x[static_cast<size_t>(mapped.index)] += x * mapped.factor;
      }
    }
    solution.x.resize(original_variable_size_);
    return solution;
  }

  size_t original_variable_size_;
  std::vector<variable> variables_;
  std::vector<constraint> constraints_;
  std::vector<mapped> mapped_variables_;
  double objective_term_;
};

class tableau {
 public:
  tableau(const stdModel& m)
      : num_rows_(m.constraints_.size() + 1), num_slacks_(m.constraints_.size()), num_cols_(m.variables_.size() + num_slacks_ + 1),
        tableau_(num_rows_ * num_cols_, 0.0), basic_variables_(m.constraints_.size()) {
    for (size_t i = 0; i < m.variables_.size(); i++) {
      at(num_rows_ - 1, i) = -m.variables_[i].obj;
    }
    for (size_t i = 0; i < m.constraints_.size(); i++) {
      const auto& c = m.constraints_[i];
      double multiplier = (c.t == constraint_type::LEQ ? 1.0 : -1.0);
      for (const auto& el : c.a) {
        at(i, el.first) = multiplier * el.second;
      }
      at(i, i + (numVars() - num_slacks_)) = 1.0;
      rhs(i) = multiplier * c.rhs;
      basic_variables_[i] = i + m.variables_.size();
    }
  }

  void objRow(size_t& obj_row) const {
    if (obj_row == lastRow()) return;
    obj_row = lastRow();
    double inf_row_value = std::numeric_limits<double>::max();
    for (size_t row = 0; row < num_rows_ - 1; row++) {
      if (rhs(row) < 0.0 && rhs(row) < inf_row_value) {
        obj_row = row;
        inf_row_value = rhs(row);
      }
    }
  }

  size_t pivotColumn(size_t obj_row) const {
    size_t pivot_col = 0;
    double pivot_factor = at(obj_row, 0);
    for (size_t col = 1; col < numVars(); col++) {
      if (at(obj_row, col) < pivot_factor) {
        pivot_col = col;
        pivot_factor = at(obj_row, col);
      }
    }
    if (pivot_factor >= 0.0) {
      return std::numeric_limits<size_t>::max();
    }
    return pivot_col;
  }

  size_t pivotRow(size_t pivot_column, size_t obj_row) const {
    size_t pivot_row = std::numeric_limits<size_t>::max();
    double min_ratio = std::numeric_limits<double>::max();
    for (size_t row = 0; row < num_rows_ - 1; row++) {
      if (at(row, pivot_column) > 0.0) {
        double ratio = std::max(0.0, rhs(row) / at(row, pivot_column));
        if (ratio < min_ratio) {
          pivot_row = row;
          min_ratio = ratio;
        }
      }
    }
    if (pivot_row == std::numeric_limits<size_t>::max() && obj_row != lastRow()) {
      return obj_row;
    }
    return pivot_row;
  }

  void pivot(size_t pivot_col, size_t pivot_row) {
    double pivot = at(pivot_row, pivot_col);
    for (size_t r = 0; r < num_rows_; r++) {
      if (r == pivot_row) {
        continue;
      }
      double pivot_factor = at(r, pivot_col) / pivot;
      for (size_t c = 0; c < num_cols_; c++) {
        if (c == pivot_col) {
          at(r, c) = 0.0;
        } else {
          at(r, c) -= pivot_factor * at(pivot_row, c);
        }
      }
    }
    for (size_t i = 0; i < num_cols_; i++) {
      at(pivot_row, i) /= pivot;
    }
    basic_variables_[pivot_row] = pivot_col;
  }

  solution sol() const {
    solution result{std::vector<double>(numVars() - num_slacks_, 0.0), rhs(num_rows_ - 1)};
    for (size_t i = 0; i < basic_variables_.size(); i++) {
      if (basic_variables_[i] >= result.x.size()) {
        continue;
      }
      result.x[basic_variables_[i]] = rhs(i);
    }
    return result;
  }

  size_t lastRow() const { return num_rows_ - 1; }

  std::string toString() const {
    std::stringstream ss;
    for (size_t r = 0; r < num_rows_; r++) {
      if (r < basic_variables_.size()) {
        ss << "x" << basic_variables_[r] << ": ";
      } else {
        ss << "ob: ";
      }
      for (size_t c = 0; c < num_cols_; c++) {
        ss << at(r, c) << ", ";
      }
      ss << std::endl;
    }
    return ss.str();
  }

 private:
  double& at(size_t row, size_t col) { return tableau_[col * num_rows_ + row]; }
  double at(size_t row, size_t col) const { return tableau_[col * num_rows_ + row]; }

  double& rhs(size_t row) { return at(row, num_cols_ - 1); }
  double rhs(size_t row) const { return at(row, num_cols_ - 1); }

  size_t numVars() const { return num_cols_ - 1; }

  size_t num_rows_;
  size_t num_slacks_;
  size_t num_cols_;
  std::vector<double> tableau_;
  std::vector<size_t> basic_variables_;
  std::vector<mapped> mapped_variables_;
};

}

class lp {
 public:
  lp() : variables_(), constraints_() {}

  size_t addVariable(double obj, double lower_bound = 0.0, double upper_bound = std::numeric_limits<double>::infinity()) {
    variables_.emplace_back(obj, lower_bound, upper_bound);
    return variables_.size() - 1;
  }

  size_t numVariables() const { return variables_.size(); }

  void addConstraint(std::map<size_t, double> constraint, constraint_type type, double rhs) {
    for (const auto& element : constraint) {
      if (element.first >= variables_.size()) {
        throw std::runtime_error("Adding constraint with invalid variable " + std::to_string(element.first));
      }
    }
    constraints_.emplace_back(std::move(constraint), type, rhs);
  }

  solution optimize() const {
    details::stdModel m(variables_, constraints_);
    details::tableau t(m);
    std::cout << t.toString();

    size_t its = 0;
    size_t obj_row = 0;
    t.objRow(obj_row);
    size_t entering_pivot = t.pivotColumn(obj_row);
    while (entering_pivot != std::numeric_limits<size_t>::max()) {
      std::cout << "objr: " << obj_row << std::endl;
      std::cout << "col: " << entering_pivot << std::endl;
      size_t leaving_pivot = t.pivotRow(entering_pivot, obj_row);
      std::cout << "row: " << leaving_pivot << std::endl;
      if (leaving_pivot == std::numeric_limits<size_t>::max()) {
        throw std::runtime_error("Unbounded model");
      }
      t.pivot(entering_pivot, leaving_pivot);
      std::cout << std::endl << t.toString();
      t.objRow(obj_row);
      entering_pivot = t.pivotColumn(obj_row);
      its++;
    }

    if (obj_row != t.lastRow()) {
      throw std::runtime_error("Infeasible model");
    }
    std::cout << "its: " << its << std::endl;

    return m.convertSolution(t.sol());
  }

 private:
  std::vector<details::variable> variables_;
  std::vector<details::constraint> constraints_;
};

}  // namespace pH
