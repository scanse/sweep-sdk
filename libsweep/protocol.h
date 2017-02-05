#ifndef SWEEP_PROTOCOL_2EADE195E243_H
#define SWEEP_PROTOCOL_2EADE195E243_H

/*
 * Device communication protocol specifics.
 * Implementation detail; not exported.
 */

#include <stdint.h>

#include "serial.h"
#include "sweep.h"

namespace sweep {
namespace protocol {

typedef struct error* error_s;

const char* error_message(error_s error);
void error_destruct(error_s error);

// Command Symbols

extern const uint8_t DATA_ACQUISITION_START[2];
extern const uint8_t DATA_ACQUISITION_STOP[2];
extern const uint8_t MOTOR_SPEED_ADJUST[2];
extern const uint8_t MOTOR_INFORMATION[2];
extern const uint8_t VERSION_INFORMATION[2];
extern const uint8_t DEVICE_INFORMATION[2];
extern const uint8_t RESET_DEVICE[2];

// Packets for communication

typedef struct {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamTerm;
} SWEEP_PACKED cmd_packet_s;

typedef struct {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamByte1;
  uint8_t cmdParamByte2;
  uint8_t cmdParamTerm;
} SWEEP_PACKED cmd_param_packet_s;

typedef struct {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdStatusByte1;
  uint8_t cmdStatusByte2;
  uint8_t cmdSum;
  uint8_t term1;
} SWEEP_PACKED response_header_s;

typedef struct {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamByte1;
  uint8_t cmdParamByte2;
  uint8_t term1;
  uint8_t cmdStatusByte1;
  uint8_t cmdStatusByte2;
  uint8_t cmdSum;
  uint8_t term2;
} SWEEP_PACKED response_param_s;

typedef struct {
  uint8_t sync_error;
  uint16_t angle; // see u16_to_f32
  uint16_t distance;
  uint8_t signal_strength;
  uint8_t checksum;
} SWEEP_PACKED response_scan_packet_s;

typedef struct {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t bit_rate[6];
  uint8_t laser_state;
  uint8_t mode;
  uint8_t diagnostic;
  uint8_t motor_speed[2];
  uint8_t sample_rate[4];
  uint8_t term;
} SWEEP_PACKED response_info_device_s;

typedef struct {
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
} SWEEP_PACKED response_info_version_s;

typedef struct {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t motor_speed[2];
  uint8_t term;
} SWEEP_PACKED response_info_motor_s;

// Read and write specific packets

void write_command(sweep::serial::device_s serial, const uint8_t cmd[2], error_s* error);

void write_command_with_arguments(sweep::serial::device_s serial, const uint8_t cmd[2], const uint8_t arg[2], error_s* error);

void read_response_header(sweep::serial::device_s serial, const uint8_t cmd[2], response_header_s* header, error_s* error);

void read_response_param(sweep::serial::device_s serial, const uint8_t cmd[2], response_param_s* param, error_s* error);

void read_response_scan(sweep::serial::device_s serial, response_scan_packet_s* scan, error_s* error);

void read_response_info_motor(sweep::serial::device_s serial, const uint8_t cmd[2], response_info_motor_s* info, error_s* error);

// Some protocol conversion utilities
inline float u16_to_f32(uint16_t v) { return ((float)(v >> 4u)) + (v & 15u) / 16.0f; }

inline void speed_to_ascii_bytes(int32_t speed, uint8_t bytes[2]) {
  SWEEP_ASSERT(speed >= 0);
  SWEEP_ASSERT(speed <= 10);
  SWEEP_ASSERT(bytes);

  // Speed values are still ASCII codes, numbers begin at code point 48
  const uint8_t ASCIINumberBlockOffset = 48;

  uint8_t num1 = (speed / 10) + ASCIINumberBlockOffset;
  uint8_t num2 = (speed % 10) + ASCIINumberBlockOffset;

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
