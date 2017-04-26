'use strict';

var sweepjs = require('.');
var assert = require('assert');
var util = require('util')


if (require.main === module) {
  console.log('self-testing module');

  var portName = process.argv[2];
  var sweep = new sweepjs.Sweep(portName);

  var speed = sweep.getMotorSpeed();
  var rate = sweep.getSampleRate();

  console.log(util.format('Motor speed: %d Hz', speed));
  console.log(util.format('Sample rate: %d Hz', rate));

  console.log('Starting data acquisition as soon as motor is ready...');
  sweep.startScanning();
  
  console.log('Retrieving 1 scan...');
  sweep.scan(function (err, samples) {
    if (err) {
      console.log(err);
      return;
    }

    console.log("Printing samples from first (partial) scan...");
    samples.forEach(function (sample) {
      var fmt = util.format('angle: %d distance %d signal strength: %d',
        sample.angle, sample.distance, sample.signal);
      console.log(fmt);
    });
  
    console.log('Stopping data acquisition...');
    sweep.stopScanning();
  });
}

module.exports = sweepjs;
