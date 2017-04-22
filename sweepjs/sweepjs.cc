#include <stdexcept>
#include <utility>

#include "sweepjs.h"

struct SweepError final : std::runtime_error {
  using Base = std::runtime_error;
  using Base::Base;
};

// Translates error to throwing an exception
struct ErrorToException {
  operator ::sweep_error_s*() { return &error; }

  ~ErrorToException() noexcept(false) {
    if (error) {
      SweepError e{::sweep_error_message(error)};
      ::sweep_error_destruct(error);
      throw e;
    }
  }

  ::sweep_error_s error = nullptr;
};

// Translates error to setting exception state in v8
struct ErrorToNanException {
  operator ::sweep_error_s*() { return &error; }

  ~ErrorToNanException() {
    if (error) {
      auto* what = ::sweep_error_message(error);
      ::sweep_error_destruct(error);
      Nan::ThrowError(what);
    }
  }

  ::sweep_error_s error = nullptr;
};

Sweep::Sweep(const char* port) {
  auto devptr = ::sweep_device_construct_simple(port, ErrorToException{});
  auto defer = [](::sweep_device_s dev) { ::sweep_device_destruct(dev); };

  device = {devptr, defer};
}

Sweep::Sweep(const char* port, int32_t bitrate) {
  auto devptr = ::sweep_device_construct(port, bitrate, ErrorToException{});
  auto defer = [](::sweep_device_s dev) { ::sweep_device_destruct(dev); };

  device = {devptr, defer};
}

NAN_MODULE_INIT(Sweep::Init) {
  const auto whoami = Nan::New("Sweep").ToLocalChecked();

  auto fnTp = Nan::New<v8::FunctionTemplate>(New);
  fnTp->SetClassName(whoami);
  fnTp->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(fnTp, "startScanning", startScanning);
  SetPrototypeMethod(fnTp, "stopScanning", stopScanning);
  SetPrototypeMethod(fnTp, "scan", scan);
  SetPrototypeMethod(fnTp, "getMotorReady", getMotorReady);
  SetPrototypeMethod(fnTp, "getMotorSpeed", getMotorSpeed);
  SetPrototypeMethod(fnTp, "setMotorSpeed", setMotorSpeed);
  SetPrototypeMethod(fnTp, "getSampleRate", getSampleRate);
  SetPrototypeMethod(fnTp, "setSampleRate", setSampleRate);
  SetPrototypeMethod(fnTp, "reset", reset);

  const auto fn = Nan::GetFunction(fnTp).ToLocalChecked();
  constructor().Reset(fn);

  Nan::Set(target, whoami, fn);
}

NAN_METHOD(Sweep::New) {
  // auto-detect or port, bitrate
  const auto simple = info.Length() == 1 && info[0]->IsString();
  const auto config = info.Length() == 2 && info[0]->IsString() && info[1]->IsNumber();

  if (!simple && !config) {
    return Nan::ThrowTypeError("No arguments for auto-detection or serial port, bitrate expected");
  }

  if (info.IsConstructCall()) {
    Sweep* self = nullptr;

    try {
      const Nan::Utf8String utf8port{info[0]};
      if (!(*utf8port)) {
        return Nan::ThrowError("UTF-8 conversion error for serial port string");
      }
      const auto port = *utf8port;

      if (simple) {
        self = new Sweep(port);
      } else if (config) {
        const auto bitrate = Nan::To<int32_t>(info[1]).FromJust();
        self = new Sweep(port, bitrate);
      } else {
        return Nan::ThrowError("Unable to create device"); // unreachable
      }
    } catch (const SweepError& e) {
      return Nan::ThrowError(e.what());
    }

    self->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    auto init = Nan::New(constructor());
    info.GetReturnValue().Set(init->NewInstance());
  }
}

NAN_METHOD(Sweep::startScanning) {
  auto* const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  ::sweep_device_start_scanning(self->device.get(), ErrorToNanException{});
}

NAN_METHOD(Sweep::stopScanning) {
  auto* const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  ::sweep_device_stop_scanning(self->device.get(), ErrorToNanException{});
}

class AsyncScanWorker final : public Nan::AsyncWorker {
public:
  AsyncScanWorker(Nan::Callback* callback, std::shared_ptr<::sweep_device> device)
      : Nan::AsyncWorker(callback), device{std::move(device)} {}

  ~AsyncScanWorker() {}

  // Executed inside the worker-thread.
  void Execute() {
    // Note: do not throw here (ErrorTo*) - Nan::AsyncWorker interface provides special SetErrorMessage
    ::sweep_error_s error = nullptr;
    scan = ::sweep_device_get_scan(device.get(), &error);

    if (error) {
      SetErrorMessage(::sweep_error_message(error));
      ::sweep_error_destruct(error);
    }
  }

  // Executed when the async work is complete
  void HandleOKCallback() {
    Nan::HandleScope scope;

    auto n = ::sweep_scan_get_number_of_samples(scan);
    auto samples = Nan::New<v8::Array>(n);

    for (int32_t i = 0; i < n; ++i) {
      const auto angle = Nan::New<v8::Number>(::sweep_scan_get_angle(scan, i));
      const auto distance = Nan::New<v8::Number>(::sweep_scan_get_distance(scan, i));
      const auto signal = Nan::New<v8::Number>(::sweep_scan_get_signal_strength(scan, i));

      const auto anglekey = Nan::New<v8::String>("angle").ToLocalChecked();
      const auto distancekey = Nan::New<v8::String>("distance").ToLocalChecked();
      const auto signalkey = Nan::New<v8::String>("signal").ToLocalChecked();

      // sample = {'angle': 360, 'distance': 20, 'signal': 1}
      const auto sample = Nan::New<v8::Object>();
      Nan::Set(sample, anglekey, angle).FromJust();
      Nan::Set(sample, distancekey, distance).FromJust();
      Nan::Set(sample, signalkey, signal).FromJust();

      Nan::Set(samples, i, sample).FromJust();
    }

    const constexpr auto argc = 2u;
    v8::Local<v8::Value> argv[argc] = {Nan::Null(), samples};

    callback->Call(argc, argv);
    ;
  }

private:
  std::shared_ptr<::sweep_device> device;
  ::sweep_scan_s scan;
};

NAN_METHOD(Sweep::scan) {
  auto* const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 1 || !info[0]->IsFunction()) {
    return Nan::ThrowTypeError("Callback expected");
  }

  auto* callback = new Nan::Callback(info[0].As<v8::Function>());
  Nan::AsyncQueueWorker(new AsyncScanWorker(callback, self->device));
}

NAN_METHOD(Sweep::getMotorReady) {
  auto* const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  const auto ready = ::sweep_device_get_motor_ready(self->device.get(), ErrorToNanException{});

  info.GetReturnValue().Set(Nan::New(ready));
}

NAN_METHOD(Sweep::getMotorSpeed) {
  auto* const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  const auto speed = ::sweep_device_get_motor_speed(self->device.get(), ErrorToNanException{});

  info.GetReturnValue().Set(Nan::New(speed));
}

NAN_METHOD(Sweep::setMotorSpeed) {
  auto* const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 1 && !info[0]->IsNumber()) {
    return Nan::ThrowTypeError("Motor speed in Hz as number expected");
  }

  const auto speed = Nan::To<int32_t>(info[0]).FromJust();

  ::sweep_device_set_motor_speed(self->device.get(), speed, ErrorToNanException{});
}

NAN_METHOD(Sweep::getSampleRate) {
  auto* const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  const auto rate = ::sweep_device_get_sample_rate(self->device.get(), ErrorToNanException{});

  info.GetReturnValue().Set(Nan::New(rate));
}

NAN_METHOD(Sweep::setSampleRate) {
  auto* const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 1 && !info[0]->IsNumber()) {
    return Nan::ThrowTypeError("Sample rate in Hz as number expected");
  }

  const auto rate = Nan::To<int32_t>(info[0]).FromJust();

  ::sweep_device_set_sample_rate(self->device.get(), rate, ErrorToNanException{});
}

NAN_METHOD(Sweep::reset) {
  auto* const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  ::sweep_device_reset(self->device.get(), ErrorToNanException{});
}

Nan::Persistent<v8::Function>& Sweep::constructor() {
  static Nan::Persistent<v8::Function> init;
  return init;
}
