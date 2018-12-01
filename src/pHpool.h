#pragma once

#include <thread>
#include <deque>
#include <vector>
#include <condition_variable>

namespace pH {

template <typename Callable>
class pool {
 public:
  pool(size_t num_workers, bool synched = false)
    : synched_(synched),
      workers_(),
      jobs_(),
      work_left_(0),
      mutex_(),
      worker_cv_(),
      wait_cv_(),
      abort_(false) {
    for (size_t i = 0; i < num_workers; i++) {
      workers_.emplace_back([this] { work(); });
    }
  }

  void push(Callable job) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (synched_) {
        wait_cv_.wait(lock, [this] { return work_left_ < workers_.size(); });
      }
      jobs_.push_back(std::move(job));
      work_left_++;
    }
    worker_cv_.notify_one();
  }

  template <typename... Args>
  void emplace(Args&&... args) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (synched_) {
        wait_cv_.wait(lock, [this] { return work_left_ < workers_.size(); });
      }
      jobs_.emplace_back(std::forward<Args>(args)...);
      work_left_++;
    }
    worker_cv_.notify_one();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    wait_cv_.wait(lock, [this] { return work_left_ == 0; });
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    work_left_ -= jobs_.size();
    jobs_.clear();
  }

  ~pool() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wait_cv_.wait(lock, [this] { return work_left_ == 0; });
      abort_ = true;
    }
    worker_cv_.notify_all();
    for (auto& worker : workers_) {
      worker.join();
    }
  }

 private:
  void work() {
    while (true) {
      std::unique_lock<std::mutex> lock(mutex_);
      worker_cv_.wait(lock, [this] { return !jobs_.empty() || abort_; });
      if (abort_) return;

      Callable job(std::move(jobs_.front()));
      jobs_.pop_front();
      lock.unlock();

      job();

      lock.lock();
      work_left_--;
      wait_cv_.notify_all();
    }
  }

  bool synched_;
  std::vector<std::thread> workers_;

  std::deque<Callable> jobs_;
  size_t work_left_;

  std::mutex mutex_;
  std::condition_variable worker_cv_;
  std::condition_variable wait_cv_;

  bool abort_;
};

using fpool = pool<std::function<void()>>;

}  // namespace pH
