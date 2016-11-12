# SweepJs

NodeJS Scanse Sweep LiDAR library.

Requires `libsweep.so` to be installed.

### Quick Start

    npm install
    npm test

See the [example](example) directory for an example streaming data from the device to the browser in real-time using a Websocket server.

### Interface

```javascript
sweep = new Sweep();

sweep.startScanning();
sweep.stopScanning();

speed = sweep.getMotorSpeed();
sweep.setMotorSpeed(Number);

sweep.scan(function (err, samples) {
  handle(err);

  samples.forEach(function (sample) {
    use(sample.angle, sample.distance, sample.signal);
  });
});

sweep.reset();
```

### License

Copyright © 2016 Daniel J. Hofmann

Distributed under the MIT License (MIT).
