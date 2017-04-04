import itertools
import sys
from . import Sweep


def main():
    if len(sys.argv) < 2:
        sys.exit('python -m sweeppy /dev/ttyUSB0')

    dev = sys.argv[1]

    with Sweep(dev) as sweep:
        speed = sweep.get_motor_speed()
        rate = sweep.get_sample_rate()

        print('Motor Speed: {} Hz'.format(speed))
        print('Sample Rate: {} Hz'.format(rate))

        # Starts scanning as soon as the motor is ready
        sweep.start_scanning()

        # get_scans is coroutine-based generator lazily returning scans ad infinitum
        for scan in itertools.islice(sweep.get_scans(), 3):
            print(scan)

main()
