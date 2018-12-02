#include "pHad.h"

#include <iostream>
#include <chrono>
#include <string>

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
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double, std::milli>(now - start).count();
}

template <typename F, typename... Args>
double time(size_t iterations, F func, Args&&... args) {
  double min_duration = std::numeric_limits<double>::max();
  for (size_t i = 0; i < iterations; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    func(std::forward<Args>(args)...);
    double dur = getDuration(start);
    min_duration = std::min(min_duration, dur);
  }
  return min_duration;
}

std::pair<double, std::vector<double>> finite_diff(const std::vector<double>& variables, std::function<double(const std::vector<double>&)> func) {
  auto result = std::make_pair(func(variables), std::vector<double>());
  result.second.reserve(variables.size());

  const double h = 1e-6;
  std::vector<double> var_copy = variables;
  for (size_t i = 0; i < variables.size(); i++) {
    if (i != 0) var_copy[i-1] = variables[i-1];
    var_copy[i] += h;
    result.second.push_back((func(var_copy) - result.first) / h);
  }
  return result;
}

std::pair<double, std::vector<double>> auto_diff(const std::vector<double>& variables, pH::ad& ad) {
  auto result = std::make_pair(ad.eval(variables), std::vector<double>());
  result.second = ad.gradient();
  return result;
}

template <typename T>
T rosenbrock(const std::vector<T>& vars) {
  if (vars.size() % 2 != 0) {
    throw std::runtime_error("Even number of variables required");
  }
  T f = 0.0;
  for (size_t i = 0; i < vars.size() / 2; i++) {
    f += 100.0 * pow(pow(vars.at(2*i), 2.0) - vars.at(2*i + 1), 2.0) + pow(vars.at(2*i) - 1.0, 2.0);
  }
 return f;
}

template <typename T>
T testAllOperations(const std::vector<T>& vars) {
  T f = 0.0;
  f += vars.at(0) * vars.at(1);
  f += vars.at(1) - vars.at(2);
  f += vars.at(2) + vars.at(3);
  f += pow(vars.at(3), vars.at(4));
  f += pow(vars.at(4), 1.0);
  f += exp(vars.at(5));
  f += sin(vars.at(6));
  f += cos(vars.at(7));
  f *= 1.5;
  T g = log(f);
  return g * f;
}

void testPerformance(std::vector<size_t>&& num_vars) {
  std::cout << "vars,finite_diff(ms),auto_diff(ms),speedup" << std::endl;
  for (size_t vars : num_vars) {
    pH::ad ad_objf(vars, rosenbrock<pH::ad::var>);

    std::vector<double> point(vars, 1.01);

    double fd_time = time(10, finite_diff, point, rosenbrock<double>);
    double ad_time = time(10, auto_diff, point, ad_objf);

    std::cout << vars << "," << fd_time << "," << ad_time << "," << fd_time/ad_time << std::endl;
  }
}

int testFunctions(std::vector<double> point,
                  std::function<pH::ad::var(const std::vector<pH::ad::var>& variables)> ad_func,
                  std::function<double(const std::vector<double>& variables)> fd_func) {
  pH::ad ad_tape(point.size(), ad_func);

  auto ad = auto_diff(point, ad_tape);
  auto fd = finite_diff(point, fd_func);

  ASSERT_REL_EQ(ad.first, fd.first, 1e-6);
  ASSERT_REL_EQ(ad.second, fd.second, 1e-3);

  return 0;
}

void example1() {
  size_t num_vars = 10;
  // Record a tape, specifying number of variables and a std::function which generates a tape of operations
  // The std::function is only called once in the constructor, then the operations are saved in a format which
  // supports evaluation and adjoint differentiation in arbitrary points
  pH::ad tape(num_vars, rosenbrock<pH::ad::var>);

  // Create a std::vector representing a point in the space where we want out objective value and gradient, here x=[1.1,...,1.1]
  std::vector<double> point(num_vars, 1.1);

  // pH::ad::eval evaluates the function at a specific point
  // It will also save states along the computation path that can be used for reverse differentiation
  std::cout << "Objective function: " << tape.eval(point) << std::endl;
  // Objective function: 6.1

  // pH::ad::gradient computes the gradient at the last point that eval was called with
  std::cout << "Gradient: " << toString(tape.gradient()) << std::endl;
  // Gradient: [ 48.600000, -22.000000, 48.600000, -22.000000, 48.600000, -22.000000, 48.600000, -22.000000, 48.600000, -22.000000 ]

  // Verify result with a finite difference approximation
  auto fd = finite_diff(point, rosenbrock<double>);

  std::cout << "Finite diff objective function: " << fd.first << std::endl;
  // Finite diff objective function: 6.1

  std::cout << "Finite diff gradient: " << toString(fd.second) << std::endl;
  // Finite diff gradient: [ 48.600507, -21.999900, 48.600507, -21.999900, 48.600507, -21.999900, 48.600507, -21.999900, 48.600507, -21.999900 ]
}

int main() {
  int r = testFunctions(std::vector<double>(100, 1.01), rosenbrock<pH::ad::var>, rosenbrock<double>);
  int ao = testFunctions(std::vector<double>(8, 8.2), testAllOperations<pH::ad::var>, testAllOperations<double>);

  if (r + ao) return r + ao;

  testPerformance({2, 10, 20, 50, 100, 200, 500, 1000, 2000});

  return 0;
}
