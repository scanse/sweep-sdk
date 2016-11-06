# Exports:
#  LIBSWEEP_FOUND
#  LIBSWEEP_INCLUDE_DIR
#  LIBSWEEP_LIBRARY
# Hints:
#  LIBSWEEP_LIBRARY_DIR

find_path(LIBSWEEP_INCLUDE_DIR
          sweep/sweep.h sweep/sweep.hpp)

find_library(LIBSWEEP_LIBRARY
             NAMES sweep
             HINTS "${LIBSWEEP_LIBRARY_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sweep DEFAULT_MSG LIBSWEEP_LIBRARY LIBSWEEP_INCLUDE_DIR)
mark_as_advanced(LIBSWEEP_INCLUDE_DIR LIBSWEEP_LIBRARY)
