#include "sweep.h"

#include <chrono>
#include <string>
#include <thread>

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

typedef struct sweep_error { std::string what; } sweep_error;

typedef struct sweep_device {
  bool is_scanning;
  int32_t motor_speed;
  int32_t sample_rate;
  int32_t nth_scan_request;
} sweep_device;

typedef struct sweep_scan {
  int32_t count;
  int32_t nth;
} sweep_scan;

const char* sweep_error_message(sweep_error_s error) {
  SWEEP_ASSERT(error);

  return error->what.c_str();
}

void sweep_error_destruct(sweep_error_s error) {
  SWEEP_ASSERT(error);

  delete error;
}

sweep_device_s sweep_device_construct_simple(const char* port, sweep_error_s* error) {
  SWEEP_ASSERT(error);

  return sweep_device_construct(port, 115200, error);
}

sweep_device_s sweep_device_construct(const char* port, int32_t bitrate, sweep_error_s* error) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);
  SWEEP_ASSERT(error);
  (void)port;
  (void)bitrate;
  (void)error;

  auto out = new sweep_device{/*is_scanning=*/false, /*motor_speed=*/5, /*sample_rate*/ 500, /*nth_scan_request=*/0};
  return out;
}

void sweep_device_destruct(sweep_device_s device) {
  SWEEP_ASSERT(device);

  delete device;
}

void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);
  (void)error;

  if (device->is_scanning)
    return;

  device->is_scanning = true;
}

void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  (void)error;

  device->is_scanning = false;
}

void sweep_device_wait_until_motor_ready(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  (void)device;
  (void)error;
}

sweep_scan_s sweep_device_get_scan(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(device->is_scanning);
  (void)error;

  auto out = new sweep_scan{/*count=*/device->is_scanning ? 16 : 0, /*nth=*/device->nth_scan_request};

  device->nth_scan_request += 1;

  // Artificially introduce slowdown, to simulate device rotation
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return out;
}

bool sweep_device_get_motor_ready(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);
  (void)device;
  (void)error;

  return true;
}

int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);
  (void)device;
  (void)error;

  return device->motor_speed;
}

void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz >= 0 && hz <= 10);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);
  (void)error;

  device->motor_speed = hz;
}

int32_t sweep_device_get_sample_rate(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);
  (void)error;

  return device->sample_rate;
}

void sweep_device_set_sample_rate(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz == 500 || hz == 750 || hz == 1000);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);
  (void)error;

  device->sample_rate = hz;
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
  (void)scan;
  (void)sample;

  return 2 * 100; // 2 meter
}

int32_t sweep_scan_get_signal_strength(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");
  (void)scan;
  (void)sample;

  return 200;
}

void sweep_scan_destruct(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);

  delete scan;
}

void sweep_device_reset(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);
  (void)device;
  (void)error;
}

void sweep_device_attempt_start_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);
  (void)error;

  if (device->is_scanning)
    return;

  device->is_scanning = true;
}

sweep_scan_s sweep_device_get_scan_direct(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(device->is_scanning);
  (void)error;

  auto out = new sweep_scan{/*count=*/device->is_scanning ? 16 : 0, /*nth=*/device->nth_scan_request};

  device->nth_scan_request += 1;

  // Artificially introduce slowdown, to simulate device rotation
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return out;
}

void sweep_device_attempt_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz >= 0 && hz <= 10);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);
  (void)error;

  device->motor_speed = hz;
}
