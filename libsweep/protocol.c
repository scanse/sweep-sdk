#include <stdio.h>
#include <stdlib.h>

#include "protocol.h"

const uint8_t SWEEP_PROTOCOL_DATA_ACQUISITION_START[2] = { 'D', 'S' };
const uint8_t SWEEP_PROTOCOL_DATA_ACQUISITION_STOP[2] = { 'D', 'X' };
const uint8_t SWEEP_PROTOCOL_MOTOR_SPEED_ADJUST[2] = { 'M', 'S' };
const uint8_t SWEEP_PROTOCOL_MOTOR_INFORMATION[2] = { 'M', 'I' };
const uint8_t SWEEP_PROTOCOL_VERSION_INFORMATION[2] = { 'I', 'V' };
const uint8_t SWEEP_PROTOCOL_DEVICE_INFORMATION[2] = { 'I', 'D' };
const uint8_t SWEEP_PROTOCOL_RESET_DEVICE[2] = { 'R', 'R' };

typedef struct sweep_protocol_error {
  const char* what; // always literal, do not free
} sweep_protocol_error;

// Constructor hidden from users
static sweep_protocol_error_s sweep_protocol_error_construct(const char* what) {
  SWEEP_ASSERT(what);

  sweep_protocol_error_s out = (sweep_protocol_error_s)malloc(sizeof(sweep_protocol_error));
  SWEEP_ASSERT(out && "out of memory during error reporting");

  out->what = what;
  return out;
}

const char* sweep_protocol_error_message(sweep_protocol_error_s error) {
  SWEEP_ASSERT(error);

  return error->what;
}

void sweep_protocol_error_destruct(sweep_protocol_error_s error) {
  SWEEP_ASSERT(error);

  free(error);
}

static uint8_t sweep_protocol_checksum_response_header(sweep_protocol_response_header_s* v) {
  SWEEP_ASSERT(v);

  return ((v->cmdStatusByte1 + v->cmdStatusByte2) & 0x3F) + 0x30;
}

static uint8_t sweep_protocol_checksum_response_param(sweep_protocol_response_param_s* v) {
  SWEEP_ASSERT(v);

  return ((v->cmdStatusByte1 + v->cmdStatusByte2) & 0x3F) + 0x30;
}

static uint8_t sweep_protocol_checksum_response_scan_packet(sweep_protocol_response_scan_packet_s* v) {
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

void sweep_protocol_write_command(sweep_serial_device_s serial, const uint8_t cmd[2], sweep_protocol_error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(error);

  sweep_protocol_cmd_packet_s packet;
  packet.cmdByte1 = cmd[0];
  packet.cmdByte2 = cmd[1];
  packet.cmdParamTerm = '\n';

  sweep_serial_error_s serialerror = NULL;

  sweep_serial_device_write(serial, &packet, sizeof(sweep_protocol_cmd_packet_s), &serialerror);

  if (serialerror) {
    *error = sweep_protocol_error_construct("unable to write command");
    sweep_serial_error_destruct(serialerror);
    return;
  }
}

void sweep_protocol_write_command_with_arguments(sweep_serial_device_s serial, const uint8_t cmd[2],
                                                 const uint8_t arg[2], sweep_protocol_error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(arg);
  SWEEP_ASSERT(error);

  sweep_protocol_cmd_param_packet_s packet;
  packet.cmdByte1 = cmd[0];
  packet.cmdByte2 = cmd[1];
  packet.cmdParamByte1 = arg[0];
  packet.cmdParamByte2 = arg[1];
  packet.cmdParamTerm = '\n';

  sweep_serial_error_s serialerror = NULL;

  sweep_serial_device_write(serial, &packet, sizeof(sweep_protocol_cmd_param_packet_s), &serialerror);

  if (serialerror) {
    *error = sweep_protocol_error_construct("unable to write command with arguments");
    sweep_serial_error_destruct(serialerror);
    return;
  }
}

void sweep_protocol_read_response_header(sweep_serial_device_s serial, const uint8_t cmd[2],
                                         sweep_protocol_response_header_s* header, sweep_protocol_error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(header);
  SWEEP_ASSERT(error);

  const uint8_t* cmdBytes = (const uint8_t*)cmd;

  sweep_serial_error_s serialerror = NULL;

  sweep_serial_device_read(serial, header, sizeof(sweep_protocol_response_header_s), &serialerror);

  if (serialerror) {
    *error = sweep_protocol_error_construct("unable to read response header");
    sweep_serial_error_destruct(serialerror);
    return;
  }

  uint8_t checksum = sweep_protocol_checksum_response_header(header);

  if (checksum != header->cmdSum) {
    *error = sweep_protocol_error_construct("invalid response header checksum");
    return;
  }

  bool ok = header->cmdByte1 == cmdBytes[0] && header->cmdByte2 == cmdBytes[1];

  if (!ok) {
    *error = sweep_protocol_error_construct("invalid header response commands");
    return;
  }
}

void sweep_protocol_read_response_param(sweep_serial_device_s serial, const uint8_t cmd[2],
                                        sweep_protocol_response_param_s* param, sweep_protocol_error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(param);
  SWEEP_ASSERT(error);

  const uint8_t* cmdBytes = (const uint8_t*)cmd;

  sweep_serial_error_s serialerror = NULL;

  sweep_serial_device_read(serial, param, sizeof(sweep_protocol_response_param_s), &serialerror);

  if (serialerror) {
    *error = sweep_protocol_error_construct("unable to read response param header");
    sweep_serial_error_destruct(serialerror);
    return;
  }

  uint8_t checksum = sweep_protocol_checksum_response_param(param);

  if (checksum != param->cmdSum) {
    *error = sweep_protocol_error_construct("invalid response param header checksum");
    return;
  }

  bool ok = param->cmdByte1 == cmdBytes[0] && param->cmdByte2 == cmdBytes[1];

  if (!ok) {
    *error = sweep_protocol_error_construct("invalid param response commands");
    return;
  }
}

void sweep_protocol_read_response_scan(sweep_serial_device_s serial, sweep_protocol_response_scan_packet_s* scan,
                                       sweep_protocol_error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(error);

  sweep_serial_error_s serialerror = NULL;

  sweep_serial_device_read(serial, scan, sizeof(sweep_protocol_response_scan_packet_s), &serialerror);

  if (serialerror) {
    *error = sweep_protocol_error_construct("invalid response scan packet checksum");
    sweep_serial_error_destruct(serialerror);
    return;
  }

  uint8_t checksum = sweep_protocol_checksum_response_scan_packet(scan);

  if (checksum != scan->checksum) {
    *error = sweep_protocol_error_construct("invalid scan response commands");
    return;
  }
}

void sweep_protocol_read_response_info_motor(sweep_serial_device_s serial, const uint8_t cmd[2],
                                             sweep_protocol_response_info_motor_s* info, sweep_protocol_error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(cmd);
  SWEEP_ASSERT(info);
  SWEEP_ASSERT(error);

  const uint8_t* cmdBytes = (const uint8_t*)cmd;

  sweep_serial_error_s serialerror = NULL;

  sweep_serial_device_read(serial, info, sizeof(sweep_protocol_response_info_motor_s), &serialerror);

  if (serialerror) {
    *error = sweep_protocol_error_construct("unable to read response motor info");
    sweep_serial_error_destruct(serialerror);
    return;
  }

  bool ok = info->cmdByte1 == cmdBytes[0] && info->cmdByte2 == cmdBytes[1];

  if (!ok) {
    *error = sweep_protocol_error_construct("invalid motor info response commands");
    return;
  }
}
