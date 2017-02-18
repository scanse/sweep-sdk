#include "sweep.h"
#include "protocol.h"
#include "serial.h"
#include "../include/Error.hpp"

#include <chrono>
#include <thread>
#include <string>
#include <memory>

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

typedef struct sweep_error {
  const std::string what; // always literal, do not deallocate
} sweep_error;

typedef struct sweep_device {
  sweep::serial::device_s serial; // serial port communication
  bool is_scanning;
} sweep_device;

#define SWEEP_MAX_SAMPLES 4096

struct sweep_scan {
  int32_t angle[SWEEP_MAX_SAMPLES];           // in millidegrees
  int32_t distance[SWEEP_MAX_SAMPLES];        // in cm
  int32_t signal_strength[SWEEP_MAX_SAMPLES]; // range 0:255
  int32_t count;
};

const char* sweep_error_message(sweep_error_s error) {
  SWEEP_ASSERT(error);

  return error->what.c_str();
}

void sweep_error_destruct(sweep_error_s error) {
  SWEEP_ASSERT(error);

  delete error;
}

// internal implementations of functions - can throw exceptions
namespace {

struct Error : ::sweep::ErrorBase {
  using ::sweep::ErrorBase::ErrorBase;
};

// forward declarations for internal definitions
sweep_device_s sweep_device_construct_simple();
sweep_device_s sweep_device_construct(const char* port, int32_t bitrate);
void sweep_device_destruct_impl(sweep_device_s device);
void sweep_device_start_scanning(sweep_device_s device);
void sweep_device_stop_scanning(sweep_device_s device);
sweep_scan_s sweep_device_get_scan(sweep_device_s device);
int32_t sweep_device_get_motor_speed(sweep_device_s device);
void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz);
int32_t sweep_device_get_sample_rate(sweep_device_s device);
void sweep_device_set_sample_rate(sweep_device_s device, int32_t hz);
void sweep_device_reset(sweep_device_s device);

sweep_device_s sweep_device_construct_simple(const char* port) {
	return sweep_device_construct(port, 115200); 
}

sweep_device_s sweep_device_construct(const char* port, int32_t bitrate) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);

  sweep::serial::device_s serial = sweep::serial::device_construct(port, bitrate);

  auto out = std::unique_ptr<sweep_device>(new sweep_device{serial, /*is_scanning=*/true});

  sweep_device_stop_scanning(out.get());

  return out.release();
}

void sweep_device_destruct_impl(sweep_device_s device) {
  SWEEP_ASSERT(device);

  try {
    sweep_device_stop_scanning(device);
  } catch (...) {
    // nothing we can do here
  }

  sweep::serial::device_destruct(device->serial);

  delete device;
}

void sweep_device_start_scanning(sweep_device_s device) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(!device->is_scanning);

  if (device->is_scanning)
    return;

  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_START);

  sweep::protocol::response_header_s response;
  sweep::protocol::read_response_header(device->serial, sweep::protocol::DATA_ACQUISITION_START, &response);

  device->is_scanning = true;
}

void sweep_device_stop_scanning(sweep_device_s device) {
  SWEEP_ASSERT(device);

  if (!device->is_scanning)
    return;

  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_STOP);

  // Wait until device stopped sending
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  sweep::serial::device_flush(device->serial);

  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_STOP);

  sweep::protocol::response_header_s response;
  sweep::protocol::read_response_header(device->serial, sweep::protocol::DATA_ACQUISITION_STOP, &response);

  device->is_scanning = false;
}

sweep_scan_s sweep_device_get_scan(sweep_device_s device) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(device->is_scanning);

  sweep::protocol::response_scan_packet_s responses[SWEEP_MAX_SAMPLES];

  int32_t received = 0;

  for (int32_t received = 0; received < SWEEP_MAX_SAMPLES; ++received) {
    sweep::protocol::read_response_scan(device->serial, &responses[received]);

    const bool is_sync = responses[received].sync_error & sweep::protocol::response_scan_packet_sync::sync;
    const bool has_error = (responses[received].sync_error >> 1) != 0; // shift out sync bit, others are errors

    if (!has_error) {
      received++;
    }

    if (is_sync) {
      break;
    }
  }

  auto out = std::unique_ptr<sweep_scan>(new sweep_scan);

  out->count = received;

  for (int32_t it = 0; it < received; ++it) {
    // Convert angle from compact serial format to float (in degrees).
    // In addition convert from degrees to milli-degrees.
    out->angle[it] = static_cast<int32_t>(sweep::protocol::u16_to_f32(responses[it].angle) * 1000.f);
    out->distance[it] = responses[it].distance;
    out->signal_strength[it] = responses[it].signal_strength;
  }

  return out.release();
}

int32_t sweep_device_get_motor_speed(sweep_device_s device) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::write_command(device->serial, sweep::protocol::MOTOR_INFORMATION);

  sweep::protocol::response_info_motor_s response;
  sweep::protocol::read_response_info_motor(device->serial, sweep::protocol::MOTOR_INFORMATION, &response);

  int32_t speed = sweep::protocol::ascii_bytes_to_integral(response.motor_speed);
  SWEEP_ASSERT(speed >= 0);

  return speed;
}

void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz >= 0 && hz <= 10);
  SWEEP_ASSERT(!device->is_scanning);

  uint8_t args[2] = {0};
  sweep::protocol::integral_to_ascii_bytes(hz, args);

  sweep::protocol::write_command_with_arguments(device->serial, sweep::protocol::MOTOR_SPEED_ADJUST, args);

  sweep::protocol::response_param_s response;
  sweep::protocol::read_response_param(device->serial, sweep::protocol::MOTOR_SPEED_ADJUST, &response);
}

int32_t sweep_device_get_sample_rate(sweep_device_s device) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::write_command(device->serial, sweep::protocol::SAMPLE_RATE_INFORMATION);

  sweep::protocol::response_info_sample_rate_s response;
  sweep::protocol::read_response_info_sample_rate(device->serial, sweep::protocol::SAMPLE_RATE_INFORMATION, &response);

  // 01: 500-600Hz, 02: 750-800Hz, 03: 1000-1050Hz
  int32_t code = sweep::protocol::ascii_bytes_to_integral(response.sample_rate);
  int32_t rate = 0;

  switch (code) {
  case 1:
    rate = 500;
    break;
  case 2:
    rate = 750;
    break;
  case 3:
    rate = 1000;
    break;
  default:
    SWEEP_ASSERT(false && "sample rate code unknown");
  }

  return rate;
}

void sweep_device_set_sample_rate(sweep_device_s device, int32_t hz) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz == 500 || hz == 750 || hz == 1000);
  SWEEP_ASSERT(!device->is_scanning);

  // 01: 500-600Hz, 02: 750-800Hz, 03: 1000-1050Hz
  int32_t code = 1;

  switch (hz) {
  case 500:
    code = 1;
    break;
  case 750:
    code = 2;
    break;
  case 1000:
    code = 3;
    break;
  default:
    SWEEP_ASSERT(false && "sample rate unknown");
  }

  uint8_t args[2] = {0};
  sweep::protocol::integral_to_ascii_bytes(code, args);

  sweep::protocol::write_command_with_arguments(device->serial, sweep::protocol::SAMPLE_RATE_ADJUST, args);

  sweep::protocol::response_param_s response;
  sweep::protocol::read_response_param(device->serial, sweep::protocol::SAMPLE_RATE_ADJUST, &response);
}

void sweep_device_reset(sweep_device_s device) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::write_command(device->serial, sweep::protocol::RESET_DEVICE);
}

template <class F> auto translateException(F&& f, sweep_error_s* error) -> decltype(std::forward<F>(f)()) {
  try {
    return std::forward<F>(f)();
  } catch (const sweep::ErrorBase& e) {
    *error = new sweep_error{e.what()};
  } catch (const std::bad_alloc&) {
    *error = new sweep_error{"Could not allocate enough memory"};
  } catch (...) {
    *error = new sweep_error{"Unknown exception"};
  }
  return {};
}

template <class F> void translateExceptionV(F&& f, sweep_error_s* error) {
  try {
    std::forward<F>(f)();
  } catch (const sweep::ErrorBase& e) {
    *error = new sweep_error{e.what()};
  } catch (const std::bad_alloc&) {
    *error = new sweep_error{"Could not allocate enough memory"};
  } catch (...) {
    *error = new sweep_error{"Unknown exception"};
  }
}
} // local namespace

/* ###################### implementation of public API ################################# */
sweep_device_s sweep_device_construct_simple(const char* port, sweep_error_s* error) {
  SWEEP_ASSERT(error);
  return translateException([&] { return sweep_device_construct_simple(port); }, error);
}

sweep_device_s sweep_device_construct(const char* port, int32_t bitrate, sweep_error_s* error) {
  SWEEP_ASSERT(error);
  return translateException([&] { return sweep_device_construct(port, bitrate); }, error);
}

void sweep_device_destruct(sweep_device_s device) {
  sweep_error_s* error = nullptr;
  translateExceptionV([&] { sweep_device_destruct_impl(device); }, error);
}

void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(error);
  translateExceptionV([&] { sweep_device_start_scanning(device); }, error);
}

void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(error);
  translateExceptionV([&] { sweep_device_stop_scanning(device); }, error);
}

sweep_scan_s sweep_device_get_scan(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(error);
  return translateException([&] { return sweep_device_get_scan(device); }, error);
}

int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(error);
  return translateException([&] { return sweep_device_get_motor_speed(device); }, error);
}

void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(error);
  translateExceptionV([&] { sweep_device_set_motor_speed(device, hz); }, error);
}

int32_t sweep_device_get_sample_rate(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(error);
  return translateException([&] { return sweep_device_get_sample_rate(device); }, error);
}

void sweep_device_set_sample_rate(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(error);
  translateExceptionV([&] { sweep_device_set_sample_rate(device, hz); }, error);
}

void sweep_device_reset(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(error);
  translateExceptionV([&] { sweep_device_reset(device); }, error);
}

/*### scan handling ###*/

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
