package io.scanse.sweep;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Objects;

import io.scanse.sweep.jna.DeviceJNAPointer;
import io.scanse.sweep.jna.ErrorJNAPointer;
import io.scanse.sweep.jna.ErrorReturnJNA;
import io.scanse.sweep.jna.ScanJNAPointer;
import io.scanse.sweep.jna.SweepJNA;

public class SweepDevice implements AutoCloseable {

    private static final ThreadLocal<ErrorReturnJNA> ERROR = new ThreadLocal<ErrorReturnJNA>() {

        @Override
        protected ErrorReturnJNA initialValue() {
            return new ErrorReturnJNA();
        }
    };

    private static <T> T maybeThrow(T result) {
        ErrorJNAPointer error = ERROR.get().returnedError;
        if (error == null) {
            return result;
        }
        try {
            String msg = SweepJNA.sweep_error_message(error);
            if (msg != null) {
                throw new RuntimeException(msg);
            }
        } finally {
            SweepJNA.sweep_error_destruct(error);
        }
        return result;
    }

    private DeviceJNAPointer handle;

    public SweepDevice(String port) {
        this.handle = maybeThrow(SweepJNA.sweep_device_construct_simple(port, ERROR.get()));
    }

    public SweepDevice(String port, int bitrate) {
        this.handle = maybeThrow(SweepJNA.sweep_device_construct(port, bitrate, ERROR.get()));
    }

    private void checkHandle() {
        Objects.requireNonNull(this.handle, "handle was destroyed");
    }

    public void startScanning() {
        checkHandle();
        if (!Sweep.ABI_COMPATIBLE) {
            throw new AssertionError(
                    "Your installed libsweep is not ABI compatible with these bindings");
        }
        SweepJNA.sweep_device_start_scanning(this.handle, ERROR.get());
        maybeThrow(null);
    }

    public void stopScanning() {
        checkHandle();
        SweepJNA.sweep_device_stop_scanning(this.handle, ERROR.get());
        maybeThrow(null);
    }

    public boolean isMotorReady() {
        checkHandle();
        return maybeThrow(SweepJNA.sweep_device_get_motor_ready(this.handle, ERROR.get()));
    }

    public List<SweepSample> nextScan() {
        checkHandle();
        ScanJNAPointer scan = maybeThrow(SweepJNA.sweep_device_get_scan(this.handle, ERROR.get()));
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

    private final Iterable<List<SweepSample>> iterable = new Iterable<List<SweepSample>>() {

        @Override
        public Iterator<List<SweepSample>> iterator() {
            return new Iterator<List<SweepSample>>() {

                private boolean hasErrored = false;

                @Override
                public List<SweepSample> next() {
                    if (hasErrored) {
                        throw new NoSuchElementException(
                                "An error previously occurred.");
                    }
                    try {
                        return nextScan();
                    } catch (RuntimeException e) {
                        hasErrored = true;
                        throw e;
                    }
                }

                @Override
                public boolean hasNext() {
                    return !hasErrored;
                }

                @Override
                public void remove() {
                    throw new UnsupportedOperationException();
                }

            };
        }
    };

    public Iterable<List<SweepSample>> scans() {
        return iterable;
    }

    public int getMotorSpeed() {
        checkHandle();
        return maybeThrow(SweepJNA.sweep_device_get_motor_speed(this.handle, ERROR.get()));
    }

    public void setMotorSpeed(int hz) {
        checkHandle();
        SweepJNA.sweep_device_set_motor_speed(this.handle, hz, ERROR.get());
        maybeThrow(null);
    }

    public int getSampleRate() {
        checkHandle();
        return maybeThrow(SweepJNA.sweep_device_get_sample_rate(this.handle, ERROR.get()));
    }

    public void setSampleRate(int hz) {
        checkHandle();
        SweepJNA.sweep_device_set_sample_rate(this.handle, hz, ERROR.get());
        maybeThrow(null);
    }

    public void reset() {
        checkHandle();
        SweepJNA.sweep_device_reset(this.handle, ERROR.get());
        maybeThrow(null);
    }

    @Override
    public void close() {
        if (handle != null) {
            SweepJNA.sweep_device_destruct(handle);
            handle = null;
        }
    }

}
