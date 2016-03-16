//
//  wrapper.js
//  For cuckooClock
//
//  Created by Eric Levin on 3/15/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script handles the logic for growing a plant when it has water poured on it
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var SCRIPT_URL = Script.resolvePath("cuckooClockEntityScript.js?v1" + Math.random())
var CLOCK_BODY_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/cuckooClock/cuckoo2_BODY.fbx";
var CLOCK_FACE_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/cuckooClock2_FACE.fbx";
var CLOCK_HOUR_HAND_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/cuckooClock2_HOUR_HAND.fbx";
var CLOCK_MINUTE_HAND_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/cuckooClock2_MINUTE_HAND.fbx";

MyCuckooClock = function(spawnPosition, spawnRotation) {
  var clockBody, clockFace, clockHourHand;

  function createClockBody() {

    clockBody = Entities.addEntity({
      type: "Model",
      modelURL: CLOCK_BODY_URL,
      animation: {
        url: CLOCK_BODY_URL,
        running: true,
        currentFrame: 100,
        loop: true
      },
      position: spawnPosition,
      dimensions: {
        x: 0.8181,
        y: 1.3662,
        z: 0.8181
      },
      script: SCRIPT_URL
    })

    var clockFaceOffset = {x: -0.0345, y: 0.2587, z: 0.1255};
    var clockFacePosition = Vec3.sum(spawnPosition, clockFaceOffset);
    clockFace = Entities.addEntity({
      type: "Model",
      modelURL: CLOCK_FACE_URL,
      animation: {
        url: CLOCK_FACE_URL,
        running: true,
        currentFrame: 0,
        loop: true
      },
      position: clockFacePosition,
      dimensions: {
        x: 0.2397,
        y: 0.2402,
        z: 0.0212
      },
      // script: SCRIPT_URL
    })

    var clockHandOffset = {x: -0.0007, y: -0.0015, z: 0.0121};

    clockHourHand = Entities.addEntity({
      type: "Model",
      modelURL: CLOCK_HOUR_HAND_URL,
      position: Vec3.sum(clockFacePosition, clockHandOffset),
      registrationPoint: {x: 0.5, y: 0.05, z: 0.5},
      dimensions: {
        x: 0.0263,
        y: 0.0982,
        z: 0.0024
      },
      // script: SCRIPT_URL
    });


    var myDate = new Date()
    var seconds = myDate.getSeconds();

    print("EBL SECONDS " + seconds)
    var rollDegrees = -seconds * 6;
    clockMinuteHand = Entities.addEntity({
      type: "Model",
      modelURL: CLOCK_MINUTE_HAND_URL,
      position: Vec3.sum(clockFacePosition, clockHandOffset),
      dimensions: {
        x: 0.0263,
        y: 0.0982,
        z: 0.0024
      },
      registrationPoint: {x: 0.5, y: 0.05, z: 0.5},
      rotation: Quat.fromPitchYawRollDegrees(0, 0, rollDegrees),
      angularDamping: 0,
      angularVelocity: {x: 0, y: 0, z: -0.10472}
      // script: SCRIPT_URL
    });

  }

  print("EBL IM A CUCKOO CLOCK");

  createClockBody();

  function cleanup() {
    print('cuckoo cleanup')
    // Entities.deleteEntity(clockBody);
    // Entities.deleteEntity(clockFace);
    // Entities.deleteEntity(clockHourHand);
    // Entities.deleteEntity(clockMinuteHand);

  }

  this.cleanup = cleanup;
  return this
}