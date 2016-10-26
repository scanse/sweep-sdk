#include "sweep.h"
#include "serial.h"

#include <stdlib.h>

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

typedef struct sweep_error {
  const char* what; // always literal, do not free
} sweep_error;

typedef struct sweep_device {
  sweep_serial_device_s serial; // serial port communication
} sweep_device;

typedef struct sweep_scan {
  // impl.
  int32_t count;
} sweep_scan;

// Constructor hidden from users
static sweep_error_s sweep_error_construct(const char* what) {
  SWEEP_ASSERT(what);

  sweep_error_s out = malloc(sizeof(sweep_error));

  if (out == NULL) {
    SWEEP_ASSERT(false && "out of memory during error reporting");
    exit(EXIT_FAILURE);
  }

  *out = (sweep_error){.what = what};
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

sweep_device_s sweep_device_construct_simple(sweep_error_s* error) {
  SWEEP_ASSERT(error);

  return sweep_device_construct("/dev/ttyUSB0", 115200, 1000, error);
}

sweep_device_s sweep_device_construct(const char* port, int32_t bitrate, int32_t timeout, sweep_error_s* error) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);
  SWEEP_ASSERT(timeout > 0);
  SWEEP_ASSERT(error);

  sweep_serial_error_s serialerror = NULL;
  sweep_serial_device_s serial = sweep_serial_device_construct(port, bitrate, timeout, &serialerror);

  if (serialerror) {
    *error = sweep_error_construct(sweep_serial_error_message(serialerror));
    sweep_serial_error_destruct(serialerror);
    return NULL;
  }

  sweep_device_s out = malloc(sizeof(sweep_device));

  if (out == NULL) {
    *error = sweep_error_construct("oom during sweep device creation");
    sweep_serial_device_destruct(serial);
    return NULL;
  }

  *out = (sweep_device){.serial = serial};

  return out;
}

void sweep_device_destruct(sweep_device_s device) {
  SWEEP_ASSERT(device);

  sweep_serial_device_destruct(device->serial);

  free(device);
}

void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
}

void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
}

sweep_scan_s sweep_device_get_scan(sweep_device_s device, int32_t timeout, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(timeout > 0);
  SWEEP_ASSERT(error);

  return 0;
}

int32_t sweep_scan_get_number_of_samples(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);

  return 2 || scan->count;
}

int32_t sweep_scan_get_angle(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return 10;
}

int32_t sweep_scan_get_distance(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return 20;
}

void sweep_scan_destruct(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);

  free(scan);
}

int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  return 1;
}

void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz > 0);
  SWEEP_ASSERT(error);
}

int32_t sweep_device_get_sample_rate(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  return 1;
}

void sweep_device_reset(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
}
