//  handControllerGrab.js
//
//  Created by Seth Alves on 2016-9-7
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/* global MyAvatar, Vec3, Controller, Quat */


// var GRAB_POINT_SPHERE_OFFSET = { x: 0, y: 0.2, z: 0 };
// var GRAB_POINT_SPHERE_OFFSET = { x: 0.1, y: 0.175, z: 0.04 };

// this offset needs to match the one in libraries/display-plugins/src/display-plugins/hmd/HmdDisplayPlugin.cpp
var GRAB_POINT_SPHERE_OFFSET = { x: 0.1, y: 0.32, z: 0.04 };

getGrabPointSphereOffset = function(handController) {
    if (handController === Controller.Standard.RightHand) {
        return GRAB_POINT_SPHERE_OFFSET;
    }
    return {
        x: GRAB_POINT_SPHERE_OFFSET.x * -1,
        y: GRAB_POINT_SPHERE_OFFSET.y,
        z: GRAB_POINT_SPHERE_OFFSET.z
    };
};

// controllerLocation is where the controller would be, in-world.
getControllerWorldLocation = function (handController, doOffset) {
    var orientation;
    var position;
    var pose = Controller.getPoseValue(handController);
    if (pose.valid) {
        orientation = Quat.multiply(MyAvatar.orientation, pose.rotation);
        position = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation), MyAvatar.position);
        // add to the real position so the grab-point is out in front of the hand, a bit
        if (doOffset) {
            position = Vec3.sum(position, Vec3.multiplyQbyV(orientation, getGrabPointSphereOffset(handController)));
        }
    }
    return {position: position,
            translation: position,
            orientation: orientation,
            rotation: orientation,
            valid: pose.valid};
};
