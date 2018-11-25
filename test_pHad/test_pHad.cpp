#include "../pHad.h"

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

int main() {
  pH::ad objf(2, [] (std::vector<pH::ad::var>& variables) {
    return variables[0] * variables[1] + variables[0].sin();
  });

  std::cout << objf.eval({2.0, 3.0}) << std::endl;
  std::cout << print(objf.gradient()) << std::endl;

  return 0;
}
