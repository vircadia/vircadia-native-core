//  handControllerGrab.js
//
//  Created by Seth Alves on 2016-9-7
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/* global MyAvatar, Vec3, Controller, Quat */

var GRAB_COMMUNICATIONS_SETTING = "io.highfidelity.isFarGrabbing";
setGrabCommunications = function setFarGrabCommunications(on) {
    Settings.setValue(GRAB_COMMUNICATIONS_SETTING, on ? "on" : "");
}
getGrabCommunications = function getFarGrabCommunications() {
    return !!Settings.getValue(GRAB_COMMUNICATIONS_SETTING, "");
}

// this offset needs to match the one in libraries/display-plugins/src/display-plugins/hmd/HmdDisplayPlugin.cpp:378
var GRAB_POINT_SPHERE_OFFSET = { x: 0.04, y: 0.13, z: 0.039 };  // x = upward, y = forward, z = lateral

getGrabPointSphereOffset = function(handController, ignoreSensorToWorldScale) {
    var offset = GRAB_POINT_SPHERE_OFFSET;
    if (handController === Controller.Standard.LeftHand) {
        offset = {
            x: -GRAB_POINT_SPHERE_OFFSET.x,
            y: GRAB_POINT_SPHERE_OFFSET.y,
            z: GRAB_POINT_SPHERE_OFFSET.z
        };
    }
    if (ignoreSensorToWorldScale) {
        return offset;
    } else {
        return Vec3.multiply(MyAvatar.sensorToWorldScale, offset);
    }
};

// controllerWorldLocation is where the controller would be, in-world, with an added offset
getControllerWorldLocation = function (handController, doOffset) {
    var orientation;
    var position;
    var pose = Controller.getPoseValue(handController);
    var valid = pose.valid;
    var controllerJointIndex;
    if (pose.valid) {
        if (handController === Controller.Standard.RightHand) {
            controllerJointIndex = MyAvatar.getJointIndex("_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND");
        } else {
            controllerJointIndex = MyAvatar.getJointIndex("_CAMERA_RELATIVE_CONTROLLER_LEFTHAND");
        }
        orientation = Quat.multiply(MyAvatar.orientation, MyAvatar.getAbsoluteJointRotationInObjectFrame(controllerJointIndex));
        position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, MyAvatar.getAbsoluteJointTranslationInObjectFrame(controllerJointIndex)));

        // add to the real position so the grab-point is out in front of the hand, a bit
        if (doOffset) {
            var offset = getGrabPointSphereOffset(handController);
            position = Vec3.sum(position, Vec3.multiplyQbyV(orientation, offset));
        }

    } else if (!HMD.isHandControllerAvailable()) {
        // NOTE: keep this offset in sync with scripts/system/controllers/handControllerPointer.js:493
        var VERTICAL_HEAD_LASER_OFFSET = 0.1 * MyAvatar.sensorToWorldScale;
        position = Vec3.sum(Camera.position, Vec3.multiplyQbyV(Camera.orientation, {x: 0, y: VERTICAL_HEAD_LASER_OFFSET, z: 0}));
        orientation = Quat.multiply(Camera.orientation, Quat.angleAxis(-90, { x: 1, y: 0, z: 0 }));
        valid = true;
    }

    return {position: position,
            translation: position,
            orientation: orientation,
            rotation: orientation,
            valid: valid};
};
