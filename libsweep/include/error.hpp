#ifndef SWEEP_ERROR_1E0FA029CE94_HPP
#define SWEEP_ERROR_1E0FA029CE94_HPP

/*
 * Internal base error sub-system errors inherit from.
 * Implementation detail; not exported.
 */

#include <stdexcept>

namespace sweep {
namespace error {

struct error : std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

} // ns errors
} // ns sweep

#endif
