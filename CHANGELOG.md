# Changelog

## v1.2.1
This release is compatible with device firmware v1.4.

Changes:
- libsweep:
  - Fixed c++ interface, to resolve linker errors when header is included in multiple translation units.

## v1.2.0
This release is compatible with device firmware v1.4.

Changes:
- libsweep:
  - Mac OS support
  - Propagate dynamic type of exception from background thread
- sweeppy
  - Added patch version (ie: MAJOR.MINOR.PATCH)

## v1.1.2

This release is compatible with device firmware v1.4.

Changes:
- libsweep: 
  - Adapts device construction to be compatible with RaspberryPi
  - Removes implementation methods from API
  - Propagates errors from background thread when accumulating scans

## v1.1.1

This release is compatible with device firmware v1.2.

Changes:
- libsweep: Adapts motor calibration timeout to firmware changes


## v1.1.0

This release is compatible with device firmware v1.1.

Changes:
- libsweep: Adaptes protocol implementation to firmware changes
- libsweep: Accumulates scans in a background worker thread


## v0.1.0

This is the initial release v0.1.0 and is compatible with the Sweep device firmware version v1.0.

It comes with C99, C++11, Python2 / Python3, and Node.js bindings.

Some examples we include:

    Real-time point cloud viewer
    Pub-Sub networking example
    Websocket daemon for device

Happy scanning!
