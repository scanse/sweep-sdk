project(sweep)

cmake_minimum_required(VERSION 3.5)

SET(COMMON_SRC_FILES sweep.cc protocol.cc sweep.h sweep.hpp serial.h protocol.h)

if (MSVC)
    SET ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX" )
    SET (PALTFORM_SRC_FILES serial_win.cc)
else ()
    SET ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Werror" )
	SET (PALTFORM_SRC_FILESserial_unix.cc)
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS true)


add_library(${PROJECT_NAME} SHARED ${COMMON_SRC_FILES} ${PALTFORM_SRC_FILES})

