import ctypes
import ctypes.util
import collections

libsweep = ctypes.cdll.LoadLibrary(ctypes.util.find_library('sweep'))

libsweep.sweep_get_version.restype = ctypes.c_int32
libsweep.sweep_get_version.argtypes = None

libsweep.sweep_is_abi_compatible.restype = ctypes.c_bool
libsweep.sweep_is_abi_compatible.argtypes = None

libsweep.sweep_error_message.restype = ctypes.c_char_p
libsweep.sweep_error_message.argtypes = [ctypes.c_void_p]

libsweep.sweep_error_destruct.restype = None
libsweep.sweep_error_destruct.argtypes = [ctypes.c_void_p]

libsweep.sweep_device_construct_simple.restype = ctypes.c_void_p
libsweep.sweep_device_construct_simple.argtypes = [ctypes.c_char_p, ctypes.c_void_p]

libsweep.sweep_device_construct.restype = ctypes.c_void_p
libsweep.sweep_device_construct.argtypes = [ctypes.c_char_p, ctypes.c_int32, ctypes.c_void_p]

libsweep.sweep_device_destruct.restype = None
libsweep.sweep_device_destruct.argtypes = [ctypes.c_void_p]

libsweep.sweep_device_start_scanning.restype = None
libsweep.sweep_device_start_scanning.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libsweep.sweep_device_stop_scanning.restype = None
libsweep.sweep_device_stop_scanning.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libsweep.sweep_device_get_scan.restype = ctypes.c_void_p
libsweep.sweep_device_get_scan.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libsweep.sweep_scan_destruct.restype = None
libsweep.sweep_scan_destruct.argtypes = [ctypes.c_void_p]

libsweep.sweep_scan_get_number_of_samples.restype = ctypes.c_int32
libsweep.sweep_scan_get_number_of_samples.argtypes = [ctypes.c_void_p]

libsweep.sweep_scan_get_angle.restype = ctypes.c_int32
libsweep.sweep_scan_get_angle.argtypes = [ctypes.c_void_p, ctypes.c_int32]

libsweep.sweep_scan_get_distance.restype = ctypes.c_int32
libsweep.sweep_scan_get_distance.argtypes = [ctypes.c_void_p, ctypes.c_int32]

libsweep.sweep_scan_get_signal_strength.restype = ctypes.c_int32
libsweep.sweep_scan_get_signal_strength.argtypes = [ctypes.c_void_p, ctypes.c_int32]

libsweep.sweep_device_get_motor_ready.restype = ctypes.c_bool
libsweep.sweep_device_get_motor_ready.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libsweep.sweep_device_get_motor_speed.restype = ctypes.c_int32
libsweep.sweep_device_get_motor_speed.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libsweep.sweep_device_set_motor_speed.restype = None
libsweep.sweep_device_set_motor_speed.argtypes = [ctypes.c_void_p, ctypes.c_int32, ctypes.c_void_p]

libsweep.sweep_device_get_sample_rate.restype = ctypes.c_int32
libsweep.sweep_device_get_sample_rate.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libsweep.sweep_device_set_sample_rate.restype = None
libsweep.sweep_device_set_sample_rate.argtypes = [ctypes.c_void_p, ctypes.c_int32, ctypes.c_void_p]

libsweep.sweep_device_reset.restype = None
libsweep.sweep_device_reset.argtypes = [ctypes.c_void_p, ctypes.c_void_p]


def _error_to_exception(error):
    assert error
    what = libsweep.sweep_error_message(error)
    libsweep.sweep_error_destruct(error)
    return RuntimeError(what.decode('ascii'))


class Scan(collections.namedtuple('Scan', 'samples')):
    pass


class Sample(collections.namedtuple('Sample', 'angle distance signal_strength')):
    pass


class Sweep:
    def __init__(_, port, bitrate = None):
        _.scoped = False
        _.args = [port, bitrate]

    def __enter__(_):
        _.scoped = True
        _.device = None

        assert libsweep.sweep_is_abi_compatible(), 'Your installed libsweep is not ABI compatible with these bindings'

        error = ctypes.c_void_p()

        simple = not _.args[1]
        config = all(_.args)

        assert simple or config, 'No arguments for bitrate, required'

        if simple:
            port = ctypes.string_at(_.args[0].encode('ascii'))
            device = libsweep.sweep_device_construct_simple(port, ctypes.byref(error))

        if config:
            port = ctypes.string_at(_.args[0].encode('ascii'))
            bitrate = ctypes.c_int32(_.args[1])
            device = libsweep.sweep_device_construct(port, bitrate, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

        _.device = device

        return _

    def __exit__(_, *args):
        _.scoped = False

        if _.device:
            libsweep.sweep_device_destruct(_.device)

    def _assert_scoped(_):
        assert _.scoped, 'Use the `with` statement to guarantee for deterministic resource management'

    def start_scanning(_):
        _._assert_scoped()

        error = ctypes.c_void_p();
        libsweep.sweep_device_start_scanning(_.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

    def stop_scanning(_):
        _._assert_scoped()

        error = ctypes.c_void_p()
        libsweep.sweep_device_stop_scanning(_.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

    def get_motor_ready(_):
        _._assert_scoped()

        error = ctypes.c_void_p()
        is_ready = libsweep.sweep_device_get_motor_ready(_.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

        return is_ready

    def get_motor_speed(_):
        _._assert_scoped()

        error = ctypes.c_void_p()
        speed = libsweep.sweep_device_get_motor_speed(_.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

        return speed

    def set_motor_speed(_, speed):
        _._assert_scoped()

        error = ctypes.c_void_p()
        libsweep.sweep_device_set_motor_speed(_.device, speed, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

    def get_sample_rate(_):
        _._assert_scoped()

        error = ctypes.c_void_p()
        speed = libsweep.sweep_device_get_sample_rate(_.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

        return speed

    def set_sample_rate(_, speed):
        _._assert_scoped()

        error = ctypes.c_void_p()
        libsweep.sweep_device_set_sample_rate(_.device, speed, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

    def get_scans(_):
        _._assert_scoped()

        error = ctypes.c_void_p()

        while True:
            scan = libsweep.sweep_device_get_scan(_.device, ctypes.byref(error))

            if error:
                raise _error_to_exception(error)

            num_samples = libsweep.sweep_scan_get_number_of_samples(scan)

            samples = [Sample(angle=libsweep.sweep_scan_get_angle(scan, n),
                              distance=libsweep.sweep_scan_get_distance(scan, n),
                              signal_strength=libsweep.sweep_scan_get_signal_strength(scan, n))
                       for n in range(num_samples)]

            libsweep.sweep_scan_destruct(scan)

            yield Scan(samples=samples)


    def reset(_):
        _._assert_scoped()

        error = ctypes.c_void_p()
        libsweep.sweep_device_reset(_.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)
