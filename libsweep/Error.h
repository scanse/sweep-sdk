#pragma once
#include <stdexcept>

namespace sweep {
struct ErrorBase : std::exception {
  // parameter MUST be a STRING LITTERAL
  template <size_t N> ErrorBase(const char (&msg)[N]) : _msg{&msg[0]} {}
  const char* what() const noexcept override { return _msg; };

private:
  const char* _msg;
};
}
