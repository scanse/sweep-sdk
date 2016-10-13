#ifndef SWEEP_SERIAL_575F0FB571D1_H
#define SWEEP_SERIAL_575F0FB571D1_H

/*
 * Communication with serial devices.
 * Implementation detail; not exported.
 *
 * TODO(daniel-j-h)
 */

#include "sweep.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct sweep_serial* sweep_serial_s;

sweep_serial_s sweep_serial_construct_simple(sweep_error_s* error);
sweep_serial_s sweep_serial_construct(const char* port, int32_t baud, int32_t timeout, sweep_error_s* error);
void sweep_serial_destruct(sweep_serial_s serial);

int32_t sweep_serial_read(void* to, int32_t len, sweep_error_s* error);
int32_t sweep_serial_write(const void* from, int32_t len, sweep_error_s* error);
void sweep_serial_flush(sweep_serial_s serial, sweep_error_s* error);

#endif
