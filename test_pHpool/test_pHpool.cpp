#include "../pHpool.h"

#include <iostream>

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
    if (i > 0) result += ',';
    result += toString(vec.at(i));
  }
  return result + " ]";
}


#define ASSERT_EQ(expr, expected) if ((expr) != (expected)) { printf("Assert failed at line %d:\n  %s != %s\n", __LINE__, toString(expr).c_str(), #expected); return 1; }

int main() {
  pH::pool p(1);

  std::vector<int> numbers;
  p.push([&numbers] () { std::cout << "print(numbers)" << std::endl; numbers.push_back(0); });
  std::cout << print(numbers) << std::endl;
  p.wait();
  std::cout << print(numbers) << std::endl;

  return 0;
}
