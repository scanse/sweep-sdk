#pragma once

#include "protocol.h"
#include "serial.h"

#include <chrono>
#include <string>
#include <thread>

#include "sweep.h"
#include "sweep_device.hpp"

#define SWEEP_MAX_SAMPLES 4096

struct sweep_scan {
  int32_t angle[SWEEP_MAX_SAMPLES];           // in millidegrees
  int32_t distance[SWEEP_MAX_SAMPLES];        // in cm
  int32_t signal_strength[SWEEP_MAX_SAMPLES]; // range 0:255
  int32_t count;
};

struct sweep_device {
public:
  sweep_device();
  sweep_device(const char* port, int32_t bitrate);
  ~sweep_device();

  void start_scanning();
  void stop_scanning();

  sweep_scan_s get_scan();

  int32_t get_motor_speed();
  void set_motor_speed(int32_t hz);

  void reset();

private:
  sweep::serial::Device serial; // serial port communication
  bool is_scanning;
};

sweep_device::sweep_device() : sweep_device("/dev/ttyUSB0", 115200) {}

sweep_device::sweep_device(const char* port, int32_t bitrate) : serial{port, bitrate}, is_scanning{true} {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);
  stop_scanning();
}

sweep_device::~sweep_device() {
  try {
    stop_scanning();
  } catch (...) {
  }
}

void sweep_device::start_scanning() {
  if (is_scanning)
    return;

  sweep::protocol::write_command(serial, sweep::protocol::DATA_ACQUISITION_START);
  sweep::protocol::read_response_header(serial, sweep::protocol::DATA_ACQUISITION_START);

  is_scanning = true;
}

void sweep_device::stop_scanning() {
  if (!is_scanning)
    return;

  sweep::protocol::write_command(serial, sweep::protocol::DATA_ACQUISITION_STOP);

  // Wait until device stopped sending
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  serial.flush();
  sweep::protocol::write_command(serial, sweep::protocol::DATA_ACQUISITION_STOP);
  sweep::protocol::read_response_header(serial, sweep::protocol::DATA_ACQUISITION_STOP);
  is_scanning = false;
}

sweep_scan_s sweep_device::get_scan() {
  sweep::protocol::response_scan_packet_s responses[SWEEP_MAX_SAMPLES];

  int32_t first = 0;
  int32_t last = SWEEP_MAX_SAMPLES;

  for (int32_t received = 0; received < SWEEP_MAX_SAMPLES; ++received) {

    sweep::protocol::read_response_scan(serial, &responses[received]);

    // Only gather a full scan. We could improve this logic to improve on throughput
    // with complicating the code much more (think spsc queue of all responses).
    // On the other hand, we could also discard the sync bit and check repeating angles.
    if (responses[received].sync_error == 1) {
      if (first != 0 && last == SWEEP_MAX_SAMPLES) {
        last = received;
        break;
      }

      if (first == 0 && last == SWEEP_MAX_SAMPLES) {
        first = received;
      }
    }
  }

  auto out = new sweep_scan;

  SWEEP_ASSERT(last - first >= 0);
  SWEEP_ASSERT(last - first < SWEEP_MAX_SAMPLES);

  out->count = last - first;

  for (int32_t it = 0; it < last - first; ++it) {
    // Convert angle from compact serial format to float (in degrees).
    // In addition convert from degrees to milli-degrees.
    out->angle[it] = static_cast<int32_t>(sweep::protocol::u16_to_f32(responses[first + it].angle) * 1000.f);
    out->distance[it] = responses[first + it].distance;
    out->signal_strength[it] = responses[first + it].signal_strength;
  }

  return out;
}

int32_t sweep_device::get_motor_speed() {
  sweep::protocol::write_command(serial, sweep::protocol::MOTOR_INFORMATION);

  auto response = sweep::protocol::read_response_info_motor(serial, sweep::protocol::MOTOR_INFORMATION);
  int32_t speed = sweep::protocol::ascii_bytes_to_speed(response.motor_speed);

  return speed;
}

void sweep_device::set_motor_speed(int32_t hz) {
  SWEEP_ASSERT(hz >= 0 && hz <= 10);

  uint8_t args[2] = {0};
  sweep::protocol::speed_to_ascii_bytes(hz, args);

  sweep::protocol::write_command_with_arguments(serial, sweep::protocol::MOTOR_SPEED_ADJUST, args);
  sweep::protocol::read_response_param(serial, sweep::protocol::MOTOR_SPEED_ADJUST);
}

void sweep_device::reset() { sweep::protocol::write_command(serial, sweep::protocol::RESET_DEVICE); }
