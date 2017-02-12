#include <stdio.h>
#include <stdlib.h>

#include "protocol.h"

namespace sweep {
namespace protocol {

const uint8_t DATA_ACQUISITION_START[2] = {'D', 'S'};
const uint8_t DATA_ACQUISITION_STOP[2] = {'D', 'X'};
const uint8_t MOTOR_SPEED_ADJUST[2] = {'M', 'S'};
const uint8_t MOTOR_INFORMATION[2] = {'M', 'I'};
const uint8_t VERSION_INFORMATION[2] = {'I', 'V'};
const uint8_t DEVICE_INFORMATION[2] = {'I', 'D'};
const uint8_t RESET_DEVICE[2] = {'R', 'R'};

static uint8_t checksum_response_header(const response_header_s& v) {
  return ((v.cmdStatusByte1 + v.cmdStatusByte2) & 0x3F) + 0x30;
}

static uint8_t checksum_response_param(const response_param_s& v) {
  return ((v.cmdStatusByte1 + v.cmdStatusByte2) & 0x3F) + 0x30;
}

static uint8_t checksum_response_scan_packet(const response_scan_packet_s& v) {
  uint64_t checksum = 0;
  checksum += v.sync_error;
  checksum += v.angle & 0xff00;
  checksum += v.angle & 0x00ff;
  checksum += v.distance & 0xff00;
  checksum += v.distance & 0x00ff;
  checksum += v.signal_strength;
  return checksum % 255;
}

void write_command(serial::Device& serial, const uint8_t cmd[2]) {
  SWEEP_ASSERT(cmd);

  cmd_packet_s packet;
  packet.cmdByte1 = cmd[0];
  packet.cmdByte2 = cmd[1];
  packet.cmdParamTerm = '\n';

  serial.write(&packet, sizeof(cmd_packet_s));
}

void write_command_with_arguments(serial::Device& serial, const uint8_t cmd[2], const uint8_t arg[2]) {
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(arg);

  cmd_param_packet_s packet;
  packet.cmdByte1 = cmd[0];
  packet.cmdByte2 = cmd[1];
  packet.cmdParamByte1 = arg[0];
  packet.cmdParamByte2 = arg[1];
  packet.cmdParamTerm = '\n';

  serial.write(&packet, sizeof(cmd_param_packet_s));
}

response_header_s read_response_header(serial::Device& serial, const uint8_t cmd[2]) {
  SWEEP_ASSERT(cmd);

  response_header_s header;
  serial.read(&header, sizeof(header));

  if (header.cmdSum != checksum_response_header(header)) {
    throw Error{"invalid response header checksum"};
  }

  if (header.cmdByte1 != cmd[0] || header.cmdByte2 != cmd[1]) {
    throw Error{"invalid header response commands"};
  }
  return header;
}

response_param_s read_response_param(serial::Device& serial, const uint8_t cmd[2]) {
  SWEEP_ASSERT(cmd);

  response_param_s param;
  serial.read(&param, sizeof(param));

  if (param.cmdSum != checksum_response_param(param)) {
    throw Error{"invalid response param header checksum"};
  }

  if (param.cmdByte1 != cmd[0] || param.cmdByte2 != cmd[1]) {
    throw Error{"invalid param response commands"};
  }
  return param;
}

void read_response_scan(serial::Device& serial, response_scan_packet_s* scan) {
  SWEEP_ASSERT(scan);

  serial.read(scan, sizeof(*scan));

  if (scan->checksum != checksum_response_scan_packet(*scan)) {
    throw Error{"invalid scan response commands"};
  }
}

response_info_motor_s read_response_info_motor(serial::Device& serial, const uint8_t cmd[2]) {
  SWEEP_ASSERT(cmd);
  response_info_motor_s info;

  serial.read(&info, sizeof(info));

  if (info.cmdByte1 != cmd[0] || info.cmdByte2 != cmd[1]) {
    throw Error{"invalid motor info response commands"};
  }
  return info;
}

} // ns protocol
} // ns sweep
