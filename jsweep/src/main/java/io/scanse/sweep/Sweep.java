package io.scanse.sweep;

import io.scanse.sweep.jna.SweepJNA;

public class Sweep {

    public static final int VERSION = SweepJNA.sweep_get_version();
    public static final boolean ABI_COMPATIBLE =
            SweepJNA.sweep_is_abi_compatible();

}
