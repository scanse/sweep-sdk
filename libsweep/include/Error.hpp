#include <stdexcept>

namespace sweep {
    struct ErrorBase : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}