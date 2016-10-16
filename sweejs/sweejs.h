#ifndef SWEEJS_3153F5A5574F_H
#define SWEEJS_3153F5A5574F_H

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

  static NAN_METHOD(getMotorSpeed);
  static NAN_METHOD(setMotorSpeed);
  static NAN_METHOD(getSampleRate);
  static NAN_METHOD(reset);

  static Nan::Persistent<v8::Function>& constructor();

  // Wrapper Object

  Sweep();
  Sweep(const char* port, int32_t baudrate, int32_t timeout);

  ~Sweep();

  // Non-Copyable
  Sweep(const Sweep&) = delete;
  Sweep& operator=(const Sweep&) = delete;

  // Non-Moveable
  Sweep(Sweep&&) = delete;
  Sweep& operator=(Sweep&&) = delete;

  ::sweep_device_s device;
};

NODE_MODULE(sweejs, Sweep::Init)

#endif
