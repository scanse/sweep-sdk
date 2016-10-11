#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sweep/sweep.h>

static void die(sweep_error_t error) {
  assert(error);
  fprintf(stderr, "Error: %s\n", sweep_error_message(error));
  sweep_error_destruct(error);
  exit(EXIT_FAILURE);
}

static void check(sweep_error_t error) {
  if (error)
    die(error);
}

static void on_scan(void* data, int32_t angle, int32_t distance) {
  (void)data;
  fprintf(stdout, "Angle %" PRId32 ", Distance %" PRId32 "\n", angle, distance);
}

int main() {
  assert(sweep_is_abi_compatible());

  sweep_error_t error = NULL;

  sweep_device_t sweep = sweep_device_construct_simple(&error);
  check(error);

  int32_t speed = sweep_device_get_motor_speed(sweep, &error);
  check(error);

  int32_t rate = sweep_device_get_sample_rate(sweep, &error);
  check(error);

  fprintf(stdout, "Motor Speed: %" PRId32 " Hz, Sample Rate: %" PRId32 " S/s\n", speed, rate);

  sweep_device_start_scanning(sweep, &error);
  check(error);

  int32_t timeout = (1 / speed) * 2;

  for (int32_t num_scans = 0; num_scans < 10; ++num_scans) {
    sweep_device_scan(sweep, on_scan, NULL, timeout, &error);
    check(error);
  }

  sweep_device_destruct(sweep);
  return EXIT_SUCCESS;
}
