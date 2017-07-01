#include "error.hpp"
#include "protocol.hpp"
#include "queue.hpp"
#include "serial.hpp"

#include "sweep.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <string>
#include <thread>

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

struct sweep_error {
  std::string what;
};

#define SWEEP_MAX_SAMPLES 4096

struct sample {
  int32_t angle;           // in millidegrees
  int32_t distance;        // in cm
  int32_t signal_strength; // range 0:255
};

struct sweep_scan {
  sample samples[SWEEP_MAX_SAMPLES];
  int32_t count;
};

static sample parse_payload(const sweep::protocol::response_scan_packet_s& msg) {
  sample ret;
  ret.angle = msg.get_angle_millideg();
  ret.distance = msg.distance;
  ret.signal_strength = msg.signal_strength;
  return ret;
}

struct sweep_device {
  sweep::serial::device_s serial; // serial port communication
  bool is_scanning;
  std::atomic<bool> stop_thread;

  struct Element {
    std::unique_ptr<sweep_scan> scan;
    std::exception_ptr error;
  };

  sweep::queue::queue<Element> scan_queue;
};

// Constructor hidden from users
static sweep_error_s sweep_error_construct(const char* what) {
  SWEEP_ASSERT(what);

  auto out = new sweep_error{what};
  return out;
}

const char* sweep_error_message(sweep_error_s error) {
  SWEEP_ASSERT(error);

  return error->what.c_str();
}

void sweep_error_destruct(sweep_error_s error) {
  SWEEP_ASSERT(error);

  delete error;
}

// Blocks until the device is ready.
// Device is ready when the calibration completes and motor speed stabilizes
static void sweep_device_wait_until_motor_ready(sweep_device_s device, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  // Motor adjustments can take 7-9 seconds, so timeout after 10 seconds to be safe
  // (20 iterations with 500ms pause)
  for (int32_t i = 0; i < 20; ++i) {
    if (i > 0) {
      // only check every 500ms, to avoid unecessary processing if this is executing in a dedicated thread
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (sweep_device_get_motor_ready(device, error))
      return;
  }

  *error = sweep_error_construct("timed out waiting for motor to stabilize");
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
}

// Accumulates scans in a queue. Used by background thread
static void sweep_device_accumulate_scans(sweep_device_s device) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(device->is_scanning);

  sample buffer[SWEEP_MAX_SAMPLES];
  int32_t received = 0;

  while (!device->stop_thread && received < SWEEP_MAX_SAMPLES) {

    sweep::protocol::response_scan_packet_s response;
    sweep::protocol::read_response_scan(device->serial, &response);

    if (!response.has_error()) {
      buffer[received] = parse_payload(response);
      received++;
    }

    if (response.is_sync() && received > 1) {

      // package the previous scan without the sync reading from the subsequent scan
      auto out = std::unique_ptr<sweep_scan>(new sweep_scan);
      out->count = received - 1; // minus 1 to exclude sync reading

      std::copy_n(std::begin(buffer), received - 1, std::begin(out->samples));

      // place the scan in the queue
      device->scan_queue.enqueue({std::move(out), nullptr});

      // place the sync reading at the start for the next scan
      buffer[0] = buffer[received - 1];

      // reset received
      received = 1;
    }
  }
} catch (...) {
  // worker thread is dead at this point
  device->scan_queue.enqueue({nullptr, std::current_exception()});
}

// Attempts to start scanning without waiting for motor ready. Can error on failure.
// Does NOT start background thread to accumulate scans.
static void sweep_device_attempt_start_scanning(sweep_device_s device, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  if (device->is_scanning)
    return;

  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_START);

  sweep::protocol::response_header_s response;
  sweep::protocol::read_response_header(device->serial, sweep::protocol::DATA_ACQUISITION_START, &response);

  // Check the status bytes do not indicate failure
  const uint8_t status_bytes[2] = {response.cmdStatusByte1, response.cmdStatusByte2};
  int32_t status_code = sweep::protocol::ascii_bytes_to_integral(status_bytes);
  switch (status_code) {
  case 12:
    *error = sweep_error_construct("Failed to start scanning because motor speed has not stabilized.");
    return;
  case 13:
    *error = sweep_error_construct("Failed to start scanning because motor is stationary.");
    return;
  default:
    break;
  }
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
}

// Attempts to set motor speed without waiting for motor ready. Can error on failure.
static void sweep_device_attempt_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz >= 0 && hz <= 10);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  uint8_t args[2] = {0};
  sweep::protocol::integral_to_ascii_bytes(hz, args);

  sweep::protocol::write_command_with_arguments(device->serial, sweep::protocol::MOTOR_SPEED_ADJUST, args);

  sweep::protocol::response_param_s response;
  sweep::protocol::read_response_param(device->serial, sweep::protocol::MOTOR_SPEED_ADJUST, &response);

  // Check the status bytes do not indicate failure
  const uint8_t status_bytes[2] = {response.cmdStatusByte1, response.cmdStatusByte2};
  int32_t status_code = sweep::protocol::ascii_bytes_to_integral(status_bytes);
  switch (status_code) {
  case 11:
    *error = sweep_error_construct("Failed to set motor speed because provided parameter was invalid.");
    return;
  case 12:
    *error = sweep_error_construct("Failed to set motor speed because prior speed has not yet stabilized.");
    return;
  default:
    break;
  }
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
}

sweep_device_s sweep_device_construct_simple(const char* port, sweep_error_s* error) try {
  SWEEP_ASSERT(error);
  return sweep_device_construct(port, 115200, error);
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
  return nullptr;
}

sweep_device_s sweep_device_construct(const char* port, int32_t bitrate, sweep_error_s* error) try {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);
  SWEEP_ASSERT(error);

  sweep::serial::device_s serial = sweep::serial::device_construct(port, bitrate);

  // initialize assuming the device is scanning
  auto out = new sweep_device{serial, /*is_scanning=*/true, /*stop_thread=*/{false}, /*scan_queue=*/{20}};

  // send a stop scanning command in case the scanner was powered on and scanning
  sweep_device_stop_scanning(out, error);

  return out;
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
  return nullptr;
}

void sweep_device_destruct(sweep_device_s device) {
  SWEEP_ASSERT(device);

  try {
    sweep_error_s ignore = nullptr;
    sweep_device_stop_scanning(device, &ignore);
  } catch (...) {
    // nothing we can do here
  }

  sweep::serial::device_destruct(device->serial);

  delete device;
}

void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  if (device->is_scanning)
    return;

  // Get the current motor speed setting
  int32_t speed = sweep_device_get_motor_speed(device, error);

  // Check that the motor is not stationary
  if (speed == 0 /*Hz*/) {
    // If the motor is stationary, adjust it to default 5Hz
    sweep_device_set_motor_speed(device, 5 /*Hz*/, error);
  }

  // Make sure the motor is stabilized so the DS command doesn't fail
  sweep_device_wait_until_motor_ready(device, error);

  // Attempt to start scanning
  sweep_device_attempt_start_scanning(device, error);

  // Start SCAN WORKER
  device->scan_queue.clear();
  device->is_scanning = true;
  // START background worker thread
  device->stop_thread = false;
  // create a thread
  std::thread th = std::thread(sweep_device_accumulate_scans, device);
  // detach the thread so that it runs in the background and cleans itself up
  th.detach();
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
}

void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  // STOP the background thread from accumulating scans
  device->stop_thread = true;

  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_STOP);

  // Wait some time for a few reasons:
  // 1. allow time for the device to register the stop cmd and stop sending data blocks
  // 2. allow time for slower performing devices (RaspberryPi) to buffer the stop response
  // 3. allow a grace period for the device to perform its logging routine before sending a second stop command
  std::this_thread::sleep_for(std::chrono::milliseconds(35));

  // Read the response from the first stop command
  // It is possible this will contain garbage bytes (leftover from data stream), and will error
  // But we are guaranteed to read at least as many bytes as the length of a stop response
  sweep::protocol::response_header_s garbage_response;
  try {
    sweep::protocol::read_response_header(device->serial, sweep::protocol::DATA_ACQUISITION_STOP, &garbage_response);
  } catch (const std::exception& ignore) {
    // Catch and ignore the error in the case of the stop response containing garbage bytes
    // Occurs if the device was actively streaming data before the stop cmd
    (void)ignore;
  }

  // Flush any bytes left over, in case device was actively streaming data before the stop cmd
  sweep::serial::device_flush(device->serial);

  // Write another stop cmd so we can read a valid response
  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_STOP);

  // read the response
  sweep::protocol::response_header_s response;
  sweep::protocol::read_response_header(device->serial, sweep::protocol::DATA_ACQUISITION_STOP, &response);

  device->is_scanning = false;
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
}

// Retrieves a scan from the queue (will block until scan is available)
sweep_scan_s sweep_device_get_scan(sweep_device_s device, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(device->is_scanning);

  auto out = device->scan_queue.dequeue();

  if (out.error != nullptr) {
    std::rethrow_exception(out.error);
  }

  return out.scan.release();

} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
  return nullptr;
}

bool sweep_device_get_motor_ready(sweep_device_s device, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::write_command(device->serial, sweep::protocol::MOTOR_READY);

  sweep::protocol::response_info_motor_ready_s response;
  sweep::protocol::read_response_info_motor_ready(device->serial, sweep::protocol::MOTOR_READY, &response);

  int32_t ready_code = sweep::protocol::ascii_bytes_to_integral(response.motor_ready);
  SWEEP_ASSERT(ready_code >= 0);

  return ready_code == 0;
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
  return false;
}

int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::write_command(device->serial, sweep::protocol::MOTOR_INFORMATION);

  sweep::protocol::response_info_motor_speed_s response;
  sweep::protocol::read_response_info_motor_speed(device->serial, sweep::protocol::MOTOR_INFORMATION, &response);

  int32_t speed = sweep::protocol::ascii_bytes_to_integral(response.motor_speed);
  SWEEP_ASSERT(speed >= 0);

  return speed;
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
  return -1;
}

void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz >= 0 && hz <= 10);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  // Make sure the motor is stabilized so the MS command doesn't fail
  sweep_device_wait_until_motor_ready(device, error);

  // Attempt to set motor speed
  sweep_device_attempt_set_motor_speed(device, hz, error);
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
}

int32_t sweep_device_get_sample_rate(sweep_device_s device, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
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
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
  return -1;
}

void sweep_device_set_sample_rate(sweep_device_s device, int32_t hz, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz == 500 || hz == 750 || hz == 1000);
  SWEEP_ASSERT(error);
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

  // Check the status bytes do not indicate failure
  const uint8_t status_bytes[2] = {response.cmdStatusByte1, response.cmdStatusByte2};
  int32_t status_code = sweep::protocol::ascii_bytes_to_integral(status_bytes);
  switch (status_code) {
  case 11:
    *error = sweep_error_construct("Failed to set motor speed because provided parameter was invalid.");
    return;
  default:
    break;
  }
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
}

int32_t sweep_scan_get_number_of_samples(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(scan->count >= 0);

  return scan->count;
}

int32_t sweep_scan_get_angle(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->samples[sample].angle;
}

int32_t sweep_scan_get_distance(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->samples[sample].distance;
}

int32_t sweep_scan_get_signal_strength(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->samples[sample].signal_strength;
}

void sweep_scan_destruct(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);

  delete scan;
}

void sweep_device_reset(sweep_device_s device, sweep_error_s* error) try {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::write_command(device->serial, sweep::protocol::RESET_DEVICE);
} catch (const std::exception& e) {
  *error = sweep_error_construct(e.what());
}
