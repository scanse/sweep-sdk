#include "serial.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

typedef struct sweep_serial_error {
  const char* what; // always literal, do not free
} sweep_serial_error;

typedef struct sweep_serial_device {
  HANDLE fd;
  OVERLAPPED ro_;
  OVERLAPPED wo_;
} sweep_serial_device;

// Constructor hidden from users
static sweep_serial_error_s sweep_serial_error_construct(const char* what) {
  SWEEP_ASSERT(what);

  sweep_serial_error_s out = (sweep_serial_error_s)malloc(sizeof(sweep_serial_error));
  SWEEP_ASSERT(out && "out of memory during error reporting");

  out->what = what;
  return out;
}

const char* sweep_serial_error_message(sweep_serial_error_s error) {
  SWEEP_ASSERT(error);

  return error->what;
}

void sweep_serial_error_destruct(sweep_serial_error_s error) {
  SWEEP_ASSERT(error);

  free(error);
}

sweep_serial_device_s sweep_serial_device_construct(const char* port, int32_t bitrate, sweep_serial_error_s* error) {
  SWEEP_ASSERT(port);
  SWEEP_ASSERT(bitrate > 0);
  SWEEP_ASSERT(error);

  if (bitrate != 115200) {
    *error = sweep_serial_error_construct("baud rate is not supported");
    return NULL;
  }

  // try to open the port
  HANDLE hComm = CreateFile(port,                         // port name
                            GENERIC_READ | GENERIC_WRITE, // read/write
                            0,                            // No Sharing (serial ports can't be shared)
                            NULL,                         // No Security
                            OPEN_EXISTING,                // Open existing port only
                            0,                            // Non Overlapped I/O
                            NULL);                        // Null for serial ports

  if (hComm == INVALID_HANDLE_VALUE) {
    *error = sweep_serial_error_construct("opening serial port failed");
    return NULL;
  }

  // retrieve the current comm state
  DCB dcbSerialParams = {0}; // Initializing DCB structure
  dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
  if (!GetCommState(hComm, &dcbSerialParams)) {
    *error = sweep_serial_error_construct("retrieving current serial port state failed");
    CloseHandle(hComm);
    return NULL;
  }

  // set the parameters to match the uART settings from the Sweep Comm Protocol
  dcbSerialParams.BaudRate = CBR_115200; // BaudRate = 115200 (115.2kb/s)
  dcbSerialParams.ByteSize = 8;          // ByteSize = 8
  dcbSerialParams.StopBits = ONESTOPBIT; // # StopBits = 1
  dcbSerialParams.Parity = NOPARITY;     // Parity = None
  dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;

  // set the serial port parameters to the specified values
  if (!SetCommState(hComm, &dcbSerialParams)) {
    *error = sweep_serial_error_construct("setting serial port parameters failed");
    CloseHandle(hComm);
    return NULL;
  }

  // specify timeouts (all values in milliseconds)
  COMMTIMEOUTS timeouts = {0};
  timeouts.ReadIntervalTimeout = 50;         // max time between arrival of two bytes before ReadFile() returns
  timeouts.ReadTotalTimeoutConstant = 50;    // used to calculate total time-out period for read operations
  timeouts.ReadTotalTimeoutMultiplier = 10;  // used to calculate total time-out period for read operations
  timeouts.WriteTotalTimeoutConstant = 50;   // used to calculate total time-out period for write operations
  timeouts.WriteTotalTimeoutMultiplier = 10; // used to calculate total time-out period for write operations

  // set the timeouts
  if (!SetCommTimeouts(hComm, &timeouts)) {
    *error = sweep_serial_error_construct("setting serial port timeouts failed");
    CloseHandle(hComm);
    return NULL;
  }

  // create the overlapped os reader
  OVERLAPPED osReader = {0};
  OVERLAPPED osWrite = {0};

  // Create the overlapped read event. Must be closed before exiting
  osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (osReader.hEvent == NULL) {
    *error = sweep_serial_error_construct("creating overlapped read event failed");
    CloseHandle(hComm);
    return NULL;
  }
  // Create the overlapped write event. Must be closed before exiting
  osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (osWrite.hEvent == NULL) {
    *error = sweep_serial_error_construct("creating overlapped write event failed");
    CloseHandle(hComm);
    return NULL;
  }

  // set the comm mask
  if (!SetCommMask(hComm, EV_RXCHAR | EV_ERR)) {
    *error = sweep_serial_error_construct("setting comm mask failed");
    CloseHandle(hComm);
    CloseHandle(osReader.hEvent);
    CloseHandle(osWrite.hEvent);
    return NULL;
  }

  // purge the comm port of any pre-existing data or errors
  if (!PurgeComm(hComm, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR)) {
    *error = sweep_serial_error_construct("flushing serial port failed during serial device construction");
    CloseHandle(hComm);
    CloseHandle(osReader.hEvent);
    CloseHandle(osWrite.hEvent);
    return NULL;
  }

  // create the serial device
  sweep_serial_device_s out = (sweep_serial_device_s)malloc(sizeof(sweep_serial_device));

  if (out == NULL) {
    *error = sweep_serial_error_construct("oom during serial device creation");
    CloseHandle(hComm);
    CloseHandle(osReader.hEvent);
    CloseHandle(osWrite.hEvent);
    return NULL;
  }

  out->fd = hComm;
  out->ro_ = osReader;
  out->wo_ = osWrite;

  return out;
}

void sweep_serial_device_destruct(sweep_serial_device_s serial) {
  SWEEP_ASSERT(serial);

  sweep_serial_error_s ignore = NULL;
  sweep_serial_device_flush(serial, &ignore);

  // try to close the serial port
  if (!CloseHandle(serial->fd)) {
    ignore = sweep_serial_error_construct("closing serial port failed");
  }

  // try to close the overlapped read event
  if (!CloseHandle(serial->ro_.hEvent)) {
    ignore = sweep_serial_error_construct("closing serial port overlapped read event failed");
  }

  // try to close the overlapped write event
  if (!CloseHandle(serial->wo_.hEvent)) {
    ignore = sweep_serial_error_construct("closing serial port overlapped write event failed");
  }

  (void)ignore; // nothing we can do here

  free(serial);
}

void sweep_serial_device_read(sweep_serial_device_s serial, void* to, int32_t len, sweep_serial_error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(to);
  SWEEP_ASSERT(len >= 0);
  SWEEP_ASSERT(error);

  DWORD rx_read = 0;
  DWORD totalNumBytesRead = 0;

  // read bytes until "len" bytes have been read
  while (totalNumBytesRead < (DWORD)len) {

    if (!ReadFile(serial->fd, (char*)to + totalNumBytesRead, len - totalNumBytesRead, &rx_read, &(serial->ro_))) {
      *error = sweep_serial_error_construct("reading from serial device failed");
      return;
    }
    totalNumBytesRead += rx_read;
  }

  SWEEP_ASSERT(totalNumBytesRead == (DWORD)len && "reliable read failed to read requested number of bytes");
}

void sweep_serial_device_write(sweep_serial_device_s serial, const void* from, int32_t len, sweep_serial_error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(from);
  SWEEP_ASSERT(len >= 0);
  SWEEP_ASSERT(error);

  DWORD err;
  DWORD tx_written = 0;
  DWORD totalNumBytesWritten = 0;

  // clear the comm errors, and purge if need be
  if (ClearCommError(serial->fd, &err, NULL) && err > 0) {
    if (!PurgeComm(serial->fd, PURGE_TXABORT | PURGE_TXCLEAR)) {
      *error = sweep_serial_error_construct("purging tx buffer failed during seiral port write");
      return;
    }
  }

  // write bytes until "len" bytes have been written
  while (tx_written < (DWORD)len) {
    if (!WriteFile(serial->fd,                               // Handle to the Serial port
                   (const char*)from + totalNumBytesWritten, // Data to be written to the port
                   len - totalNumBytesWritten,               // No of bytes to write
                   &tx_written,                              // Bytes written
                   &(serial->wo_))) {
      *error = sweep_serial_error_construct("writing to serial device failed");
      return;
    }
    totalNumBytesWritten += tx_written;
  }

  SWEEP_ASSERT(((int32_t)totalNumBytesWritten == len) && "reliable write failed to write requested number of bytes");
}

void sweep_serial_device_flush(sweep_serial_device_s serial, sweep_serial_error_s* error) {
  SWEEP_ASSERT(serial);
  SWEEP_ASSERT(error);

  // TODO: Before flushing/purging the port, should check for
  // and handle or explicitly ignore any pending port erros

  // flush serial port
  // - Empty Tx and Rx buffers
  // - Abort any pending read/write operations
  if (!PurgeComm(serial->fd, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR)) {
    *error = sweep_serial_error_construct("flushing serial port failed");
  }
}