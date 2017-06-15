#define _POSIX_C_SOURCE 200809L

#include "serial.hpp"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

namespace sweep {
namespace serial {

struct device {
  int32_t fd;
};

static speed_t get_baud(int32_t bitrate) {
  SWEEP_ASSERT(bitrate > 0);
  if (bitrate != 115200) {
    throw error{"Only baud rate 115200 is supported at this time."};
    return -1;
  }
  return bitrate;
}

static bool wait_readable(device_s serial) {
  SWEEP_ASSERT(serial);

  // Setup a select call to block for serial data
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(serial->fd, &readfds);

  int32_t ret = select(serial->fd + 1, &readfds, nullptr, nullptr, nullptr);

  if (ret == -1) {
    // Select was interrupted
    if (errno == EINTR) {
      return false;
    }

    // Otherwise there was some error
    throw error{"blocking on data to read failed"};

  } else if (ret) {
    // Data Available
    return true;
  } else {
    return false;
  }

  return false;
}

device_s device_construct(const char* port, int32_t bitrate) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);

  int32_t fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (fd == -1)
    throw error{"opening serial port failed"};

  if (!isatty(fd))
    throw error{"serial port is not a TTY"};

  struct termios options;

  if (tcgetattr(fd, &options) == -1)
    throw error{"querying terminal options failed"};

  // Input Flags
  options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IGNBRK);

  // SW Flow Control OFF
  options.c_iflag &= ~(IXON | IXOFF | IXANY);

  // Output Flags
  options.c_oflag &= ~(OPOST);

  // Local Flags
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG);

  // IEXTEN

  // Control Flags
  options.c_cflag |= (CLOCAL | CREAD | CS8);
  options.c_cflag &= ~(PARENB | CSTOPB | CSIZE);

  // setup baud rate
  speed_t baud = get_baud(bitrate);

  cfsetispeed(&options, baud);
  cfsetospeed(&options, baud);

  // flush the port
  if (tcflush(fd, TCIFLUSH) == -1)
    throw error{"flushing the serial port failed"};

  // set port attributes
  if (tcsetattr(fd, TCSANOW, &options) == -1) {
    if (close(fd) == -1)
      SWEEP_ASSERT(false && "closing file descriptor during error handling failed");

    throw error{"setting terminal options failed"};
  }

  auto out = new device{fd};
  return out;
}

void device_destruct(device_s serial) {
  SWEEP_ASSERT(serial);

  try {
    device_flush(serial);
  } catch (...) {
    // nothing we can do here
  }

  if (close(serial->fd) == -1)
    SWEEP_ASSERT(false && "closing file descriptor during destruct failed");

  delete serial;
}

void device_read(device_s serial, void* to, int32_t len) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(to);
  SWEEP_ASSERT(len >= 0);

  // the following implements reliable full read xor error
  int32_t bytes_read = 0;

  while (bytes_read < len) {
    if (wait_readable(serial)) {
      int ret = read(serial->fd, (char*)to + bytes_read, len - bytes_read);

      if (ret == -1) {
        if (errno == EAGAIN || errno == EINTR) {
          continue;
        } else {
          throw error{"reading from serial device failed"};
        }
      } else {
        bytes_read += ret;
      }
    }
  }

  SWEEP_ASSERT(bytes_read == len && "reliable read failed to read requested size of bytes");
}

void device_write(device_s serial, const void* from, int32_t len) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(from);
  SWEEP_ASSERT(len >= 0);

  // the following implements reliable full write xor error
  int32_t bytes_written = 0;

  while (bytes_written < len) {
    int32_t ret = write(serial->fd, (const char*)from + bytes_written, len - bytes_written);

    if (ret == -1) {
      if (errno == EAGAIN || errno == EINTR) {
        continue;
      } else {
        throw error{"writing to serial device failed"};
      }
    } else {
      bytes_written += ret;
    }
  }

  SWEEP_ASSERT(bytes_written == len && "reliable write failed to write requested size of bytes");
}

void device_flush(device_s serial) {
  SWEEP_ASSERT(serial);

  if (tcflush(serial->fd, TCIFLUSH) == -1)
    throw error{"flushing the serial port failed"};
}

} // ns serial
} // ns sweep
