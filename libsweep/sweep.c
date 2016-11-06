#include "sweep.h"
#include "protocol.h"
#include "serial.h"
#include "time.h"

#include <stdlib.h>

int32_t sweep_get_version(void) { return SWEEP_VERSION; }
bool sweep_is_abi_compatible(void) { return sweep_get_version() >> 16u == SWEEP_VERSION_MAJOR; }

typedef struct sweep_error {
  const char* what; // always literal, do not free
} sweep_error;

typedef struct sweep_device {
  sweep_serial_device_s serial; // serial port communication
  bool is_scanning;
} sweep_device;

#define SWEEP_MAX_SAMPLES 4096

typedef struct sweep_scan {
  int32_t angle[SWEEP_MAX_SAMPLES];
  int32_t distance[SWEEP_MAX_SAMPLES];
  int32_t signal_strength[SWEEP_MAX_SAMPLES];
  int32_t count;
} sweep_scan;

// Constructor hidden from users
static sweep_error_s sweep_error_construct(const char* what) {
  SWEEP_ASSERT(what);

  sweep_error_s out = malloc(sizeof(sweep_error));
  SWEEP_ASSERT(out && "out of memory during error reporting");

  *out = (sweep_error){.what = what};
  return out;
}

const char* sweep_error_message(sweep_error_s error) {
  SWEEP_ASSERT(error);

  return error->what;
}

void sweep_error_destruct(sweep_error_s error) {
  SWEEP_ASSERT(error);

  free(error);
}

sweep_device_s sweep_device_construct_simple(sweep_error_s* error) {
  SWEEP_ASSERT(error);

  return sweep_device_construct("/dev/ttyUSB0", 115200, error);
}

sweep_device_s sweep_device_construct(const char* port, int32_t bitrate, sweep_error_s* error) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);
  SWEEP_ASSERT(error);

  sweep_serial_error_s serialerror = NULL;
  sweep_serial_device_s serial = sweep_serial_device_construct(port, bitrate, &serialerror);

  if (serialerror) {
    *error = sweep_error_construct(sweep_serial_error_message(serialerror));
    sweep_serial_error_destruct(serialerror);
    return NULL;
  }

  sweep_device_s out = malloc(sizeof(sweep_device));

  if (out == NULL) {
    *error = sweep_error_construct("oom during sweep device creation");
    sweep_serial_device_destruct(serial);
    return NULL;
  }

  *out = (sweep_device){.serial = serial, .is_scanning = true};

  sweep_error_s stoperror = NULL;
  sweep_device_stop_scanning(out, &stoperror);

  if (stoperror) {
    *error = stoperror;
    sweep_device_destruct(out);
    return NULL;
  }

  return out;
}

void sweep_device_destruct(sweep_device_s device) {
  SWEEP_ASSERT(device);

  sweep_serial_device_destruct(device->serial);

  free(device);
}

void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  if (device->is_scanning)
    return;

  sweep_protocol_error_s protocolerror = NULL;
  sweep_protocol_write_command(device->serial, SWEEP_PROTOCOL_DATA_ACQUISITION_START, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send start scanning command");
    sweep_protocol_error_destruct(protocolerror);
    return;
  }

  sweep_protocol_response_header_s response;
  sweep_protocol_read_response_header(device->serial, SWEEP_PROTOCOL_DATA_ACQUISITION_START, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive start scanning command response");
    sweep_protocol_error_destruct(protocolerror);
    return;
  }

  device->is_scanning = true;
}

void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  if (!device->is_scanning)
    return;

  sweep_protocol_error_s protocolerror = NULL;
  sweep_protocol_write_command(device->serial, SWEEP_PROTOCOL_DATA_ACQUISITION_STOP, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send stop scanning command");
    sweep_protocol_error_destruct(protocolerror);
    return;
  }

  sweep_sleep_microseconds(5000);

  sweep_serial_error_s serialerror = NULL;
  sweep_serial_device_flush(device->serial, &serialerror);

  if (serialerror) {
    *error = sweep_error_construct("unable to flush serial device for stopping scanning command");
    sweep_serial_error_destruct(serialerror);
    return;
  }

  sweep_protocol_write_command(device->serial, SWEEP_PROTOCOL_DATA_ACQUISITION_STOP, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send stop scanning command");
    sweep_protocol_error_destruct(protocolerror);
    return;
  }

  sweep_protocol_response_header_s response;
  sweep_protocol_read_response_header(device->serial, SWEEP_PROTOCOL_DATA_ACQUISITION_STOP, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive stop scanning command response");
    sweep_protocol_error_destruct(protocolerror);
    return;
  }

  device->is_scanning = false;
}

sweep_scan_s sweep_device_get_scan(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  sweep_protocol_error_s protocolerror = NULL;

  sweep_protocol_response_scan_packet_s responses[SWEEP_MAX_SAMPLES];

  int32_t first = 0;
  int32_t last = SWEEP_MAX_SAMPLES;

  for (int32_t received = 0; received < SWEEP_MAX_SAMPLES; ++received) {
    sweep_protocol_read_response_scan(device->serial, &responses[received], &protocolerror);

    if (protocolerror) {
      *error = sweep_error_construct("unable to receive sweep scan response");
      sweep_protocol_error_destruct(protocolerror);
      return NULL;
    }

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

  sweep_scan_s out = malloc(sizeof(sweep_scan));

  if (out == NULL) {
    *error = sweep_error_construct("oom during sweep scan creation");
    return NULL;
  }

  SWEEP_ASSERT(last - first >= 0);
  SWEEP_ASSERT(last - first < SWEEP_MAX_SAMPLES);

  out->count = last - first;

  for (int32_t it = 0; it < last - first; ++it) {
    out->angle[it] = sweep_protocol_u16_to_f32(responses[first + it].angle) * 1000.f;
    out->distance[it] = responses[first + it].distance;
    out->signal_strength[it] = (responses[first + it].signal_strength / 256.f) * 100.f;
  }

  return out;
}

int32_t sweep_scan_get_number_of_samples(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(scan->count >= 0);

  return scan->count;
}

int32_t sweep_scan_get_angle(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->angle[sample];
}

int32_t sweep_scan_get_distance(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->distance[sample];
}

int32_t sweep_scan_get_signal_strength(sweep_scan_s scan, int32_t sample) {
  SWEEP_ASSERT(scan);
  SWEEP_ASSERT(sample >= 0 && sample < scan->count && "sample index out of bounds");

  return scan->signal_strength[sample];
}

void sweep_scan_destruct(sweep_scan_s scan) {
  SWEEP_ASSERT(scan);

  free(scan);
}

int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  sweep_protocol_error_s protocolerror = NULL;

  sweep_protocol_write_command(device->serial, SWEEP_PROTOCOL_MOTOR_INFORMATION, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send motor speed command");
    sweep_protocol_error_destruct(protocolerror);
    return 0;
  }

  sweep_protocol_response_info_motor_s response;
  sweep_protocol_read_response_info_motor(device->serial, SWEEP_PROTOCOL_MOTOR_INFORMATION, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive motor speed command response");
    sweep_protocol_error_destruct(protocolerror);
    return 0;
  }

  int32_t speed = sweep_protocol_ascii_bytes_to_speed(&response.motor_speed[0], &response.motor_speed[1]);

  return speed;
}

void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(hz >= 0 && hz <= 10);
  SWEEP_ASSERT(error);

  char args[2] = {0};
  sweep_protocol_speed_to_ascii_bytes(hz, &args[0], &args[1]);

  sweep_protocol_error_s protocolerror = NULL;

  sweep_protocol_write_command_with_arguments(device->serial, SWEEP_PROTOCOL_MOTOR_SPEED_ADJUST, &args, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send motor speed command");
    sweep_protocol_error_destruct(protocolerror);
    return;
  }

  sweep_protocol_response_param_s response;
  sweep_protocol_read_response_param(device->serial, SWEEP_PROTOCOL_MOTOR_SPEED_ADJUST, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive motor speed command response");
    sweep_protocol_error_destruct(protocolerror);
    return;
  }
}

int32_t sweep_device_get_sample_rate(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  // TODO
  return 1;
}

void sweep_device_reset(sweep_device_s device, sweep_error_s* error) {
  SWEEP_ASSERT(device);
  SWEEP_ASSERT(error);

  sweep_protocol_error_s protocolerror = NULL;

  sweep_protocol_write_command(device->serial, SWEEP_PROTOCOL_RESET_DEVICE, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to send device reset command");
    sweep_protocol_error_destruct(protocolerror);
    return;
  }

  sweep_protocol_response_header_s response;
  sweep_protocol_read_response_header(device->serial, SWEEP_PROTOCOL_RESET_DEVICE, &response, &protocolerror);

  if (protocolerror) {
    *error = sweep_error_construct("unable to receive device reset command response");
    sweep_protocol_error_destruct(protocolerror);
    return;
  }
}
