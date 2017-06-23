# Based upon the manylinux image from pypa
FROM quay.io/pypa/manylinux1_x86_64

COPY docker /opt/sweep/bin
COPY libsweep /opt/sweep/libsweep
COPY jsweep /opt/sweep/jsweep

ENV PATH /opt/sweep/bin:$PATH

RUN install-yum.sh

CMD build.sh
