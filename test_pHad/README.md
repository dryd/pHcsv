pH::ad
========

pH::ad implements a simple reverse (adjoint) automatic differentiation library.

It differs from standard AAD libraries in that it only supports a one-record-multiple-evaluations pattern. The imagined use is in nonlinear optimizers, where the same function is evaluated many times for different inputs. Since it is meant to be used in optimization, it only supports functions R^n -> R.

The assumption is that the time to generate the tape is negligible compared to the time of evaluating the function in the optimization algorithm.

Example
-------

```cpp
#include <pHad.h>
#include <iostream>

std::string toString(const std::vector<int>& vec) { /* prints a vector */ }

// Define an objective function for nonlinear optimization
template <typename T>
T rosenbrock(const std::vector<T>& vars) {
  T f = 0.0;
  for (size_t i = 0; i < vars.size() / 2; i++) {
    f += 100.0 * pow(pow(vars.at(2*i), 2.0) - vars.at(2*i + 1), 2.0) + pow(vars.at(2*i) - 1.0, 2.0);
  }
 return f;
}

// Calculate finite difference approximation (simulating the functionality of pH::ad)
std::pair<double, std::vector<double>> finite_diff(const std::vector<double>& vars, std::function<double(const std::vector<double>&)> func) {
  auto result = std::make_pair(func(vars), std::vector<double>());
  result.second.reserve(vars.size());

  const double h = 1e-6;
  std::vector<double> vars_copy = vars;
  for (size_t i = 0; i < vars.size(); i++) {
    if (i != 0) vars_copy[i-1] = vars[i-1];
    vars_copy[i] += h;
    result.second.push_back((func(vars_copy) - result.first) / h);
  }
  return result;
}

int main() {
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
```

Performance
-----------

Performance was measured with this program (with some function definitions in the example above):

```cpp
#include <pHad.h>
#include <chrono>

// Help function for timing
double getDuration(std::chrono::time_point<std::chrono::high_resolution_clock> start) {
  return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
}

// Help function to time any function, evaluating it a number of times and returning the minimum
template <typename F, typename... Args>
double time(size_t iterations, F func, Args&&... args) {
  double min_duration = std::numeric_limits<double>::max();
  for (size_t i = 0; i < iterations; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    func(std::forward<Args>(args)...);
    min_duration = std::min(min_duration, getDuration(start));
  }
  return min_duration;
}

std::pair<double, std::vector<double>> auto_diff(const std::vector<double>& variables, pH::ad& ad) {
  auto result = std::make_pair(ad.eval(variables), std::vector<double>());
  result.second = ad.gradient();
  return result;
}

int main() {
  std::cout << "vars,finite_diff(ms),auto_diff(ms),speedup" << std::endl;
  for (size_t num_vars : {2, 10, 20, 50, 100, 200, 500, 1000, 2000}) {
    // Set up a tape with num_vars variables
    pH::ad ad_objf(num_vars, rosenbrock<pH::ad::var>);
    std::vector<double> point(num_vars, 1.01);

    // Time finite difference
    double fd_time = time(10, finite_diff, point, rosenbrock<double>);

    // Time pH::ad (as explained, we assume that creating the tape takes negligible time compared to later evaluations)
    double ad_time = time(10, auto_diff, point, ad_objf);

    std::cout << num_vars << "," << fd_time << "," << ad_time << "," << fd_time/ad_time << std::endl;
  }
}
```

Results for running g++ with no compiler optimizations:

| vars | finite_diff(ms) | auto_diff(ms) | speedup  |
|------|-----------------|---------------|----------|
| 2    | 0.001221        | 0.002406      | 0.507481 |
| 10   | 0.005906        | 0.007636      | 0.773442 |
| 20   | 0.018771        | 0.014057      | 1.33535  |
| 50   | 0.103547        | 0.033154      | 3.12321  |
| 100  | 0.399046        | 0.065095      | 6.13021  |
| 200  | 1.57105         | 0.129807      | 12.103   |
| 500  | 9.65391         | 0.324815      | 29.7213  |
| 1000 | 38.4374         | 0.645933      | 59.5067  |
| 2000 | 153.104         | 1.29092       | 118.6    |

Results for running g++ with -O3:

| vars | finite_diff(ms) | auto_diff(ms) | speedup  |
|------|-----------------|---------------|----------|
| 2    | 0.000176        | 0.000473      | 0.372093 |
| 10   | 0.000481        | 0.001853      | 0.259579 |
| 20   | 0.001225        | 0.003373      | 0.363178 |
| 50   | 0.005376        | 0.007989      | 0.672925 |
| 100  | 0.019504        | 0.01615       | 1.20768  |
| 200  | 0.075975        | 0.033098      | 2.29546  |
| 500  | 0.45805         | 0.092453      | 4.95441  |
| 1000 | 1.79524         | 0.168681      | 10.6428  |
| 2000 | 7.14253         | 0.336954      | 21.1973  |
