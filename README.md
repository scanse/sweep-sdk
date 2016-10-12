# libsweep

C99 library for Scanse Sweep LiDAR.

##### Quick Start

    cd libsweep
    make
    sudo make install
    sudo ldconfig

##### I need a static library

No you don't. We do our best to provide both ABI and API stability exactly so that you can just drop in a new version.
If you _really_ need a static library help yourself: `ar rcs libsweep.a *.o`.

##### License

Copyright © 2016 Daniel J. Hofmann

Distributed under the MIT License (MIT).
