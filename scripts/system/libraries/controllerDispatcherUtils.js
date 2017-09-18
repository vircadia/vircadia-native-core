"use strict";

//  controllerDispatcherUtils.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Camera, HMD, MyAvatar, controllerDispatcherPlugins:true, Quat, Vec3, Overlays,
   MSECS_PER_SEC:true , LEFT_HAND:true, RIGHT_HAND:true, NULL_UUID:true, AVATAR_SELF_ID:true, FORBIDDEN_GRAB_TYPES:true,
   HAPTIC_PULSE_STRENGTH:true, HAPTIC_PULSE_DURATION:true, ZERO_VEC:true, ONE_VEC:true,
   DEFAULT_REGISTRATION_POINT:true, INCHES_TO_METERS:true,
   TRIGGER_OFF_VALUE:true,
   TRIGGER_ON_VALUE:true,
   PICK_MAX_DISTANCE:true,
   DEFAULT_SEARCH_SPHERE_DISTANCE:true,
   NEAR_GRAB_PICK_RADIUS:true,
   COLORS_GRAB_SEARCHING_HALF_SQUEEZE:true,
   COLORS_GRAB_SEARCHING_FULL_SQUEEZE:true,
   COLORS_GRAB_DISTANCE_HOLD:true,
   NEAR_GRAB_RADIUS:true,
   DISPATCHER_PROPERTIES:true,
   HAPTIC_PULSE_STRENGTH:true,
   HAPTIC_PULSE_DURATION:true,
   Entities,
   makeDispatcherModuleParameters:true,
   makeRunningValues:true,
   enableDispatcherModule:true,
   disableDispatcherModule:true,
   getEnabledModuleByName:true,
   getGrabbableData:true,
   entityIsGrabbable:true,
   entityIsDistanceGrabbable:true,
   getControllerJointIndex:true,
   propsArePhysical:true,
   controllerDispatcherPluginsNeedSort:true,
   projectOntoXYPlane:true,
   projectOntoEntityXYPlane:true,
   projectOntoOverlayXYPlane:true,
   entityHasActions:true,
   ensureDynamic:true,
   findGroupParent:true,
   BUMPER_ON_VALUE:true
*/

MSECS_PER_SEC = 1000.0;
INCHES_TO_METERS = 1.0 / 39.3701;

HAPTIC_PULSE_STRENGTH = 1.0;
HAPTIC_PULSE_DURATION = 13.0;

ZERO_VEC = { x: 0, y: 0, z: 0 };
ONE_VEC = { x: 1, y: 1, z: 1 };

LEFT_HAND = 0;
RIGHT_HAND = 1;

NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

FORBIDDEN_GRAB_TYPES = ["Unknown", "Light", "PolyLine", "Zone"];

HAPTIC_PULSE_STRENGTH = 1.0;
HAPTIC_PULSE_DURATION = 13.0;

DEFAULT_REGISTRATION_POINT = { x: 0.5, y: 0.5, z: 0.5 };

TRIGGER_OFF_VALUE = 0.1;
TRIGGER_ON_VALUE = TRIGGER_OFF_VALUE + 0.05; // Squeezed just enough to activate search or near grab
BUMPER_ON_VALUE = 0.5;

PICK_MAX_DISTANCE = 500; // max length of pick-ray
DEFAULT_SEARCH_SPHERE_DISTANCE = 1000; // how far from camera to search intersection?
NEAR_GRAB_PICK_RADIUS = 0.25; // radius used for search ray vs object for near grabbing.

COLORS_GRAB_SEARCHING_HALF_SQUEEZE = { red: 10, green: 10, blue: 255 };
COLORS_GRAB_SEARCHING_FULL_SQUEEZE = { red: 250, green: 10, blue: 10 };
COLORS_GRAB_DISTANCE_HOLD = { red: 238, green: 75, blue: 214 };

NEAR_GRAB_RADIUS = 1.0;

DISPATCHER_PROPERTIES = [
    "position",
    "registrationPoint",
    "rotation",
    "gravity",
    "collidesWith",
    "dynamic",
    "collisionless",
    "locked",
    "name",
    "shapeType",
    "parentID",
    "parentJointIndex",
    "density",
    "dimensions",
    "userData"
];

// priority -- a lower priority means the module will be asked sooner than one with a higher priority in a given update step
// activitySlots -- indicates which "slots" must not yet be in use for this module to start
// requiredDataForReady -- which "situation" parts this module looks at to decide if it will start
// sleepMSBetweenRuns -- how long to wait between calls to this module's "run" method
makeDispatcherModuleParameters = function (priority, activitySlots, requiredDataForReady, sleepMSBetweenRuns) {
    return {
        priority: priority,
        activitySlots: activitySlots,
        requiredDataForReady: requiredDataForReady,
        sleepMSBetweenRuns: sleepMSBetweenRuns
    };
};

makeRunningValues = function (active, targets, requiredDataForRun) {
    return {
        active: active,
        targets: targets,
        requiredDataForRun: requiredDataForRun
    };
};

enableDispatcherModule = function (moduleName, module, priority) {
    if (!controllerDispatcherPlugins) {
        controllerDispatcherPlugins = {};
    }
    controllerDispatcherPlugins[moduleName] = module;
    controllerDispatcherPluginsNeedSort = true;
};

disableDispatcherModule = function (moduleName) {
    delete controllerDispatcherPlugins[moduleName];
    controllerDispatcherPluginsNeedSort = true;
};

getEnabledModuleByName = function (moduleName) {
    if (controllerDispatcherPlugins.hasOwnProperty(moduleName)) {
        return controllerDispatcherPlugins[moduleName];
    }
    return null;
};

getGrabbableData = function (props) {
    // look in userData for a "grabbable" key, return the value or some defaults
    var grabbableData = {};
    var userDataParsed = null;
    try {
        if (!props.userDataParsed) {
            props.userDataParsed = JSON.parse(props.userData);
        }
        userDataParsed = props.userDataParsed;
    } catch (err) {
        userDataParsed = {};
    }
    if (userDataParsed.grabbableKey) {
        grabbableData = userDataParsed.grabbableKey;
    }
    if (!grabbableData.hasOwnProperty("grabbable")) {
        grabbableData.grabbable = true;
    }
    if (!grabbableData.hasOwnProperty("ignoreIK")) {
        grabbableData.ignoreIK = true;
    }
    if (!grabbableData.hasOwnProperty("kinematic")) {
        grabbableData.kinematic = true;
    }
    if (!grabbableData.hasOwnProperty("wantsTrigger")) {
        grabbableData.wantsTrigger = false;
    }
    if (!grabbableData.hasOwnProperty("triggerable")) {
        grabbableData.triggerable = false;
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

entityIsDistanceGrabbable = function(props) {
    if (!entityIsGrabbable(props)) {
        return false;
    }

    // we can't distance-grab non-physical
    var isPhysical = propsArePhysical(props);
    if (!isPhysical) {
        return false;
    }

    // XXX
    // var distance = Vec3.distance(props.position, handPosition);
    // this.otherGrabbingUUID = entityIsGrabbedByOther(entityID);
    // if (this.otherGrabbingUUID !== null) {
    //     // don't distance grab something that is already grabbed.
    //     if (debug) {
    //         print("distance grab is skipping '" + props.name + "': already grabbed by another.");
    //     }
    //     return false;
    // }

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
    var isPhysical = (props.shapeType && props.shapeType !== 'none');
    return isPhysical;
};

projectOntoXYPlane = function (worldPos, position, rotation, dimensions, registrationPoint) {
    var invRot = Quat.inverse(rotation);
    var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(worldPos, position));
    var invDimensions = {
        x: 1 / dimensions.x,
        y: 1 / dimensions.y,
        z: 1 / dimensions.z
    };

    var normalizedPos = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), registrationPoint);
    return {
        x: normalizedPos.x * dimensions.x,
        y: (1 - normalizedPos.y) * dimensions.y // flip y-axis
    };
};

projectOntoEntityXYPlane = function (entityID, worldPos, props) {
    return projectOntoXYPlane(worldPos, props.position, props.rotation, props.dimensions, props.registrationPoint);
};

projectOntoOverlayXYPlane = function projectOntoOverlayXYPlane(overlayID, worldPos) {
    var position = Overlays.getProperty(overlayID, "position");
    var rotation = Overlays.getProperty(overlayID, "rotation");
    var dimensions;

    var dpi = Overlays.getProperty(overlayID, "dpi");
    if (dpi) {
        // Calculate physical dimensions for web3d overlay from resolution and dpi; "dimensions" property is used as a scale.
        var resolution = Overlays.getProperty(overlayID, "resolution");
        resolution.z = 1; // Circumvent divide-by-zero.
        var scale = Overlays.getProperty(overlayID, "dimensions");
        scale.z = 0.01; // overlay dimensions are 2D, not 3D.
        dimensions = Vec3.multiplyVbyV(Vec3.multiply(resolution, INCHES_TO_METERS / dpi), scale);
    } else {
        dimensions = Overlays.getProperty(overlayID, "dimensions");
        if (dimensions.z) {
            dimensions.z = 0.01; // overlay dimensions are 2D, not 3D.
        }
    }

    return projectOntoXYPlane(worldPos, position, rotation, dimensions, DEFAULT_REGISTRATION_POINT);
};

entityHasActions = function (entityID) {
    return Entities.getActionIDs(entityID).length > 0;
};

ensureDynamic = function (entityID) {
    // if we distance hold something and keep it very still before releasing it, it ends up
    // non-dynamic in bullet.  If it's too still, give it a little bounce so it will fall.
    var props = Entities.getEntityProperties(entityID, ["velocity", "dynamic", "parentID"]);
    if (props.dynamic && props.parentID === NULL_UUID) {
        var velocity = props.velocity;
        if (Vec3.length(velocity) < 0.05) { // see EntityMotionState.cpp DYNAMIC_LINEAR_VELOCITY_THRESHOLD
            velocity = { x: 0.0, y: 0.2, z: 0.0 };
            Entities.editEntity(entityID, { velocity: velocity });
        }
    }
};

findGroupParent = function (controllerData, targetProps) {
    while (targetProps.parentID && targetProps.parentID !== NULL_UUID) {
        // XXX use controllerData.nearbyEntityPropertiesByID ?
        var parentProps = Entities.getEntityProperties(targetProps.parentID, DISPATCHER_PROPERTIES);
        if (!parentProps) {
            break;
        }
        parentProps.id = targetProps.parentID;
        targetProps = parentProps;
        controllerData.nearbyEntityPropertiesByID[targetProps.id] = targetProps;
    }

    return targetProps;
};

if (typeof module !== 'undefined') {
    module.exports = {
        makeDispatcherModuleParameters: makeDispatcherModuleParameters,
        enableDispatcherModule: enableDispatcherModule,
        disableDispatcherModule: disableDispatcherModule,
        makeRunningValues: makeRunningValues,
        LEFT_HAND: LEFT_HAND,
        RIGHT_HAND: RIGHT_HAND,
        BUMPER_ON_VALUE: BUMPER_ON_VALUE
    };
}
