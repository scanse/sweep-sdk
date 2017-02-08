cmake_minimum_required(VERSION 2.8.11)
project(sweep-examples C CXX)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -pedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -pedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # Turn on the ability to create folders to organize projects (.vcproj)
  # It creates "CMakePredefinedTargets" folder by default and adds CMake
  # defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
else()
  message(FATAL_ERROR "System not yet supported. Please open a ticket.")
endif()

find_package(Threads REQUIRED)
find_package(Sweep REQUIRED)

add_executable(example-c++ example.cc)
target_link_libraries(example-c++ PRIVATE ${LIBSWEEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(example-c++ SYSTEM PRIVATE ${LIBSWEEP_INCLUDE_DIR})

add_executable(example-c example.c)
target_link_libraries(example-c PRIVATE ${LIBSWEEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(example-c SYSTEM PRIVATE ${LIBSWEEP_INCLUDE_DIR})


# Optional SFML2 based viewer
include(FindPkgConfig)
pkg_check_modules(LIBSFML sfml-all)

if (LIBSFML_FOUND)
  add_executable(example-viewer viewer.cc)
  target_link_libraries(example-viewer PRIVATE ${LIBSWEEP_LIBRARY} ${LIBSFML_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  target_include_directories(example-viewer SYSTEM PRIVATE ${LIBSWEEP_INCLUDE_DIR} ${LIBSFML_INCLUDE_DIRS})
else()
  message(STATUS "SFML2 required for real-time viewer (libsfml-dev)")
endif()


# Optional Protobuf + ZeroMQ networking example
find_package(Protobuf 2.4.1)
pkg_check_modules(LIBZMQ libzmq)

if (PROTOBUF_FOUND AND LIBZMQ_FOUND)
  protobuf_generate_cpp(ProtoSources ProtoHeaders net.proto)
  add_executable(example-net net.cc ${ProtoSources} ${ProtoHeaders})
  target_link_libraries(example-net PRIVATE ${LIBSWEEP_LIBRARY} ${PROTOBUF_LITE_LIBRARIES} ${LIBZMQ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  target_include_directories(example-net SYSTEM PRIVATE ${LIBSWEEP_INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIRS} ${LIBZMQ_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
else()
  message(STATUS "Protobuf2 and ZeroMQ required for networking example (libprotobuf-dev protobuf-compiler libzmq-dev)")
endif()
