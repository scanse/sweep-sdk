#ifndef SWEEP_6144CCE8BA67_H
#define SWEEP_6144CCE8BA67_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __GNUC__ >= 4
#define SWEEP_API __attribute__((visibility("default")))
#define SWEEP_PACKED __attribute__((packed))
#else
#error "Only Clang and GCC supported at the moment, please open a ticket"
#define SWEEP_API
#define SWEEP_PACKED
#endif

#define SWEEP_VERSION_MAJOR 0
#define SWEEP_VERSION_MINOR 1
#define SWEEP_VERSION ((SWEEP_VERSION_MAJOR << 16u) | SWEEP_VERSION_MINOR)

#ifndef SWEEP_ASSERT
#include <assert.h>
#define SWEEP_ASSERT(x) assert(x)
#endif

SWEEP_API int32_t sweep_get_version(void);
SWEEP_API bool sweep_is_abi_compatible(void);

typedef struct sweep_error* sweep_error_s;
typedef struct sweep_device* sweep_device_s;
typedef struct sweep_scan* sweep_scan_s;

SWEEP_API const char* sweep_error_message(sweep_error_s error);
SWEEP_API void sweep_error_destruct(sweep_error_s error);

SWEEP_API sweep_device_s sweep_device_construct_simple(const char* port, sweep_error_s* error);
SWEEP_API sweep_device_s sweep_device_construct(const char* port, int32_t bitrate, sweep_error_s* error);
SWEEP_API void sweep_device_destruct(sweep_device_s device);

SWEEP_API void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error);
SWEEP_API void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error);

SWEEP_API sweep_scan_s sweep_device_get_scan(sweep_device_s device, sweep_error_s* error);
SWEEP_API void sweep_scan_destruct(sweep_scan_s scan);

SWEEP_API int32_t sweep_scan_get_number_of_samples(sweep_scan_s scan);
SWEEP_API int32_t sweep_scan_get_angle(sweep_scan_s scan, int32_t sample);
SWEEP_API int32_t sweep_scan_get_distance(sweep_scan_s scan, int32_t sample);
SWEEP_API int32_t sweep_scan_get_signal_strength(sweep_scan_s scan, int32_t sample);

SWEEP_API int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error);
SWEEP_API void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error);

SWEEP_API void sweep_device_reset(sweep_device_s device, sweep_error_s* error);

#ifdef __cplusplus
}
#endif

#endif // SWEEP_6144CCE8BA67_H
