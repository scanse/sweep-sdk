'use strict';

const sweejs = require('node-cmake')('sweejs');
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

  sweep.reset(); // this resets the hardware and is not needed to shutdown the library
}

module.exports = sweejs;
