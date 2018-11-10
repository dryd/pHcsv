#pragma once

#include <thread>
#include <queue>
#include <limits>
#include <mutex>
#include <atomic>
#include <functional>
#include <condition_variable>

namespace pH {

class pool {
 public:
  pool(size_t num_workers, size_t num_jobs_limit = std::numeric_limits<size_t>::max())
    : num_jobs_limit_(num_jobs_limit),
      abort_(false),
      workers_(),
      jobs_(),
      work_left_(0),
      mutex_(),
      cv_() {
    if (num_jobs_limit_ == 0) {
      throw std::runtime_error("A thread pool with 0 jobs allowed will not work");
    }
    for (size_t i = 0; i < num_workers; i++) {
      workers_.emplace_back([this] { work(); });
    }
  }

  void push(std::function<void()> job) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (num_jobs_limit_ != std::numeric_limits<size_t>::max()) {
      cv_.wait(lock, [this] { return work_left_ < num_jobs_limit_; });
    }
    jobs_.push(job);
    work_left_++;
    lock.unlock();
    cv_.notify_all();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { printf("checking wait\n"); return work_left_ == 0; });
  }

  ~pool() {
    wait();
    abort_ = true;
    cv_.notify_all();
    for (auto& worker : workers_) {
      worker.join();
    }
  }

 private:
  void work() {
    while (true) {
      printf("worker waiting\n");
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return !jobs_.empty() || abort_; });
      printf("getting job\n");
      if (abort_) break;
      printf("not aborting\n");
      std::function<void()> job = jobs_.front();
      jobs_.pop();
      lock.unlock();
      printf("notifying\n");
      cv_.notify_all();
      printf("starting work\n");
      job();
      printf("finished work\n");
      work_left_--;
      cv_.notify_all();
    }
  }

  size_t num_jobs_limit_;
  std::atomic<bool> abort_;
  std::vector<std::thread> workers_;

  std::queue<std::function<void()>> jobs_;
  std::atomic<size_t> work_left_;

  std::mutex mutex_;
  std::condition_variable cv_;
};

}  // namespace pH
