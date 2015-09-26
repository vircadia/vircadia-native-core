//
//  collectHifiStats.js
//
//  Created by Thijs Wenker on 24 Sept 2015
//  Additions by James B. Pollack @imgntn on 25 Sept 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Collects stats for analysis to a REST endpoint.  Defaults to batching stats, but can be customized.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// The url where the data will be posted.
var ENDPOINT_URL = "";

var BATCH_STATS = true;
var BATCH_SIZE = 5;

var batch = [];

if (BATCH_STATS) {

  var RECORD_EVERY = 1000; // 1 seconds
  var batchCount = 0;

  Script.setInterval(function() {

    if (batchCount === BATCH_SIZE) {
      sendBatchToEndpoint(batch);
      batchCount = 0;
    }
    Stats.forceUpdateStats();
    batch.push(getBatchStats());
    batchCount++;
  }, RECORD_EVERY);


} else {
  // Send the data every:
  var SEND_EVERY = 30000; // 30 seconds

  Script.setInterval(function() {
    Stats.forceUpdateStats();
    var req = new XMLHttpRequest();
    req.open("POST", ENDPOINT_URL, false);
    req.send(getStats());
  }, SEND_EVERY);
}

function getStats() {
  return JSON.stringify({
    username: GlobalServices.username,
    location: Window.location.hostname,
    framerate: Stats.framerate,
    simrate: Stats.simrate,
    ping: {
      audio: Stats.audioPing,
      avatar: Stats.avatarPing,
      entities: Stats.entitiesPing,
      asset: Stats.assetPing
    },
    position: Camera.position,
    yaw: Stats.yaw,
    rotation: Camera.orientation.x + ',' + Camera.orientation.y + ',' + Camera.orientation.z + ',' + Camera.orientation.w
  })
}

function getBatchStats() {
  // print('GET BATCH STATS');
  return {
    username: GlobalServices.username,
    location: Window.location.hostname,
    framerate: Stats.framerate,
    simrate: Stats.simrate,
    ping: {
      audio: Stats.audioPing,
      avatar: Stats.avatarPing,
      entities: Stats.entitiesPing,
      asset: Stats.assetPing
    },
    position: Camera.position,
    yaw: Stats.yaw,
    rotation: Camera.orientation.x + ',' + Camera.orientation.y + ',' + Camera.orientation.z + ',' + Camera.orientation.w
  }
}

function sendBatchToEndpoint(batch) {
  // print('SEND BATCH TO ENDPOINT');
  var req = new XMLHttpRequest();
  req.open("POST", ENDPOINT_URL, false);
  req.send(JSON.stringify(batch));
  batch = [];
}
