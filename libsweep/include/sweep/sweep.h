#ifndef SWEEP_6144CCE8BA67_H
#define SWEEP_6144CCE8BA67_H

#include <stdbool.h>
#include <stdint.h>

#include <sweep/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__ || defined __MINGW32__
// If we are building a dll, set SWEEP_API to export symbols
#ifdef SWEEP_EXPORTS
#ifdef __GNUC__
#define SWEEP_API __attribute__((dllexport))
#else
#define SWEEP_API __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define SWEEP_API __attribute__((dllimport))
#else
#define SWEEP_API __declspec(dllimport)
#endif
#endif
#else
#if __GNUC__ >= 4
#define SWEEP_API __attribute__((visibility("default")))
#else
#define SWEEP_API
#endif
#endif

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

// Blocks until device is ready to start scanning, then starts scanning
SWEEP_API void sweep_device_start_scanning(sweep_device_s device, sweep_error_s* error);
// Stops stream, blocks while leftover stream is flushed, and sends stop once more to validate response
SWEEP_API void sweep_device_stop_scanning(sweep_device_s device, sweep_error_s* error);

// Blocks until the device is ready (calibration complete and motor speed stabilized)
void sweep_device_wait_until_motor_ready(sweep_device_s device, sweep_error_s* error);

// Retrieves a scan from the queue (will block until scan is available)
SWEEP_API sweep_scan_s sweep_device_get_scan(sweep_device_s device, sweep_error_s* error);
// Accumulates scans in the queue  (method to be used by background thread)
void sweep_device_accumulate_scans(sweep_device_s device);

SWEEP_API bool sweep_device_get_motor_ready(sweep_device_s device, sweep_error_s* error);
SWEEP_API int32_t sweep_device_get_motor_speed(sweep_device_s device, sweep_error_s* error);
// Blocks until device is ready to adjust motor speed, then adjusts motor speed
SWEEP_API void sweep_device_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error);

SWEEP_API int32_t sweep_device_get_sample_rate(sweep_device_s device, sweep_error_s* error);
SWEEP_API void sweep_device_set_sample_rate(sweep_device_s device, int32_t hz, sweep_error_s* error);

SWEEP_API int32_t sweep_scan_get_number_of_samples(sweep_scan_s scan);
SWEEP_API int32_t sweep_scan_get_angle(sweep_scan_s scan, int32_t sample);
SWEEP_API int32_t sweep_scan_get_distance(sweep_scan_s scan, int32_t sample);
SWEEP_API int32_t sweep_scan_get_signal_strength(sweep_scan_s scan, int32_t sample);

SWEEP_API void sweep_scan_destruct(sweep_scan_s scan);

SWEEP_API void sweep_device_reset(sweep_device_s device, sweep_error_s* error);

//------- Alternative methods for Low Level development (can error on failure) ------ //
// Not yet part of the public API
// Attempts to start scanning without waiting for motor ready, does NOT start background thread to accumulate scans
void sweep_device_attempt_start_scanning(sweep_device_s device, sweep_error_s* error);
// Read incoming scan directly (not retrieving from the queue)
sweep_scan_s sweep_device_get_scan_direct(sweep_device_s device, sweep_error_s* error);
// Attempts to set motor speed without waiting for motor ready
void sweep_device_attempt_set_motor_speed(sweep_device_s device, int32_t hz, sweep_error_s* error);

#ifdef __cplusplus
}
#endif

#endif // SWEEP_6144CCE8BA67_H
