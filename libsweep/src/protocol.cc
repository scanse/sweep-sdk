#include <stdio.h>
#include <stdlib.h>

#include "protocol.h"
#include "error.hpp"

namespace sweep {
namespace protocol {

struct error : ::sweep::error_base {
  using ::sweep::error_base::error_base;
};

const uint8_t DATA_ACQUISITION_START[2] = {'D', 'S'};
const uint8_t DATA_ACQUISITION_STOP[2] = {'D', 'X'};
const uint8_t MOTOR_SPEED_ADJUST[2] = {'M', 'S'};
const uint8_t MOTOR_INFORMATION[2] = {'M', 'I'};
const uint8_t SAMPLE_RATE_ADJUST[2] = {'L', 'R'};
const uint8_t SAMPLE_RATE_INFORMATION[2] = {'L', 'I'};
const uint8_t VERSION_INFORMATION[2] = {'I', 'V'};
const uint8_t DEVICE_INFORMATION[2] = {'I', 'D'};
const uint8_t RESET_DEVICE[2] = {'R', 'R'};

static uint8_t checksum_response_header(response_header_s* v) {
  SWEEP_ASSERT(v);

  return ((v->cmdStatusByte1 + v->cmdStatusByte2) & 0x3F) + 0x30;
}

static uint8_t checksum_response_param(response_param_s* v) {
  SWEEP_ASSERT(v);

  return ((v->cmdStatusByte1 + v->cmdStatusByte2) & 0x3F) + 0x30;
}

static uint8_t checksum_response_scan_packet(response_scan_packet_s* v) {
  SWEEP_ASSERT(v);

  uint64_t checksum = 0;
  checksum += v->sync_error;
  checksum += v->angle & 0xff00;
  checksum += v->angle & 0x00ff;
  checksum += v->distance & 0xff00;
  checksum += v->distance & 0x00ff;
  checksum += v->signal_strength;
  return checksum % 255;
}

void write_command(serial::device_s serial, const uint8_t cmd[2]) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);

  cmd_packet_s packet;
  packet.cmdByte1 = cmd[0];
  packet.cmdByte2 = cmd[1];
  packet.cmdParamTerm = '\n';

  serial::device_write(serial, &packet, sizeof(cmd_packet_s));
}

void write_command_with_arguments(serial::device_s serial, const uint8_t cmd[2], const uint8_t arg[2]) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(arg);

  cmd_param_packet_s packet;
  packet.cmdByte1 = cmd[0];
  packet.cmdByte2 = cmd[1];
  packet.cmdParamByte1 = arg[0];
  packet.cmdParamByte2 = arg[1];
  packet.cmdParamTerm = '\n';

  serial::device_write(serial, &packet, sizeof(cmd_param_packet_s));
}

void read_response_header(serial::device_s serial, const uint8_t cmd[2], response_header_s* header) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(header);

  serial::device_read(serial, header, sizeof(response_header_s));

  uint8_t checksum = checksum_response_header(header);

  if (checksum != header->cmdSum) {
    throw error{"invalid response header checksum"};
  }

  bool ok = header->cmdByte1 == cmd[0] && header->cmdByte2 == cmd[1];

  if (!ok) {
    throw error{"invalid header response commands"};
  }
}

void read_response_param(serial::device_s serial, const uint8_t cmd[2], response_param_s* param) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(param);

  serial::device_read(serial, param, sizeof(response_param_s));

  uint8_t checksum = checksum_response_param(param);

  if (checksum != param->cmdSum) {
    throw error{"invalid response param header checksum"};
  }

  bool ok = param->cmdByte1 == cmd[0] && param->cmdByte2 == cmd[1];

  if (!ok) {
    throw error{"invalid param response commands"};
  }
}

void read_response_scan(serial::device_s serial, response_scan_packet_s* scan) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(scan);

  serial::device_read(serial, scan, sizeof(response_scan_packet_s));

  uint8_t checksum = checksum_response_scan_packet(scan);

  if (checksum != scan->checksum) {
    throw error{"invalid scan response commands"};
  }
}

void read_response_info_motor(serial::device_s serial, const uint8_t cmd[2], response_info_motor_s* info) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(info);

  serial::device_read(serial, info, sizeof(response_info_motor_s));

  bool ok = info->cmdByte1 == cmd[0] && info->cmdByte2 == cmd[1];

  if (!ok) {
    throw error{"invalid motor info response commands"};
  }
}

void read_response_info_sample_rate(sweep::serial::device_s serial, const uint8_t cmd[2], response_info_sample_rate_s* info) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(info);

  serial::device_read(serial, info, sizeof(response_info_sample_rate_s));

  bool ok = info->cmdByte1 == cmd[0] && info->cmdByte2 == cmd[1];

  if (!ok) {
    throw error{"invalid sample rate info response commands"};
  }
}

} // ns protocol
} // ns sweep
