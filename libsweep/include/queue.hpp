#ifndef SWEEP_QUEUE_62C8F42E8DD5_HPP
#define SWEEP_QUEUE_62C8F42E8DD5_HPP

/*
 * Thread-safe queue.
 * Implementation detail; not exported.
 */

#include <stdint.h>

#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <queue>
#include <utility>

namespace sweep {
namespace queue {

struct ReadCanceled : std::exception {
  const char* what() const noexcept override { return "Read was canceled by user"; }
};

template <typename T> class queue {
public:
  queue(int32_t max) : max_size(max) {}

  // Empty the queue
  void clear() {
    std::unique_lock<std::mutex> lock(the_mutex);
    while (!the_queue.empty()) {
      the_queue.pop();
    }
  }

  // Add an element to the queue.
  void push(T v) {
    {
      std::lock_guard<std::mutex> lock(the_mutex);

      // if necessary, remove the oldest scan to make room for new
      if (static_cast<int32_t>(the_queue.size()) >= max_size)
        the_queue.pop();

      the_queue.push(v);
    }
    the_cond_var.notify_one();
  }

  // If the queue is empty, wait till an element is avaiable.
  T pop() {
    std::unique_lock<std::mutex> lock(the_mutex);
    // wait until queue is not empty
    the_cond_var.wait(lock, [this] { return error || !the_queue.empty(); });

    if (error) {
      auto t = error;
      error = t;
      std::rethrow_exception(error);
    }

    auto v = the_queue.front();
    the_queue.pop();
    return v;
  }

  void push_error(std::exception_ptr eptr) {
    {
      std::lock_guard<std::mutex> lock(the_mutex);
      error = eptr;
    }
    the_cond_var.notify_one();
  }

  void cancel_read() { push_error(ReadCanceled{}); }

private:
  std::exception_ptr error;
  int32_t max_size;
  std::queue<T> the_queue;
  mutable std::mutex the_mutex;
  mutable std::condition_variable the_cond_var;
};

} // namespace queue
} // namespace sweep

#endif
