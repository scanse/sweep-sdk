# SweepPy

Python Scanse Sweep LiDAR library.

Requires `libsweep.so` to be installed.

### Quick Start

```python
from sweeppy import Sweep

with Sweep() as sweep:
    sweep.start_scanning()

    for scan in sweep.get_scans():
        print('{}\n'.format(scan))
```

Note: `Sweep` objects need to be scoped using the `with` statement for resource management.

See [sweeppy.py](sweeppy/__init__.py) for interface and example.

### Interface

```
class Sweep:
    def start_scanning(self) -> None
    def stop_scanning(self) -> None

    def get_motor_speed(self) -> int
    def set_motor_speed(self, speed) -> None

    def get_sample_rate(self) -> int
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
