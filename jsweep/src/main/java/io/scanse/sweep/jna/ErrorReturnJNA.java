package io.scanse.sweep.jna;

import java.util.Arrays;
import java.util.List;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

public class ErrorReturnJNA extends Structure {

    private static class EJNAPtrPtr extends ErrorJNAPointer
            implements Structure.ByReference {
        private EJNAPtrPtr(Pointer p) {
            super(p);
        }
    }

    // the actual pointer at the location, opaque data
    public EJNAPtrPtr returnedError;

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("returnedError");
    }

}
