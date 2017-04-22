#ifndef SWEEPJS_3153F5A5574F_H
#define SWEEPJS_3153F5A5574F_H

#include <memory>

#include <nan.h>

#include <sweep/sweep.h>

class Sweep final : public Nan::ObjectWrap {
public:
  static NAN_MODULE_INIT(Init);

private:
  static NAN_METHOD(New);

  static NAN_METHOD(startScanning);
  static NAN_METHOD(stopScanning);

  static NAN_METHOD(scan);

  static NAN_METHOD(getMotorReady);
  static NAN_METHOD(getMotorSpeed);
  static NAN_METHOD(setMotorSpeed);

  static NAN_METHOD(getSampleRate);
  static NAN_METHOD(setSampleRate);

  static NAN_METHOD(reset);

  static Nan::Persistent<v8::Function>& constructor();

  // Wrapped Object

  Sweep(const char* port);
  Sweep(const char* port, int32_t bitrate);

  // Non-Copyable
  Sweep(const Sweep&) = delete;
  Sweep& operator=(const Sweep&) = delete;

  // Non-Moveable
  Sweep(Sweep&&) = delete;
  Sweep& operator=(Sweep&&) = delete;

  // Ref-counted to keep alive until last in-flight callback is finished
  std::shared_ptr<::sweep_device> device;
};

NODE_MODULE(sweepjs, Sweep::Init)

#endif
