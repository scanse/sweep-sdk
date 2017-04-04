#include <cstdlib>
#include <cstring>

#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include <sweep/sweep.hpp>
#include <zmq.hpp>

#include "net.pb.h"

static void usage() {
  std::cout << "Usage: example-net /dev/ttyUSB0 [ publisher | subscriber ]\n";
  std::exit(EXIT_SUCCESS);
}

void subscriber() {
  zmq::context_t ctx{/*io_threads=*/1};
  zmq::socket_t sub{ctx, ZMQ_SUB};

  sub.connect("tcp://127.0.0.1:5555");
  sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);

  std::cout << "Subscribing." << std::endl;

  for (;;) {
    zmq::message_t msg;

    if (!sub.recv(&msg))
      continue;

    sweep::proto::scan in;
    in.ParseFromArray(msg.data(), msg.size());

    const auto n = in.angle_size();

    for (auto i = 0; i < n; ++i) {
      std::cout << "Angle: " << in.angle(i)                      //
                << " Distance: " << in.distance(i)               //
                << " Signal strength: " << in.signal_strength(i) //
                << std::endl;
    }
  }
}

void publisher(const std::string& dev) try {
  zmq::context_t ctx{/*io_threads=*/1};
  zmq::socket_t pub{ctx, ZMQ_PUB};

  pub.bind("tcp://127.0.0.1:5555");

  sweep::sweep device{dev.c_str()};
  // Begins data acquisition as soon as motor speed stabilizes
  device.start_scanning();

  std::cout << "Publishing. Each dot is a full 360 degree scan." << std::endl;

  for (;;) {
    const sweep::scan scan = device.get_scan();

    sweep::proto::scan out;

    for (const sweep::sample& sample : scan.samples) {
      out.add_angle(sample.angle);
      out.add_distance(sample.distance);
      out.add_signal_strength(sample.signal_strength);
    }

    auto encoded = out.SerializeAsString();

    zmq::message_t msg{encoded.size()};
    std::memcpy(msg.data(), encoded.data(), encoded.size());

    const auto ok = pub.send(msg);

    if (ok)
      std::cout << "." << std::flush;
  }

  device.stop_scanning();

} catch (const sweep::device_error& e) {
  std::cerr << "Error: " << e.what() << '\n';
}

int main(int argc, char* argv[]) {
  std::vector<std::string> args{argv, argv + argc};

  if (args.size() != 3)
    usage();

  const auto isPublisher = args[2] == "publisher";
  const auto isSubscriber = args[2] == "subscriber";

  if (!isPublisher && !isSubscriber)
    usage();

  GOOGLE_PROTOBUF_VERIFY_VERSION;

  struct AtExit {
    ~AtExit() { ::google::protobuf::ShutdownProtobufLibrary(); }
  } sentry;

  if (isPublisher)
    publisher(args[1]);

  if (isSubscriber)
    subscriber();
}
