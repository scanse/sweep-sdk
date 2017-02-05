#include "sweep.h"
#include "sweep_time.h"
#include <windows.h>

void sweep_sleep_milliseconds(int32_t milliseconds) {
  int32_t microseconds = milliseconds * 1000;
  SWEEP_ASSERT(microseconds > 0ll);
  SWEEP_ASSERT(microseconds < 1000000ll);

  Sleep(milliseconds);
}