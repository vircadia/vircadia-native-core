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

var MINUTE_HAND_CLOCK_SCRIPT_URL = Script.resolvePath("cuckooClockMinuteHandEntityScript.js?v1" + Math.random())
var CLOCK_BODY_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/cuckooClock/cuckoo2_BODY.fbx";
var CLOCK_FACE_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/cuckooClock2_FACE.fbx";
var CLOCK_HOUR_HAND_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/cuckooClock2_HOUR_HAND.fbx";
var CLOCK_MINUTE_HAND_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/cuckooClock2_MINUTE_HAND.fbx";

MyCuckooClock = function(spawnPosition, spawnRotation) {
  var clockBody, clockFace, clockMinuteHand, clockHourHand, clockSecondHand;

  function createClock() {

    clockBody = Entities.addEntity({
      type: "Model",
      modelURL: CLOCK_BODY_URL,
      name: "home_model_clockbody",
      animation: {
        url: CLOCK_BODY_URL,
        running: false,
        currentFrame: 0,
        firstFrame: 0,
        lastFrame: 100,
        loop: false
      },
      position: spawnPosition,
      dimensions: {
        x: 0.8181,
        y: 1.3662,
        z: 0.8181
      },
    })

    var clockFaceOffset = {
      x: -0.0345,
      y: 0.2587,
      z: 0.1255
    };
    var clockFacePosition = Vec3.sum(spawnPosition, clockFaceOffset);
    clockFace = Entities.addEntity({
      type: "Model",
      name: "home_model_clockface",
      modelURL: CLOCK_FACE_URL,
      position: clockFacePosition,
      dimensions: {
        x: 0.2397,
        y: 0.2402,
        z: 0.0212
      }    
    });

    // CLOCK HANDS
    // __________
    // _\|/_
    //  /|\
    // ___________

    var clockHandOffset = {
      x: -0.0007,
      y: -0.0015,
      z: 0.0121
    };
    var myDate = new Date()

    // HOUR HAND *************************
    var DEGREES_FOR_HOUR = 30
    var hours = myDate.getHours();
    var hourRollDegrees = -hours * DEGREES_FOR_HOUR;
    var ANGULAR_ROLL_SPEED_HOUR_RADIANS = 0.000029098833;
    clockHourHand = Entities.addEntity({
      type: "Model",
      name: "home_model_clockHourHand",
      modelURL: CLOCK_HOUR_HAND_URL,
      position: Vec3.sum(clockFacePosition, clockHandOffset),
      registrationPoint: {
        x: 0.5,
        y: 0.05,
        z: 0.5
      },
      rotation: Quat.fromPitchYawRollDegrees(0, 0, hourRollDegrees),
      angularDamping: 0,
      angularVelocity: {
        x: 0,
        y: 0,
        z: -ANGULAR_ROLL_SPEED_HOUR_RADIANS
      },
      dimensions: {
        x: 0.0263,
        y: 0.0982,
        z: 0.0024
      },
      // script: SCRIPT_URL
    });


    // MINUTE HAND ************************
    var DEGREES_FOR_MINUTE = 6;
    var minutes = myDate.getMinutes();
    var minuteRollDegrees = -minutes * DEGREES_FOR_MINUTE;
    var ANGULAR_ROLL_SPEED_MINUTE_RADIANS = 0.00174533;
    clockMinuteHand = Entities.addEntity({
      type: "Model",
      modelURL: CLOCK_MINUTE_HAND_URL,
      name: "home_model_clockMinuteHand",
      position: Vec3.sum(clockFacePosition, clockHandOffset),
      registrationPoint: {
        x: 0.5,
        y: 0.05,
        z: 0.5
      },
      rotation: Quat.fromPitchYawRollDegrees(0, 0, minuteRollDegrees),
      angularDamping: 0,
      angularVelocity: {
        x: 0,
        y: 0,
        z: -ANGULAR_ROLL_SPEED_MINUTE_RADIANS
      },
      dimensions: {
        x: 0.0251,
        y: 0.1179,
        z: 0.0032
      },
      script: MINUTE_HAND_CLOCK_SCRIPT_URL,
      userData: JSON.stringify({clockBody: clockBody})
    });
    // *******************************************

    var DEGREES_FOR_SECOND = 6;
    var seconds = myDate.getSeconds();
    secondRollDegrees = -seconds * DEGREES_FOR_SECOND;
    var ANGULAR_ROLL_SPEED_SECOND_RADIANS = 0.10472
    clockSecondHand = Entities.addEntity({
      type: "Box",
      // modelURL: CLOCK_SECOND_HAND_URL,
      name: "home_model_clockSecondHand",
      position: Vec3.sum(clockFacePosition, clockHandOffset),
      dimensions: {
        x: 0.00263,
        y: 0.0982,
        z: 0.0024
      },
      color: {
        red: 200,
        green: 10,
        blue: 200
      },
      registrationPoint: {
        x: 0.5,
        y: 0.05,
        z: 0.5
      },
      rotation: Quat.fromPitchYawRollDegrees(0, 0, secondRollDegrees),
      angularDamping: 0,
      angularVelocity: {
        x: 0,
        y: 0,
        z: -ANGULAR_ROLL_SPEED_SECOND_RADIANS
      }
    });

  }

  createClock();

  function cleanup() {
    print('cuckoo cleanup')
    Entities.deleteEntity(clockBody);
    Entities.deleteEntity(clockFace);
    Entities.deleteEntity(clockHourHand);
    Entities.deleteEntity(clockMinuteHand);
    Entities.deleteEntity(clockSecondHand);

  }

  this.cleanup = cleanup;
  return this
}