from sweeppy import Sweep


def main():
    with Sweep() as sweep:

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
