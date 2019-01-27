pH::lp
======

pH::lp is a minimal simplex solver for Linear Programming (LP)

It uses a dense tableau, supports less-than and greater-than constraints, all types of variable bounds, doesn't introduce artificial variables (but solves infeasible bases before working on the objective).


pH::lp
------

```cpp
#include <pHpool.h>
#include <iostream>

int main() {
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
```