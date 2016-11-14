#ifndef SWEEP_PROTOCOL_2EADE195E243_H
#define SWEEP_PROTOCOL_2EADE195E243_H

/*
 * Device communication protocol specifics.
 * Implementation detail; not exported.
 */

#include <stdint.h>

#include "serial.h"
#include "sweep.h"

typedef struct sweep_protocol_error* sweep_protocol_error_s;

const char* sweep_protocol_error_message(sweep_protocol_error_s error);
void sweep_protocol_error_destruct(sweep_protocol_error_s error);

// Command Symbols

#define SWEEP_PROTOCOL_DATA_ACQUISITION_START                                                                                    \
  (const char[]) { 'D', 'S' }
#define SWEEP_PROTOCOL_DATA_ACQUISITION_STOP                                                                                     \
  (const char[]) { 'D', 'X' }
#define SWEEP_PROTOCOL_MOTOR_SPEED_ADJUST                                                                                        \
  (const char[]) { 'M', 'S' }
#define SWEEP_PROTOCOL_MOTOR_INFORMATION                                                                                         \
  (const char[]) { 'M', 'I' }
#define SWEEP_PROTOCOL_VERSION_INFORMATION                                                                                       \
  (const char[]) { 'I', 'V' }
#define SWEEP_PROTOCOL_DEVICE_INFORMATION                                                                                        \
  (const char[]) { 'I', 'D' }
#define SWEEP_PROTOCOL_RESET_DEVICE                                                                                              \
  (const char[]) { 'R', 'R' }

// Packets for communication

typedef struct {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamTerm;
} SWEEP_PACKED sweep_protocol_cmd_packet_s;

typedef struct {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdParamByte1;
  uint8_t cmdParamByte2;
  uint8_t cmdParamTerm;
} SWEEP_PACKED sweep_protocol_cmd_param_packet_s;

typedef struct {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t cmdStatusByte1;
  uint8_t cmdStatusByte2;
  uint8_t cmdSum;
  uint8_t term1;
} SWEEP_PACKED sweep_protocol_response_header_s;

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
} SWEEP_PACKED sweep_protocol_response_param_s;

typedef struct {
  uint8_t sync_error;
  uint16_t angle; // see sweep_protocol_u16_to_f32
  uint16_t distance;
  uint8_t signal_strength;
  uint8_t checksum;
} SWEEP_PACKED sweep_protocol_response_scan_packet_s;

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
} SWEEP_PACKED sweep_protocol_response_info_device_s;

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
} SWEEP_PACKED sweep_protocol_response_info_version_s;

typedef struct {
  uint8_t cmdByte1;
  uint8_t cmdByte2;
  uint8_t motor_speed[2];
  uint8_t term;
} SWEEP_PACKED sweep_protocol_response_info_motor_s;

// Read and write specific packets

void sweep_protocol_write_command(sweep_serial_device_s serial, const void* cmd, sweep_protocol_error_s* error);

void sweep_protocol_write_command_with_arguments(sweep_serial_device_s serial, const void* cmd, const void* args,
                                                 sweep_protocol_error_s* error);

void sweep_protocol_read_response_header(sweep_serial_device_s serial, const void* cmd, sweep_protocol_response_header_s* header,
                                         sweep_protocol_error_s* error);

void sweep_protocol_read_response_param(sweep_serial_device_s serial, const void* cmd, sweep_protocol_response_param_s* param,
                                        sweep_protocol_error_s* error);

void sweep_protocol_read_response_scan(sweep_serial_device_s serial, sweep_protocol_response_scan_packet_s* scan,
                                       sweep_protocol_error_s* error);

void sweep_protocol_read_response_info_motor(sweep_serial_device_s serial, const void* cmd,
                                             sweep_protocol_response_info_motor_s* info, sweep_protocol_error_s* error);

// Some protocol conversion utilities
inline float sweep_protocol_u16_to_f32(uint16_t v) { return ((float)(v >> 4u)) + (v & 15u) / 16.0f; }

inline void sweep_protocol_speed_to_ascii_bytes(int32_t speed, void* byte1, void* byte2) {
  SWEEP_ASSERT(speed >= 0);
  SWEEP_ASSERT(speed <= 10);
  SWEEP_ASSERT(byte1);
  SWEEP_ASSERT(byte2);

  uint8_t* b1 = byte1;
  uint8_t* b2 = byte2;

  // Speed values are still ASCII codes, numbers begin at code point 48
  const uint8_t ASCIINumberBlockOffset = 48;

  uint8_t num1 = (speed / 10) + ASCIINumberBlockOffset;
  uint8_t num2 = (speed % 10) + ASCIINumberBlockOffset;

  *b1 = num1;
  *b2 = num2;
}

inline int32_t sweep_protocol_ascii_bytes_to_speed(const void* byte1, const void* byte2) {
  SWEEP_ASSERT(byte1);
  SWEEP_ASSERT(byte2);

  const uint8_t* b1 = byte1;
  const uint8_t* b2 = byte2;

  // Speed values are still ASCII codes, numbers begin at code point 48
  const uint8_t ASCIINumberBlockOffset = 48;

  uint8_t num1 = *b1 - ASCIINumberBlockOffset;
  uint8_t num2 = *b2 - ASCIINumberBlockOffset;

  int32_t speed = (num1 * 10) + (num2 * 1);

  SWEEP_ASSERT(speed >= 0);
  SWEEP_ASSERT(speed <= 10);

  return speed;
}

#endif
