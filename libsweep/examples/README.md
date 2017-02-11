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

**Note:**
- The viewer requires SFML2 to be installed.
- The pub-sub networking example requires Protobuf and ZeroMQ to be installed.


```bash
./example-c
./example-c++
```

Real-time viewer:

```bash
./example-viewer
```

Pub-Sub networking example.
Start a publisher sending out full 360 degree scans via the network (localhost).
Then start some subscribers connecting to the publisher.

```bash
./example-net publisher
./example-net subscriber
```

### License

Copyright © 2016 Daniel J. Hofmann

Distributed under the MIT License (MIT).
