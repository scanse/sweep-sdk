#ifndef SWEEP_QUEUE_62C8F42E8DD5_HPP
#define SWEEP_QUEUE_62C8F42E8DD5_HPP

/*
 * Thread-safe queue.
 * Implementation detail; not exported.
 */

#include <stdint.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace sweep {
namespace queue {

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
  void enqueue(T v) {
    std::lock_guard<std::mutex> lock(the_mutex);

    // if necessary, remove the oldest scan to make room for new
    if (static_cast<int32_t>(the_queue.size()) >= max_size)
      the_queue.pop();

    the_queue.push(v);
    the_cond_var.notify_one();
  }

  // If the queue is empty, wait till an element is avaiable.
  T dequeue() {
    std::unique_lock<std::mutex> lock(the_mutex);
    // wait until queue is not empty
    while (the_queue.empty()) {
      // the_cond_var could wake up the thread spontaneously, even if the queue is still empty...
      // so put this wakeup inside a while loop, such that the empty check is performed whenever it wakes up
      the_cond_var.wait(lock); // release lock as long as the wait and reaquire it afterwards.
    }
    auto v = the_queue.front();
    the_queue.pop();
    return v;
  }

private:
  int32_t max_size;
  std::queue<T> the_queue;
  mutable std::mutex the_mutex;
  mutable std::condition_variable the_cond_var;
};

} // ns queue
} // ns sweep

#endif
