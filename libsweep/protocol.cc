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

typedef struct error {
  const char* what; // always literal, do not deallocate
} error;

// Constructor hidden from users
static error_s error_construct(const char* what) {
  SWEEP_ASSERT(what);

  auto out = new error{what};
  return out;
}

const char* error_message(error_s error) {
  SWEEP_ASSERT(error);

  return error->what;
}

void error_destruct(error_s error) {
  SWEEP_ASSERT(error);

  delete error;
}

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

void write_command(serial::device_s serial, const uint8_t cmd[2], error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(error);

  cmd_packet_s packet;
  packet.cmdByte1 = cmd[0];
  packet.cmdByte2 = cmd[1];
  packet.cmdParamTerm = '\n';

  serial::error_s serialerror = nullptr;

  serial::device_write(serial, &packet, sizeof(cmd_packet_s), &serialerror);

  if (serialerror) {
    *error = error_construct("unable to write command");
    serial::error_destruct(serialerror);
    return;
  }
}

void write_command_with_arguments(serial::device_s serial, const uint8_t cmd[2], const uint8_t arg[2], error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(arg);
  SWEEP_ASSERT(error);

  cmd_param_packet_s packet;
  packet.cmdByte1 = cmd[0];
  packet.cmdByte2 = cmd[1];
  packet.cmdParamByte1 = arg[0];
  packet.cmdParamByte2 = arg[1];
  packet.cmdParamTerm = '\n';

  serial::error_s serialerror = nullptr;

  serial::device_write(serial, &packet, sizeof(cmd_param_packet_s), &serialerror);

  if (serialerror) {
    *error = error_construct("unable to write command with arguments");
    serial::error_destruct(serialerror);
    return;
  }
}

void read_response_header(serial::device_s serial, const uint8_t cmd[2], response_header_s* header, error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(header);
  SWEEP_ASSERT(error);

  const uint8_t* cmdBytes = (const uint8_t*)cmd;

  serial::error_s serialerror = nullptr;

  serial::device_read(serial, header, sizeof(response_header_s), &serialerror);

  if (serialerror) {
    *error = error_construct("unable to read response header");
    serial::error_destruct(serialerror);
    return;
  }

  uint8_t checksum = checksum_response_header(header);

  if (checksum != header->cmdSum) {
    *error = error_construct("invalid response header checksum");
    return;
  }

  bool ok = header->cmdByte1 == cmdBytes[0] && header->cmdByte2 == cmdBytes[1];

  if (!ok) {
    *error = error_construct("invalid header response commands");
    return;
  }
}

void read_response_param(serial::device_s serial, const uint8_t cmd[2], response_param_s* param, error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(param);
  SWEEP_ASSERT(error);

  const uint8_t* cmdBytes = (const uint8_t*)cmd;

  serial::error_s serialerror = nullptr;

  serial::device_read(serial, param, sizeof(response_param_s), &serialerror);

  if (serialerror) {
    *error = error_construct("unable to read response param header");
    serial::error_destruct(serialerror);
    return;
  }

  uint8_t checksum = checksum_response_param(param);

  if (checksum != param->cmdSum) {
    *error = error_construct("invalid response param header checksum");
    return;
  }

  bool ok = param->cmdByte1 == cmdBytes[0] && param->cmdByte2 == cmdBytes[1];

  if (!ok) {
    *error = error_construct("invalid param response commands");
    return;
  }
}

void read_response_scan(serial::device_s serial, response_scan_packet_s* scan, error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(error);

  serial::error_s serialerror = nullptr;

  serial::device_read(serial, scan, sizeof(response_scan_packet_s), &serialerror);

  if (serialerror) {
    *error = error_construct("invalid response scan packet checksum");
    serial::error_destruct(serialerror);
    return;
  }

  uint8_t checksum = checksum_response_scan_packet(scan);

  if (checksum != scan->checksum) {
    *error = error_construct("invalid scan response commands");
    return;
  }
}

void read_response_info_motor(serial::device_s serial, const uint8_t cmd[2], response_info_motor_s* info, error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(info);
  SWEEP_ASSERT(error);

  const uint8_t* cmdBytes = (const uint8_t*)cmd;

  serial::error_s serialerror = nullptr;

  serial::device_read(serial, info, sizeof(response_info_motor_s), &serialerror);

  if (serialerror) {
    *error = error_construct("unable to read response motor info");
    serial::error_destruct(serialerror);
    return;
  }

  bool ok = info->cmdByte1 == cmdBytes[0] && info->cmdByte2 == cmdBytes[1];

  if (!ok) {
    *error = error_construct("invalid motor info response commands");
    return;
  }
}

} // ns protocol
} // ns sweep
