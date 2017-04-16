package io.scanse.sweep.jna;

import java.util.function.BiConsumer;
import java.util.function.BiFunction;

public class ErrorHandler {

    @FunctionalInterface
    public interface TriConsumer<I1, I2, I3> {

        void accept(I1 i1, I2 i2, I3 i3);

    }

    @FunctionalInterface
    public interface TriFunction<I1, I2, I3, O> {

        O apply(I1 i1, I2 i2, I3 i3);

    }

    private static void maybeThrow(ErrorJNAPointer error) {
        if (error == null) {
            return;
        }
        try {
            String msg = SweepJNA.sweep_error_message(error);
            if (msg != null) {
                throw new RuntimeException(msg);
            }
        } finally {
            SweepJNA.sweep_error_destruct(error);
        }
    }

    public static <I> void call(BiConsumer<I, ErrorReturnJNA> call, I in) {
        ErrorReturnJNA ptr = new ErrorReturnJNA();
        call.accept(in, ptr);
        maybeThrow(ptr.returnedError);
    }

    public static <I, O> O call(BiFunction<I, ErrorReturnJNA, O> call, I in) {
        ErrorReturnJNA ptr = new ErrorReturnJNA();
        O res = call.apply(in, ptr);
        maybeThrow(ptr.returnedError);
        return res;
    }

    public static <I1, I2> void call(TriConsumer<I1, I2, ErrorReturnJNA> call,
            I1 i1, I2 i2) {
        ErrorReturnJNA ptr = new ErrorReturnJNA();
        call.accept(i1, i2, ptr);
        maybeThrow(ptr.returnedError);
    }

    public static <I1, I2, O> O call(
            TriFunction<I1, I2, ErrorReturnJNA, O> call,
            I1 i1, I2 i2) {
        ErrorReturnJNA ptr = new ErrorReturnJNA();
        O res = call.apply(i1, i2, ptr);
        maybeThrow(ptr.returnedError);
        return res;
    }

}
