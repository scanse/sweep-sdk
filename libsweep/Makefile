include config.mk

all: libsweep.so

libsweep.so: sweep.o protocol.o serial_unix.o time_unix.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

dummy: dummy.o time_unix.o
	$(CC) $(LDFLAGS) -o libsweep.so $^ $(LDLIBS)

install:
	mkdir -p $(PREFIX)/include/sweep
	install -m 0644 sweep.h $(PREFIX)/include/sweep
	install -m 0644 sweep.hpp $(PREFIX)/include/sweep
	mkdir -p $(PREFIX)/lib
	install -m 0644 libsweep.so $(PREFIX)/lib
	ln -sf $(PREFIX)/lib/libsweep.so $(PREFIX)/lib/libsweep.so.$(VERSION_MAJOR)
	ln -sf $(PREFIX)/lib/libsweep.so $(PREFIX)/lib/libsweep.so.$(VERSION_MAJOR).$(VERSION_MINOR)

watch:
	while ! inotifywait -e modify *.c *.h; do make; done

clean:
	@$(RM) *.so *.o

.PHONY: clean install watch
