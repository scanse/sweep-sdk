package io.scanse.sweep;

import java.util.List;

public class Example {

    public static void main(String[] args) throws Exception {
        try (SweepDevice device = new SweepDevice("/dev/ttyUSB0")) {
            device.startScanning();
            while (!device.isMotorReady()) {
                Thread.sleep(10);
            }

            for (List<SweepSample> s : device.scans()) {
                System.err.println(s);
            }
        }
    }

}
