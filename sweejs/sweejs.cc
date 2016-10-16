#include <stdexcept>

#include "sweejs.h"

// Wrapper Object

struct SweepError final : std::runtime_error {
  using Base = std::runtime_error;

  SweepError(const char *what) : Base{what} {}
};

Sweep::Sweep() {
  ::sweep_error_s error = nullptr;
  device = ::sweep_device_construct_simple(&error);

  if (error) {
    throw SweepError{"device construction failed"};
  }
}

Sweep::Sweep(const char *port, int32_t baudrate, int32_t timeout) {
  ::sweep_error_s error = nullptr;
  device = ::sweep_device_construct(port, baudrate, timeout, &error);

  if (error) {
    throw SweepError{"device construction failed"};
  }
}

Sweep::~Sweep() { ::sweep_device_destruct(device); }

// NAN Bindings

NAN_MODULE_INIT(Sweep::Init) {
  const auto whoami = Nan::New("Sweep").ToLocalChecked();

  auto fnTp = Nan::New<v8::FunctionTemplate>(New);
  fnTp->SetClassName(whoami);
  fnTp->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(fnTp, "startScanning", startScanning);
  SetPrototypeMethod(fnTp, "stopScanning", startScanning);
  SetPrototypeMethod(fnTp, "getMotorSpeed", getMotorSpeed);
  SetPrototypeMethod(fnTp, "setMotorSpeed", setMotorSpeed);
  SetPrototypeMethod(fnTp, "getSampleRate", getSampleRate);
  SetPrototypeMethod(fnTp, "reset", reset);

  const auto fn = Nan::GetFunction(fnTp).ToLocalChecked();
  constructor().Reset(fn);

  Nan::Set(target, whoami, fn);
}

NAN_METHOD(Sweep::New) {
  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No types expected");
  }

  if (info.IsConstructCall()) {
    auto *const self = new Sweep;
    self->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    auto init = Nan::New(constructor());
    info.GetReturnValue().Set(init->NewInstance());
  }
}

NAN_METHOD(Sweep::startScanning) {
  auto *const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  ::sweep_error_s error = nullptr;
  ::sweep_device_start_scanning(self->device, &error);

  if (error) {
    Nan::ThrowError(::sweep_error_message(error));
    ::sweep_error_destruct(error);
  }
}

NAN_METHOD(Sweep::stopScanning) {
  auto *const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  ::sweep_error_s error = nullptr;
  ::sweep_device_stop_scanning(self->device, &error);

  if (error) {
    Nan::ThrowError(::sweep_error_message(error));
    ::sweep_error_destruct(error);
  }
}

NAN_METHOD(Sweep::getMotorSpeed) {
  auto *const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  ::sweep_error_s error = NULL;
  const auto speed = ::sweep_device_get_motor_speed(self->device, &error);

  if (error) {
    Nan::ThrowError(::sweep_error_message(error));
    ::sweep_error_destruct(error);
    return;
  }

  info.GetReturnValue().Set(Nan::New(speed));
}

NAN_METHOD(Sweep::setMotorSpeed) {
  auto *const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 1 && !info[0]->IsNumber()) {
    return Nan::ThrowTypeError("Motor speed in Hz as number expected");
  }

  const auto speed = Nan::To<int32_t>(info[0]).FromJust();

  ::sweep_error_s error = NULL;
  ::sweep_device_set_motor_speed(self->device, speed, &error);

  if (error) {
    Nan::ThrowError(::sweep_error_message(error));
    ::sweep_error_destruct(error);
    return;
  }
}

NAN_METHOD(Sweep::getSampleRate) {
  auto *const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  ::sweep_error_s error = NULL;
  const auto rate = ::sweep_device_get_sample_rate(self->device, &error);

  if (error) {
    Nan::ThrowError(::sweep_error_message(error));
    ::sweep_error_destruct(error);
    return;
  }

  info.GetReturnValue().Set(Nan::New(rate));
}

NAN_METHOD(Sweep::reset) {
  auto *const self = Nan::ObjectWrap::Unwrap<Sweep>(info.Holder());

  if (info.Length() != 0) {
    return Nan::ThrowTypeError("No arguments expected");
  }

  ::sweep_error_s error = NULL;
  ::sweep_device_reset(self->device, &error);

  if (error) {
    Nan::ThrowError(::sweep_error_message(error));
    ::sweep_error_destruct(error);
    return;
  }
}

Nan::Persistent<v8::Function> &Sweep::constructor() {
  static Nan::Persistent<v8::Function> init;
  return init;
}
