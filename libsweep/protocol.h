#ifndef SWEEP_PROTOCOL_2EADE195E243_H
#define SWEEP_PROTOCOL_2EADE195E243_H

#include <stdint.h>

#include "sweep.h"

#define SWEEP_PROTOCOL_DATA_ACQUISITION_START                                                                                    \
  (const char[]) { 'D', 'S' }
#define SWEEP_PROTOCOL_DATA_ACQUISITION_STOP                                                                                     \
  (const char[]) { 'D', 'X' }
#define SWEEP_PROTOCOL_MOTOR_START                                                                                               \
  (const char[]) { 'M', 'S' }
#define SWEEP_PROTOCOL_MOTOR_STOP                                                                                                \
  (const char[]) { 'M', 'X' }
#define SWEEP_PROTOCOL_MOTOR_SPEED_ADJUST                                                                                        \
  (const char[]) { 'M', 'A' }
#define SWEEP_PROTOCOL_MOTOR_INFORMATION                                                                                         \
  (const char[]) { 'M', 'I' }
#define SWEEP_PROTOCOL_VERSION_INFORMATION                                                                                       \
  (const char[]) { 'I', 'V' }
#define SWEEP_PROTOCOL_DEVICE_INFORMATION                                                                                        \
  (const char[]) { 'I', 'D' }
#define SWEEP_PROTOCOL_PRINT_COMMANDS                                                                                            \
  (const char[]) { 'P', 'C' }
#define SWEEP_PROTOCOL_PRINT_VERSION_INFORMATION                                                                                 \
  (const char[]) { 'P', 'V' }
#define SWEEP_PROTOCOL_PRINT_DEVICE_INFORMATION                                                                                  \
  (const char[]) { 'P', 'D' }
#define SWEEP_PROTOCOL_PRINT_MOTOR_INFORMATION                                                                                   \
  (const char[]) { 'P', 'M' }
#define SWEEP_PROTOCOL_PRINT_LIDAR_INFORMATION                                                                                   \
  (const char[]) { 'P', 'L' }
#define SWEEP_PROTOCOL_RESET_DEVICE                                                                                              \
  (const char[]) { 'R', 'R' }

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
  uint8_t cmdSum; // see sweep_protocol_checksum_response_header
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
  uint8_t cmdSum; // see sweep_protocol_checksum_response_param
  uint8_t term2;
} SWEEP_PACKED sweep_protocol_response_param_s;

typedef struct {
  uint8_t sync_error;
  uint16_t angle; // see sweep_protocol_u16_to_f32
  uint16_t distance;
  uint8_t signal_strength;
  uint8_t checksum; // see sweep_protocol_checksum_response_scan_packet
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

inline uint8_t sweep_protocol_checksum_response_header(sweep_protocol_response_header_s* v) {
  return ((v->cmdStatusByte1 + v->cmdStatusByte2) & 0x3F) + 0x30;
}

inline uint8_t sweep_protocol_checksum_response_param(sweep_protocol_response_param_s* v) {
  return ((v->cmdStatusByte1 + v->cmdStatusByte2) & 0x3F) + 0x30;
}

inline uint8_t sweep_protocol_checksum_response_scan_packet(sweep_protocol_response_scan_packet_s* v) {
  uint64_t checksum = 0;
  checksum += v->sync_error;
  checksum += v->angle & 0xff00;
  checksum += v->angle & 0x00ff;
  checksum += v->distance & 0xff00;
  checksum += v->distance & 0x00ff;
  checksum += v->signal_strength;
  return checksum % 255;
}

inline float sweep_protocol_u16_to_f32(uint16_t v) { return ((float)(v >> 4u)) + (v & 15u) / 16.0f; }

#endif
