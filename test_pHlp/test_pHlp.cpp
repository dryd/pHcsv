#include "pHlp.h"

#include <iostream>
#include <chrono>

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

#define ASSERT_EQ(expr, expected) if ((expr) != (expected)) { printf("Assert failed at line %d:\n  %s != %s\n", __LINE__, toString(expr).c_str(), #expected); return 1; }

double getDuration(std::chrono::time_point<std::chrono::high_resolution_clock> start) {
  return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
}

int test1() {
  pH::lp lp({3, 2, 0, 0, 0});
  lp.addConstraint({{0, 2}, {1, 1}, {2, 1}}, 18);
  lp.addConstraint({{0, 2}, {1, 3}, {3, 1}}, 42);
  lp.addConstraint({{0, 3}, {1, 1}, {4, 1}}, 24);
  auto solution = lp.optimize();
  std::vector<double> expected{1.5, 1.0, 0.0, 5.5, 3.0, 0.0};
  ASSERT_EQ(solution.first, expected);
  return 0;
}

int test2() {
  pH::lp lp({4, 3, 0, 0, 0, 0});
  lp.addConstraint({{0, 2}, {1, 3}, {2, 1}}, 6);
  lp.addConstraint({{0, -3}, {1, 2}, {3, 1}}, 3);
  lp.addConstraint({{1, 2}, {4, 1}}, 5);
  lp.addConstraint({{0, 2}, {1, 1}, {5, 1}}, 4);
  auto solution = lp.optimize();
  std::vector<double> expected{1.5, 1.0, 0.0, 5.5, 3.0, 0.0};
  ASSERT_EQ(solution.first, expected);
  return 0;
}

int main() {
  return test1() + test2();
}
