# Libsweep Examples

Examples for the `libsweep` library.

Requires `libsweep.so` to be installed.
This can be either the dummy library always returning static point cloud data or the device library requiring the Scanse Sweep device to be plugged in.

### Quick Start

To build: 

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
    ./example-c
    ./example-c++
    ./example-viewer
```

### License

Copyright © 2016 Daniel J. Hofmann

Distributed under the MIT License (MIT).
