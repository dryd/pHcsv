#include "pHad.h"

#include <iostream>
#include <chrono>
#include <string>

template<typename T>
inline std::string toString(const T& val) {
  return std::to_string(val);
}

template<>
inline std::string toString(const std::string& val) {
  return val;
}

template<typename T>
inline std::string print(const std::vector<T>& vec) {
  std::string result("[ ");
  for (size_t i = 0; i < vec.size(); i++) {
    if (i > 0) result += ", ";
    result += toString(vec.at(i));
  }
  return result + " ]";
}

#define ASSERT_EQ(expr, expected) if ((expr) != (expected)) { printf("Assert failed at line %d:\n  %s != %s\n", __LINE__, toString(expr).c_str(), #expected); return 1; }

double getDuration(std::chrono::time_point<std::chrono::high_resolution_clock> start) {
  return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
}

void finite_difference(std::vector<double>& variables, std::function<double(std::vector<double>&)> func) {
  const double h = 1e-6;
  double obj = func(variables);
  std::cout << "fd: " << obj << std::endl;
  std::vector<double> gradient;
  gradient.reserve(variables.size());
  for (size_t i = 0; i < variables.size(); i++) {
    std::vector<double> var_copy = variables;
    var_copy[i] += h;
    gradient.push_back((func(var_copy) - obj) / h);
  }
  std::cout << "fd: " << print(gradient) << std::endl;
}

int main() {
  pH::ad ad_objf(2, [] (std::vector<pH::ad::var>& vars) {
    pH::ad::var a = 2.0;
    pH::ad::var b = a + 1.0;
    return pow(b, a) * pow(vars.at(0), vars.at(1));
  });

  std::vector<double> point{ 2.0, 3.0 };
  std::cout << "ad: " << ad_objf.eval(point) << std::endl;
  std::cout << "ad: " << print(ad_objf.gradient()) << std::endl;

  finite_difference(point, [] (std::vector<double>& vars) {
    return std::pow(3, 2) * std::pow(vars.at(0), vars.at(1));
  });

  return 0;
}
