# Libsweep Examples

Examples for the `libsweep` library.

This can be either the dummy library always returning static point cloud data or the device library requiring the Scanse Sweep device to be plugged in.

### Quick Start

#### To build on Linux: 

Requires `libsweep.so` be installed, in `"/usr/lib"` for example.

```bash
    # build the examples
    mkdir build
    cd build
    cmake ..
    cmake --build .
```

**Note:** the viewer requires SFML2 to be installed.

```bash
    # run the examples
    ./example-c /dev/ttyUSB0
    ./example-c++ /dev/ttyUSB0
    ./example-viewer /dev/ttyUSB0
```


#### To build on windows with MinGW:
Requires that `libsweep.dll` be installed somewhere on the user environment variable "PATH", such as `"C:\MinGW\bin"` for example.

```bash
    # build the examples
    mkdir build
    cd build
    cmake -G "MSYS Makefiles" ..
    cmake --build .
```
**Note:** the viewer example is not compatible with windows currently.

```bash
    # run the examples (the number 5 is just an example, replace it with your COM port number)
    ./example-c COM5
    ./example-c++ COM5
```

### License

Copyright © 2016 Daniel J. Hofmann

Distributed under the MIT License (MIT).
