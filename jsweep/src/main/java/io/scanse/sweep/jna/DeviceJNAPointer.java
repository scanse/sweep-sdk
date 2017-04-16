package io.scanse.sweep.jna;

import java.util.Arrays;
import java.util.List;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

public class DeviceJNAPointer extends Structure {

    public Pointer device;

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("device");
    }

}
