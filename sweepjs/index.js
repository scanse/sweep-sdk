'use strict';

var sweepjs = require('.');
var assert = require('assert');
var util = require('util')


if (require.main === module) {
  console.log('self-testing module');

  var sweep = new sweepjs.Sweep();

  sweep.startScanning();

  sweep.scan(function (err, samples) {
    if (err) {
      return console.log(err);
    }

    samples.forEach(function (sample) {
      var fmt = util.format('angle: %d distance %d signal strength: %d',
                            sample.angle, sample.distance, sample.signal);
      console.log(fmt);
    });
  });
}

module.exports = sweepjs;
