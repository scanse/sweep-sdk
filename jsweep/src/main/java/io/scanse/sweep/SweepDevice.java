package io.scanse.sweep;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Objects;

import com.google.common.collect.AbstractIterator;

import io.scanse.sweep.jna.DeviceJNAPointer;
import io.scanse.sweep.jna.ErrorHandler;
import io.scanse.sweep.jna.ScanJNAPointer;
import io.scanse.sweep.jna.SweepJNA;

public class SweepDevice implements AutoCloseable {

    private DeviceJNAPointer handle;

    public SweepDevice(String port) {
        this.handle = ErrorHandler.call(SweepJNA::sweep_device_construct_simple,
                port);
    }

    public SweepDevice(String port, int bitrate) {
        this.handle = ErrorHandler.call(SweepJNA::sweep_device_construct,
                port, bitrate);
    }

    private void checkHandle() {
        Objects.requireNonNull(this.handle, "handle was destroyed");
    }

    public void startScanning() {
        checkHandle();
        if (!Sweep.ABI_COMPATIBLE) {
            throw new AssertionError("Your installed libsweep is not ABI compatible with these bindings");
        }
        ErrorHandler.call(SweepJNA::sweep_device_start_scanning, this.handle);
    }

    public void stopScanning() {
        checkHandle();
        ErrorHandler.call(SweepJNA::sweep_device_stop_scanning, this.handle);
    }

    public List<SweepSample> nextScan() {
        checkHandle();
        ScanJNAPointer scan =
                ErrorHandler.call(SweepJNA::sweep_device_get_scan, this.handle);
        int count = SweepJNA.sweep_scan_get_number_of_samples(scan);
        List<SweepSample> samples = new ArrayList<>(count);
        for (int i = 0; i < count; i++) {
            int angle = SweepJNA.sweep_scan_get_angle(scan, i);
            int dist = SweepJNA.sweep_scan_get_distance(scan, i);
            int sigStr = SweepJNA.sweep_scan_get_signal_strength(scan, i);
            samples.add(new SweepSample(angle, dist, sigStr));
        }
        return samples;
    }

    public Iterable<List<SweepSample>> scans() {
        return this::iterator;
    }

    private Iterator<List<SweepSample>> iterator() {
        return new AbstractIterator<List<SweepSample>>() {

            @Override
            protected List<SweepSample> computeNext() {
                return nextScan();
            }
        };
    }

    public int getMotorSpeed() {
        checkHandle();
        return ErrorHandler.call(SweepJNA::sweep_device_get_motor_speed,
                this.handle);
    }

    public void setMotorSpeed(int hz) {
        checkHandle();
        ErrorHandler.call(SweepJNA::sweep_device_set_motor_speed, this.handle,
                hz);
    }

    public int getSampleRate() {
        checkHandle();
        return ErrorHandler.call(SweepJNA::sweep_device_get_sample_rate,
                this.handle);
    }

    public void setSampleRate(int hz) {
        checkHandle();
        ErrorHandler.call(SweepJNA::sweep_device_set_sample_rate, this.handle,
                hz);
    }

    public void reset() {
        checkHandle();
        ErrorHandler.call(SweepJNA::sweep_device_reset, this.handle);
    }

    @Override
    public void close() {
        if (handle != null) {
            SweepJNA.sweep_device_destruct(handle);
            handle = null;
        }
    }

}
