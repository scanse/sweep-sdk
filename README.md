# Sweep SDK

[![Continuous Integration](https://travis-ci.org/scanse/sweep-sdk.svg?branch=master)](https://travis-ci.org/scanse/sweep-sdk)
[![Continuous Integration](https://ci.appveyor.com/api/projects/status/github/scanse/sweep-sdk?svg=true)](https://ci.appveyor.com/project/kent-williams/sweep-sdk)

SDK for Scanse Sweep LiDAR.

- [libsweep](libsweep/README.md): low-level ABI/API-stable C library
- [SweepPy](sweeppy/README.md): Python bindings
- [SweepJs](sweepjs/README.md): NodeJS bindings

In addition you can use the `scanse/sweep-sdk` Docker image which bundles up the SDK.

    docker run --device /dev/ttyUSB0:/dev/ttyUSB0 -it scanse/sweep-sdk /bin/sh

Real-time viewer for a device speed of 5 Hz:

![viewer](https://cloud.githubusercontent.com/assets/527241/20300444/92ade432-ab1f-11e6-9d96-a585df3fe471.png)

Density-based clustering on the point cloud:

![dbscan](https://cloud.githubusercontent.com/assets/527241/20300478/b5ae968e-ab1f-11e6-8ee0-d24aedd835f9.png)

### License

Copyright © 2016 Daniel J. Hofmann

Distributed under the MIT License (MIT).
