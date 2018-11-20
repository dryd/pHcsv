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
    if (i > 0) result += ", ";
    result += toString(vec.at(i));
  }
  return result + " ]";
}

#define ASSERT_EQ(expr, expected) if ((expr) != (expected)) { printf("Assert failed at line %d:\n  %s != %s\n", __LINE__, toString(expr).c_str(), #expected); return 1; }

double getDuration(std::chrono::time_point<std::chrono::high_resolution_clock> start) {
  return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
}

class VectorAdder {
 public:
  VectorAdder(int i, std::vector<int>& v) : i_(i), v_(v) {}

  void operator()() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    v_.push_back(i_);
  }

 private:
  int i_;
  std::vector<int>& v_;
};

int addNumbersTyped() {
  pH::pool<VectorAdder> pool(2, false);

  std::vector<int> numbers;
  auto start = std::chrono::high_resolution_clock::now();

  pool.push(VectorAdder(1, numbers));
  pool.emplace(2, numbers);
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  pool.emplace(3, numbers);
  pool.clear();

  ASSERT_EQ(numbers.size(), 0);
  std::cout << "Before wait: " << print(numbers) << ", " << getDuration(start) << "ms" << std::endl;

  pool.wait();
  ASSERT_EQ(numbers.size(), 2);
  std::cout << "After wait: " << print(numbers) << ", " << getDuration(start) << "ms" << std::endl;
  return 0;
}

int addNumbers(size_t num_threads, bool synched) {
  pH::fpool pool(num_threads, synched);

  std::vector<int> numbers;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 1; i <= 3; i++) {
    pool.push([&numbers, i] () {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      numbers.push_back(i);
    });
  }
  if (!synched) {
    ASSERT_EQ(numbers.size(), 0);
  }
  std::cout << "Before wait: " << print(numbers) << ", " << getDuration(start) << "ms" << std::endl;
  pool.wait();
  std::cout << "After wait: " << print(numbers) << ", " << getDuration(start) << "ms" << std::endl;
  ASSERT_EQ(numbers.size(), 3);
  return 0;
}


int main() {
  if (addNumbers(1, false)) return 1;
  if (addNumbers(1, true)) return 1;
  if (addNumbers(3, false)) return 1;
  std::cout << std::endl;
  if (addNumbersTyped()) return 1;
  return 0;
}
