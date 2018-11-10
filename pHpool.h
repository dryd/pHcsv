#pragma once

#include <thread>
#include <deque>
#include <vector>
#include <condition_variable>

namespace pH {

class pool {
 public:
  pool(size_t num_workers, bool synched = false)
    : synched_(synched),
      workers_(),
      jobs_(),
      work_left_(0),
      mutex_(),
      cv_(),
      abort_(false) {
    for (size_t i = 0; i < num_workers; i++) {
      workers_.emplace_back([this] { work(); });
    }
  }

  void push(std::function<void()> job) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (synched_) {
        cv_.wait(lock, [this] { return work_left_ < workers_.size(); });
      }
      jobs_.push_back(job);
      work_left_++;
    }
    cv_.notify_all();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return work_left_ == 0; });
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    work_left_ -= jobs_.size();
    jobs_.clear();
  }

  ~pool() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return work_left_ == 0; });
      abort_ = true;
    }
    cv_.notify_all();
    for (auto& worker : workers_) {
      worker.join();
    }
  }

 private:
  void work() {
    while (true) {
      std::function<void()> job;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !jobs_.empty() || abort_; });
        if (abort_) return;

        job = jobs_.front();
        jobs_.pop_front();
      }
      cv_.notify_all();

      job();
      {
        std::lock_guard<std::mutex> lock(mutex_);
        work_left_--;
      }
      cv_.notify_all();
    }
  }

  bool synched_;
  std::vector<std::thread> workers_;

  std::deque<std::function<void()>> jobs_;
  size_t work_left_;

  std::mutex mutex_;
  std::condition_variable cv_;

  bool abort_;
};

}  // namespace pH
