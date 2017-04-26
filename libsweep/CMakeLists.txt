cmake_minimum_required(VERSION 2.8.11)
project(sweep C CXX)


# Major for breaking ABI changes. Minor for features. Patch for bugfixes.

set(SWEEP_VERSION_MAJOR 1)
set(SWEEP_VERSION_MINOR 1)
set(SWEEP_VERSION_PATCH 1)


option(DUMMY "Build dummy libsweep always returning static point cloud data. No device needed." OFF)


# Platform specific compiler and linker options.

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic -fvisibility=hidden -fPIC -fno-rtti")
  set(libsweep_OS unix)

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic -fvisibility=hidden -fPIC -fno-rtti")
  set(libsweep_OS unix)

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(libsweep_OS win)

  # Turn on the ability to create folders to organize projects (.vcproj)
  # It creates "CMakePredefinedTargets" folder by default and adds CMake
  # defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj to that folder
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # define SWEEP_EXPORTS such that sweep.h will export symbols marked by SWEEP_API
  add_definitions(-DSWEEP_EXPORTS)

else()
  message(FATAL_ERROR "System not yet supported. Please open a ticket.")

endif()


# Generate include/sweep/config.h on the fly and insert version numbers.

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/sweep/config.h)


# C++11 requires us to link in pthreads and equivalents.

find_package(Threads REQUIRED)


# libsweep target.

file(GLOB libsweep_OS_SOURCES src/${libsweep_OS}/*.cc)

if (DUMMY)
  set(libsweep_IMPL_SOURCES src/dummy.cc)
else()
  set(libsweep_IMPL_SOURCES src/sweep.cc)
endif()

set(libsweep_SOURCES ${libsweep_OS_SOURCES} ${libsweep_IMPL_SOURCES} src/protocol.cc)
file(GLOB libsweep_HEADERS include/*.h include/sweep/*.h include/sweep/*.hpp)

add_library(sweep SHARED ${libsweep_SOURCES} ${libsweep_HEADERS})
target_include_directories(sweep PRIVATE include include/sweep ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(sweep ${CMAKE_THREAD_LIBS_INIT})

set_property(TARGET sweep PROPERTY VERSION "${SWEEP_VERSION_MAJOR}.${SWEEP_VERSION_MINOR}.${SWEEP_VERSION_PATCH}")
set_property(TARGET sweep PROPERTY SOVERSION "${SWEEP_VERSION_MAJOR}")

install(TARGETS sweep DESTINATION lib)
install(DIRECTORY include/sweep DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/sweep/config.h DESTINATION include/sweep)


# sweep-ctl target.


add_executable(sweep-ctl src/sweep-ctl.cc)
target_include_directories(sweep-ctl PRIVATE include include/sweep ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(sweep-ctl sweep ${CMAKE_THREAD_LIBS_INIT})

include(GNUInstallDirs)
install(TARGETS sweep-ctl DESTINATION bin)
install(FILES man/sweep-ctl.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# Make FindPackage(Sweep) work for CMake users.
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/SweepConfig.cmake DESTINATION lib/cmake/sweep)


# make uninstall target.

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMakeUninstall.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/cmake/CMakeUninstall.cmake"
               IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/CMakeUninstall.cmake)
