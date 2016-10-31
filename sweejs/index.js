'use strict';

const sweejs = require('.');
const assert = require('assert');


if (require.main === module) {
  console.log('self-testing module');

  const sweep = new sweejs.Sweep();

  sweep.startScanning();
  sweep.stopScanning();
  sweep.startScanning();

  const speed = sweep.getMotorSpeed();

  sweep.setMotorSpeed(speed + 1);

  const newspeed = sweep.getMotorSpeed();

  assert.equal(newspeed, speed + 1);

  const rate = sweep.getSampleRate();

  sweep.scan(2000, (err, samples) => {
    if (err) {
      return console.log(err);
    }

    samples.forEach((sample) => {
      console.log(`angle: ${sample.angle}, distance: ${sample.distance}, singal: ${sample.signal}`);
    });
  });
}

module.exports = sweejs;
