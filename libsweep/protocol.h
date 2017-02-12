#ifndef SWEEP_PROTOCOL_2EADE195E243_H
#define SWEEP_PROTOCOL_2EADE195E243_H

/*
 * Device communication protocol specifics.
 * Implementation detail; not exported.
 */

#include <stdint.h>

#include "Error.h"
#include "serial.h"
#include "sweep.h"

namespace sweep {
namespace protocol {

struct Error : ErrorBase {
  using ErrorBase::ErrorBase;
};

// Command Symbols

extern const uint8_t DATA_ACQUISITION_START[2];
extern const uint8_t DATA_ACQUISITION_STOP[2];
extern const uint8_t MOTOR_SPEED_ADJUST[2];
extern const uint8_t MOTOR_INFORMATION[2];
extern const uint8_t VERSION_INFORMATION[2];
extern const uint8_t DEVICE_INFORMATION[2];
extern const uint8_t RESET_DEVICE[2];

// Packets for communication

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

struct response_scan_packet_s {
  uint8_t sync_error;
  uint16_t angle; // see u16_to_f32
  uint16_t distance;
  uint8_t signal_strength;
  uint8_t checksum;
};

static_assert(sizeof(response_scan_packet_s) == 7, "response scan packet size mismatch");

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

struct response_info_motor_s {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t motor_speed[2];
  uint8_t term;
};

static_assert(sizeof(response_info_motor_s) == 5, "response info motor size mismatch");

#pragma pack(pop)

// Read and write specific packets

void write_command(sweep::serial::Device& serial, const uint8_t cmd[2]);

void write_command_with_arguments(sweep::serial::Device& serial, const uint8_t cmd[2], const uint8_t arg[2]);

response_header_s read_response_header(sweep::serial::Device& serial, const uint8_t cmd[2]);

response_param_s read_response_param(sweep::serial::Device& serial, const uint8_t cmd[2]);

void read_response_scan(sweep::serial::Device& serial, response_scan_packet_s* scan);

response_info_motor_s read_response_info_motor(sweep::serial::Device& serial, const uint8_t cmd[2]);

// Some protocol conversion utilities
inline float u16_to_f32(uint16_t v) { return ((float)(v >> 4u)) + (v & 15u) / 16.0f; }

inline void speed_to_ascii_bytes(int32_t speed, uint8_t bytes[2]) {
  SWEEP_ASSERT(speed >= 0);
  SWEEP_ASSERT(speed <= 10);
  SWEEP_ASSERT(bytes);

  // Speed values are still ASCII codes, numbers begin at code point 48
  const uint8_t ASCIINumberBlockOffset = 48;

  uint8_t num1 = (uint8_t)(speed / 10) + ASCIINumberBlockOffset;
  uint8_t num2 = (uint8_t)(speed % 10) + ASCIINumberBlockOffset;

  bytes[0] = num1;
  bytes[1] = num2;
}

inline int32_t ascii_bytes_to_speed(const uint8_t bytes[2]) {
  SWEEP_ASSERT(bytes);

  // Speed values are still ASCII codes, numbers begin at code point 48
  const uint8_t ASCIINumberBlockOffset = 48;

  uint8_t num1 = bytes[0] - ASCIINumberBlockOffset;
  uint8_t num2 = bytes[1] - ASCIINumberBlockOffset;

  int32_t speed = (num1 * 10) + (num2 * 1);

  SWEEP_ASSERT(speed >= 0);
  SWEEP_ASSERT(speed <= 10);

  return speed;
}

} // ns protocol
} // ns sweep

#endif
