# Libsweep Examples

Examples for the `libsweep` library.

This can be either the dummy library always returning static point cloud data or the device library requiring the Scanse Sweep device to be plugged in.

### Quick Start

#### Linux

Requires `libsweep.so` be installed.

```bash
# build the examples
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

```bash
# run the examples (use your device's COM port number)
./example-c /dev/ttyUSB0
./example-c++ /dev/ttyUSB0
```

Real-time viewer:

**Note:** The viewer requires SFML2 to be installed.

```bash
./example-viewer /dev/ttyUSB0
```

Pub-Sub networking example:

**Note:** The pub-sub networking example requires Protobuf and ZeroMQ to be installed.

Start a publisher sending out full 360 degree scans via the network (localhost).
Then start some subscribers connecting to the publisher.

```bash
./example-net publisher
./example-net subscriber
```


#### Windows

First make sure that the installation directories for the library and includes have been added to the environment variable `PATH`.

```bash
# build the examples
mkdir build
cd build
# Use the appropriate VS for your computer
cmake .. -G "Visual Studio 14 2015 Win64"
cmake --build . --config Release
```

**Note:** in the above command, `Visual Studio 14 2015 Win64` will build x64 (64bit) versions of the examples. This requires that the x64 verison of `libsweep` be installed. If you installed the x86 version, drop the `Win64`. See `libsweep/README` for more info.

If you do not know the COM port number of your device, check under Device Manager -> Ports (COM & LP).

```bash
cd Release
# run the examples (use your device's COM port number)
example-c.exe COM5
example-c++.exe COM5
```

### License

Copyright © 2016 Daniel J. Hofmann

Distributed under the MIT License (MIT).
