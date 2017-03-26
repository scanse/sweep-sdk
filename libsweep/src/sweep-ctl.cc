#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include <string>
#include <vector>

#include <sweep/sweep.hpp>

static const auto kMotorSpeedCmd = "motor_speed";
static const auto kSampleRateCmd = "sample_rate";

static void usage() {
  std::fprintf(stderr, "Usage:\n");
  std::fprintf(stderr, "  sweep-ctl dev get (motor_speed|sample_rate)\n");
  std::fprintf(stderr, "  sweep-ctl dev set (motor_speed|sample_rate) <value>\n");
  std::exit(EXIT_FAILURE);
}

int main(int argc, char** argv) try {
  std::vector<std::string> args{argv, argv + argc};

  const auto get = args.size() == 4 && args[2] == "get";
  const auto set = args.size() == 5 && args[2] == "set";

  if (!get && !set)
    usage();

  const auto& dev = args[1];
  const auto& cmd = args[3];

  sweep::sweep device{dev.c_str(), 115200}; // TODO: simple dev ctor

  if (get && cmd == kMotorSpeedCmd) {
    std::printf("%" PRId32 "\n", device.get_motor_speed());
    return EXIT_SUCCESS;
  }

  if (get && cmd == kSampleRateCmd) {
    std::printf("%" PRId32 "\n", device.get_sample_rate());
    return EXIT_SUCCESS;
  }

  if (set && cmd == kMotorSpeedCmd) {
    device.set_motor_speed(std::stoi(args[4]));
    std::printf("%" PRId32 "\n", device.get_motor_speed());
    return EXIT_SUCCESS;
  }

  if (set && cmd == kSampleRateCmd) {
    device.set_sample_rate(std::stoi(args[4]));
    std::printf("%" PRId32 "\n", device.get_sample_rate());
    return EXIT_SUCCESS;
  }

  usage();

} catch (const std::exception& e) {
  std::fprintf(stderr, "Error: %s\n", e.what());
  return EXIT_FAILURE;
}
