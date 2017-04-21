// Make use of the CMake build system or compile manually, e.g. with:
// gcc -std=c99 example.c -lsweep

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sweep/sweep.h>

// Utility functions for error handling: we simply shut down here; you should do a better job
static void die(sweep_error_s error) {
  assert(error);
  fprintf(stderr, "Error: %s\n", sweep_error_message(error));
  sweep_error_destruct(error);
  exit(EXIT_FAILURE);
}

static void check(sweep_error_s error) {
  if (error)
    die(error);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stdout, "Usage: ./example-c device\n");
    return EXIT_FAILURE;
  }

  // Makes sure the installed library is compatible with the interface
  assert(sweep_is_abi_compatible());

  // Grab the port name from the input argument
  const char* port = argv[1];

  // All functions which can potentially fail write into an error object
  sweep_error_s error = NULL;

  // Create a Sweep device from the specified USB serial port; there is a second constructor for advanced usage
  sweep_device_s sweep = sweep_device_construct_simple(port, &error);
  check(error);

  // The Sweep's rotating speed in Hz
  int32_t speed = sweep_device_get_motor_speed(sweep, &error);
  check(error);

  fprintf(stdout, "Motor Speed Setting: %" PRId32 " Hz\n", speed);

  // The Sweep's sample rate in Hz
  int32_t rate = sweep_device_get_sample_rate(sweep, &error);
  check(error);

  fprintf(stdout, "Sample Rate Setting: %" PRId32 " Hz\n", rate);

  // Capture scans
  fprintf(stdout, "Beginning data acquisition as soon as motor speed stabilizes...\n");
  sweep_device_start_scanning(sweep, &error);
  check(error);

  // Let's do 10 full 360 degree scans
  for (int32_t num_scans = 0; num_scans < 10; ++num_scans) {
    // This blocks until a full 360 degree scan is available
    sweep_scan_s scan = sweep_device_get_scan(sweep, &error);
    check(error);

    // For each sample in a full 360 degree scan print angle and distance.
    // In case you're doing expensive work here consider using a decoupled producer / consumer pattern.
    for (int32_t n = 0; n < sweep_scan_get_number_of_samples(scan); ++n) {
      int32_t angle = sweep_scan_get_angle(scan, n);
      int32_t distance = sweep_scan_get_distance(scan, n);
      int32_t signal = sweep_scan_get_signal_strength(scan, n);
      fprintf(stdout, "Angle %" PRId32 ", Distance %" PRId32 ", Signal Strength: %" PRId32 "\n", angle, distance, signal);
    }

    // Cleanup scan response
    sweep_scan_destruct(scan);
  }

  // Stop capturing scans
  sweep_device_stop_scanning(sweep, &error);
  check(error);

  // Shut down and cleanup Sweep device
  sweep_device_destruct(sweep);
  return EXIT_SUCCESS;
}
