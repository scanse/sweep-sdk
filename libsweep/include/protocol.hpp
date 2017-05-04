#ifndef SWEEP_PROTOCOL_2EADE195E243_HPP
#define SWEEP_PROTOCOL_2EADE195E243_HPP

/*
 * Device communication protocol specifics.
 * Implementation detail; not exported.
 */

#include "error.hpp"
#include "serial.hpp"

#include "sweep.h"

#include <stdint.h>

namespace sweep {
namespace protocol {

struct error : sweep::error::error {
  using base = sweep::error::error;
  using base::base;
};

// Command Symbols

constexpr uint8_t DATA_ACQUISITION_START[2] = { 'D', 'S' };
constexpr uint8_t DATA_ACQUISITION_STOP[2] = { 'D', 'X' };
constexpr uint8_t MOTOR_SPEED_ADJUST[2] = { 'M', 'S' };
constexpr uint8_t MOTOR_READY[2] = { 'M', 'Z' };
constexpr uint8_t MOTOR_INFORMATION[2] = { 'M', 'I' };
constexpr uint8_t SAMPLE_RATE_ADJUST[2] = { 'L', 'R' };
constexpr uint8_t SAMPLE_RATE_INFORMATION[2] = { 'L', 'I' };
constexpr uint8_t VERSION_INFORMATION[2] = { 'I', 'V' };
constexpr uint8_t DEVICE_INFORMATION[2] = { 'I', 'D' };
constexpr uint8_t RESET_DEVICE[2] = { 'R', 'R' };

// Packets for communication

// Make in-memory representations correspond to bytes we send over the wire.
#pragma pack(push, 1)

struct cmd_packet_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamTerm;
};

static_assert(sizeof(cmd_packet_s) == 3, "cmd packet size mismatch");

struct cmd_param_packet_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamByte1;
  uint8_t cmdParamByte2;
  uint8_t cmdParamTerm;
};

static_assert(sizeof(cmd_param_packet_s) == 5, "cmd param packet size mismatch");

struct response_header_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdStatusByte1;
  uint8_t cmdStatusByte2;
  uint8_t cmdSum;
  uint8_t term1;
};

static_assert(sizeof(response_header_s) == 6, "response header size mismatch");

struct response_param_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamByte1;
  uint8_t cmdParamByte2;
  uint8_t term1;
  uint8_t cmdStatusByte1;
  uint8_t cmdStatusByte2;
  uint8_t cmdSum;
  uint8_t term2;
};

static_assert(sizeof(response_param_s) == 9, "response param size mismatch");

struct response_scan_packet_s{
  uint8_t sync_error; // see response_scan_packet_sync::bits below
  uint16_t angle;     // see u16_to_f32
  uint16_t distance;
  uint8_t signal_strength;
  uint8_t checksum;
};

static_assert(sizeof(response_scan_packet_s) == 7, "response scan packet size mismatch");

namespace response_scan_packet_sync {
enum bits : uint8_t {
  sync = 1 << 0,                // beginning of new full scan
  communication_error = 1 << 1, // communication error

  // Reserved for future error bits
  reserved2 = 1 << 2,
  reserved3 = 1 << 3,
  reserved4 = 1 << 4,
  reserved5 = 1 << 5,
  reserved6 = 1 << 6,
  reserved7 = 1 << 7,
};
}

struct response_info_device_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t bit_rate[6];
  uint8_t laser_state;
  uint8_t mode;
  uint8_t diagnostic;
  uint8_t motor_speed[2];
  uint8_t sample_rate[4];
  uint8_t term;
};

static_assert(sizeof(response_info_device_s) == 18, "response info device size mismatch");

struct response_info_version_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t model[5];
  uint8_t protocol_major;
  uint8_t protocol_min;
  uint8_t firmware_major;
  uint8_t firmware_minor;
  uint8_t hardware_version;
  uint8_t serial_no[8];
  uint8_t term;
};

static_assert(sizeof(response_info_version_s) == 21, "response info version size mismatch");

struct response_info_motor_ready_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t motor_ready[2];
  uint8_t term;
};

static_assert(sizeof(response_info_motor_ready_s) == 5, "response info motor ready size mismatch");

struct response_info_motor_speed_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t motor_speed[2];
  uint8_t term;
};

static_assert(sizeof(response_info_motor_speed_s) == 5, "response info motor speed size mismatch");

struct response_info_sample_rate_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t sample_rate[2];
  uint8_t term;
};

static_assert(sizeof(response_info_sample_rate_s) == 5, "response info sample rate size mismatch");

// Done with in-memory representations for packets we send over the wire.
#pragma pack(pop)

// Read and write specific packets

void write_command(sweep::serial::device_s serial, const uint8_t cmd[2]);

void write_command_with_arguments(sweep::serial::device_s serial, const uint8_t cmd[2], const uint8_t arg[2]);

void read_response_header(sweep::serial::device_s serial, const uint8_t cmd[2], response_header_s* header);

void read_response_param(sweep::serial::device_s serial, const uint8_t cmd[2], response_param_s* param);

void read_response_scan(sweep::serial::device_s serial, response_scan_packet_s* scan);

void read_response_info_motor_ready(sweep::serial::device_s serial, const uint8_t cmd[2], response_info_motor_ready_s* info);

void read_response_info_motor_speed(sweep::serial::device_s serial, const uint8_t cmd[2], response_info_motor_speed_s* info);

void read_response_info_sample_rate(sweep::serial::device_s serial, const uint8_t cmd[2], response_info_sample_rate_s* info);

// Some protocol conversion utilities
inline float u16_to_f32(uint16_t v) { return ((float)(v >> 4u)) + (v & 15u) / 16.0f; }

inline void integral_to_ascii_bytes(const int32_t integral, uint8_t bytes[2]) {
  SWEEP_ASSERT(integral >= 0);
  SWEEP_ASSERT(integral <= 99);
  SWEEP_ASSERT(bytes);

  // Numbers begin at ASCII code point 48
  const uint8_t ASCIINumberBlockOffset = '0';

  const uint8_t num1 = (integral / 10) + ASCIINumberBlockOffset;
  const uint8_t num2 = (integral % 10) + ASCIINumberBlockOffset;

  bytes[0] = num1;
  bytes[1] = num2;
}

inline int32_t ascii_bytes_to_integral(const uint8_t bytes[2]) {
  SWEEP_ASSERT(bytes);

  const uint8_t ASCIINumberBlockOffset = '0';

  SWEEP_ASSERT(bytes[0] >= ASCIINumberBlockOffset);
  SWEEP_ASSERT(bytes[1] >= ASCIINumberBlockOffset);

  const uint8_t num1 = static_cast<uint8_t>(bytes[0] - ASCIINumberBlockOffset);
  const uint8_t num2 = static_cast<uint8_t>(bytes[1] - ASCIINumberBlockOffset);

  SWEEP_ASSERT(num1 <= 9);
  SWEEP_ASSERT(num2 <= 9);

  const int32_t integral = (num1 * 10) + (num2 * 1);

  SWEEP_ASSERT(integral >= 0);
  SWEEP_ASSERT(integral <= 99);

  return integral;
}

} // ns protocol
} // ns sweep

#endif
