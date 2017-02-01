// Make use of the CMake build system or compile manually, e.g. with:
// g++ -std=c++11 example.cc -lsweep

#include <iostream>

#include <sweep/sweep.hpp>

int main() try {
  sweep::sweep device;

  device.start_scanning();

  for (auto n = 0; n < 10; ++n) {
    const sweep::scan scan = device.get_scan();

    for (const sweep::sample& sample : scan.samples) {
      std::cout << "Angle: " << sample.angle / 1000.f << ", Distance: " << sample.distance << ", Strength "
                << sample.signal_strength << "\n";
    }
  }

  device.stop_scanning();

} catch (const sweep::device_error& e) {
  std::cerr << "Error: " << e.what() << std::endl;
}
