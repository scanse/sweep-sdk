#ifndef SWEEP_SERIAL_575F0FB571D1_HPP
#define SWEEP_SERIAL_575F0FB571D1_HPP

/*
 * Communication with serial devices.
 * Implementation detail; not exported.
 */

#include "error.hpp"

#include "sweep.h"

#include <stdint.h>

namespace sweep {
namespace serial {

struct error : sweep::error::error {
  using base = sweep::error::error;
  using base::base;
};

typedef struct device* device_s;

device_s device_construct(const char* port, int32_t bitrate);
void device_destruct(device_s serial);

void device_read(device_s serial, void* to, int32_t len);
void device_write(device_s serial, const void* from, int32_t len);
void device_flush(device_s serial);

} // ns serial
} // ns sweep

#endif
