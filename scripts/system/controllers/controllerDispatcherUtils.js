"use strict";

//  controllerDispatcherUtils.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Camera, HMD, MyAvatar, controllerDispatcherPlugins,
   MSECS_PER_SEC, LEFT_HAND, RIGHT_HAND, NULL_UUID, AVATAR_SELF_ID, FORBIDDEN_GRAB_TYPES,
   HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION,
   makeDispatcherModuleParameters,
   enableDispatcherModule,
   disableDispatcherModule,
   getGrabbableData,
   entityIsGrabbable,
   getControllerJointIndex,
   propsArePhysical
*/

MSECS_PER_SEC = 1000.0;

LEFT_HAND = 0;
RIGHT_HAND = 1;

NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

FORBIDDEN_GRAB_TYPES = ["Unknown", "Light", "PolyLine", "Zone"];

HAPTIC_PULSE_STRENGTH = 1.0;
HAPTIC_PULSE_DURATION = 13.0;


// priority -- a lower priority means the module will be asked sooner than one with a higher priority in a given update step
// activitySlots -- indicates which "slots" must not yet be in use for this module to start
// requiredDataForStart -- which "situation" parts this module looks at to decide if it will start
// sleepMSBetweenRuns -- how long to wait between calls to this module's "run" method
makeDispatcherModuleParameters = function (priority, activitySlots, requiredDataForStart, sleepMSBetweenRuns) {
    return {
        priority: priority,
        activitySlots: activitySlots,
        requiredDataForStart: requiredDataForStart,
        sleepMSBetweenRuns: sleepMSBetweenRuns
    };
};


enableDispatcherModule = function (moduleName, module, priority) {
    if (!controllerDispatcherPlugins) {
        controllerDispatcherPlugins = {};
    }
    controllerDispatcherPlugins[moduleName] = module;
};

disableDispatcherModule = function (moduleName) {
    delete controllerDispatcherPlugins[moduleName];
};

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
    if (!grabbableData.hasOwnProperty("ignoreIK")) {
        grabbableData.ignoreIK = true;
    }
    if (!grabbableData.hasOwnProperty("kinematicGrab")) {
        grabbableData.kinematicGrab = false;
    }

    return grabbableData;
};

entityIsGrabbable = function (props) {
    var grabbable = getGrabbableData(props).grabbable;
    if (!grabbable ||
        props.locked ||
        FORBIDDEN_GRAB_TYPES.indexOf(props.type) >= 0) {
        return false;
    }
    return true;
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

propsArePhysical = function (props) {
    if (!props.dynamic) {
        return false;
    }
    var isPhysical = (props.shapeType && props.shapeType != 'none');
    return isPhysical;
};
