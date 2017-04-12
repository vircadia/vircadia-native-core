//
//  proceduralFootPoseExample.js
//  examples/controllers
//
//  Created by Brad Hefta-Gaub on 2015/12/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var MAPPING_NAME = "com.highfidelity.examples.proceduralFootPose";
var mapping = Controller.newMapping(MAPPING_NAME);
var translation = { x: 0, y: 0.1, z: 0 };
var translationDx = 0.01;
var translationDy = 0.01;
var translationDz = -0.01;
var TRANSLATION_LIMIT = 0.5;

var pitch = 45;
var yaw = 0;
var roll = 45;
var pitchDelta = 1;
var yawDelta = -1;
var rollDelta = 1;
var ROTATION_MIN = -90;
var ROTATION_MAX = 90;

mapping.from(function() {

    // adjust the hand translation in a periodic back and forth motion for each of the 3 axes
    translation.x = translation.x + translationDx;
    translation.y = translation.y + translationDy;
    translation.z = translation.z + translationDz;
    if ((translation.x > TRANSLATION_LIMIT) || (translation.x < (-1 * TRANSLATION_LIMIT))) {
        translationDx = translationDx * -1;
    }
    if ((translation.y > TRANSLATION_LIMIT) || (translation.y < (-1 * TRANSLATION_LIMIT))) {
        translationDy = translationDy * -1;
    }
    if ((translation.z > TRANSLATION_LIMIT) || (translation.z < (-1 * TRANSLATION_LIMIT))) {
        translationDz = translationDz * -1;
    }

    // adjust the hand rotation in a periodic back and forth motion for each of pitch/yaw/roll
    pitch = pitch + pitchDelta;
    yaw = yaw + yawDelta;
    roll = roll + rollDelta;
    if ((pitch > ROTATION_MAX) || (pitch < ROTATION_MIN)) {
        pitchDelta = pitchDelta * -1;
    }
    if ((yaw > ROTATION_MAX) || (yaw < ROTATION_MIN)) {
        yawDelta = yawDelta * -1;
    }
    if ((roll > ROTATION_MAX) || (roll < ROTATION_MIN)) {
        rollDelta = rollDelta * -1;
    }

    var rotation = Quat.fromPitchYawRollDegrees(pitch, yaw, roll);

    var pose = {
            translation: translation,
            rotation: rotation,
            velocity: { x: 0, y: 0, z: 0 },
            angularVelocity: { x: 0, y: 0, z: 0 }
        };

    Vec3.print("foot translation:", translation);
    return pose;
}).debug(true).to(Controller.Standard.LeftFoot);

//mapping.from(Controller.Standard.LeftFoot).debug(true).to(Controller.Actions.LeftFoot);


Controller.enableMapping(MAPPING_NAME);


Script.scriptEnding.connect(function(){
    mapping.disable();
});
