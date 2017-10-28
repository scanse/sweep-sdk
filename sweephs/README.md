# SweepHs

Haskell Scanse Sweep LiDAR library.

Requires `libsweep.so` to be installed.

### Todo

- [ ] Look into `ForeignPtr` vs `Ptr`
- [ ] Look into `bracket` and `CondT`
- [ ] Level up: `resourceT`
- [ ] Level up: `conduit`

### Installation

```bash
stack setup
stack build
```

### Example for testing

```bash
stack exec -- sweephs-exe
```

### Example for testing

In the following, replace `/dev/ttyUSB0` with your device's port name. This executes [`__main__.py`](sweeppy/__main__.py) (also works without the installation step).

```bash
python -m sweeppy /dev/ttyUSB0
```

### License

Copyright © 2017 Daniel J. Hofmann

Distributed under the MIT License (MIT).
