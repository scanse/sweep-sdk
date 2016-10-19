'use strict';

const sweejs = require('..');
const wss = require('ws').Server({port: 8081});


const sweep = new sweejs.Sweep();
sweep.startScanning();


function pushScanInto(ws) {
  return function push() {
    sweep.scan(2000, (err, samples) => {
      if (err) return;

      samples.forEach((sample) => {
        const message = {angle: sample.angle, distance: sample.distance};
        ws.send(JSON.stringify(message));
      });
    });
  };
}

wss.on('connection', ws => {
  setInterval(pushScanInto(ws), 2000);
});
