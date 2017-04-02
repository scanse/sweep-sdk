#ifndef SWEEP_ERROR_2EADE195E243_H
#define SWEEP_ERROR_2EADE195E243_H

#include <stdexcept>

namespace sweep {
    struct error_base : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}

#endif