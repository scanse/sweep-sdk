#ifndef SWEEP_6144CCE8BA67_H
#define SWEEP_6144CCE8BA67_H

/*
 * The following provides a high-level interface overview for libsweep.
 *
 * TODO(daniel-j-h)
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ABI Stability */

#if __GNUC__ >= 4
#define SWEEP_API __attribute__((visibility("default")))
#else
#define SWEEP_API
#endif

#define SWEEP_VERSION_MAJOR 0
#define SWEEP_VERSION_MINOR 1
#define SWEEP_VERSION ((SWEEP_VERSION_MAJOR << 16u) | SWEEP_VERSION_MINOR)

SWEEP_API int32_t sweep_get_version(void);
SWEEP_API bool sweep_is_abi_compatible(void);

/* Error handling */

typedef struct sweep_error* sweep_error_t;

SWEEP_API const char* sweep_error_message(sweep_error_t error);
SWEEP_API void sweep_error_destruct(sweep_error_t error);

/* Sweep device */

typedef struct sweep_device* sweep_device_t;

SWEEP_API sweep_device_t sweep_device_construct_simple(sweep_error_t* error);
SWEEP_API sweep_device_t sweep_device_construct(const char* tty, int32_t baud, int32_t timeout, sweep_error_t* error);
SWEEP_API void sweep_device_destruct(sweep_device_t device);

SWEEP_API void sweep_device_start_scanning(sweep_device_t device, sweep_error_t* error);
SWEEP_API void sweep_device_stop_scanning(sweep_device_t device, sweep_error_t* error);

typedef void (*sweep_scan_handler_t)(void* data, int32_t angle, int32_t distance);

SWEEP_API void sweep_device_scan(sweep_device_t device, sweep_scan_handler_t handler, void* data, int32_t timeout,
                                 sweep_error_t* error);

SWEEP_API int32_t sweep_device_get_motor_speed(sweep_device_t device, sweep_error_t* error);
SWEEP_API int32_t sweep_device_get_sample_rate(sweep_device_t device, sweep_error_t* error);
SWEEP_API void sweep_device_set_motor_speed(sweep_device_t device, int32_t hz, sweep_error_t* error);

SWEEP_API void sweep_device_reset(sweep_device_t device, sweep_error_t* error);

#ifdef __cplusplus
}
#endif

#endif // SWEEP_6144CCE8BA67_H
