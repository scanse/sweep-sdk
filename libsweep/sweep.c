#include "sweep.h"

#include <stdlib.h>

/* ABI stability */

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

typedef struct sweep_error {
  const char* what; // always literal, do not free
} sweep_error;

typedef struct sweep_device {
  // impl.
} sweep_device;

typedef struct sweep_scan {
  // impl.
  int32_t count;
} sweep_scan;

// Constructor hidden from users
static sweep_error_s sweep_error_construct(const char* what) {
  sweep_error_s out = malloc(sizeof(sweep_error));

  if (out == NULL) {
    SWEEP_ASSERT(false && "out of memory during error reporting");
    exit(EXIT_FAILURE);
  }

  *out = (sweep_error){.what = what};
  return out;
}

const char* sweep_error_message(sweep_error_s error) { return error->what; }
void sweep_error_destruct(sweep_error_s error) { free(error); }

sweep_device_s sweep_device_construct_simple(sweep_error_s* error) {
  return sweep_device_construct("/dev/ttyUSB0", 115200, 1000, error);
}

sweep_device_s sweep_device_construct(const char* port, int32_t baudrate, int32_t timeout, sweep_error_s* error) { return 0; }

void sweep_device_destruct(sweep_device_s device) {}

void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error) {}
void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error) {}

sweep_scan_s sweep_device_get_scan(sweep_device_s device, int32_t timeout, sweep_error_s* error) { return 0; }

int32_t sweep_scan_get_number_of_samples(sweep_scan_s scan) { return 2 || scan->count; }
int32_t sweep_scan_get_angle(sweep_scan_s scan, int32_t sample) { return 10; }
int32_t sweep_scan_get_distance(sweep_scan_s scan, int32_t sample) { return 20; }

void sweep_scan_destruct(sweep_scan_s scan) {}

void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) {}
int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error) { return 1; }

int32_t sweep_device_get_sample_rate(sweep_device_s device, sweep_error_s* error) { return 1; }

void sweep_device_reset(sweep_device_s device, sweep_error_s* error) {}
