#define _POSIX_C_SOURCE 200809L

#include "serial.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

typedef struct sweep_serial_error {
  const char* what; // always literal, do not free
} sweep_serial_error;

typedef struct sweep_serial_device {
  int32_t fd;
  int32_t timeout;
} sweep_serial_device;

// Constructor hidden from users
static sweep_serial_error_s sweep_serial_error_construct(const char* what) {
  SWEEP_ASSERT(what);

  sweep_serial_error_s out = malloc(sizeof(sweep_serial_error));

  if (out == NULL) {
    SWEEP_ASSERT(false && "out of memory during error reporting");
    exit(EXIT_FAILURE);
  }

  *out = (sweep_serial_error){.what = what};
  return out;
}

const char* sweep_serial_error_message(sweep_serial_error_s error) { return error->what; }

void sweep_serial_error_destruct(sweep_serial_error_s error) { free(error); }

static speed_t sweep_serial_detail_get_baud(int32_t baudrate, sweep_serial_error_s* error) {
  SWEEP_ASSERT(baudrate > 0);
  SWEEP_ASSERT(error);

  speed_t baud;

  switch (baudrate) {
#ifdef B0
  case 0:
    baud = B0;
    break;
#endif
#ifdef B50
  case 50:
    baud = B50;
    break;
#endif
#ifdef B75
  case 75:
    baud = B75;
    break;
#endif
#ifdef B110
  case 110:
    baud = B110;
    break;
#endif
#ifdef B134
  case 134:
    baud = B134;
    break;
#endif
#ifdef B150
  case 150:
    baud = B150;
    break;
#endif
#ifdef B200
  case 200:
    baud = B200;
    break;
#endif
#ifdef B300
  case 300:
    baud = B300;
    break;
#endif
#ifdef B600
  case 600:
    baud = B600;
    break;
#endif
#ifdef B1200
  case 1200:
    baud = B1200;
    break;
#endif
#ifdef B1800
  case 1800:
    baud = B1800;
    break;
#endif
#ifdef B2400
  case 2400:
    baud = B2400;
    break;
#endif
#ifdef B4800
  case 4800:
    baud = B4800;
    break;
#endif
#ifdef B7200
  case 7200:
    baud = B7200;
    break;
#endif
#ifdef B9600
  case 9600:
    baud = B9600;
    break;
#endif
#ifdef B14400
  case 14400:
    baud = B14400;
    break;
#endif
#ifdef B19200
  case 19200:
    baud = B19200;
    break;
#endif
#ifdef B28800
  case 28800:
    baud = B28800;
    break;
#endif
#ifdef B57600
  case 57600:
    baud = B57600;
    break;
#endif
#ifdef B76800
  case 76800:
    baud = B76800;
    break;
#endif
#ifdef B38400
  case 38400:
    baud = B38400;
    break;
#endif
#ifdef B115200
  case 115200:
    baud = B115200;
    break;
#endif
#ifdef B128000
  case 128000:
    baud = B128000;
    break;
#endif
#ifdef B153600
  case 153600:
    baud = B153600;
    break;
#endif
#ifdef B230400
  case 230400:
    baud = B230400;
    break;
#endif
#ifdef B256000
  case 256000:
    baud = B256000;
    break;
#endif
#ifdef B460800
  case 460800:
    baud = B460800;
    break;
#endif
#ifdef B576000
  case 576000:
    baud = B576000;
    break;
#endif
#ifdef B921600
  case 921600:
    baud = B921600;
    break;
#endif
#ifdef B1000000
  case 1000000:
    baud = B1000000;
    break;
#endif
#ifdef B1152000
  case 1152000:
    baud = B1152000;
    break;
#endif
#ifdef B1500000
  case 1500000:
    baud = B1500000;
    break;
#endif
#ifdef B2000000
  case 2000000:
    baud = B2000000;
    break;
#endif
#ifdef B2500000
  case 2500000:
    baud = B2500000;
    break;
#endif
#ifdef B3000000
  case 3000000:
    baud = B3000000;
    break;
#endif
#ifdef B3500000
  case 3500000:
    baud = B3500000;
    break;
#endif
#ifdef B4000000
  case 4000000:
    baud = B4000000;
    break;
#endif
  default:
    *error = sweep_serial_error_construct("baudrate could not be determined");
    baud = -1;
  }

  return baud;
}

static bool sweep_serial_detail_wait_readable(sweep_serial_device_s serial, sweep_serial_error_s* error) {
  SWEEP_ASSERT(error);

  // Setup a select call to block for serial data or a timeout
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(serial->fd, &readfds);

  struct timeval timeout = (struct timeval){.tv_sec = serial->timeout / 1000, .tv_usec = (serial->timeout % 1000) * 1000};

  int32_t ret = select(serial->fd + 1, &readfds, NULL, NULL, &timeout);

  if (ret == -1) {
    // Select was interrupted
    if (errno == EINTR) {
      return false;
    }

    // Otherwise there was some error
    *error = sweep_serial_error_construct("blocking on data to read failed");
    return false;
  } else if (ret) {
    // Data Available
    return true;
  } else {
    // Timeout occurred
    return false;
  }

  return false;
}

sweep_serial_device_s sweep_serial_device_construct(const char* port, int32_t baudrate, int32_t timeout,
                                                    sweep_serial_error_s* error) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(baudrate > 0);
  SWEEP_ASSERT(timeout > 0);
  SWEEP_ASSERT(error);

  int32_t fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (fd == -1) {
    *error = sweep_serial_error_construct("opening serial port failed");
    return NULL;
  }

  if (!isatty(fd)) {
    *error = sweep_serial_error_construct("serial port is not a TTY");
    return NULL;
  }

  struct termios options;

  if (tcgetattr(fd, &options) == -1) {
    *error = sweep_serial_error_construct("querying terminal options failed");
    return NULL;
  }

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
  sweep_serial_error_s bauderror = NULL;
  speed_t baud = sweep_serial_detail_get_baud(baudrate, &bauderror);

  if (bauderror) {
    *error = bauderror;
    return NULL;
  }

  cfsetispeed(&options, baud);
  cfsetospeed(&options, baud);

  // flush the port
  if (tcflush(fd, TCIFLUSH) == -1) {
    *error = sweep_serial_error_construct("flushing the serial port failed");
    return NULL;
  }

  // set port attributes
  if (tcsetattr(fd, TCSANOW, &options) == -1) {
    *error = sweep_serial_error_construct("setting terminal options failed");

    if (close(fd) == -1) {
      SWEEP_ASSERT(false && "closing file descriptor during error handling failed");
    }

    return NULL;
  }

  sweep_serial_device_s out = malloc(sizeof(sweep_serial_device));

  if (out == NULL) {
    *error = sweep_serial_error_construct("oom during serial device creation");
    return NULL;
  }

  *out = (sweep_serial_device){.fd = fd, .timeout = timeout};

  return out;
}

void sweep_serial_device_destruct(sweep_serial_device_s serial) { free(serial); }

void sweep_serial_device_read(sweep_serial_device_s serial, void* to, int32_t len, sweep_serial_error_s* error) {
  SWEEP_ASSERT(to);
  SWEEP_ASSERT(len >= 0);
  SWEEP_ASSERT(error);

  // the following implements reliable full read xor error
  int32_t bytes_read = 0;

  sweep_serial_error_s waiterror = NULL;

  while (bytes_read < len) {
    if (sweep_serial_detail_wait_readable(serial, &waiterror) && !waiterror) {
      int ret = read(serial->fd, (char*)to + bytes_read, len - bytes_read);

      if (ret == -1) {
        if (errno == EAGAIN || errno == EINTR) {
          continue;
        } else {
          *error = sweep_serial_error_construct("reading from serial device failed");
          return;
        }
      } else {
        bytes_read += ret;
      }

    } else if (waiterror) {
      *error = waiterror;
      return;
    }
  }

  SWEEP_ASSERT(bytes_read == len && "reliable read failed to read requested size of bytes");
}

void sweep_serial_device_write(sweep_serial_device_s serial, const void* from, int32_t len, sweep_serial_error_s* error) {
  SWEEP_ASSERT(from);
  SWEEP_ASSERT(len >= 0);
  SWEEP_ASSERT(error);

  // the following implements reliable full write xor error
  int32_t bytes_written = 0;

  while (bytes_written < len) {
    int32_t ret = write(serial->fd, (const char*)from + bytes_written, len - bytes_written);

    if (ret == -1) {
      if (errno == EAGAIN || errno == EINTR) {
        continue;
      } else {
        *error = sweep_serial_error_construct("writing to serial device failed");
        return;
      }
    } else {
      bytes_written += ret;
    }
  }

  SWEEP_ASSERT(bytes_written == len && "reliable write failed to write requested size of bytes");
}

void sweep_serial_device_flush(sweep_serial_device_s serial, sweep_serial_error_s* error) {
  SWEEP_ASSERT(error);

  if (tcflush(serial->fd, TCIFLUSH) == -1) {
    *error = sweep_serial_error_construct("flushing the serial port failed");
  }
}
