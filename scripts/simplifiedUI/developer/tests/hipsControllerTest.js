//
//  hipsControllerTest.js
//
//  Created by Anthony Thibault on 4/24/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Test procedural manipulation of the Avatar hips via the controller system.
//  Pull the left and right triggers on your hand controllers, you hips should begin to gyrate in an amusing mannor.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Xform */
Script.include("/~/system/libraries/Xform.js");

var triggerPressHandled = false;
var rightTriggerPressed = false;
var leftTriggerPressed = false;

var MAPPING_NAME = "com.highfidelity.hipsIkTest";

var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from([Controller.Standard.RTClick]).peek().to(function (value) {
    rightTriggerPressed = (value !== 0) ? true : false;
});
mapping.from([Controller.Standard.LTClick]).peek().to(function (value) {
    leftTriggerPressed = (value !== 0) ? true : false;
});

Controller.enableMapping(MAPPING_NAME);

var CONTROLLER_MAPPING_NAME = "com.highfidelity.hipsIkTest.controller";
var controllerMapping;

var ZERO = {x: 0, y: 0, z: 0};
var X_AXIS = {x: 1, y: 0, z: 0};
var Y_AXIS = {x: 0, y: 1, z: 0};
var Y_180 = {x: 0, y: 1, z: 0, w: 0};
var Y_180_XFORM = new Xform(Y_180, {x: 0, y: 0, z: 0});

var hips = undefined;

function computeCurrentXform(jointIndex) {
    var currentXform = new Xform(MyAvatar.getAbsoluteJointRotationInObjectFrame(jointIndex),
                                 MyAvatar.getAbsoluteJointTranslationInObjectFrame(jointIndex));
    return currentXform;
}

function calibrate() {
    hips = computeCurrentXform(MyAvatar.getJointIndex("Hips"));
}

function circleOffset(radius, theta, normal) {
    var pos = {x: radius * Math.cos(theta), y: radius * Math.sin(theta), z: 0};
    var lookAtRot = Quat.lookAt(normal, ZERO, X_AXIS);
    return Vec3.multiplyQbyV(lookAtRot, pos);
}

var calibrationCount = 0;

function update(dt) {
    if (rightTriggerPressed && leftTriggerPressed) {
        if (!triggerPressHandled) {
            triggerPressHandled = true;
            if (controllerMapping) {
                hips = undefined;
                Controller.disableMapping(CONTROLLER_MAPPING_NAME + calibrationCount);
                controllerMapping = undefined;
            } else {
                calibrate();
                calibrationCount++;
                controllerMapping = Controller.newMapping(CONTROLLER_MAPPING_NAME + calibrationCount);

                var n = Y_AXIS;
                var t = 0;
                if (hips) {
                    controllerMapping.from(function () {
                        t += (1 / 60) * 4;
                        return {
                            valid: true,
                            translation: Vec3.sum(hips.pos, circleOffset(0.1, t, n)),
                            rotation: hips.rot,
                            velocity: ZERO,
                            angularVelocity: ZERO
                        };
                    }).to(Controller.Standard.Hips);
                }
                Controller.enableMapping(CONTROLLER_MAPPING_NAME + calibrationCount);
            }
        }
    } else {
        triggerPressHandled = false;
    }
}

Script.update.connect(update);

Script.scriptEnding.connect(function () {
    Controller.disableMapping(MAPPING_NAME);
    if (controllerMapping) {
        Controller.disableMapping(CONTROLLER_MAPPING_NAME + calibrationCount);
    }
    Script.update.disconnect(update);
});

