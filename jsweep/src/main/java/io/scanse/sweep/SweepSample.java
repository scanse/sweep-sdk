package io.scanse.sweep;

public final class SweepSample {

    private final int angle;
    private final int distance;
    private final int signalStrength;

    public SweepSample(int angle, int distance, int signalStrength) {
        this.angle = angle;
        this.distance = distance;
        this.signalStrength = signalStrength;
    }

    public int getAngle() {
        return angle;
    }

    public int getDistance() {
        return distance;
    }

    public int getSignalStrength() {
        return signalStrength;
    }

    @Override
    public String toString() {
        return "Sample[angle=" + angle + ",distance=" + distance
                + ",signalStrength=" + signalStrength + "]";
    }

}
