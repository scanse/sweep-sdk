#ifndef SWEEP_DC649F4E94D3_HPP
#define SWEEP_DC649F4E94D3_HPP

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "sweep.h"

namespace sweep {

// Error reporting
struct device_error final : std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

// Some implementation details
namespace detail {
template <typename T> using non_owning = T;

struct error_to_exception {
  operator non_owning<::sweep_error_s*>() { return &error; }

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

// Interface

struct sample {
  const std::int32_t angle;
  const std::int32_t distance;
};

struct scan {
  std::vector<sample> samples;
};

class sweep {
public:
  sweep();
  sweep(const char* port, std::int32_t baudrate, std::int32_t timeout);

  void start_scanning();
  void stop_scanning();

  std::int32_t get_motor_speed();
  void set_motor_speed(std::int32_t speed);

  std::int32_t get_sample_rate();

  scan get_scan(std::int32_t timeout = 2000);

  void reset();

private:
  std::unique_ptr<::sweep_device, decltype(&::sweep_device_destruct)> device;
};

// Implementation

sweep::sweep() : device{::sweep_device_construct_simple(detail::error_to_exception{}), &::sweep_device_destruct} {}

sweep::sweep(const char* port, std::int32_t baudrate, std::int32_t timeout)
    : device{::sweep_device_construct(port, baudrate, timeout, detail::error_to_exception{}), &::sweep_device_destruct} {}

void sweep::start_scanning() { ::sweep_device_start_scanning(device.get(), detail::error_to_exception{}); }

void sweep::stop_scanning() { ::sweep_device_start_scanning(device.get(), detail::error_to_exception{}); }

std::int32_t sweep::get_motor_speed() { return ::sweep_device_get_motor_speed(device.get(), detail::error_to_exception{}); }

void sweep::set_motor_speed(std::int32_t speed) {
  ::sweep_device_set_motor_speed(device.get(), speed, detail::error_to_exception{});
}

std::int32_t sweep::get_sample_rate() { return ::sweep_device_get_sample_rate(device.get(), detail::error_to_exception{}); }

scan sweep::get_scan(std::int32_t timeout) {
  using scan_owner = std::unique_ptr<::sweep_scan, decltype(&::sweep_scan_destruct)>;

  scan_owner releasing_scan{::sweep_device_get_scan(device.get(), timeout, detail::error_to_exception{}), &::sweep_scan_destruct};

  auto num_samples = ::sweep_scan_get_number_of_samples(releasing_scan.get());

  scan result;
  result.samples.reserve(num_samples);

  for (std::int32_t n = 0; n < num_samples; ++n) {
    auto angle = ::sweep_scan_get_angle(releasing_scan.get(), n);
    auto distance = ::sweep_scan_get_distance(releasing_scan.get(), n);

    result.samples.push_back(sample{angle, distance});
  }

  return result;
};

void sweep::reset() { ::sweep_device_reset(device.get(), detail::error_to_exception{}); }

} // ns

#endif
