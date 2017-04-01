FROM alpine:3.5

RUN mkdir -p /opt/sweep-sdk
COPY . /opt/sweep-sdk
WORKDIR /opt/sweep-sdk

RUN apk --no-cache add cmake make gcc g++ zeromq protobuf python2 python3 nodejs && \
    cd libsweep && \
    mkdir build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . && \
    cmake --build . --target install && \
    cd ../../ && \
    cd sweeppy && \
    python2 -m ensurepip && \
    python3 -m ensurepip && \
    pip2 install setuptools && \
    pip3 install setuptools && \
    python2 setup.py install && \
    python3 setup.py install && \
    cd .. && \
    cd sweepjs && \
    npm install --unsafe-perm
