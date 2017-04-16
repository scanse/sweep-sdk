package io.scanse.sweep.jna;

import java.util.Arrays;
import java.util.List;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

public class ErrorJNAPointer extends Structure {

    public ErrorJNAPointer() {
    }

    public ErrorJNAPointer(Pointer p) {
        super(p);
    }

    // the actual pointer at the location, opaque data
    public Pointer ref;

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("ref");
    }

}
