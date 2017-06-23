package io.scanse.sweep.jna;

import com.sun.jna.Native;

public class SweepJNA {

    static {
        Native.register("sweep");
    }

    public static native int sweep_get_version();

    public static native boolean sweep_is_abi_compatible();

    public static native String sweep_error_message(ErrorJNAPointer error);

    public static native void sweep_error_destruct(ErrorJNAPointer error);

    public static native DeviceJNAPointer sweep_device_construct_simple(String port,
            ErrorReturnJNA error);

    public static native DeviceJNAPointer sweep_device_construct(String port,
            int bitrate, ErrorReturnJNA error);

    public static native void sweep_device_destruct(DeviceJNAPointer device);

    public static native void sweep_device_start_scanning(
            DeviceJNAPointer device,
            ErrorReturnJNA error);

    public static native void sweep_device_stop_scanning(
            DeviceJNAPointer device,
            ErrorReturnJNA error);

    public static native boolean sweep_device_get_motor_ready(
            DeviceJNAPointer device,
            ErrorReturnJNA error);

    public static native ScanJNAPointer sweep_device_get_scan(DeviceJNAPointer device,
            ErrorReturnJNA error);

    public static native void sweep_scan_destruct(ScanJNAPointer scan);

    public static native int
            sweep_scan_get_number_of_samples(ScanJNAPointer scan);

    public static native int sweep_scan_get_angle(ScanJNAPointer scan,
            int sample);

    public static native int sweep_scan_get_distance(ScanJNAPointer scan,
            int sample);

    public static native int sweep_scan_get_signal_strength(ScanJNAPointer scan,
            int sample);

    public static native int sweep_device_get_motor_speed(
            DeviceJNAPointer device,
            ErrorReturnJNA error);

    public static native void sweep_device_set_motor_speed(
            DeviceJNAPointer device,
            int hz, ErrorReturnJNA error);

    public static native int sweep_device_get_sample_rate(
            DeviceJNAPointer device,
            ErrorReturnJNA error);

    public static native void sweep_device_set_sample_rate(
            DeviceJNAPointer device,
            int hz, ErrorReturnJNA error);

    public static native void sweep_device_reset(DeviceJNAPointer device,
            ErrorReturnJNA error);

}
