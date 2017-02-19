#include "sweep.h"
#include "protocol.h"
#include "serial.h"

#include <chrono>
#include <thread>

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

typedef struct sweep_error {
  const char* what; // always literal, do not deallocate
} sweep_error;

typedef struct sweep_device {
  sweep::serial::device_s serial; // serial port communication
  bool is_scanning;
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

sweep_device_s sweep_device_construct_simple(sweep_error_s* error) {
  SWEEP_ASSERT(error);

  return sweep_device_construct("/dev/ttyUSB0", 115200, error);
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

  auto out = new sweep_device{serial, /*is_scanning=*/true};

  sweep_error_s stoperror = nullptr;
  sweep_device_stop_scanning(out, &stoperror);

  if (stoperror) {
    *error = stoperror;
    sweep_device_destruct(out);
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

  device->is_scanning = true;
}

void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  if (!device->is_scanning)
    return;

  sweep::protocol::error_s protocolerror = nullptr;
  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_STOP, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send stop scanning command");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

  // Wait until device stopped sending
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  sweep::serial::error_s serialerror = nullptr;
  sweep::serial::device_flush(device->serial, &serialerror);

  if (serialerror) {
    *error = sweep_error_construct("unable to flush serial device for stopping scanning command");
    sweep::serial::error_destruct(serialerror);
    return;
  }

  sweep::protocol::write_command(device->serial, sweep::protocol::DATA_ACQUISITION_STOP, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send stop scanning command");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

  sweep::protocol::response_header_s response;
  sweep::protocol::read_response_header(device->serial, sweep::protocol::DATA_ACQUISITION_STOP, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive stop scanning command response");
    sweep::protocol::error_destruct(protocolerror);
    return;
  }

  device->is_scanning = false;
}

sweep_scan_s sweep_device_get_scan(sweep_device_s device, sweep_error_s* error) {
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
    out->angle[it] = sweep::protocol::u16_to_f32(responses[it].angle) * 1000.f;
    out->distance[it] = responses[it].distance;
    out->signal_strength[it] = responses[it].signal_strength;
  }

  return out;
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
  SWEEP_ASSERT(!device->is_scanning);

  sweep::protocol::error_s protocolerror = nullptr;

  sweep::protocol::write_command(device->serial, sweep::protocol::MOTOR_INFORMATION, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send motor speed command");
    sweep::protocol::error_destruct(protocolerror);
    return 0;
  }

  sweep::protocol::response_info_motor_s response;
  sweep::protocol::read_response_info_motor(device->serial, sweep::protocol::MOTOR_INFORMATION, &response, &protocolerror);

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
