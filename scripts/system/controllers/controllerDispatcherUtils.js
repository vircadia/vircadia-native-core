"use strict";

//  controllerDispatcherUtils.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Camera, HMD, MyAvatar, getControllerJointIndex,
   LEFT_HAND, RIGHT_HAND, NULL_UUID, AVATAR_SELF_ID, getGrabbableData */

LEFT_HAND = 0;
RIGHT_HAND = 1;

NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

getGrabbableData = function (props) {
    // look in userData for a "grabbable" key, return the value or some defaults
    var grabbableData = {};
    var userDataParsed = null;
    try {
        userDataParsed = JSON.parse(props.userData);
    } catch (err) {
    }
    if (userDataParsed && userDataParsed.grabbable) {
        grabbableData = userDataParsed.grabbable;
    }
    if (!grabbableData.hasOwnProperty("grabbable")) {
        grabbableData.grabbable = true;
    }

    return grabbableData;
};


getControllerJointIndex = function (hand) {
    if (HMD.isHandControllerAvailable()) {
        var controllerJointIndex = -1;
        if (Camera.mode === "first person") {
            controllerJointIndex = MyAvatar.getJointIndex(hand === RIGHT_HAND ?
                                                          "_CONTROLLER_RIGHTHAND" :
                                                          "_CONTROLLER_LEFTHAND");
        } else if (Camera.mode === "third person") {
            controllerJointIndex = MyAvatar.getJointIndex(hand === RIGHT_HAND ?
                                                          "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" :
                                                          "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND");
        }

        return controllerJointIndex;
    }

    return MyAvatar.getJointIndex("Head");
};
