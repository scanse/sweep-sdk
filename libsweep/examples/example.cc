// Make use of the CMake build system or compile manually, e.g. with:
// g++ -std=c++11 example.cc -lsweep

#include <iostream>

#include <sweep/sweep.hpp>

int main() try {
  sweep::sweep device;

  device.start_scanning();

  std::cout << "Motor Speed: " << device.get_motor_speed() << " Hz" << std::endl;
  std::cout << "Sample Rate: " << device.get_sample_rate() << " Hz" << std::endl;

  for (auto n = 0; n < 10; ++n) {
    const sweep::scan scan = device.get_scan();

    for (const sweep::sample& sample : scan.samples) {
      std::cout << "angle " << sample.angle << " distance " << sample.distance << " strength " << sample.signal_strength << "\n";
    }
  }

  device.stop_scanning();

} catch (const sweep::device_error& e) {
  std::cerr << "Error: " << e.what() << std::endl;
}
