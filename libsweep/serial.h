#ifndef SWEEP_SERIAL_575F0FB571D1_H
#define SWEEP_SERIAL_575F0FB571D1_H

/*
 * Communication with serial devices.
 * Implementation detail; not exported.
 *
 * TODO(daniel-j-h)
 */

#include "sweep.h"

#include <stdint.h>

typedef struct sweep_serial_device* sweep_serial_device_s;
typedef struct sweep_serial_error* sweep_serial_error_s;

const char* sweep_serial_error_message(sweep_serial_error_s error);
void sweep_serial_error_destruct(sweep_serial_error_s error);

sweep_serial_device_s sweep_serial_device_construct(const char* port, int32_t baudrate, int32_t timeout,
                                                    sweep_serial_error_s* error);
void sweep_serial_device_destruct(sweep_serial_device_s serial);

void sweep_serial_device_read(sweep_serial_device_s serial, void* to, int32_t len, sweep_serial_error_s* error);
void sweep_serial_device_write(sweep_serial_device_s serial, const void* from, int32_t len, sweep_serial_error_s* error);
void sweep_serial_device_flush(sweep_serial_device_s serial, sweep_serial_error_s* error);

#endif
