#define _POSIX_C_SOURCE 200809L

#include "time.h"
#include "sweep.h"

#include <errno.h>
#include <time.h>

void sweep_sleep_microseconds(int32_t microseconds) {
  // nanosleep timespec requirements: nanosecond tv_nsec field < 1s.
  // If this is an issue split into tv_sec and tv_nsec.
  // Note: if we have to wait > 1s for the Sweep device we're probably doing something wrong.
  SWEEP_ASSERT(microseconds > 0ll);
  SWEEP_ASSERT(microseconds < 1000000ll);

  int64_t nanoseconds = microseconds * 1000;

  struct timespec req = {0, 0};
  req.tv_nsec = nanoseconds;

  while (nanosleep(&req, &req) == -1) {
    if (errno == EINTR)
      continue;
  }
}
