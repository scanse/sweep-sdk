#include "serial.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <windows.h>

namespace sweep {
namespace serial {

typedef struct device {
  HANDLE h_comm;
  OVERLAPPED os_reader;
  bool waiting_on_read;      // Used to prevent creation of new read operation if one is outstanding
  DWORD read_timeout_millis; // timeout interval for entire read operation
} device;

static int32_t detail_get_port_number(const char* port) {
  SWEEP_ASSERT(port);

  if (strlen(port) <= 3)
    throw error{"invalid port name"};

  auto parsed_int = std::strtoll(port + 3, nullptr, 10);

  if (parsed_int >= 0 && parsed_int <= 255)
    return static_cast<int32_t>(parsed_int);
  else
    throw error{"invalid port name"};

  return -1;
}

device_s device_construct(const char* port, int32_t bitrate) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);

  if (bitrate != 115200)
    throw error{"baud rate is not supported"};

  const auto port_num = detail_get_port_number(port);

  // Construct formal serial port name
  std::string port_name{"\\\\.\\COM"};
  port_name += std::to_string(port_num);

  // try to open the port
  auto h_comm = CreateFile(port_name.c_str(),            // port name
                           GENERIC_READ | GENERIC_WRITE, // read/write
                           0,                            // No Sharing (serial ports can't be shared)
                           NULL,                         // No Security
                           OPEN_EXISTING,                // Open existing port only
                           0,                            // Non Overlapped I/O
                           NULL);                        // Null for serial ports

  if (h_comm == INVALID_HANDLE_VALUE)
    throw error{"opening serial port failed"};

  // retrieve the current comm state
  DCB dcb_serial_params;
  dcb_serial_params.DCBlength = sizeof(dcb_serial_params);
  if (!GetCommState(h_comm, &dcb_serial_params)) {
    CloseHandle(h_comm);
    throw error{"retrieving current serial port state failed"};
  }

  // set the parameters to match the uART settings from the Sweep Comm Protocol
  dcb_serial_params.BaudRate = CBR_115200; // BaudRate = 115200 (115.2kb/s)
  dcb_serial_params.ByteSize = 8;          // ByteSize = 8
  dcb_serial_params.StopBits = ONESTOPBIT; // # StopBits = 1
  dcb_serial_params.Parity = NOPARITY;     // Parity = None
  dcb_serial_params.fDtrControl = DTR_CONTROL_DISABLE;

  // set the serial port parameters to the specified values
  if (!SetCommState(h_comm, &dcb_serial_params)) {
    CloseHandle(h_comm);
    throw error{"setting serial port parameters failed"};
  }

  // specify timeouts (all values in milliseconds)
  COMMTIMEOUTS timeouts;
  if (!GetCommTimeouts(h_comm, &timeouts)) {
    CloseHandle(h_comm);
    throw error{"retrieving current serial port timeouts failed"};
  }
  timeouts.ReadIntervalTimeout = 50;         // max time between arrival of two bytes before ReadFile() returns
  timeouts.ReadTotalTimeoutConstant = 50;    // used to calculate total time-out period for read operations
  timeouts.ReadTotalTimeoutMultiplier = 10;  // used to calculate total time-out period for read operations
  timeouts.WriteTotalTimeoutConstant = 50;   // used to calculate total time-out period for write operations
  timeouts.WriteTotalTimeoutMultiplier = 10; // used to calculate total time-out period for write operations

  // set the timeouts
  if (!SetCommTimeouts(h_comm, &timeouts)) {
    CloseHandle(h_comm);
    throw error{"setting serial port timeouts failed"};
  }

  // create the overlapped event handle for reading
  OVERLAPPED os_reader = {0};

  // Create the overlapped read event. Must be closed before exiting
  os_reader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (os_reader.hEvent == NULL) {
    CloseHandle(h_comm);
    throw error{"creating overlapped read event failed"};
  }

  // set the comm mask
  if (!SetCommMask(h_comm, EV_RXCHAR | EV_ERR)) {
    CloseHandle(h_comm);
    CloseHandle(os_reader.hEvent);
    throw error{"setting comm mask failed"};
  }

  // purge the comm port of any pre-existing data or errors
  if (!PurgeComm(h_comm, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR)) {
    CloseHandle(h_comm);
    CloseHandle(os_reader.hEvent);
    throw error{"flushing serial port failed during serial device construction"};
  }

  // create the serial device
  auto out = new device{h_comm, os_reader, FALSE, 500};

  return out;
}

void device_destruct(device_s serial) {
  SWEEP_ASSERT(serial);

  try {
    device_flush(serial);
  } catch (...) {
    // nothing we can do here
  }

  // close the serial port
  CloseHandle(serial->h_comm);

  // close the overlapped read event
  CloseHandle(serial->os_reader.hEvent);

  delete serial;
}

void device_read(device_s serial, void* to, int32_t len) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(to);
  SWEEP_ASSERT(len >= 0);

  DWORD dw_num_read;
  DWORD dw_num_to_read = (DWORD)len;
  bool f_result = false;

  if (!serial->waiting_on_read) {
    // Issue read operation.
    if (!ReadFile(serial->h_comm, (unsigned char*)to, dw_num_to_read, &dw_num_read, &(serial->os_reader))) {
      // If the read did not return immediately, check if it was pending
      if (GetLastError() != ERROR_IO_PENDING) {
        // Error in communications; report it.
        throw error{"reading from serial device failed"};
      } else {
        serial->waiting_on_read = true;
      }
    } else {
      // read completed immediately
      f_result = true;
    }
  }

  DWORD dwRes;

  // If the read is pending, wait for it to finish... but permit timeout
  if (serial->waiting_on_read) {
    dwRes = WaitForSingleObject(serial->os_reader.hEvent, serial->read_timeout_millis);
    switch (dwRes) {
    // Read completed.
    case WAIT_OBJECT_0:
      if (!GetOverlappedResult(serial->h_comm, &(serial->os_reader), &dw_num_read, FALSE)) {
        // Error in communications; report it.
        throw error{"error in communications during serial read"};
      } else {
        // Read completed successfully.
        f_result = true;
      }

      //  Reset flag so that another opertion can be issued.
      serial->waiting_on_read = false;
      break;

    case WAIT_TIMEOUT:
      // Operation isn't complete yet. serial->waiting_on_read flag isn't changed since we'll loop back
      // around, and we don't want to issue another read until the first one finishes.
      break;

    default:
      // Error in the WaitForSingleObject; abort.
      // Indicates a problem with the OVERLAPPED structure's event handle.
      throw error{"problem with overlapped structure during serial read"};
      break;
    }
  }

  // Check that the number of bytes actually read is equal to the requested quantity
  if (f_result == true)
    f_result = (dw_num_read == (DWORD)len);

  SWEEP_ASSERT(f_result && "reliable read failed to read requested number of bytes");
  if (!f_result)
    throw error{"reading from serial device failed"};
}

void device_write(device_s serial, const void* from, int32_t len) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(from);
  SWEEP_ASSERT(len >= 0);

  DWORD err = 0;
  OVERLAPPED os_write = {0};
  DWORD dw_written;
  DWORD dw_to_write = (DWORD)len;
  bool f_result;

  // Create this write's OVERLAPPED structure hEvent.
  os_write.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (os_write.hEvent == NULL) {
    // Error creating overlapped event handle.
    throw error{"creating overlapped event handle failed during serial port write"};
  }

  // check for a comm error (clears any error flag present)
  if (!ClearCommError(serial->h_comm, &err, NULL)) {
    throw error{"checking for/clearing comm error failed during serial port write"};
  }
  // in the event of a comm error, purge before writing
  if (err > 0) {
    if (!PurgeComm(serial->h_comm, PURGE_TXABORT | PURGE_TXCLEAR)) {
      throw error{"purging tx buffer failed during serial port write"};
    }
  }

  // Issue write.
  if (!WriteFile(serial->h_comm, (const unsigned char*)from, dw_to_write, &dw_written, &os_write)) {
    if (GetLastError() != ERROR_IO_PENDING) {
      // WriteFile failed, but it isn't delayed. Report error and abort.
      throw error{"WriteFile failed, but is not delayed during serial port write"};
    } else {
      // Write is pending indefinitely (ie: fWait param is TRUE, so this is effectively NON-OVERLAPPED io)
      if (!GetOverlappedResult(serial->h_comm, &os_write, &dw_written, TRUE)) {
        throw error{"Failed to get result during serial port write"};
      } else {
        // Write operation completed successfully.
        f_result = true;
      }
    }
  } else {
    // WriteFile completed immediately.
    f_result = true;
  }

  // Check that the number of bytes actually transferred is equal to the requested quantity
  if (dw_written != (DWORD)len)
    f_result = false;

  // Close the overlapped event handle
  CloseHandle(os_write.hEvent);

  SWEEP_ASSERT(f_result && "reliable write failed to write requested number of bytes");
  if (f_result == false)
    throw error{"writing to serial device failed"};
}

void device_flush(device_s serial) {
  SWEEP_ASSERT(serial);

  DWORD err = 0;
  // check for a comm error (clears any error flag present)
  if (!ClearCommError(serial->h_comm, &err, NULL)) {
    throw error{"checking for/clearing comm error failed during serial port flush"};
  }

  // flush serial port
  // - Empty Tx and Rx buffers
  // - Abort any pending read/write operations
  if (!PurgeComm(serial->h_comm, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR)) {
    throw error{"flushing serial port failed"};
  }
}

} // ns serial
} // ns sweep
