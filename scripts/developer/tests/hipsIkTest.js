//
//  hipsIKTest.js
//
//  Created by Anthony Thibault on 4/24/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Test procedural manipulation of the Avatar hips IK target.
//  Pull the left and right triggers on your hand controllers, you hips should begin to gyrate in an amusing mannor.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Xform */
Script.include("/~/system/libraries/Xform.js");

var calibrated = false;
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

var ANIM_VARS = [
    "headType",
    "hipsType",
    "hipsPosition",
    "hipsRotation"
];

var ZERO = {x: 0, y: 0, z: 0};
var X_AXIS = {x: 1, y: 0, z: 0};
var Y_AXIS = {x: 0, y: 1, z: 0};
var Y_180 = {x: 0, y: 1, z: 0, w: 0};
var Y_180_XFORM = new Xform(Y_180, {x: 0, y: 0, z: 0});

var hips = undefined;

function computeCurrentXform(jointIndex) {
    var currentXform = new Xform(MyAvatar.getAbsoluteJointRotationInObjectFrame(jointIndex),
                                 MyAvatar.getAbsoluteJointTranslationInObjectFrame(jointIndex));
    return Xform.mul(Y_180_XFORM, currentXform);
}

function calibrate() {
    hips = computeCurrentXform(MyAvatar.getJointIndex("Hips"));
}

var ikTypes = {
    RotationAndPosition: 0,
    RotationOnly: 1,
    HmdHead: 2,
    HipsRelativeRotationAndPosition: 3,
    Off: 4
};

function circleOffset(radius, theta, normal) {
    var pos = {x: radius * Math.cos(theta), y: radius * Math.sin(theta), z: 0};
    var lookAtRot = Quat.lookAt(normal, ZERO, X_AXIS);
    return Vec3.multiplyQbyV(lookAtRot, pos);
}

var handlerId;

function update(dt) {
    if (rightTriggerPressed && leftTriggerPressed) {
        if (!calibrated) {
            calibrate();
            calibrated = true;

            if (handlerId) {
                MyAvatar.removeAnimationStateHandler(handlerId);
                handlerId = undefined;
            } else {

                var n = Y_AXIS;
                var t = 0;
                handlerId = MyAvatar.addAnimationStateHandler(function (props) {

                    t += (1 / 60) * 4;
                    var result = {}, xform;
                    if (hips) {
                        xform = hips;
                        result.hipsType = ikTypes.RotationAndPosition;
                        result.hipsPosition = Vec3.sum(xform.pos, circleOffset(0.1, t, n));
                        result.hipsRotation = xform.rot;
                        result.headType = ikTypes.RotationAndPosition;
                    } else {
                        result.headType = ikTypes.HmdHead;
                        result.hipsType = props.hipsType;
                        result.hipsPosition = props.hipsPosition;
                        result.hipsRotation = props.hipsRotation;
                    }

                    return result;
                }, ANIM_VARS);
            }
        }
    } else {
        calibrated = false;
    }
}

Script.update.connect(update);

Script.scriptEnding.connect(function () {
    Controller.disableMapping(MAPPING_NAME);
    Script.update.disconnect(update);
});

