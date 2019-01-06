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

namespace details {

struct constraint {
  constraint(std::map<size_t, double> a, constraint_type t, double rhs) : a(std::move(a)), t(t), rhs(rhs) {
    if (t == constraint_type::GEQ || t == constraint_type::EQ) {
      throw std::runtime_error("Unsupported constraint type");
    }
  }

  std::map<size_t, double> a;
  constraint_type t;
  double rhs;
};

class tableau {
 public:
  tableau(const std::vector<double>& c, const std::vector<constraint>& constraints)
      : num_rows_(constraints.size() + 1), num_slacks_(constraints.size()), num_cols_(c.size() + num_slacks_ + 1),
        tableau_(num_rows_ * num_cols_, 0.0), basic_variables_(constraints.size()) {
    for (size_t i = 0; i < c.size(); i++) {
      obj(i) = -c.at(i);
    }
    for (size_t i = 0; i < constraints.size(); i++) {
      for (const auto& el : constraints.at(i).a) {
        at(i, el.first) = el.second;
      }
      at(i, i + (numVars() - num_slacks_)) = 1.0;
      rhs(i) = constraints.at(i).rhs;
      basic_variables_.at(i) = i + (c.size() - constraints.size());
    }
  }

  size_t pivotColumn() const {
    std::pair<size_t, double> pivot(0, obj(0));
    for (size_t col = 1; col < numVars(); col++) {
      if (obj(col) < pivot.second) {
        pivot.first = col;
        pivot.second = obj(col);
      }
    }
    if (pivot.second >= 0.0) {
      return std::numeric_limits<size_t>::max();
    }
    return pivot.first;
  }

  size_t pivotRow(size_t pivot_column) const {
    std::pair<size_t, double> pivot_row(std::numeric_limits<size_t>::max(), std::numeric_limits<double>::max());
    for (size_t row = 0; row < num_rows_ - 1; row++) {
      if (at(row, pivot_column) > 0.0 && rhs(row) > 0.0) {
        double pivot = rhs(row) / at(row, pivot_column);
        if (pivot < pivot_row.second) {
          pivot_row.first = row;
          pivot_row.second = pivot;
        }
      }
    }
    return pivot_row.first;
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
    basic_variables_.at(pivot_row) = pivot_col;
  }

  std::pair<std::vector<double>, double> solution() const {
    std::pair<std::vector<double>, double> result(std::vector<double>(numVars() - num_slacks_, 0.0), rhs(num_rows_ - 1));
    for (size_t i = 0; i < basic_variables_.size(); i++) {
      if (basic_variables_.at(i) >= result.first.size()) {
        continue;
      }
      result.first.at(basic_variables_.at(i)) = rhs(i);
    }
    return result;
  }

  std::string toString() const {
    std::stringstream ss;
    for (size_t r = 0; r < num_rows_; r++) {
      for (size_t c = 0; c < num_cols_; c++) {
        ss << at(r, c) << ", ";
      }
      ss << std::endl;
    }
    return ss.str();
  }

 private:
  double& at(size_t row, size_t col) { return tableau_.at(col * num_rows_ + row); }
  double at(size_t row, size_t col) const { return tableau_.at(col * num_rows_ + row); }

  double& obj(size_t var) { return at(num_rows_ - 1, var); }
  double obj(size_t var) const { return at(num_rows_ - 1, var); }

  double& rhs(size_t row) { return at(row, num_cols_ - 1); }
  double rhs(size_t row) const { return at(row, num_cols_ - 1); }

  size_t numVars() const { return num_cols_ - 1; }

  size_t num_rows_;
  size_t num_slacks_;
  size_t num_cols_;
  std::vector<double> tableau_;
  std::vector<size_t> basic_variables_;
};

}

class lp {
 public:
  lp(std::vector<double> objective_coefficients)
    : c_(std::move(objective_coefficients)),
      constraints_() {}

  size_t numVariables() const { return c_.size(); }

  size_t addConstraint(std::map<size_t, double> constraint, constraint_type type, double rhs) {
    constraints_.emplace_back(std::move(constraint), type, rhs);
    return constraints_.size() - 1;
  }

  std::pair<std::vector<double>, double> optimize() const {
    details::tableau t(c_, constraints_);
    std::cout << t.toString() << std::endl;

    size_t entering_pivot = t.pivotColumn();
    while (entering_pivot != std::numeric_limits<size_t>::max()) {
      std::cout << entering_pivot << std::endl;
      size_t leaving_pivot = t.pivotRow(entering_pivot);
      std::cout << leaving_pivot << std::endl;
      if (leaving_pivot == std::numeric_limits<size_t>::max()) {
        throw std::runtime_error("Unbounded model");
      }
      t.pivot(entering_pivot, leaving_pivot);
      std::cout << t.toString() << std::endl;
      entering_pivot = t.pivotColumn();
    }

    return t.solution();
  }

 private:
  std::vector<double> c_;
  std::vector<details::constraint> constraints_;
};

}  // namespace pH
