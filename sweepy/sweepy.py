import ctypes

libsweep = ctypes.cdll.LoadLibrary('libsweep.so')

libsweep.sweep_get_version.restype = ctypes.c_int32
libsweep.sweep_get_version.argtypes = None

libsweep.sweep_is_abi_compatible.restype = ctypes.c_bool
libsweep.sweep_is_abi_compatible.argtypes = None

libsweep.sweep_error_message.restype = ctypes.c_char_p
libsweep.sweep_error_message.argtypes = [ctypes.c_void_p]

libsweep.sweep_error_destruct.restype = None
libsweep.sweep_error_destruct.argtypes = [ctypes.c_void_p]

libsweep.sweep_device_construct_simple.restype = ctypes.c_void_p
libsweep.sweep_device_construct_simple.argtypes = [ctypes.c_void_p]

libsweep.sweep_device_construct.restype = ctypes.c_void_p
libsweep.sweep_device_construct.argtypes = [ctypes.c_char_p, ctypes.c_int32, ctypes.c_int32, ctypes.c_void_p]

libsweep.sweep_device_destruct.restype = None
libsweep.sweep_device_destruct.argtypes = [ctypes.c_void_p]

libsweep.sweep_device_start_scanning.restype = None
libsweep.sweep_device_start_scanning.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libsweep.sweep_device_stop_scanning.restype = None
libsweep.sweep_device_stop_scanning.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libsweep.sweep_device_get_scan.restype = ctypes.c_void_p
libsweep.sweep_device_get_scan.argtypes = [ctypes.c_void_p, ctypes.c_int32, ctypes.c_void_p]

libsweep.sweep_scan_destruct.restype = None
libsweep.sweep_scan_destruct.argtypes = [ctypes.c_void_p]

libsweep.sweep_scan_get_number_of_samples.restype = ctypes.c_int32
libsweep.sweep_scan_get_number_of_samples.argtypes = [ctypes.c_void_p]

libsweep.sweep_scan_get_angle.restype = ctypes.c_int32
libsweep.sweep_scan_get_angle.argtypes = [ctypes.c_void_p, ctypes.c_int32]

libsweep.sweep_scan_get_distance.restype = ctypes.c_int32
libsweep.sweep_scan_get_distance.argtypes = [ctypes.c_void_p, ctypes.c_int32]

libsweep.sweep_device_get_motor_speed.restype = ctypes.c_int32
libsweep.sweep_device_get_motor_speed.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libsweep.sweep_device_set_motor_speed.restype = None
libsweep.sweep_device_set_motor_speed.argtypes = [ctypes.c_void_p, ctypes.c_int32, ctypes.c_void_p]

libsweep.sweep_device_get_sample_rate.restype = ctypes.c_int32
libsweep.sweep_device_get_sample_rate.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libsweep.sweep_device_reset.restype = None
libsweep.sweep_device_reset.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

if __name__ == '__main__':
    print('self-testing module')
    print('version: {}'.format(libsweep.sweep_get_version()))
    print('abi compatible: {}'.format(libsweep.sweep_is_abi_compatible()))

    error = ctypes.c_void_p()

    device = libsweep.sweep_device_construct_simple(ctypes.byref(error));
    assert not error

    libsweep.sweep_device_start_scanning(device, ctypes.byref(error));
    assert not error

    libsweep.sweep_device_stop_scanning(device, ctypes.byref(error));
    assert not error

    libsweep.sweep_device_start_scanning(device, ctypes.byref(error));
    assert not error

    speed = libsweep.sweep_device_get_motor_speed(device, ctypes.byref(error))
    assert not error

    libsweep.sweep_device_set_motor_speed(device, speed + 1, ctypes.byref(error))
    assert not error

    newspeed = libsweep.sweep_device_get_motor_speed(device, ctypes.byref(error))
    assert not error
    assert newspeed == speed + 1

    rate = libsweep.sweep_device_get_sample_rate(device, ctypes.byref(error))
    assert not error

    scan = libsweep.sweep_device_get_scan(device, 2000, ctypes.byref(error))
    assert not error

    samples = libsweep.sweep_scan_get_number_of_samples(scan, ctypes.byref(error))
    assert not error

    for n in range(samples):
        print('angle: {}'.format(libsweep.sweep_scan_get_angle(scan, n)))
        print('distance: {}'.format(libsweep.sweep_scan_get_distance(scan, n)))

    libsweep.sweep_scan_destruct(scan)

    libsweep.sweep_device_reset(device, ctypes.byref(error))
    assert not error

    libsweep.sweep_device_destruct(device)
