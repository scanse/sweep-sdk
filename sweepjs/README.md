# SweepJs

NodeJS Scanse Sweep LiDAR library.

Requires `libsweep.so` to be installed.

### Quick Start

On Linux:

```bash
npm install
npm test
```

See the [examples](examples) directory for an example streaming data from the device to the browser in real-time using a Websocket server.

On Windows:

The verison (x86 or x64) of the installed libsweep library must match the installed version of node-gyp. The provided `binding.gyp` file supports the default installation directories shown below.

| Files        | Installation Directory (x64)             | Installation Directory (x86)                   |
| ------------ | :--------------------------------------: | :--------------------------------------------: |
| header files | C:/Program Files/sweep/includes/sweep/   | C:/Program Files (x86)/sweep/includes/sweep/   |
| lib files    | C:/Program Files/sweep/lib/              | C:/Program Files (x86)/sweep/lib/              |

If your installation path differs, modify the `binding.gyp` file.

Install module and run tests with your device's portname. For a device on com port 5:

```bash
npm install
node index COM5
```


### Interface

```javascript
sweep = new Sweep('/dev/ttyUSB0');

sweep.startScanning();
sweep.stopScanning();

speed = sweep.getMotorSpeed();
sweep.setMotorSpeed(Number);

rate = sweep.getSampleRate();
sweep.setSampleRate(Number);

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
