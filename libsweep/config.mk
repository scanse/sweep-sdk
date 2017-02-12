VERSION_MAJOR = 0
VERSION_MINOR = 1

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
  PREFIX ?= /usr
  CXXFLAGS += -O2 -Wall -Wextra -pedantic -std=c++11 -fvisibility=hidden -fPIC -fno-rtti -pthread
  LDFLAGS += -shared -Wl,-soname,libsweep.so.$(VERSION_MAJOR)
  LDLIBS += -lstdc++ -lpthread
else ifeq ($(UNAME), Darwin)
  $(error macOS build system support missing)
else
  $(error system not supported)
endif
