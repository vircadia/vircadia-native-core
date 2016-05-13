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

var MINUTE_HAND_CLOCK_SCRIPT_URL = Script.resolvePath("cuckooClockMinuteHandEntityScript.js")
var CLOCK_BODY_URL = "atp:/cuckooClock/cuckoo2_BODY.fbx";
var CLOCK_BODY_COLLISION_HULL_URL = "atp:/cuckooClock/clockHull.obj";
var CLOCK_FACE_URL = "atp:/cuckooClock/cuckooClock2_FACE.fbx";
var CLOCK_HOUR_HAND_URL = "atp:/cuckooClock/cuckooClock2_HOUR_HAND.fbx";
var CLOCK_MINUTE_HAND_URL = "atp:/cuckooClock/cuckooClock2_MINUTE_HAND.fbx";
var CLOCK_SECOND_HAND_URL = "atp:/cuckooClock/secondHand.fbx";

MyCuckooClock = function(spawnPosition, spawnRotation) {
  var clockBody, clockFace, clockMinuteHand, clockHourHand, clockSecondHand;

  function createClock() {
    var clockRotation = Quat.fromPitchYawRollDegrees(spawnRotation.x, spawnRotation.y, spawnRotation.z);
    clockBody = Entities.addEntity({
      type: "Model",
      modelURL: CLOCK_BODY_URL,
      shapeType: "compound",
      compoundShapeURL: CLOCK_BODY_COLLISION_HULL_URL,
      name: "home_model_clockBody",
      animation: {
        url: CLOCK_BODY_URL,
        running: false,
        currentFrame: 0,
        firstFrame: 0,
        lastFrame: 100,
        loop: false
      },
      position: spawnPosition,
      rotation: clockRotation,
      dimensions: Vec3.multiply(0.5, {
        x: 0.8181,
        y: 1.3662,
        z: 0.8181
      }),
      userData: JSON.stringify({
        hifiHomeKey: {
          reset: true
        }
      })
    })

    var forwardOffset = 0.5 * -0.13;
    var upOffset = 0.5 * 0.255;
    var sideOffset = 0.5 * -0.03;
    var clockFacePosition = spawnPosition;
    clockFacePosition = Vec3.sum(clockFacePosition, Vec3.multiply(Quat.getFront(clockRotation), forwardOffset));
    clockFacePosition = Vec3.sum(clockFacePosition, Vec3.multiply(Quat.getUp(clockRotation), upOffset));
    clockFacePosition = Vec3.sum(clockFacePosition, Vec3.multiply(Quat.getRight(clockRotation), sideOffset));

    clockFace = Entities.addEntity({
      type: "Model",
      parentID: clockBody,
      rotation: clockRotation,
      name: "home_model_clockFace",
      modelURL: CLOCK_FACE_URL,
      position: clockFacePosition,
      dimensions: Vec3.multiply(0.5, {
        x: 0.2397,
        y: 0.2402,
        z: 0.0212
      }),
      userData: JSON.stringify({
        hifiHomeKey: {
          reset: true
        }
      })

    });

    // CLOCK HANDS
    // __________
    // _\|/_
    //  /|\
    // ___________

    var myDate = new Date()
      // HOUR HAND *************************
    var clockHandForwardOffset = (0.5 * -0.017);


    var hourHandPosition = Vec3.sum(clockFacePosition, Vec3.multiply(Quat.getFront(clockRotation), clockHandForwardOffset));
    var DEGREES_FOR_HOUR = 30
    var hours = myDate.getHours();
    var hourRollDegrees = -hours * DEGREES_FOR_HOUR;
    var localClockHandRotation = Quat.fromPitchYawRollDegrees(0, 0, hourRollDegrees);
    var worldClockHandRotation = Quat.multiply(clockRotation, localClockHandRotation);

    var ANGULAR_ROLL_SPEED_HOUR_RADIANS = 0.000029098833;
    var localAngularVelocity = {
      x: 0,
      y: 0,
      z: -ANGULAR_ROLL_SPEED_HOUR_RADIANS
    };
    var worldAngularVelocity = Vec3.multiplyQbyV(clockRotation, localAngularVelocity);

    clockHourHand = Entities.addEntity({
      type: "Model",
      name: "home_model_clockHourHand",
      parentID: clockFace,
      modelURL: CLOCK_HOUR_HAND_URL,
      position: hourHandPosition,
      registrationPoint: {
        x: 0.5,
        y: 0.05,
        z: 0.5
      },
      rotation: worldClockHandRotation,
      angularDamping: 0,
      angularVelocity: worldAngularVelocity,
      dimensions: Vec3.multiply(0.5, {
        x: 0.0263,
        y: 0.0982,
        z: 0.0024
      }),
      userData: JSON.stringify({
        hifiHomeKey: {
          reset: true
        }
      })
    });


    // **************** SECOND HAND *********************
    var DEGREES_FOR_SECOND = 6;
    var seconds = myDate.getSeconds();
    secondRollDegrees = -seconds * DEGREES_FOR_SECOND;
    var localClockHandRotation = Quat.fromPitchYawRollDegrees(0, 0, secondRollDegrees);
    var worldClockHandRotation = Quat.multiply(clockRotation, localClockHandRotation);
    var ANGULAR_ROLL_SPEED_SECOND_RADIANS = 0.10472
    var localAngularVelocity = {
      x: 0,
      y: 0,
      z: -ANGULAR_ROLL_SPEED_SECOND_RADIANS
    };
    var worldAngularVelocity = Vec3.multiplyQbyV(clockRotation, localAngularVelocity);
    clockSecondHand = Entities.addEntity({
      type: "Model",
      parentID: clockBody,
      modelURL: CLOCK_SECOND_HAND_URL,
      name: "home_model_clockSecondHand",
      position: hourHandPosition,
      dimensions: Vec3.multiply(0.5, {
        x: 0.0043,
        y: 0.1117,
        z: 0.0008
      }),
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
      rotation: worldClockHandRotation,
      angularDamping: 0,
      angularVelocity: worldAngularVelocity,
      userData: JSON.stringify({
        hifiHomeKey: {
          reset: true
        }
      })
    });


    // MINUTE HAND ************************
    var DEGREES_FOR_MINUTE = 6;
    var minutes = myDate.getMinutes();
    var minuteRollDegrees = -minutes * DEGREES_FOR_MINUTE;

    var localClockHandRotation = Quat.fromPitchYawRollDegrees(0, 0, minuteRollDegrees);
    var worldClockHandRotation = Quat.multiply(clockRotation, localClockHandRotation);

    var ANGULAR_ROLL_SPEED_MINUTE_RADIANS = 0.00174533;
    var localAngularVelocity = {
      x: 0,
      y: 0,
      z: -ANGULAR_ROLL_SPEED_MINUTE_RADIANS
    };
    var worldAngularVelocity = Vec3.multiplyQbyV(clockRotation, localAngularVelocity);
    clockMinuteHand = Entities.addEntity({
      type: "Model",
      modelURL: CLOCK_HOUR_HAND_URL,
      name: 'home_model_clockMinuteHand',
      parentID: clockFace,
      position: hourHandPosition,
      registrationPoint: {
        x: 0.5,
        y: 0.05,
        z: 0.5
      },
      rotation: worldClockHandRotation,
      angularDamping: 0,
      angularVelocity: worldAngularVelocity,
      dimensions: Vec3.multiply(0.5, {
        x: 0.0251,
        y: 0.1179,
        z: 0.0032
      }),
      script: MINUTE_HAND_CLOCK_SCRIPT_URL,
      userData: JSON.stringify({
        clockBody: clockBody,
        secondHand: clockSecondHand,
        hifiHomeKey: {
          reset: true
        }
      })
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