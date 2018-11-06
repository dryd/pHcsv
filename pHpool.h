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
  pool(size_t num_workers, size_t max_jobs_in_queue = std::numeric_limits<size_t>::max())
    : max_jobs_in_queue_(max_jobs_in_queue),
      abort_(false),
      jobs_(),
      jobs_mutex_(),
      jobs_changed_(),
      workers_() {
    for (size_t i = 0; i < num_workers; i++) {
      workers_.emplace_back([this] { work(); });
    }
  }

  void push(std::function<void()> job) {
    std::unique_lock<std::mutex> lock(jobs_mutex_);
    jobs_.push(job);
    lock.unlock();
    jobs_changed_.notify_one();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(jobs_mutex_);
    jobs_changed_.wait(lock, [this] { printf("checking wait\n"); return jobs_.empty() && working_ == 0; });
  }

  ~pool() {
    wait();
    abort_ = true;
    jobs_changed_.notify_all();
    for (auto& worker : workers_) {
      worker.join();
    }
  }

 private:
  void work() {
    while (true) {
      std::unique_lock<std::mutex> lock(jobs_mutex_);
      jobs_changed_.wait(lock, [this] { return !jobs_.empty() || abort_; });
      printf("getting job\n");
      if (abort_) break;
      printf("not aborting\n");
      std::function<void()> job = jobs_.front();
      jobs_.pop();
      working_++;
      lock.unlock();
      jobs_changed_.notify_all();
      printf("starting work\n");
      job();
      printf("finished work\n");
      working_--;
      jobs_changed_.notify_all();
    }
  }

  size_t max_jobs_in_queue_;
  std::atomic<bool> abort_;
  std::atomic<size_t> working_;
  std::queue<std::function<void()>> jobs_;
  std::mutex jobs_mutex_;
  std::condition_variable jobs_changed_;
  std::vector<std::thread> workers_;
};

}  // namespace pH
