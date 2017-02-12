#include "sweep.h"
#include "sweep_device.hpp"

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

typedef struct sweep_error { const char* what; } sweep_error;

// Constructor hidden from users
static sweep_error_s sweep_error_construct(const char* what) {
  SWEEP_ASSERT(what);

  auto out = new sweep_error{what};
  return out;
}

const char* sweep_error_message(sweep_error_s error) {
  SWEEP_ASSERT(error);

  return error->what;
}

void sweep_error_destruct(sweep_error_s error) {
  SWEEP_ASSERT(error);
  delete error;
}

template <class F> auto translateException(F&& f, sweep_error_s* error) -> decltype(std::forward<F>(f)()) {
  try {
    return std::forward<F>(f)();
  } catch (const sweep::ErrorBase& e) {
    *error = sweep_error_construct(e.what());
  } catch (const std::bad_alloc&) {
    *error = sweep_error_construct("Could not allocate enough memory");
  } catch (...) {
    *error = sweep_error_construct("Unknown exception");
  }
  return {};
}

template <class F> void translateExceptionV(F&& f, sweep_error_s* error) {
  try {
    std::forward<F>(f)();
  } catch (const sweep::ErrorBase& e) {
    *error = sweep_error_construct(e.what());
  } catch (const std::bad_alloc&) {
    *error = sweep_error_construct("Could not allocate enough memory");
  } catch (...) {
    *error = sweep_error_construct("Unknown exception");
  }
}

sweep_device_s sweep_device_construct_simple(sweep_error_s* error) {
  SWEEP_ASSERT(error);

  return sweep_device_construct("/dev/ttyUSB0", 115200, error);
}

sweep_device_s sweep_device_construct(const char* port, int32_t bitrate, sweep_error_s* error) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);
  SWEEP_ASSERT(error);
  return translateException([&] { return new sweep_device{port, bitrate}; }, error);
}

void sweep_device_destruct(sweep_device_s device) {
  SWEEP_ASSERT(device);
  delete device;
}

void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  translateExceptionV([&] { return device->start_scanning(); }, error);
}

void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  translateExceptionV([&] { return device->stop_scanning(); }, error);
}

sweep_scan_s sweep_device_get_scan(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  return translateException([&] { return device->get_scan(); }, error);
}

int32_t sweep_scan_get_number_of_samples(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(scan->count >= 0);

  return scan->count;
}

int32_t sweep_scan_get_angle(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->angle[sample];
}

int32_t sweep_scan_get_distance(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->distance[sample];
}

int32_t sweep_scan_get_signal_strength(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->signal_strength[sample];
}

void sweep_scan_destruct(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);

  delete scan;
}

int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  return translateException([&] { return device->get_motor_speed(); }, error);
}

void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz >= 0 && hz <= 10);
  SWEEP_ASSERT(error);

  translateExceptionV([&] { return device->set_motor_speed(hz); }, error);
}

void sweep_device_reset(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  translateExceptionV([&] { return device->reset(); }, error);
}
