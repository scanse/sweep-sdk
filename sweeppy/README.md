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

See [`app.py`](sweeppy/app.py) for the common use-case where you have to coordinate between scanning and your app's work:
- we start a producer thread putting scans from the Sweep device into a shared queue (non-blocking)
- we start a consumer thread pulling scans out of the shared queue and doing work on them (blocking)
- we use a third thread to set off a stop event and cleanly shutdown the pipeline

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
    self.angle -> int (milli-degree)
    self.distance -> int (cm)
    self.signal_strength -> int ([0:255])
```

See the [libsweep README](https://github.com/scanse/sweep-sdk/tree/master/libsweep#device-interaction) for a description of how to use a related interface.

Additionally, it is recommended that you read through the sweep [Theory of Operation](https://support.scanse.io/hc/en-us/articles/115006333327-Theory-of-Operation) and [Best Practices](https://support.scanse.io/hc/en-us/articles/115006055388-Best-Practices).

### Interpret Sample
```python
sample.angle
```
The sample's angle (azimuth) represents the rotation of the sensor when the range measurment was taken. The value is reported in milli-degrees, or 1/1000 of a degree. For example, a `sample.angle` value of `180000 milli-degrees` equates to `180 degrees` (half of a complete rotation). Successive samples of the same scan will have increasing angles in the range 0-360000.

```python
sample.distance
```
The sample's distance is the range measurement (in cm).

```python
sample.distance
```
The sample's signal strength is the strength or confidence of the range measurement. The value is reported in the range 0-255, where larger values are better.


### License

Copyright © 2016 Daniel J. Hofmann

Distributed under the MIT License (MIT).
