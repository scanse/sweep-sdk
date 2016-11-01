'use strict';

const sweepjs = require('.');
const assert = require('assert');


if (require.main === module) {
  console.log('self-testing module');

  const sweep = new sweepjs.Sweep();

  sweep.startScanning();

  sweep.scan((err, samples) => {
    if (err) {
      return console.log(err);
    }

    samples.forEach((sample) => {
      console.log(`angle: ${sample.angle}, distance: ${sample.distance}, singal: ${sample.signal}`);
    });
  });
}

module.exports = sweepjs;
