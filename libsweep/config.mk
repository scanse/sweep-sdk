VERSION_MAJOR = 0
VERSION_MINOR = 1

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
  PREFIX ?= /usr
  CFLAGS += -O2 -Wall -Wextra -pedantic -std=c99 -Wnonnull -fvisibility=hidden -fPIC -pthread
  LDFLAGS += -shared -Wl,-soname,libsweep.so.$(VERSION_MAJOR)
  LDLIBS += -lpthread
else ifeq ($(UNAME), Darwin)
  $(error macOS build system support missing)
else
  $(error system not supported)
endif
