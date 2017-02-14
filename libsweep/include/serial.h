#ifndef SWEEP_SERIAL_575F0FB571D1_H
#define SWEEP_SERIAL_575F0FB571D1_H

/*
 * Communication with serial devices.
 * Implementation detail; not exported.
 */

#include "sweep.h"

#include <stdint.h>

namespace sweep {
namespace serial {

typedef struct device* device_s;
typedef struct error* error_s;

const char* error_message(error_s error);
void error_destruct(error_s error);

device_s device_construct(const char* port, int32_t bitrate, error_s* error);
void device_destruct(device_s serial);

void device_read(device_s serial, void* to, int32_t len, error_s* error);
void device_write(device_s serial, const void* from, int32_t len, error_s* error);
void device_flush(device_s serial, error_s* error);

} // ns serial
} // ns sweep

#endif
