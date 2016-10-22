'use strict';

const sweejs = require('..');

const koa = require('koa');
const srv = require('koa-static');
const wss = require('ws').Server({port: 5000});


const app = koa();
app.use(srv(__dirname, {index: 'map.html'}));
app.listen(8080);


const sweep = new sweejs.Sweep();
sweep.startScanning();


function pushScanInto(ws) {
  return function push() {

    sweep.scan(2000, (err, samples) => {
      if (err) return;

      const latitude = 52.511433;
      const longitude = 13.389726;

      samples.forEach((sample) => {

        // TODO: convert angle+distance to height => sin(radians(angle)) * distance
        // TODO: convert angle+distance to distance-to-left/right => cos(radians(angle)) * distance

        const message = {latitude: latitude, longitude: longitude, angle: sample.angle, distance: sample.distance};
        ws.send(JSON.stringify(message), error => {/**/});
      });
    });
  };
}

wss.on('connection', ws => {
  setInterval(pushScanInto(ws), 2000);
});

console.log('Now open browser at http://localhost:8080')
