package io.scanse.sweep.jna;

import java.util.Arrays;
import java.util.List;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

/**
 * Pointer to a scan object with unknown content, must be queried through API.
 */
public class ScanJNAPointer extends Structure {

    public Pointer scan;

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("scan");
    }

}
