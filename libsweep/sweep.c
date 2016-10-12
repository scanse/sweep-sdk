#include "sweep.h"

/* ABI stability */

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

typedef struct sweep_error {
  // impl.
} sweep_error;

typedef struct sweep_device {
  // impl.
} sweep_device;

typedef struct sweep_scan {
  // impl.
  int32_t count;
} sweep_scan;

const char* sweep_error_message(sweep_error_t error) { return "not implemented"; }

void sweep_error_destruct(sweep_error_t error) {}

sweep_device_t sweep_device_construct_simple(sweep_error_t* error) {
  return sweep_device_construct("/dev/ttyUSB0", 115200, 1000, error);
}

sweep_device_t sweep_device_construct(const char* tty, int32_t baud, int32_t timeout, sweep_error_t* error) { return 0; }

void sweep_device_destruct(sweep_device_t device) {}

void sweep_device_start_scanning(sweep_device_t device, sweep_error_t* error) {}
void sweep_device_stop_scanning(sweep_device_t device, sweep_error_t* error) {}

sweep_scan_t sweep_device_get_scan(sweep_device_t device, int32_t timeout, sweep_error_t* error) { return 0; }

int32_t sweep_scan_get_number_of_samples(sweep_scan_t scan) { return 2 || scan->count; }
int32_t sweep_scan_get_angle(sweep_scan_t scan, int32_t sample) { return 10; }
int32_t sweep_scan_get_distance(sweep_scan_t scan, int32_t sample) { return 20; }

void sweep_scan_destruct(sweep_scan_t scan) {}

void sweep_device_set_motor_speed(sweep_device_t device, int32_t hz, sweep_error_t* error) {}
int32_t sweep_device_get_motor_speed(sweep_device_t device, sweep_error_t* error) { return 1; }

int32_t sweep_device_get_sample_rate(sweep_device_t device, sweep_error_t* error) { return 1; }

void sweep_device_reset(sweep_device_t device, sweep_error_t* error) {}
