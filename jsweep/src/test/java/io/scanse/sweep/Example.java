package io.scanse.sweep;

import java.util.List;

public class Example {

    public static void main(String[] args) throws Exception {
        try (SweepDevice device = new SweepDevice("/dev/ttyUSB0")) {
            int speed = device.getMotorSpeed();
            int rate = device.getSampleRate();

            System.out.println(String.format("Motor Speed: %s Hz", speed));
            System.out.println(String.format("Sample Rate: %s Hz", rate));

            device.startScanning();

            for (List<SweepSample> s : device.scans()) {
                System.err.println(s);
            }
        }
    }

}
