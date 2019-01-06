#include "pHlp.h"

#include <iostream>
#include <chrono>
#include <cmath>

template<typename T>
std::string toString(const T& val) {
  return std::to_string(val);
}

template<typename T>
std::string toString(const std::vector<T>& vec) {
  std::string result("[ ");
  for (size_t i = 0; i < vec.size(); i++) {
    if (i > 0) result += ", ";
    result += toString(vec.at(i));
  }
  return result + " ]";
}

template<typename T, typename U>
std::string toString(const std::pair<T, U>& val) {
  return toString(val.first) + toString(val.second);
}

template<>
std::string toString(const std::string& val) {
  return val;
}

bool relEq(double value, double expected, double tol) {
  if (expected == 0.0) return std::abs(value) < tol;
  return std::abs((value - expected) / expected) < tol;
}

bool relEq(std::vector<double> values, std::vector<double> expecteds, double tol) {
  if (values.size() != expecteds.size()) return false;
  for (size_t i = 0; i < values.size(); i++) {
    if (expecteds.at(i) == 0.0) return std::abs(values.at(i)) < tol;
    if (std::abs((values.at(i) - expecteds.at(i)) / expecteds.at(i)) > tol) return false;
  }
  return true;
}

#define ASSERT_REL_EQ(expr, expected, tol) if (!relEq((expr), (expected), (tol))) { printf("Assert failed at line %d:\n  %s !~= %s\n", __LINE__, toString(expr).c_str(), toString(expected).c_str()); return 1; }

#define ASSERT_EQ(expr, expected) if ((expr) != (expected)) { printf("Assert failed at line %d:\n  %s != %s\n", __LINE__, toString(expr).c_str(), #expected); return 1; }

double getDuration(std::chrono::time_point<std::chrono::high_resolution_clock> start) {
  return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
}

int test1() {
  pH::lp lp({3, 2});
  lp.addConstraint({{0, 2}, {1, 1}}, pH::constraint_type::LEQ, 18);
  lp.addConstraint({{0, 2}, {1, 3}}, pH::constraint_type::LEQ, 42);
  lp.addConstraint({{0, 3}, {1, 1}}, pH::constraint_type::LEQ, 24);
  auto solution = lp.optimize();
  std::vector<double> expected{3.0, 12.0};
  ASSERT_REL_EQ(solution.first, expected, 1e-10);
  ASSERT_REL_EQ(solution.second, 33.0, 1e-10);
  return 0;
}

int test2() {
  pH::lp lp({4, 3});
  lp.addConstraint({{0, 2}, {1, 3}}, pH::constraint_type::LEQ, 6);
  lp.addConstraint({{0, -3}, {1, 2}}, pH::constraint_type::LEQ, 3);
  lp.addConstraint({{1, 2}}, pH::constraint_type::LEQ, 5);
  lp.addConstraint({{0, 2}, {1, 1}}, pH::constraint_type::LEQ, 4);
  auto solution = lp.optimize();
  std::vector<double> expected{1.5, 1.0};
  ASSERT_REL_EQ(solution.first, expected, 1e-10);
  ASSERT_REL_EQ(solution.second, 9.0, 1e-10);
  return 0;
}

int main() {
  return test1() +
         test2();
}
