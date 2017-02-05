#include "sweep.h"
#include "sweep_time.h"

#include <stdlib.h>

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

typedef struct sweep_error {
  const char* what; // always literal, do not free
} sweep_error;

typedef struct sweep_device {
  bool scanning;
  int32_t motor_speed;
  int32_t nth_scan_request;
} sweep_device;

typedef struct sweep_scan {
  int32_t count;
  int32_t nth;
} sweep_scan;

// Constructor hidden from users
static sweep_error_s sweep_error_construct(const char* what) {
  SWEEP_ASSERT(what);

  sweep_error_s out = (sweep_error_s)malloc(sizeof(sweep_error));
  SWEEP_ASSERT(out && "out of memory during error reporting");

  out->what = what;
  return out;
}

const char* sweep_error_message(sweep_error_s error) {
  SWEEP_ASSERT(error);

  return error->what;
}

void sweep_error_destruct(sweep_error_s error) {
  SWEEP_ASSERT(error);

  free(error);
}

sweep_device_s sweep_device_construct_simple(const char* port, sweep_error_s* error) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(error);

  return sweep_device_construct(port, 115200, error);
}

sweep_device_s sweep_device_construct(const char* port, int32_t bitrate, sweep_error_s* error) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);
  SWEEP_ASSERT(error);

  sweep_device_s out = (sweep_device_s)malloc(sizeof(sweep_device));

  if (out == NULL) {
    *error = sweep_error_construct("oom during sweep device creation");
    return NULL;
  }

  out->scanning = false;
  out->motor_speed = 1;
  out->nth_scan_request = 0;

  return out;
}

void sweep_device_destruct(sweep_device_s device) {
  SWEEP_ASSERT(device);

  free(device);
}

void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  device->scanning = true;
}

void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  device->scanning = false;
}

sweep_scan_s sweep_device_get_scan(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  sweep_scan_s out = (sweep_scan_s)malloc(sizeof(sweep_scan));

  if (out == NULL) {
    *error = sweep_error_construct("oom during sweep device scan creation");
    return NULL;
  }

  out->count = device->scanning ? 16 : 0;
  out->nth = device->nth_scan_request;

  device->nth_scan_request += 1;

  sweep_sleep_milliseconds(0.1 /*second*/ * 1000);

  return out;
}

int32_t sweep_scan_get_number_of_samples(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);

  return scan->count;
}

int32_t sweep_scan_get_angle(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  int32_t angle = 360;
  int32_t delta = (sample % 4) * 2 + scan->nth;

  switch (sample / 4) {
  case 0:
    angle = 0;
    break;
  case 1:
    angle = 90;
    break;
  case 2:
    angle = 180;
    break;
  case 3:
    angle = 270;
    break;
  }

  return ((angle + delta) % 360) * 1000;
}

int32_t sweep_scan_get_distance(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return 2 * 100; // 2 meter
}

int32_t sweep_scan_get_signal_strength(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return 100;
}

void sweep_scan_destruct(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);

  free(scan);
}

int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  return device->motor_speed;
}

void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz >= 0 && hz <= 10);
  SWEEP_ASSERT(error);

  device->motor_speed = hz;
}

void sweep_device_reset(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
}
