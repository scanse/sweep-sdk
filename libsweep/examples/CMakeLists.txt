cmake_minimum_required(VERSION 3.1)
project(sweep-examples LANGUAGES C CXX)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
find_package(Threads REQUIRED)
find_package(Sweep REQUIRED)

add_executable(example-c++ example.cc)
target_link_libraries(example-c++ PRIVATE ${LIBSWEEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(example-c++ SYSTEM PRIVATE ${LIBSWEEP_INCLUDE_DIR})

add_executable(example-c example.c)
target_link_libraries(example-c PRIVATE ${LIBSWEEP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(example-c SYSTEM PRIVATE ${LIBSWEEP_INCLUDE_DIR})


# TOOD: make SFML based viewer optional
#include(FindPkgConfig)
#pkg_check_modules(LIBSFML REQUIRED sfml-all)

#add_executable(example-viewer viewer.cc)
#target_link_libraries(example-viewer PRIVATE ${LIBSWEEP_LIBRARY} ${LIBSFML_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
#target_include_directories(example-viewer SYSTEM PRIVATE ${LIBSWEEP_INCLUDE_DIR} ${LIBSFML_INCLUDE_DIRS})
