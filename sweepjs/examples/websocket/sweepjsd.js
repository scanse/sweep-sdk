'use strict';

const sweepjs = require('../..');

const koa = require('koa');
const srv = require('koa-static');
const wss = require('ws').Server({port: 5000});


const app = koa();
app.use(srv(__dirname, {index: 'index.html'}));
app.listen(8080);


const sweep = new sweepjs.Sweep('/dev/ttyUSB0');
sweep.startScanning();


function pushScanInto(ws) {
  return function push() {

    sweep.scan((err, samples) => {
      if (err) return;

      samples.forEach((sample) => {
        // x = cos(angle) * distance
        // y = sin(angle) * distance
        const message = {angle: sample.angle, distance: sample.distance};
        ws.send(JSON.stringify(message), error => {/**/});
      });
    });
  };
}

wss.on('connection', ws => {
  setInterval(pushScanInto(ws), 2000);
});

console.log('Now open browser at http://localhost:8080')
