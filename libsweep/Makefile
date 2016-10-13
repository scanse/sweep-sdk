include config.mk

TARGET = libsweep.so
OBJECTS = sweep.o serial_unix.o
HEADER = sweep.h serial.h

$(TARGET): $(OBJECTS) $(HEADER)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install:
	@mkdir -p $(PREFIX)/include/sweep
	install -m 0644 $(HEADER) $(PREFIX)/include/sweep
	@mkdir -p $(PREFIX)/lib
	install -m 0644 $(TARGET) $(PREFIX)/lib
	ln -sf $(PREFIX)/lib/$(TARGET) $(PREFIX)/lib/$(TARGET).$(VERSION_MAJOR)
	ln -sf $(PREFIX)/lib/$(TARGET) $(PREFIX)/lib/$(TARGET).$(VERSION_MAJOR).$(VERSION_MINOR)

watch:
	while ! inotifywait -e modify *.c *.h; do make; done

clean:
	@$(RM) $(OBJECTS) $(TARGET)

.PHONY: clean install watch
