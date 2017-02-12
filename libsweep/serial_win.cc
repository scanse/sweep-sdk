#include "serial.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <windows.h>

namespace sweep {
namespace serial {

struct State {
  HANDLE fd;
  OVERLAPPED ro_;
  OVERLAPPED wo_;
};

static int32_t detail_get_port_number(const char* port) {
  SWEEP_ASSERT(port);

  if (strlen(port) <= 3) {
    throw Error{"invalid port name"};
  }

  long parsed_int = std::strtol(port + 3, nullptr, 10);

  if (parsed_int >= 0 && parsed_int <= 255)
    return parsed_int;

  throw Error{"invalid port name"};
}

Device::Device(const char* port, int32_t bitrate) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);

  if (bitrate != 115200) {
    throw Error{"baud rate is not supported"};
  }

  // read port number from the port name
  int port_num = detail_get_port_number(port);

  // Construct formal serial port name
  std::string port_name = "\\\\.\\COM" + std::to_string(port_num);

  // try to open the port
  auto hComm = CreateFile(port_name.c_str(),            // port name
                          GENERIC_READ | GENERIC_WRITE, // read/write
                          0,                            // No Sharing (serial ports can't be shared)
                          NULL,                         // No Security
                          OPEN_EXISTING,                // Open existing port only
                          0,                            // Non Overlapped I/O
                          NULL);                        // Null for serial ports

  if (hComm == INVALID_HANDLE_VALUE) {
    throw Error{"opening serial port failed"};
  }

  // retrieve the current comm state
  DCB dcb_serial_params;
  dcb_serial_params.DCBlength = sizeof(dcb_serial_params);
  if (!GetCommState(hComm, &dcb_serial_params)) {
    CloseHandle(hComm);
    throw Error{"retrieving current serial port state failed"};
  }

  // set the parameters to match the uART settings from the Sweep Comm Protocol
  dcb_serial_params.BaudRate = CBR_115200; // BaudRate = 115200 (115.2kb/s)
  dcb_serial_params.ByteSize = 8;          // ByteSize = 8
  dcb_serial_params.StopBits = ONESTOPBIT; // # StopBits = 1
  dcb_serial_params.Parity = NOPARITY;     // Parity = None
  dcb_serial_params.fDtrControl = DTR_CONTROL_DISABLE;

  // set the serial port parameters to the specified values
  if (!SetCommState(hComm, &dcb_serial_params)) {
    CloseHandle(hComm);
    throw Error{"setting serial port parameters failed"};
  }

  // specify timeouts (all values in milliseconds)
  COMMTIMEOUTS timeouts;
  if (!GetCommTimeouts(hComm, &timeouts)) {
    CloseHandle(hComm);
    throw Error{"retrieving current serial port timeouts failed"};
  }
  timeouts.ReadIntervalTimeout = 50;         // max time between arrival of two bytes before ReadFile() returns
  timeouts.ReadTotalTimeoutConstant = 50;    // used to calculate total time-out period for read operations
  timeouts.ReadTotalTimeoutMultiplier = 10;  // used to calculate total time-out period for read operations
  timeouts.WriteTotalTimeoutConstant = 50;   // used to calculate total time-out period for write operations
  timeouts.WriteTotalTimeoutMultiplier = 10; // used to calculate total time-out period for write operations

  // set the timeouts
  if (!SetCommTimeouts(hComm, &timeouts)) {
    CloseHandle(hComm);
    throw Error{"setting serial port timeouts failed"};
  }

  // create the overlapped os reader
  OVERLAPPED os_reader = {0};
  OVERLAPPED os_write = {0};

  // Create the overlapped read event. Must be closed before exiting
  os_reader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (os_reader.hEvent == NULL) {
    CloseHandle(hComm);
    throw Error{"creating overlapped read event failed"};
  }
  // Create the overlapped write event. Must be closed before exiting
  os_write.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (os_write.hEvent == NULL) {
    CloseHandle(hComm);
    throw Error{"creating overlapped write event failed"};
  }

  // set the comm mask
  if (!SetCommMask(hComm, EV_RXCHAR | EV_ERR)) {
    CloseHandle(hComm);
    CloseHandle(os_reader.hEvent);
    CloseHandle(os_write.hEvent);
    throw Error{"setting comm mask failed"};
  }

  // purge the comm port of any pre-existing data or errors
  if (!PurgeComm(hComm, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR)) {
    CloseHandle(hComm);
    CloseHandle(os_reader.hEvent);
    CloseHandle(os_write.hEvent);
    throw Error{"flushing serial port failed during serial device construction"};
  }

  // create the serial device
  _state = std::unique_ptr<State>(new State{hComm, os_reader, os_write});
}

Device::~Device() {
  try {
    flush();
  } catch (...) {
  }

  // close the serial port
  CloseHandle(_state->fd);

  // close the overlapped read event
  CloseHandle(_state->ro_.hEvent);

  // close the overlapped write event
  CloseHandle(_state->wo_.hEvent);
}

void Device::read(void* to, int32_t len) {
  SWEEP_ASSERT(to);
  SWEEP_ASSERT(len >= 0);

  DWORD rx_read = 0;
  DWORD total_num_bytes_read = 0;

  // read bytes until "len" bytes have been read
  while (total_num_bytes_read < (DWORD)len) {

    if (!ReadFile(_state->fd, (unsigned char*)to + total_num_bytes_read, len - total_num_bytes_read, &rx_read, &(_state->ro_))) {
      throw Error{"reading from serial device failed"};
    }
    total_num_bytes_read += rx_read;
  }

  SWEEP_ASSERT(total_num_bytes_read == (DWORD)len && "reliable read failed to read requested number of bytes");
  if (total_num_bytes_read != (DWORD)len)
    throw Error{"reading from serial device failed"};
}

void Device::write(const void* from, int32_t len) {
  SWEEP_ASSERT(from);
  SWEEP_ASSERT(len >= 0);

  DWORD err = 0;
  DWORD tx_written = 0;
  DWORD total_num_bytes_written = 0;

  // check for a comm error (clears any error flag present)
  if (!ClearCommError(_state->fd, &err, NULL)) {
    throw Error{"checking for/clearing comm error failed during serial port write"};
  }
  // in the event of a comm error, purge before writing
  if (err > 0) {
    if (!PurgeComm(_state->fd, PURGE_TXABORT | PURGE_TXCLEAR)) {
      throw Error{"purging tx buffer failed during serial port write"};
    }
  }

  // write bytes until "len" bytes have been written
  while (tx_written < (DWORD)len) {
    if (!WriteFile(_state->fd,                                           // Handle to the Serial port
                   (const unsigned char*)from + total_num_bytes_written, // Data to be written to the port
                   len - total_num_bytes_written,                        // No of bytes to write
                   &tx_written,                                          // Bytes written
                   &(_state->wo_))) {
      throw Error{"writing to serial device failed"};
    }
    total_num_bytes_written += tx_written;
  }

  SWEEP_ASSERT((total_num_bytes_written == (DWORD)len) && "reliable write failed to write requested number of bytes");
  if (total_num_bytes_written != (DWORD)len)
    throw Error{"writing to serial device failed"};
}

void Device::flush() {

  DWORD err = 0;
  // check for a comm error (clears any error flag present)
  if (!ClearCommError(_state->fd, &err, NULL)) {
    throw Error{"checking for/clearing comm error failed during serial port flush"};
  }

  // flush serial port
  // - Empty Tx and Rx buffers
  // - Abort any pending read/write operations
  if (!PurgeComm(_state->fd, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR)) {
    throw Error{"flushing serial port failed"};
  }
}

} // ns serial
} // ns sweep