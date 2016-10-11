PREFIX ?= /usr/local

VERSION_MAJOR = 0
VERSION_MINOR = 1

CFLAGS += -O2 -Wall -Wextra -pedantic -std=c99 -fvisibility=hidden -fPIC -pthread
LDFLAGS += -shared -Wl,-soname,libsweep.so.$(VERSION_MAJOR)
LDLIBS += -lpthread
