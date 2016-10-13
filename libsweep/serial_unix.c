#include "serial.h"

#include <stdlib.h>

typedef struct sweep_serial {
  const char* port;
  int32_t baud;
  int32_t timeout;
} sweep_serial;

sweep_serial_s sweep_serial_construct(const char* port, int32_t baud, int32_t timeout, sweep_error_s* error) {
  sweep_serial_s out = malloc(sizeof(sweep_serial));
  *out = (sweep_serial){.port = port, .baud = baud, .timeout = timeout};
  return out;
}

void sweep_serial_destruct(sweep_serial_s serial) { free(serial); }

int32_t sweep_serial_read(void* to, int32_t len, sweep_error_s* error) { return 0; }
int32_t sweep_serial_write(const void* from, int32_t len, sweep_error_s* error) { return 0; }
void sweep_serial_flush(sweep_serial_s serial, sweep_error_s* error) {}
