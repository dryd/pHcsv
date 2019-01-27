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
  pH::lp lp;
  lp.addVariable(3.0);
  lp.addVariable(2.0);
  lp.addConstraint({{0, 2.0}, {1, 1.0}}, pH::constraint_type::LEQ, 18.0);
  lp.addConstraint({{0, 2.0}, {1, 3.0}}, pH::constraint_type::LEQ, 42.0);
  lp.addConstraint({{0, 3.0}, {1, 1.0}}, pH::constraint_type::LEQ, 24.0);
  auto solution = lp.optimize();
  std::cout << solution.toString() << std::endl;
  std::vector<double> expected{3.0, 12.0};
  ASSERT_REL_EQ(solution.x, expected, 1e-10);
  ASSERT_REL_EQ(solution.obj, 33.0, 1e-10);
  return 0;
}

int test2() {
  pH::lp lp;
  lp.addVariable(4.0);
  lp.addVariable(3.0);
  lp.addConstraint({{0, 2.0}, {1, 3.0}}, pH::constraint_type::LEQ, 6.0);
  lp.addConstraint({{0, -3.0}, {1, 2.0}}, pH::constraint_type::LEQ, 3.0);
  lp.addConstraint({{1, 1.0}}, pH::constraint_type::GEQ, 1.5);
  lp.addConstraint({{0, 1.0}, {1, 1.0}}, pH::constraint_type::GEQ, 2.25);
  lp.addConstraint({{0, 2.0}, {1, 1}}, pH::constraint_type::LEQ, 4.0);
  auto solution = lp.optimize();
  std::cout << solution.toString() << std::endl;
  std::vector<double> expected{0.75, 1.5};
  ASSERT_REL_EQ(solution.x, expected, 1e-10);
  ASSERT_REL_EQ(solution.obj, 7.5, 1e-10);
  return 0;
}

int test3() {
  pH::lp lp;
  lp.addVariable(15.0);
  lp.addVariable(10.0, 1.0, 3.0);
  lp.addConstraint({{0, 1.0}}, pH::constraint_type::LEQ, 2.0);
  lp.addConstraint({{0, 2.0}, {1, 2.0}}, pH::constraint_type::GEQ, 8.0);
  auto solution = lp.optimize();
  std::cout << solution.toString() << std::endl;
  std::vector<double> expected{2.0, 3.0};
  ASSERT_REL_EQ(solution.x, expected, 1e-10);
  ASSERT_REL_EQ(solution.obj, 60.0, 1e-10);
  return 0;
}

int testNoLowerBound() {
  pH::lp lp;
  lp.addVariable(10.0);
  lp.addVariable(15.0, -std::numeric_limits<double>::infinity(), 2);
  lp.addConstraint({{0, 1.0}, {1, 1.0}}, pH::constraint_type::LEQ, 5.0);
  auto solution = lp.optimize();
  std::cout << solution.toString() << std::endl;
  std::vector<double> expected{3.0, 2.0};
  ASSERT_REL_EQ(solution.x, expected, 1e-10);
  ASSERT_REL_EQ(solution.obj, 60.0, 1e-10);
  return 0;
}

int testUnlimitedVariable() {
  pH::lp lp;
  lp.addVariable(15.0);
  lp.addVariable(-10.0, -std::numeric_limits<double>::infinity());
  lp.addConstraint({{0, 1.0}}, pH::constraint_type::LEQ, 2.0);
  lp.addConstraint({{0, 1.0}, {1, 1.0}}, pH::constraint_type::GEQ, 6.0);
  auto solution = lp.optimize();
  std::cout << solution.toString() << std::endl;
  std::vector<double> expected{2.0, 4.0};
  ASSERT_REL_EQ(solution.x, expected, 1e-10);
  ASSERT_REL_EQ(solution.obj, -10.0, 1e-10);

  pH::lp lp2;
  lp2.addVariable(15.0);
  lp2.addVariable(-10.0, -std::numeric_limits<double>::infinity());
  lp2.addConstraint({{0, 1.0}}, pH::constraint_type::LEQ, 2.0);
  lp2.addConstraint({{0, 1.0}, {1, 1.0}}, pH::constraint_type::GEQ, 1.0);
  lp2.addConstraint({{0, 1.0}, {1, 1.0}}, pH::constraint_type::LEQ, 6.0);
  auto solution2 = lp2.optimize();
  std::cout << solution.toString() << std::endl;
  std::vector<double> expected2{2.0, -1.0};
  ASSERT_REL_EQ(solution2.x, expected2, 1e-10);
  ASSERT_REL_EQ(solution2.obj, 40.0, 1e-10);
  return 0;
}

int testLockedVariable() {
  pH::lp lp;
  lp.addVariable(15.0);
  lp.addVariable(10.0);
  lp.addConstraint({{0, 1.0}}, pH::constraint_type::LEQ, 0.0);
  lp.addConstraint({{1, 1.0}}, pH::constraint_type::LEQ, 3.0);
  auto solution = lp.optimize();
  std::cout << solution.toString() << std::endl;
  std::vector<double> expected{0.0, 3.0};
  ASSERT_REL_EQ(solution.x, expected, 1e-10);
  ASSERT_REL_EQ(solution.obj, 30.0, 1e-10);
  return 0;
}

int testInfeasible() {
  pH::lp lp;
  lp.addVariable(15.0);
  lp.addVariable(10.0);
  lp.addConstraint({{0, 1.0}, {1, 1.0}}, pH::constraint_type::LEQ, 4.0);
  lp.addConstraint({{0, 1.0}, {1, 1.0}}, pH::constraint_type::GEQ, 8.0);
  try {
    auto solution = lp.optimize();
  } catch (...) {
    return 0;
  }
  return 1;
}

int testUnbounded() {
  pH::lp lp;
  lp.addVariable(15.0);
  lp.addVariable(10.0, -std::numeric_limits<double>::infinity());
  lp.addConstraint({{0, 1.0}}, pH::constraint_type::LEQ, 2.0);
  lp.addConstraint({{0, 2.0}, {1, 2.0}}, pH::constraint_type::GEQ, 8.0);
  try {
    auto solution = lp.optimize();
  } catch (...) {
    return 0;
  }
  return 1;
}

void example() {
  // Create a new LP model
  pH::lp lp;

  // Add some variables with various bounds. First argument is objective factor, second is lower bound (default 0.0), third is upper bound (default unlimited)
  auto x0 = lp.addVariable(10.0, -std::numeric_limits<double>::infinity()); // unbounded variable
  auto x1 = lp.addVariable(15.0, 2.0, 10.0); // lower bound = 2, upper_bound = 10

   // Add limiting constraints
  lp.addConstraint({{x0, 1.0}, {x1, 1.0}}, pH::constraint_type::LEQ, 9.0);
  lp.addConstraint({{x0, 1.0}, {x1, 4.0}}, pH::constraint_type::LEQ, 24.0);

  // Add constraint to make (0, 0) infeasible
  lp.addConstraint({{x0, 1.0}, {x1, 1.0}}, pH::constraint_type::GEQ, 3.0);

  // Solve model
  pH::solution solution = lp.optimize();

  std::cout << solution.toString() << std::endl;
  // {115.000000, [4.000000, 5.000000]}
}

int main() {
  example();
  return test1() + test2() + test3() + testLockedVariable() + testInfeasible() + testUnbounded() + testUnlimitedVariable() + testNoLowerBound();
}
