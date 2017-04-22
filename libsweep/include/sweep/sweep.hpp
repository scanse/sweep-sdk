#ifndef SWEEP_DC649F4E94D3_HPP
#define SWEEP_DC649F4E94D3_HPP

/*
 * C++ Wrapper around the low-level primitives.
 * Automatically handles resource management.
 *
 * sweep::sweep  - device to interact with
 * sweep::scan   - a full scan returned by the device
 * sweep::sample - a single sample in a full scan
 *
 * On error sweep::device_error gets thrown.
 */

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include <sweep/sweep.h>

namespace sweep {

// Error reporting

struct device_error final : std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

// Interface

struct sample {
  const std::int32_t angle;
  const std::int32_t distance;
  const std::int32_t signal_strength;
};

struct scan {
  std::vector<sample> samples;
};

class sweep {
public:
  sweep(const char* port);
  sweep(const char* port, std::int32_t bitrate);
  void start_scanning();
  void stop_scanning();
  bool get_motor_ready();
  std::int32_t get_motor_speed();
  void set_motor_speed(std::int32_t speed);
  std::int32_t get_sample_rate();
  void set_sample_rate(std::int32_t speed);
  scan get_scan();
  void reset();

private:
  std::unique_ptr<::sweep_device, decltype(&::sweep_device_destruct)> device;
};

// Implementation

namespace detail {
struct error_to_exception {
  operator ::sweep_error_s*() { return &error; }

  ~error_to_exception() noexcept(false) {
    if (error) {
      device_error e{::sweep_error_message(error)};
      ::sweep_error_destruct(error);
      throw e;
    }
  }

  ::sweep_error_s error = nullptr;
};
}

sweep::sweep(const char* port)
    : device{::sweep_device_construct_simple(port, detail::error_to_exception{}), &::sweep_device_destruct} {}

sweep::sweep(const char* port, std::int32_t bitrate)
    : device{::sweep_device_construct(port, bitrate, detail::error_to_exception{}), &::sweep_device_destruct} {}

void sweep::start_scanning() { ::sweep_device_start_scanning(device.get(), detail::error_to_exception{}); }

void sweep::stop_scanning() { ::sweep_device_stop_scanning(device.get(), detail::error_to_exception{}); }

bool sweep::get_motor_ready() { return ::sweep_device_get_motor_ready(device.get(), detail::error_to_exception{}); }

std::int32_t sweep::get_motor_speed() { return ::sweep_device_get_motor_speed(device.get(), detail::error_to_exception{}); }

void sweep::set_motor_speed(std::int32_t speed) {
  ::sweep_device_set_motor_speed(device.get(), speed, detail::error_to_exception{});
}

std::int32_t sweep::get_sample_rate() { return ::sweep_device_get_sample_rate(device.get(), detail::error_to_exception{}); }

void sweep::set_sample_rate(std::int32_t rate) {
  ::sweep_device_set_sample_rate(device.get(), rate, detail::error_to_exception{});
}

scan sweep::get_scan() {
  using scan_owner = std::unique_ptr<::sweep_scan, decltype(&::sweep_scan_destruct)>;

  scan_owner releasing_scan{::sweep_device_get_scan(device.get(), detail::error_to_exception{}), &::sweep_scan_destruct};

  auto num_samples = ::sweep_scan_get_number_of_samples(releasing_scan.get());

  scan result;
  result.samples.reserve(num_samples);

  for (std::int32_t n = 0; n < num_samples; ++n) {
    auto angle = ::sweep_scan_get_angle(releasing_scan.get(), n);
    auto distance = ::sweep_scan_get_distance(releasing_scan.get(), n);
    auto signal = ::sweep_scan_get_signal_strength(releasing_scan.get(), n);

    result.samples.push_back(sample{angle, distance, signal});
  }

  return result;
}

void sweep::reset() { ::sweep_device_reset(device.get(), detail::error_to_exception{}); }

} // ns

#endif
