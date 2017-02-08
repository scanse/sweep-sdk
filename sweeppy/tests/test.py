import sys
from sweeppy import Sweep


def main():
    if len(sys.argv) < 2:
        sys.exit('python test.py /dev/ttyUSB0')

    dev = sys.argv[1]

    with Sweep(dev) as sweep:
        speed = sweep.get_motor_speed()
        rate = sweep.get_sample_rate()

        print('Motor Speed: {} Hz'.format(speed))
        print('Sample Rate: {} Hz'.format(rate))

        sweep.start_scanning()

        # get_scans is coroutine-based generator lazily returning scans ad infinitum
        for n, scan in enumerate(sweep.get_scans()):
            print('{}\n'.format(scan))

            if n == 3:
                break


if __name__ == '__main__':
    main()
