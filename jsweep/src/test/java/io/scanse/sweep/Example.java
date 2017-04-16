package io.scanse.sweep;

public class Example {

    public static void main(String[] args) {
        try (SweepDevice device = new SweepDevice("/dev/ttyUSB0")) {
            device.startScanning();

            device.scans().forEach(System.err::println);
        }
    }

}
