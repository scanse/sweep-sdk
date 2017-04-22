# SweepPy

Python Scanse Sweep LiDAR library. Work with Python2 and Python3.

Requires `libsweep.so` to be installed.

### Installation

Install `sweeppy` module for Python3 locally:

```bash
python3 setup.py install --user
```

### Example for testing

In the following, replace `/dev/ttyUSB0` with your device's port name. This executes [`__main__.py`](sweeppy/__main__.py) (also works without the installation step).

```bash
python -m sweeppy /dev/ttyUSB0
```

### Windows:

The installed sweep library architecture must match the python version. Ie: if you are using a x86 (32bit) version of python, you must install the x86 (32bit) verison of libsweep.

```bash
python.exe setup.py install --user
```

In the following: replace `COM5` with your device's port name (check "Device Manager -> COM Ports").

```bash
python.exe -m sweeppy COM5
```

### Quick Start

```python
from sweeppy import Sweep

with Sweep('/dev/ttyUSB0') as sweep:
    sweep.start_scanning()

    for scan in sweep.get_scans():
        print('{}\n'.format(scan))
```

Note: `Sweep` objects need to be scoped using the `with` statement for resource management.

See [sweeppy.py](sweeppy/__init__.py) for interface and [example](sweeppy/__main__.py) for example usage.




### Interface

```
class Sweep:
    def __init__(self, port, bitrate = None) -> Sweep

    def start_scanning(self) -> None
    def stop_scanning(self) -> None

    def get_motor_ready(self) -> bool
    def get_motor_speed(self) -> int (Hz)
    def set_motor_speed(self, speed) -> None

    def get_sample_rate(self) -> int (Hz)
    def set_sample_rate(self, speed) -> None

    def get_scans(self) -> Iterable[Scan]

    def reset(self) -> None

class Scan:
    self.samples -> Sample

class Sample:
    self.angle -> int
    self.distance -> int
    self.signal_strength -> int
```

### License

Copyright © 2016 Daniel J. Hofmann

Distributed under the MIT License (MIT).
