'use strict';

const sweejs = require('..');

const koa = require('koa');
const srv = require('koa-static');
const wss = require('ws').Server({port: 5000});


const app = koa();
app.use(srv('.', {index: 'map.html'}));
app.listen(8080);


const sweep = new sweejs.Sweep();
sweep.startScanning();


function pushScanInto(ws) {
  return function push() {
    sweep.scan(2000, (err, samples) => {
      if (err) return;

      samples.forEach((sample) => {
        const message = {angle: sample.angle, distance: sample.distance};
        ws.send(JSON.stringify(message), error => {/**/});
      });
    });
  };
}

wss.on('connection', ws => {
  setInterval(pushScanInto(ws), 2000);
});
