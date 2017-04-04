#include "sweep.h"
#include "protocol.h"
#include "serial.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

// A threadsafe-queue to store and retrieve scans
class ScanQueue {
public:
  ScanQueue(int32_t max) : the_queue(), the_mutex(), the_cond_var() { max_size = max; }

  // empty the queue
  void flush() {
    std::unique_lock<std::mutex> lock(the_mutex);
    while (!the_queue.empty()) {
      the_queue.pop();
    }
  }

  // Add an element to the queue.
  void enqueue(sweep_scan_s scan) {
    std::lock_guard<std::mutex> lock(the_mutex);

    // if necessary, remove the oldest scan to make room for new
    if (the_queue.size() >= max_size)
      the_queue.pop();

    the_queue.push(scan);
    the_cond_var.notify_one();
  }

  // If the queue is empty, wait till an element is avaiable.
  sweep_scan_s dequeue() {
    std::unique_lock<std::mutex> lock(the_mutex);
    // wait until queue is not empty
    while (the_queue.empty()) {
      // the_cond_var could wake up the thread spontaneously, even if the queue is still empty...
      // so put this wakeup inside a while loop, such that the empty check is performed whenever it wakes up
      the_cond_var.wait(lock); // release lock as long as the wait and reaquire it afterwards.
    }
    sweep_scan_s scan = the_queue.front();
    the_queue.pop();
    return scan;
  }

private:
  int32_t max_size;
  std::queue<sweep_scan_s> the_queue;
  mutable std::mutex the_mutex;
  std::condition_variable the_cond_var;
};

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

typedef struct sweep_error {
  const char* what; // always literal, do not deallocate
} sweep_error;

typedef struct sweep_device {
  sweep::serial::device_s serial; // serial port communication
  bool is_scanning;
  std::unique_ptr<ScanQueue> scan_queue;
  std::atomic<bool> stop_thread;
} sweep_device;

#define SWEEP_MAX_SAMPLES 4096

typedef struct sweep_scan {
  int32_t angle[SWEEP_MAX_SAMPLES];           // in millidegrees
  int32_t distance[SWEEP_MAX_SAMPLES];        // in cm
  int32_t signal_strength[SWEEP_MAX_SAMPLES]; // range 0:255
  int32_t count;
} sweep_scan;

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

sweep_device_s sweep_device_construct_simple(const char* port, sweep_error_s* error) {
  SWEEP_ASSERT(error);

  return sweep_device_construct(port, 115200, error);
}

sweep_device_s sweep_device_construct(const char* port, int32_t bitrate, sweep_error_s* error) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);
  SWEEP_ASSERT(error);

  sweep::serial::error_s serialerror = nullptr;
  sweep::serial::device_s serial = sweep::serial::device_construct(port, bitrate, &serialerror);

  if (serialerror) {
    *error = sweep_error_construct(sweep::serial::error_message(serialerror));
    sweep::serial::error_destruct(serialerror);
    return nullptr;
  }

  // initialize assuming the device is scanning
  auto out =
      new sweep_device{serial, /*is_scanning=*/true, std::unique_ptr<ScanQueue>(new ScanQueue(20)), /*stop_thread=*/{false}};

  // send a stop scanning command in case the scanner was powered on and scanning
  sweep_error_s stoperror = nullptr;
  sweep_device_stop_scanning(out, &stoperror);
  if (stoperror) {
    sweep_error_destruct(stoperror);
    *error = sweep_error_construct("Failed to create sweep device.");
    return nullptr;
  }

  return out;
}

void sweep_device_destruct(sweep_device_s device) {
  SWEEP_ASSERT(device);

  sweep_error_s ignore = nullptr;
  sweep_device_stop_scanning(device, &ignore);
  (void)ignore; // nothing we can do here

  sweep::serial::device_destruct(device->serial);

  delete device;
}

void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  if (device->is_scanning)
    return;

  // Get the current motor speed setting
  sweep_error_s speederror = nullptr;
  int32_t speed = sweep_device_get_motor_speed(device, &speederror);
  if (speederror) {
    *error = sweep_error_construct("unable to start scanning. could not verify motor speed.");
    sweep_error_destruct(speederror);
    return;
  }
  // Check that the motor is not stationary
  if (speed == 0 /*Hz*/) {
    // If the motor is stationary, adjust it to default 5Hz
    sweep_device_set_motor_speed(device, 5 /*Hz*/, &speederror);
    if (speederror) {
      *error = sweep_error_construct("unable to start scanning. failed to start motor.");
      sweep_error_destruct(speederror);
      return;
    }
  }

  // Make sure the motor is stabilized so the DS command doesn't fail
  sweep_error_s stabilityerror = nullptr;
  sweep_device_wait_until_motor_ready(device, &stabilityerror);
  if (stabilityerror) {
    *error = sweep_error_construct("unable to start scanning. motor stability could not be verified.");
    sweep_error_destruct(stabilityerror);
    return;
  }

  // Attempt to start scanning
  sweep_error_s starterror = nullptr;
  sweep_device_attempt_start_scanning(device, &starterror);
  if (starterror) {
    *error = sweep_error_construct("unable to start scanning.");
    sweep_error_destruct(starterror);
    return;
  }

  // Start SCAN WORKER
  device->scan_queue->flush();
  device->is_scanning = true;
  // START background worker thread
  device->stop_thread = false;
  // create a thread
  std::thread th = std::thread(sweep_device_accumulate_scans, device);
  // detach the thread so that it runs in the background and cleans itself up
  th.detach();
}

void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  // STOP the background thread from accumulating scans
  device->stop_thread = true;

  sweep::protocol::error_s protocolerror = nullptr;
  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_STOP, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send stop scanning command");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

  // Wait some time, for the device to register the stop cmd and stop sending data blocks
  std::this_thread::sleep_for(std::chrono::milliseconds(35));

  // Flush the left over data blocks, received after sending the stop cmd
  // This will also flush the response to the stop cmd
  sweep::serial::error_s serialerror = nullptr;
  sweep::serial::device_flush(device->serial, &serialerror);

  if (serialerror) {
    *error = sweep_error_construct("unable to flush serial device for stopping scanning command");
    sweep::serial::error_destruct(serialerror);
    return;
  }

  // Write another stop cmd so we can read a response
  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_STOP, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send stop scanning command");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

  // read the response
  sweep::protocol::response_header_s response;
  sweep::protocol::read_response_header(device->serial, sweep::protocol::DATA_ACQUISITION_STOP, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive stop scanning command response");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

  device->is_scanning = false;
}

void sweep_device_wait_until_motor_ready(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  sweep_error_s readyerror = nullptr;
  bool motor_ready;
  // Only check for 8 seconds (16 iterations with 500ms pause)
  for (auto i = 0; i < 16; ++i) {
    motor_ready = sweep_device_get_motor_ready(device, &readyerror);
    if (readyerror) {
      *error = readyerror;
      return;
    }
    if (motor_ready) {
      return;
    }
    // only check every 500ms, to avoid unecessary processing if this is executing in a dedicated thread
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  *error = sweep_error_construct("timed out waiting for motor to stabilize");
}

// Retrieves a scan from the queue (will block until scan is available)
sweep_scan_s sweep_device_get_scan(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(device->is_scanning);

  auto out = device->scan_queue->dequeue();
  return out;
}

// Accumulates scans in the queue  (method to be used by background thread)
void sweep_device_accumulate_scans(sweep_device_s device) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(device->is_scanning);

  sweep::protocol::error_s protocolerror = nullptr;
  sweep::protocol::response_scan_packet_s responses[SWEEP_MAX_SAMPLES];
  int32_t received = 0;

  while (!device->stop_thread && received < SWEEP_MAX_SAMPLES) {
    sweep::protocol::read_response_scan(device->serial, &responses[received], &protocolerror);
    if (protocolerror) {
      sweep::protocol::error_destruct(protocolerror);
      break;
    }

    const bool is_sync = responses[received].sync_error & sweep::protocol::response_scan_packet_sync::sync;
    const bool has_error = (responses[received].sync_error >> 1) != 0; // shift out sync bit, others are errors

    if (!has_error) {
      received++;
    }

    if (is_sync) {
      if (received <= 1)
        continue;
      // package the previous scan without the sync reading from the subsequent scan
      auto out = new sweep_scan;
      out->count = received - 1; // minus 1 to exclude sync reading
      for (int32_t it = 0; it < received - 1; ++it) {
        // Convert angle from compact serial format to float (in degrees).
        // In addition convert from degrees to milli-degrees.
        out->angle[it] = static_cast<int32_t>(sweep::protocol::u16_to_f32(responses[it].angle) * 1000.f);
        out->distance[it] = responses[it].distance;
        out->signal_strength[it] = responses[it].signal_strength;
      }

      // place the scan in the queue
      device->scan_queue->enqueue(out);

      // place the sync reading at the start for the next scan
      responses[0] = responses[received - 1];

      // reset received
      received = 1;
    }
  }
}

bool sweep_device_get_motor_ready(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::error_s protocolerror = nullptr;

  sweep::protocol::write_command(device->serial, sweep::protocol::MOTOR_READY, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send motor ready command");
    sweep::protocol::error_destruct(protocolerror);
    return false;
  }

  sweep::protocol::response_info_motor_ready_s response;
  sweep::protocol::read_response_info_motor_ready(device->serial, sweep::protocol::MOTOR_READY, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive motor ready command response");
    sweep::protocol::error_destruct(protocolerror);
    return false;
  }

  int32_t ready_code = sweep::protocol::ascii_bytes_to_integral(response.motor_ready);
  SWEEP_ASSERT(ready_code >= 0);

  return ready_code == 0;
}

int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::error_s protocolerror = nullptr;

  sweep::protocol::write_command(device->serial, sweep::protocol::MOTOR_INFORMATION, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send motor speed command");
    sweep::protocol::error_destruct(protocolerror);
    return 0;
  }

  sweep::protocol::response_info_motor_speed_s response;
  sweep::protocol::read_response_info_motor_speed(device->serial, sweep::protocol::MOTOR_INFORMATION, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive motor speed command response");
    sweep::protocol::error_destruct(protocolerror);
    return 0;
  }

  int32_t speed = sweep::protocol::ascii_bytes_to_integral(response.motor_speed);
  SWEEP_ASSERT(speed >= 0);

  return speed;
}

void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz >= 0 && hz <= 10);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  // Make sure the motor is stabilized so the MS command doesn't fail
  sweep_error_s stabilityerror = nullptr;
  sweep_device_wait_until_motor_ready(device, &stabilityerror);
  if (stabilityerror) {
    *error = sweep_error_construct("unable to set motor speed. prior motor stability could not be verified.");
    sweep_error_destruct(stabilityerror);
    return;
  }

  // Attempt to set motor speed
  sweep_device_attempt_set_motor_speed(device, hz, error);
}

int32_t sweep_device_get_sample_rate(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::error_s protocolerror = nullptr;

  sweep::protocol::write_command(device->serial, sweep::protocol::SAMPLE_RATE_INFORMATION, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send sample rate command");
    sweep::protocol::error_destruct(protocolerror);
    return 0;
  }

  sweep::protocol::response_info_sample_rate_s response;
  sweep::protocol::read_response_info_sample_rate(device->serial, sweep::protocol::SAMPLE_RATE_INFORMATION, &response,
                                                  &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive sample rate command response");
    sweep::protocol::error_destruct(protocolerror);
    return 0;
  }

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

void sweep_device_set_sample_rate(sweep_device_s device, int32_t hz, sweep_error_s* error) {
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

  sweep::protocol::error_s protocolerror = nullptr;

  sweep::protocol::write_command_with_arguments(device->serial, sweep::protocol::SAMPLE_RATE_ADJUST, args, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send sample rate command");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

  sweep::protocol::response_param_s response;
  sweep::protocol::read_response_param(device->serial, sweep::protocol::SAMPLE_RATE_ADJUST, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive sample rate command response");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

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

void sweep_device_reset(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::error_s protocolerror = nullptr;

  sweep::protocol::write_command(device->serial, sweep::protocol::RESET_DEVICE, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send device reset command");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }
}

//------- Alternative methods for Low Level development (can error on failure) ------ //
// Attempts to start scanning without waiting for motor ready, does NOT start background thread to accumulate scans
void sweep_device_attempt_start_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  if (device->is_scanning)
    return;

  sweep::protocol::error_s protocolerror = nullptr;
  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_START, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send start scanning command");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

  sweep::protocol::response_header_s response;
  sweep::protocol::read_response_header(device->serial, sweep::protocol::DATA_ACQUISITION_START, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive start scanning command response");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

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
}

// Read incoming scan directly (not retrieving from the queue)
sweep_scan_s sweep_device_get_scan_direct(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(device->is_scanning);

  sweep::protocol::error_s protocolerror = nullptr;

  sweep::protocol::response_scan_packet_s responses[SWEEP_MAX_SAMPLES];

  int32_t received = 0;

  while (received < SWEEP_MAX_SAMPLES) {
    sweep::protocol::read_response_scan(device->serial, &responses[received], &protocolerror);

    if (protocolerror) {
      *error = sweep_error_construct("unable to receive sweep scan response");
      sweep::protocol::error_destruct(protocolerror);
      return nullptr;
    }

    const bool is_sync = responses[received].sync_error & sweep::protocol::response_scan_packet_sync::sync;
    const bool has_error = (responses[received].sync_error >> 1) != 0; // shift out sync bit, others are errors

    if (!has_error) {
      received++;
    }

    if (is_sync) {
      break;
    }
  }

  auto out = new sweep_scan;

  out->count = received;

  for (int32_t it = 0; it < received; ++it) {
    // Convert angle from compact serial format to float (in degrees).
    // In addition convert from degrees to milli-degrees.
    out->angle[it] = static_cast<int32_t>(sweep::protocol::u16_to_f32(responses[it].angle) * 1000.f);
    out->distance[it] = responses[it].distance;
    out->signal_strength[it] = responses[it].signal_strength;
  }

  return out;
}

// Attempts to set motor speed without waiting for motor ready
void sweep_device_attempt_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz >= 0 && hz <= 10);
  SWEEP_ASSERT(error);
  SWEEP_ASSERT(!device->is_scanning);

  uint8_t args[2] = {0};
  sweep::protocol::integral_to_ascii_bytes(hz, args);

  sweep::protocol::error_s protocolerror = nullptr;

  sweep::protocol::write_command_with_arguments(device->serial, sweep::protocol::MOTOR_SPEED_ADJUST, args, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send motor speed command");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

  sweep::protocol::response_param_s response;
  sweep::protocol::read_response_param(device->serial, sweep::protocol::MOTOR_SPEED_ADJUST, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive motor speed command response");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

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
}