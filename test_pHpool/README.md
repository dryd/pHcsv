pH::pool
========

pH::pool implements a simple thread pool. In the constructor you specify the number of worker threads and whether it should be synched.

Synched means that pushing a job when there is no worker available will block execution until some worker is available.
Beneficial if there is an approximately equal amount of work to be done on the main thread and no thread should get ahead of the other.
Could also be used if memory is a concern, to avoid the job queue growing to a large size.

pH::fpool
---------

pH::fpool is a template specialization of pH::pool which uses a std::function<void()> as Callable

```cpp
#include <pHpool.h>
#include <iostream>

std::string print(const std::vector<int>& vec) { /* prints a vector */ }

// Help function for printouts
double getDuration(std::chrono::time_point<std::chrono::high_resolution_clock> start) {
  return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
}

// Creates thread pool and adds some ints to a vector for demonstration purposes
void addNumbers(size_t num_threads, bool synched) {
  // Create the thread pool
  pH::fpool pool(num_threads, synched);

  // Initialize some data
  std::vector<int> numbers;
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 1; i <= 3; i++) {
    // Push jobs to thread pool
    pool.push([&numbers, i] () {
      // Simulate doing some actual work with sleep_for
      std::this_thread::sleep_for(10ms);
      numbers.push_back(i); // possible race condition when push_back on vector from multiple threads, ignored
    });
  }
  // Print vector and duration immediately, without waiting
  std::cout << "Before wait: " << print(numbers) << ", " << getDuration(start) << "ms" << std::endl;

  // Wait for all jobs to finish and print vector and duration again
  pool.wait();
  std::cout << "After wait: " << print(numbers) << ", " << getDuration(start) << "ms" << std::endl;

  // Destructor of pH::pool also waits until all work has been finished before continuing execution
}

int main() {
  // One worker thread, not synched. Execution order guaranteed if one worker thread.
  addNumbers(1, false); // Possible output:
  // Before wait: [  ], 0.060163ms
  // After wait: [ 1, 2, 3 ], 30.4268ms

  // One worker thread, synched. Main thread stops and waits for worker thread to be finished before adding job.
  addNumbers(1, true); // Possible output:
  // Before wait: [ 1, 2 ], 20.2008ms
  // After wait: [ 1, 2, 3 ], 30.291ms

  // Three worker thread, not synched. Execution order not guaranteed if more than one worker thread.
  addNumbers(3, false); // Possible output:
  // Before wait: [  ], 0.009703ms
  // After wait: [ 2, 1, 3 ], 10.196ms
}
```

pH::pool
---------

pH::pool requires a move-constructible functor template argument

```cpp
#include <pHpool.h>
#include <iostream>

// Functor which holds an int and a reference to a vector<int> and overloads operator()
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

int main() {
  // Creates thread pool with VectorAdder as callable
  pH::pool<VectorAdder> pool(2, false);

  // Initialize some data
  std::vector<int> numbers;
  auto start = std::chrono::high_resolution_clock::now();

  // Use pH::pool::push to add an instantiation of the functor to the pool
  pool.push(VectorAdder(1, numbers));

  // Use pH::pool::emplace to construct the functor directly in the thread pool
  // Note that if synched is true, the VectorAdder constructor will not be called before a worker is available
  pool.emplace(2, numbers);

  // Wait until the jobs have been picked up by worker threads
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Since we have two workers busy, this job will end up in line...
  pool.emplace(3, numbers);

  // ...which we then immediately clear
  pool.clear();

  std::cout << print(numbers) << ", " << getDuration(start) << "ms" << std::endl;
  // [  ], 1.08621ms

  pool.wait();
  std::cout << print(numbers) << ", " << getDuration(start) << "ms" << std::endl;
  // [ 2, 1 ], 10.1355ms
}
```