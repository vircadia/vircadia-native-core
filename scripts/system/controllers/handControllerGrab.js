"use strict";

//  handControllerGrab.js
//
//  Created by Eric Levin on  9/2/15
//  Additions by James B. Pollack @imgntn on 9/24/2015
//  Additions By Seth Alves on 10/20/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Grabs physically moveable entities with hydra-like controllers; it works for either near or far objects.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global getEntityCustomData, flatten, Xform, Script, Quat, Vec3, MyAvatar, Entities, Overlays, Settings,
    Reticle, Controller, Camera, Messages, Mat4, getControllerWorldLocation, getGrabPointSphereOffset,
   setGrabCommunications, Menu, HMD, isInEditMode, AvatarList */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function() { // BEGIN LOCAL_SCOPE

Script.include("/~/system/libraries/utils.js");
Script.include("/~/system/libraries/Xform.js");
Script.include("/~/system/libraries/controllers.js");

//
// add lines where the hand ray picking is happening
//

var WANT_DEBUG = false;
var WANT_DEBUG_STATE = false;
var WANT_DEBUG_SEARCH_NAME = null;

var UPDATE_SLEEP_MS = 16; // how many milliseconds to wait between "update" calls

var FORCE_IGNORE_IK = false;
var SHOW_GRAB_POINT_SPHERE = false;

//
// these tune time-averaging and "on" value for analog trigger
//

var TRIGGER_SMOOTH_RATIO = 0.1; //  Time averaging of trigger - 0.0 disables smoothing
var TRIGGER_OFF_VALUE = 0.1;
var TRIGGER_ON_VALUE = TRIGGER_OFF_VALUE + 0.05; //  Squeezed just enough to activate search or near grab

var BUMPER_ON_VALUE = 0.5;

var THUMB_ON_VALUE = 0.5;

var HAPTIC_PULSE_STRENGTH = 1.0;
var HAPTIC_PULSE_DURATION = 13.0;
var HAPTIC_TEXTURE_STRENGTH = 0.1;
var HAPTIC_TEXTURE_DURATION = 3.0;
var HAPTIC_TEXTURE_DISTANCE = 0.002;
var HAPTIC_DEQUIP_STRENGTH = 0.75;
var HAPTIC_DEQUIP_DURATION = 50.0;

// triggered when stylus presses a web overlay/entity
var HAPTIC_STYLUS_STRENGTH = 1.0;
var HAPTIC_STYLUS_DURATION = 20.0;

// triggerd when ui laser presses a web overlay/entity
var HAPTIC_LASER_UI_STRENGTH = 1.0;
var HAPTIC_LASER_UI_DURATION = 20.0;

var HAND_HEAD_MIX_RATIO = 0.0; //  0 = only use hands for search/move.  1 = only use head for search/move.

var PICK_WITH_HAND_RAY = true;

var EQUIP_SPHERE_SCALE_FACTOR = 0.65;

var WEB_DISPLAY_STYLUS_DISTANCE = 0.5;
var WEB_STYLUS_LENGTH = 0.2;
var WEB_TOUCH_Y_OFFSET = 0.05; // how far forward (or back with a negative number) to slide stylus in hand

//
// distant manipulation
//
var DISTANCE_HOLDING_RADIUS_FACTOR = 3.5; // multiplied by distance between hand and object
var DISTANCE_HOLDING_ACTION_TIMEFRAME = 0.1; // how quickly objects move to their new position
var DISTANCE_HOLDING_UNITY_MASS = 1200; //  The mass at which the distance holding action timeframe is unmodified
var DISTANCE_HOLDING_UNITY_DISTANCE = 6; //  The distance at which the distance holding action timeframe is unmodified
var MOVE_WITH_HEAD = true; // experimental head-control of distantly held objects

var COLORS_GRAB_SEARCHING_HALF_SQUEEZE = {
    red: 10,
    green: 10,
    blue: 255
};

var COLORS_GRAB_SEARCHING_FULL_SQUEEZE = {
    red: 250,
    green: 10,
    blue: 10
};

var COLORS_GRAB_DISTANCE_HOLD = {
    red: 238,
    green: 75,
    blue: 214
};

var PICK_MAX_DISTANCE = 500; // max length of pick-ray

//
// near grabbing
//

var EQUIP_RADIUS = 0.2; // radius used for palm vs equip-hotspot for equipping.
// if EQUIP_HOTSPOT_RENDER_RADIUS is greater than zero, the hotspot will appear before the hand
// has reached the required position, and then grow larger once the hand is close enough to equip.
var EQUIP_HOTSPOT_RENDER_RADIUS = 0.0; // radius used for palm vs equip-hotspot for rendering hot-spots
var MAX_EQUIP_HOTSPOT_RADIUS = 1.0;
var MAX_FAR_TO_NEAR_EQUIP_HOTSPOT_RADIUS = 0.5; // radius used for far to near equipping object.
var NEAR_GRABBING_ACTION_TIMEFRAME = 0.05; // how quickly objects move to their new position

var NEAR_GRAB_RADIUS = 0.1; // radius used for palm vs object for near grabbing.
var NEAR_GRAB_MAX_DISTANCE = 1.0; // you cannot grab objects that are this far away from your hand
var FAR_TO_NEAR_GRAB_MAX_DISTANCE = 0.3; // In far to near grabbing conversion,grab the object if distancetoObject from hand is less than this.
var NEAR_GRAB_PICK_RADIUS = 0.25; // radius used for search ray vs object for near grabbing.
var NEAR_GRABBING_KINEMATIC = true; // force objects to be kinematic when near-grabbed

// if an equipped item is "adjusted" to be too far from the hand it's in, it will be unequipped.
var CHECK_TOO_FAR_UNEQUIP_TIME = 0.3; // seconds, duration between checks


var GRAB_POINT_SPHERE_RADIUS = NEAR_GRAB_RADIUS;
var GRAB_POINT_SPHERE_COLOR = { red: 240, green: 240, blue: 240 };
var GRAB_POINT_SPHERE_ALPHA = 0.85;

//
// other constants
//
var RIGHT_HAND = 1;
var LEFT_HAND = 0;

var ZERO_VEC = {
    x: 0,
    y: 0,
    z: 0
};

var ONE_VEC = {
    x: 1,
    y: 1,
    z: 1
};

var NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
var AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

var DEFAULT_REGISTRATION_POINT = { x: 0.5, y: 0.5, z: 0.5 };
var INCHES_TO_METERS = 1.0 / 39.3701;


// these control how long an abandoned pointer line or action will hang around
var ACTION_TTL = 15; // seconds
var ACTION_TTL_REFRESH = 5;
var PICKS_PER_SECOND_PER_HAND = 60;
var MSECS_PER_SEC = 1000.0;
var GRABBABLE_PROPERTIES = [
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

var GRABBABLE_DATA_KEY = "grabbableKey"; // shared with grab.js

var DEFAULT_GRABBABLE_DATA = {
    disableReleaseVelocity: false
};

// sometimes we want to exclude objects from being picked
var USE_BLACKLIST = true;
var blacklist = [];

var entitiesWithHoverOverlays = [];

var FORBIDDEN_GRAB_NAMES = ["Grab Debug Entity", "grab pointer"];
var FORBIDDEN_GRAB_TYPES = ["Unknown", "Light", "PolyLine", "Zone"];

var holdEnabled = true;
var nearGrabEnabled = true;
var farGrabEnabled = true;
var myAvatarScalingEnabled = true;
var objectScalingEnabled = true;
var mostRecentSearchingHand = RIGHT_HAND;
var DEFAULT_SPHERE_MODEL_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/equip-Fresnel-3.fbx";
var HARDWARE_MOUSE_ID = 0;  // Value reserved for hardware mouse.

// states for the state machine
var STATE_OFF = 0;
var STATE_SEARCHING = 1;
var STATE_DISTANCE_HOLDING = 2;
var STATE_DISTANCE_ROTATING = 3;
var STATE_NEAR_GRABBING = 4;
var STATE_NEAR_TRIGGER = 5;
var STATE_FAR_TRIGGER = 6;
var STATE_HOLD = 7;
var STATE_ENTITY_LASER_TOUCHING = 8;
var STATE_OVERLAY_LASER_TOUCHING = 9;
var STATE_STYLUS_TOUCHING = 10;

var CONTROLLER_STATE_MACHINE = {};

CONTROLLER_STATE_MACHINE[STATE_OFF] = {
    name: "off",
    enterMethod: "offEnter",
    updateMethod: "off"
};
CONTROLLER_STATE_MACHINE[STATE_SEARCHING] = {
    name: "searching",
    enterMethod: "searchEnter",
    updateMethod: "search"
};
CONTROLLER_STATE_MACHINE[STATE_DISTANCE_HOLDING] = {
    name: "distance_holding",
    enterMethod: "distanceHoldingEnter",
    updateMethod: "distanceHolding"
};
CONTROLLER_STATE_MACHINE[STATE_DISTANCE_ROTATING] = {
    name: "distance_rotating",
    enterMethod: "distanceRotatingEnter",
    updateMethod: "distanceRotating"
};
CONTROLLER_STATE_MACHINE[STATE_NEAR_GRABBING] = {
    name: "near_grabbing",
    enterMethod: "nearGrabbingEnter",
    updateMethod: "nearGrabbing"
};
CONTROLLER_STATE_MACHINE[STATE_HOLD] = {
    name: "hold",
    enterMethod: "nearGrabbingEnter",
    updateMethod: "nearGrabbing"
};
CONTROLLER_STATE_MACHINE[STATE_NEAR_TRIGGER] = {
    name: "trigger",
    enterMethod: "nearTriggerEnter",
    updateMethod: "nearTrigger"
};
CONTROLLER_STATE_MACHINE[STATE_FAR_TRIGGER] = {
    name: "far_trigger",
    enterMethod: "farTriggerEnter",
    updateMethod: "farTrigger"
};
CONTROLLER_STATE_MACHINE[STATE_ENTITY_LASER_TOUCHING] = {
    name: "entityLaserTouching",
    enterMethod: "entityLaserTouchingEnter",
    exitMethod: "entityLaserTouchingExit",
    updateMethod: "entityLaserTouching"
};
CONTROLLER_STATE_MACHINE[STATE_OVERLAY_LASER_TOUCHING] = {
    name: "overlayLaserTouching",
    enterMethod: "overlayLaserTouchingEnter",
    exitMethod: "overlayLaserTouchingExit",
    updateMethod: "overlayLaserTouching"
};
CONTROLLER_STATE_MACHINE[STATE_STYLUS_TOUCHING] = {
    name: "stylusTouching",
    enterMethod: "stylusTouchingEnter",
    exitMethod: "stylusTouchingExit",
    updateMethod: "stylusTouching"
};

function distance2D(a, b) {
    var dx = (a.x - b.x);
    var dy = (a.y - b.y);
    return Math.sqrt(dx * dx + dy * dy);
}

function getFingerWorldLocation(hand) {
    var fingerJointName = (hand === RIGHT_HAND) ? "RightHandIndex4" : "LeftHandIndex4";

    var fingerJointIndex = MyAvatar.getJointIndex(fingerJointName);
    var fingerPosition = MyAvatar.getAbsoluteJointTranslationInObjectFrame(fingerJointIndex);
    var fingerRotation = MyAvatar.getAbsoluteJointRotationInObjectFrame(fingerJointIndex);
    var worldFingerRotation = Quat.multiply(MyAvatar.orientation, fingerRotation);
    var worldFingerPosition = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, fingerPosition));

    return {
        position: worldFingerPosition,
        orientation: worldFingerRotation,
        rotation: worldFingerRotation,
        valid: true
    };
}

// Object assign  polyfill
if (typeof Object.assign != 'function') {
    Object.assign = function(target, varArgs) {
        if (target === null) {
            throw new TypeError('Cannot convert undefined or null to object');
        }
        var to = Object(target);
        for (var index = 1; index < arguments.length; index++) {
            var nextSource = arguments[index];
            if (nextSource !== null) {
                for (var nextKey in nextSource) {
                    if (Object.prototype.hasOwnProperty.call(nextSource, nextKey)) {
                        to[nextKey] = nextSource[nextKey];
                    }
                }
            }
        }
        return to;
    };
}

function distanceBetweenPointAndEntityBoundingBox(point, entityProps) {
    var entityXform = new Xform(entityProps.rotation, entityProps.position);
    var localPoint = entityXform.inv().xformPoint(point);
    var minOffset = Vec3.multiplyVbyV(entityProps.registrationPoint, entityProps.dimensions);
    var maxOffset = Vec3.multiplyVbyV(Vec3.subtract(ONE_VEC, entityProps.registrationPoint), entityProps.dimensions);
    var localMin = Vec3.subtract(entityXform.trans, minOffset);
    var localMax = Vec3.sum(entityXform.trans, maxOffset);

    var v = {x: localPoint.x, y: localPoint.y, z: localPoint.z};
    v.x = Math.max(v.x, localMin.x);
    v.x = Math.min(v.x, localMax.x);
    v.y = Math.max(v.y, localMin.y);
    v.y = Math.min(v.y, localMax.y);
    v.z = Math.max(v.z, localMin.z);
    v.z = Math.min(v.z, localMax.z);

    return Vec3.distance(v, localPoint);
}

function projectOntoXYPlane(worldPos, position, rotation, dimensions, registrationPoint) {
    var invRot = Quat.inverse(rotation);
    var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(worldPos, position));
    var invDimensions = { x: 1 / dimensions.x,
                          y: 1 / dimensions.y,
                          z: 1 / dimensions.z };
    var normalizedPos = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), registrationPoint);
    return { x: normalizedPos.x * dimensions.x,
             y: (1 - normalizedPos.y) * dimensions.y }; // flip y-axis
}

function projectOntoEntityXYPlane(entityID, worldPos) {
    var props = entityPropertiesCache.getProps(entityID);
    return projectOntoXYPlane(worldPos, props.position, props.rotation, props.dimensions, props.registrationPoint);
}

function projectOntoOverlayXYPlane(overlayID, worldPos) {
    var position = Overlays.getProperty(overlayID, "position");
    var rotation = Overlays.getProperty(overlayID, "rotation");
    var dimensions;

    var dpi = Overlays.getProperty(overlayID, "dpi");
    if (dpi) {
        // Calculate physical dimensions for web3d overlay from resolution and dpi; "dimensions" property is used as a scale.
        var resolution = Overlays.getProperty(overlayID, "resolution");
        resolution.z = 1;  // Circumvent divide-by-zero.
        var scale = Overlays.getProperty(overlayID, "dimensions");
        scale.z = 0.01;    // overlay dimensions are 2D, not 3D.
        dimensions = Vec3.multiplyVbyV(Vec3.multiply(resolution, INCHES_TO_METERS / dpi), scale);
    } else {
        dimensions = Overlays.getProperty(overlayID, "dimensions");
        dimensions.z = 0.01;    // overlay dimensions are 2D, not 3D.
    }

    return projectOntoXYPlane(worldPos, position, rotation, dimensions, DEFAULT_REGISTRATION_POINT);
}

function handLaserIntersectItem(position, rotation, start) {
    var worldHandPosition = start.position;
    var worldHandRotation = start.orientation;

    if (position) {
        var planePosition = position;
        var planeNormal = Vec3.multiplyQbyV(rotation, {x: 0, y: 0, z: 1.0});
        var rayStart = worldHandPosition;
        var rayDirection = Quat.getUp(worldHandRotation);
        var intersectionInfo = rayIntersectPlane(planePosition, planeNormal, rayStart, rayDirection);

        var intersectionPoint = planePosition;
        if (intersectionInfo.hit && intersectionInfo.distance > 0) {
            intersectionPoint = Vec3.sum(rayStart, Vec3.multiply(intersectionInfo.distance, rayDirection));
        } else {
            intersectionPoint = planePosition;
        }
        intersectionInfo.point = intersectionPoint;
        intersectionInfo.normal = planeNormal;
        intersectionInfo.searchRay = {
            origin: rayStart,
            direction: rayDirection,
            length: PICK_MAX_DISTANCE
        };

        return intersectionInfo;
    } else {
        // entity has been destroyed? or is no longer in cache
        return null;
    }
}

function handLaserIntersectEntity(entityID, start) {
    var props = entityPropertiesCache.getProps(entityID);
    return handLaserIntersectItem(props.position, props.rotation, start);
}

function handLaserIntersectOverlay(overlayID, start) {
    var position = Overlays.getProperty(overlayID, "position");
    var rotation = Overlays.getProperty(overlayID, "rotation");
    return handLaserIntersectItem(position, rotation, start);
}

function rayIntersectPlane(planePosition, planeNormal, rayStart, rayDirection) {
    var rayDirectionDotPlaneNormal = Vec3.dot(rayDirection, planeNormal);
    if (rayDirectionDotPlaneNormal > 0.00001 || rayDirectionDotPlaneNormal < -0.00001) {
        var rayStartDotPlaneNormal = Vec3.dot(Vec3.subtract(planePosition, rayStart), planeNormal);
        var distance = rayStartDotPlaneNormal / rayDirectionDotPlaneNormal;
        return {hit: true, distance: distance};
    } else {
        // ray is parallel to the plane
        return {hit: false, distance: 0};
    }
}

function stateToName(state) {
    return CONTROLLER_STATE_MACHINE[state] ? CONTROLLER_STATE_MACHINE[state].name : "???";
}

function getTag() {
    return "grab-" + MyAvatar.sessionUUID;
}

function colorPow(color, power) {
    return {
        red: Math.pow(color.red / 255.0, power) * 255,
        green: Math.pow(color.green / 255.0, power) * 255,
        blue: Math.pow(color.blue / 255.0, power) * 255
    };
}

function entityHasActions(entityID) {
    return Entities.getActionIDs(entityID).length > 0;
}

function findRayIntersection(pickRay, precise, include, exclude) {
    var entities = Entities.findRayIntersection(pickRay, precise, include, exclude, true);
    var overlays = Overlays.findRayIntersection(pickRay, precise, [], [HMD.tabletID]);
    if (!overlays.intersects || (entities.intersects && (entities.distance <= overlays.distance))) {
        return entities;
    }
    return overlays;
}

function entityIsGrabbedByOther(entityID) {
    // by convention, a distance grab sets the tag of its action to be grab-*owner-session-id*.
    var actionIDs = Entities.getActionIDs(entityID);
    for (var actionIndex = 0; actionIndex < actionIDs.length; actionIndex++) {
        var actionID = actionIDs[actionIndex];
        var actionArguments = Entities.getActionArguments(entityID, actionID);
        var tag = actionArguments.tag;
        if (tag === getTag()) {
            // we see a grab-*uuid* shaped tag, but it's our tag, so that's okay.
            continue;
        }
        var GRAB_PREFIX_LENGTH = 5;
        var UUID_LENGTH = 38;
        if (tag && tag.slice(0, GRAB_PREFIX_LENGTH) == "grab-") {
            // we see a grab-*uuid* shaped tag and it's not ours, so someone else is grabbing it.
            return tag.slice(GRAB_PREFIX_LENGTH, GRAB_PREFIX_LENGTH + UUID_LENGTH - 1);
        }
    }
    return null;
}

function propsArePhysical(props) {
    if (!props.dynamic) {
        return false;
    }
    var isPhysical = (props.shapeType && props.shapeType != 'none');
    return isPhysical;
}

var USE_ATTACH_POINT_SETTINGS = true;

var ATTACH_POINT_SETTINGS = "io.highfidelity.attachPoints";

function getAttachPointSettings() {
    try {
        var str = Settings.getValue(ATTACH_POINT_SETTINGS);
        if (str === "false") {
            return {};
        } else {
            return JSON.parse(str);
        }
    } catch (err) {
        print("Error parsing attachPointSettings: " + err);
        return {};
    }
}

function setAttachPointSettings(attachPointSettings) {
    var str = JSON.stringify(attachPointSettings);
    Settings.setValue(ATTACH_POINT_SETTINGS, str);
}

function getAttachPointForHotspotFromSettings(hotspot, hand) {
    var attachPointSettings = getAttachPointSettings();
    var jointName = (hand === RIGHT_HAND) ? "RightHand" : "LeftHand";
    var joints = attachPointSettings[hotspot.key];
    if (joints) {
        return joints[jointName];
    } else {
        return undefined;
    }
}

function storeAttachPointForHotspotInSettings(hotspot, hand, offsetPosition, offsetRotation) {
    var attachPointSettings = getAttachPointSettings();
    var jointName = (hand === RIGHT_HAND) ? "RightHand" : "LeftHand";
    var joints = attachPointSettings[hotspot.key];
    if (!joints) {
        joints = {};
        attachPointSettings[hotspot.key] = joints;
    }
    joints[jointName] = [offsetPosition, offsetRotation];
    setAttachPointSettings(attachPointSettings);
}

// If another script is managing the reticle (as is done by HandControllerPointer), we should not be setting it here,
// and we should not be showing lasers when someone else is using the Reticle to indicate a 2D minor mode.
var EXTERNALLY_MANAGED_2D_MINOR_MODE = true;

function isEditing() {
    return EXTERNALLY_MANAGED_2D_MINOR_MODE && isInEditMode();
}

function isIn2DMode() {
    // In this version, we make our own determination of whether we're aimed a HUD element,
    // because other scripts (such as handControllerPointer) might be using some other visualization
    // instead of setting Reticle.visible.
    return (EXTERNALLY_MANAGED_2D_MINOR_MODE &&
        (Reticle.pointingAtSystemOverlay || Overlays.getOverlayAtPoint(Reticle.position)));
}

function restore2DMode() {
    if (!EXTERNALLY_MANAGED_2D_MINOR_MODE) {
        Reticle.setVisible(true);
    }
}

function stylusTargetHasKeyboardFocus(stylusTarget) {
    if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
        return Entities.keyboardFocusEntity === stylusTarget.entityID;
    } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
        return Overlays.keyboardFocusOverlay === stylusTarget.overlayID;
    }
}

function setKeyboardFocusOnStylusTarget(stylusTarget) {
    if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID && Entities.wantsHandControllerPointerEvents(stylusTarget.entityID)) {
        Overlays.keyboardFocusOverlay = NULL_UUID;
        Entities.keyboardFocusEntity = stylusTarget.entityID;
    } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
        Overlays.keyboardFocusOverlay = stylusTarget.overlayID;
        Entities.keyboardFocusEntity = NULL_UUID;
    }
}

function sendHoverEnterEventToStylusTarget(hand, stylusTarget) {
    var pointerEvent = {
        type: "Move",
        id: hand + 1, // 0 is reserved for hardware mouse
        pos2D: stylusTarget.position2D,
        pos3D: stylusTarget.position,
        normal: stylusTarget.normal,
        direction: Vec3.subtract(ZERO_VEC, stylusTarget.normal),
        button: "None"
    };

    if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
        Entities.sendHoverEnterEntity(stylusTarget.entityID, pointerEvent);
    } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
        Overlays.sendHoverEnterOverlay(stylusTarget.overlayID, pointerEvent);
    }
}

function sendHoverOverEventToStylusTarget(hand, stylusTarget) {
    var pointerEvent = {
        type: "Move",
        id: hand + 1, // 0 is reserved for hardware mouse
        pos2D: stylusTarget.position2D,
        pos3D: stylusTarget.position,
        normal: stylusTarget.normal,
        direction: Vec3.subtract(ZERO_VEC, stylusTarget.normal),
        button: "None"
    };

    if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
        Entities.sendMouseMoveOnEntity(stylusTarget.entityID, pointerEvent);
        Entities.sendHoverOverEntity(stylusTarget.entityID, pointerEvent);
    } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
        Overlays.sendMouseMoveOnOverlay(stylusTarget.overlayID, pointerEvent);
        Overlays.sendHoverOverOverlay(stylusTarget.overlayID, pointerEvent);
    }
}

function sendTouchStartEventToStylusTarget(hand, stylusTarget) {
    var pointerEvent = {
        type: "Press",
        id: hand + 1, // 0 is reserved for hardware mouse
        pos2D: stylusTarget.position2D,
        pos3D: stylusTarget.position,
        normal: stylusTarget.normal,
        direction: Vec3.subtract(ZERO_VEC, stylusTarget.normal),
        button: "Primary",
        isPrimaryHeld: true
    };

    if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
        Entities.sendMousePressOnEntity(stylusTarget.entityID, pointerEvent);
        Entities.sendClickDownOnEntity(stylusTarget.entityID, pointerEvent);
    } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
        Overlays.sendMousePressOnOverlay(stylusTarget.overlayID, pointerEvent);
    }
}

function sendTouchEndEventToStylusTarget(hand, stylusTarget) {
    var pointerEvent = {
        type: "Release",
        id: hand + 1, // 0 is reserved for hardware mouse
        pos2D: stylusTarget.position2D,
        pos3D: stylusTarget.position,
        normal: stylusTarget.normal,
        direction: Vec3.subtract(ZERO_VEC, stylusTarget.normal),
        button: "Primary"
    };

    if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
        Entities.sendMouseReleaseOnEntity(stylusTarget.entityID, pointerEvent);
        Entities.sendClickReleaseOnEntity(stylusTarget.entityID, pointerEvent);
        Entities.sendHoverLeaveEntity(stylusTarget.entityID, pointerEvent);
    } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
        Overlays.sendMouseReleaseOnOverlay(stylusTarget.overlayID, pointerEvent);
    }
}

function sendTouchMoveEventToStylusTarget(hand, stylusTarget) {
    var pointerEvent = {
        type: "Move",
        id: hand + 1, // 0 is reserved for hardware mouse
        pos2D: stylusTarget.position2D,
        pos3D: stylusTarget.position,
        normal: stylusTarget.normal,
        direction: Vec3.subtract(ZERO_VEC, stylusTarget.normal),
        button: "Primary",
        isPrimaryHeld: true
    };

    if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
        Entities.sendMouseMoveOnEntity(stylusTarget.entityID, pointerEvent);
        Entities.sendHoldingClickOnEntity(stylusTarget.entityID, pointerEvent);
    } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
        Overlays.sendMouseMoveOnOverlay(stylusTarget.overlayID, pointerEvent);
    }
}

// will return undefined if entity does not exist.
function calculateStylusTargetFromEntity(stylusTip, entityID) {
    var props = entityPropertiesCache.getProps(entityID);
    if (props.rotation === undefined) {
        // if rotation is missing from props object, then this entity has probably been deleted.
        return;
    }

    // project stylus tip onto entity plane.
    var normal = Vec3.multiplyQbyV(props.rotation, {x: 0, y: 0, z: 1});
    Vec3.multiplyQbyV(props.rotation, {x: 0, y: 1, z: 0});
    var distance = Vec3.dot(Vec3.subtract(stylusTip.position, props.position), normal);
    var position = Vec3.subtract(stylusTip.position, Vec3.multiply(normal, distance));

    // generate normalized coordinates
    var invRot = Quat.inverse(props.rotation);
    var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(position, props.position));
    var invDimensions = { x: 1 / props.dimensions.x, y: 1 / props.dimensions.y, z: 1 / props.dimensions.z };
    var normalizedPosition = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), props.registrationPoint);

    // 2D position on entity plane in meters, relative to the bounding box upper-left hand corner.
    var position2D = { x: normalizedPosition.x * props.dimensions.x, y: (1 - normalizedPosition.y) * props.dimensions.y }; // flip y-axis

    return {
        entityID: entityID,
        overlayID: null,
        distance: distance,
        position: position,
        position2D: position2D,
        normal: normal,
        normalizedPosition: normalizedPosition,
        dimensions: props.dimensions,
        valid: true
    };
}

// will return undefined if overlayID does not exist.
function calculateStylusTargetFromOverlay(stylusTip, overlayID) {
    var overlayPosition = Overlays.getProperty(overlayID, "position");
    if (overlayPosition === undefined) {
        return;
    }

    // project stylusTip onto overlay plane.
    var overlayRotation = Overlays.getProperty(overlayID, "rotation");
    if (overlayRotation === undefined) {
        return;
    }
    var normal = Vec3.multiplyQbyV(overlayRotation, {x: 0, y: 0, z: 1});
    var distance = Vec3.dot(Vec3.subtract(stylusTip.position, overlayPosition), normal);
    var position = Vec3.subtract(stylusTip.position, Vec3.multiply(normal, distance));

    // calclulate normalized position
    var invRot = Quat.inverse(overlayRotation);
    var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(position, overlayPosition));
    var dpi = Overlays.getProperty(overlayID, "dpi");

    var dimensions;
    if (dpi) {
        // Calculate physical dimensions for web3d overlay from resolution and dpi; "dimensions" property is used as a scale.
        var resolution = Overlays.getProperty(overlayID, "resolution");
        if (resolution === undefined) {
            return;
        }
        resolution.z = 1;  // Circumvent divide-by-zero.
        var scale = Overlays.getProperty(overlayID, "dimensions");
        if (scale === undefined) {
            return;
        }
        scale.z = 0.01;    // overlay dimensions are 2D, not 3D.
        dimensions = Vec3.multiplyVbyV(Vec3.multiply(resolution, INCHES_TO_METERS / dpi), scale);
    } else {
        dimensions = Overlays.getProperty(overlayID, "dimensions");
        if (dimensions === undefined) {
            return;
        }
        if (!dimensions.z) {
            dimensions.z = 0.01;    // sometimes overlay dimensions are 2D, not 3D.
        }
    }
    var invDimensions = { x: 1 / dimensions.x, y: 1 / dimensions.y, z: 1 / dimensions.z };
    var normalizedPosition = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), DEFAULT_REGISTRATION_POINT);

    // 2D position on overlay plane in meters, relative to the bounding box upper-left hand corner.
    var position2D = { x: normalizedPosition.x * dimensions.x, y: (1 - normalizedPosition.y) * dimensions.y }; // flip y-axis

    return {
        entityID: null,
        overlayID: overlayID,
        distance: distance,
        position: position,
        position2D: position2D,
        normal: normal,
        normalizedPosition: normalizedPosition,
        dimensions: dimensions,
        valid: true
    };
}

function isNearStylusTarget(stylusTargets, edgeBorder, minNormalDistance, maxNormalDistance) {
    for (var i = 0; i < stylusTargets.length; i++) {
        var stylusTarget = stylusTargets[i];

        // check to see if the projected stylusTip is within within the 2d border
        var borderMin = {x: -edgeBorder, y: -edgeBorder};
        var borderMax = {x: stylusTarget.dimensions.x + edgeBorder, y: stylusTarget.dimensions.y + edgeBorder};
        if (stylusTarget.distance >= minNormalDistance && stylusTarget.distance <= maxNormalDistance &&
            stylusTarget.position2D.x >= borderMin.x && stylusTarget.position2D.y >= borderMin.y &&
            stylusTarget.position2D.x <= borderMax.x && stylusTarget.position2D.y <= borderMax.y) {
            return true;
        }
    }
    return false;
}

function calculateNearestStylusTarget(stylusTargets) {
    var nearestStylusTarget;

    for (var i = 0; i < stylusTargets.length; i++) {
        var stylusTarget = stylusTargets[i];

        if ((!nearestStylusTarget || stylusTarget.distance < nearestStylusTarget.distance) &&
            stylusTarget.normalizedPosition.x >= 0 && stylusTarget.normalizedPosition.y >= 0 &&
            stylusTarget.normalizedPosition.x <= 1 && stylusTarget.normalizedPosition.y <= 1) {
            nearestStylusTarget = stylusTarget;
        }
    }

    return nearestStylusTarget;
}

// EntityPropertiesCache is a helper class that contains a cache of entity properties.
// the hope is to prevent excess calls to Entity.getEntityProperties()
//
// usage:
//   call EntityPropertiesCache.addEntities with all the entities that you are interested in.
//   This will fetch their properties.  Then call EntityPropertiesCache.getProps to receive an object
//   containing a cache of all the properties previously fetched.
function EntityPropertiesCache() {
    this.cache = {};
}
EntityPropertiesCache.prototype.clear = function() {
    this.cache = {};
};
EntityPropertiesCache.prototype.addEntity = function(entityID) {
    var cacheEntry = this.cache[entityID];
    if (cacheEntry && cacheEntry.refCount) {
        cacheEntry.refCount += 1;
    } else {
        this._updateCacheEntry(entityID);
    }
};
EntityPropertiesCache.prototype.addEntities = function(entities) {
    var _this = this;
    entities.forEach(function(entityID) {
        _this.addEntity(entityID);
    });
};
EntityPropertiesCache.prototype._updateCacheEntry = function(entityID) {
    var props = Entities.getEntityProperties(entityID, GRABBABLE_PROPERTIES);

    // convert props.userData from a string to an object.
    var userData = {};
    if (props.userData) {
        try {
            userData = JSON.parse(props.userData);
        } catch (err) {
            print("WARNING: malformed userData on " + entityID + ", name = " + props.name + ", error = " + err);
        }
    }
    props.userData = userData;
    props.refCount = 1;

    this.cache[entityID] = props;
};
EntityPropertiesCache.prototype.update = function() {
    // delete any cacheEntries with zero refCounts.
    var entities = Object.keys(this.cache);
    for (var i = 0; i < entities.length; i++) {
        var props = this.cache[entities[i]];
        if (props.refCount === 0) {
            delete this.cache[entities[i]];
        } else {
            props.refCount = 0;
        }
    }
};
EntityPropertiesCache.prototype.getProps = function(entityID) {
    var obj = this.cache[entityID];
    return obj ? obj : undefined;
};
EntityPropertiesCache.prototype.getGrabbableProps = function(entityID) {
    var props = this.cache[entityID];
    if (props) {
        return props.userData.grabbableKey ? props.userData.grabbableKey : DEFAULT_GRABBABLE_DATA;
    } else {
        return undefined;
    }
};
EntityPropertiesCache.prototype.getGrabProps = function(entityID) {
    var props = this.cache[entityID];
    if (props) {
        return props.userData.grabKey ? props.userData.grabKey : {};
    } else {
        return undefined;
    }
};
EntityPropertiesCache.prototype.getWearableProps = function(entityID) {
    var props = this.cache[entityID];
    if (props) {
        return props.userData.wearable ? props.userData.wearable : {};
    } else {
        return undefined;
    }
};
EntityPropertiesCache.prototype.getEquipHotspotsProps = function(entityID) {
    var props = this.cache[entityID];
    if (props) {
        return props.userData.equipHotspots ? props.userData.equipHotspots : {};
    } else {
        return undefined;
    }
};

// global cache
var entityPropertiesCache = new EntityPropertiesCache();

// Each overlayInfoSet describes a single equip hotspot.
// It is an object with the following keys:
//   timestamp - last time this object was updated, used to delete stale hotspot overlays.
//   entityID - entity assosicated with this hotspot
//   localPosition - position relative to the entity
//   hotspot - hotspot object
//   overlays - array of overlay objects created by Overlay.addOverlay()
//   currentSize - current animated scale value
//   targetSize - the target of our scale animations
//   type - "sphere" or "model".
function EquipHotspotBuddy() {
    // holds map from {string} hotspot.key to {object} overlayInfoSet.
    this.map = {};

    // array of all hotspots that are highlighed.
    this.highlightedHotspots = [];
}
EquipHotspotBuddy.prototype.clear = function() {
    var keys = Object.keys(this.map);
    for (var i = 0; i < keys.length; i++) {
        var overlayInfoSet = this.map[keys[i]];
        this.deleteOverlayInfoSet(overlayInfoSet);
    }
    this.map = {};
    this.highlightedHotspots = [];
};
EquipHotspotBuddy.prototype.highlightHotspot = function(hotspot) {
    this.highlightedHotspots.push(hotspot.key);
};
EquipHotspotBuddy.prototype.updateHotspot = function(hotspot, timestamp) {
    var overlayInfoSet = this.map[hotspot.key];
    if (!overlayInfoSet) {
        // create a new overlayInfoSet
        overlayInfoSet = {
            timestamp: timestamp,
            entityID: hotspot.entityID,
            localPosition: hotspot.localPosition,
            hotspot: hotspot,
            currentSize: 0,
            targetSize: 1,
            overlays: []
        };

        var diameter = hotspot.radius * 2;

        // override default sphere with a user specified model, if it exists.
        overlayInfoSet.overlays.push(Overlays.addOverlay("model", {
            name: "hotspot overlay",
            url: hotspot.modelURL ? hotspot.modelURL : DEFAULT_SPHERE_MODEL_URL,
            position: hotspot.worldPosition,
            rotation: {
                x: 0,
                y: 0,
                z: 0,
                w: 1
            },
            dimensions: diameter * EQUIP_SPHERE_SCALE_FACTOR,
            scale: hotspot.modelScale,
            ignoreRayIntersection: true
        }));
        overlayInfoSet.type = "model";
        this.map[hotspot.key] = overlayInfoSet;
    } else {
        overlayInfoSet.timestamp = timestamp;
    }
};
EquipHotspotBuddy.prototype.updateHotspots = function(hotspots, timestamp) {
    var _this = this;
    hotspots.forEach(function(hotspot) {
        _this.updateHotspot(hotspot, timestamp);
    });
    this.highlightedHotspots = [];
};
EquipHotspotBuddy.prototype.update = function(deltaTime, timestamp) {

    var HIGHLIGHT_SIZE = 1.1;
    var NORMAL_SIZE = 1.0;

    var keys = Object.keys(this.map);
    for (var i = 0; i < keys.length; i++) {
        var overlayInfoSet = this.map[keys[i]];

        // this overlayInfo is highlighted.
        if (this.highlightedHotspots.indexOf(keys[i]) != -1) {
            overlayInfoSet.targetSize = HIGHLIGHT_SIZE;
        } else {
            overlayInfoSet.targetSize = NORMAL_SIZE;
        }

        // start to fade out this hotspot.
        if (overlayInfoSet.timestamp != timestamp) {
            // because this item timestamp has expired, it might not be in the cache anymore....
            entityPropertiesCache.addEntity(overlayInfoSet.entityID);
            overlayInfoSet.targetSize = 0;
        }

        // animate the size.
        var SIZE_TIMESCALE = 0.1;
        var tau = deltaTime / SIZE_TIMESCALE;
        if (tau > 1.0) {
            tau = 1.0;
        }
        overlayInfoSet.currentSize += (overlayInfoSet.targetSize - overlayInfoSet.currentSize) * tau;

        if (overlayInfoSet.timestamp != timestamp && overlayInfoSet.currentSize <= 0.05) {
            // this is an old overlay, that has finished fading out, delete it!
            overlayInfoSet.overlays.forEach(Overlays.deleteOverlay);
            delete this.map[keys[i]];
        } else {
            // update overlay position, rotation to follow the object it's attached to.

            var props = entityPropertiesCache.getProps(overlayInfoSet.entityID);
            var entityXform = new Xform(props.rotation, props.position);
            var position = entityXform.xformPoint(overlayInfoSet.localPosition);

            var dimensions;
            if (overlayInfoSet.type == "sphere") {
                dimensions = overlayInfoSet.hotspot.radius * 2 * overlayInfoSet.currentSize * EQUIP_SPHERE_SCALE_FACTOR;
            } else {
                dimensions = overlayInfoSet.hotspot.radius * 2 * overlayInfoSet.currentSize;
            }

            overlayInfoSet.overlays.forEach(function(overlay) {
                Overlays.editOverlay(overlay, {
                    position: position,
                    rotation: props.rotation,
                    dimensions: dimensions
                });
            });
        }
    }
};

function getControllerJointIndex(hand) {
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
}

// global EquipHotspotBuddy instance
var equipHotspotBuddy = new EquipHotspotBuddy();

function MyController(hand) {
    this.hand = hand;
    this.autoUnequipCounter = 0;
    this.grabPointIntersectsEntity = false;
    this.stylus = null;
    this.homeButtonTouched = false;
    this.editTriggered = false;

    // Until there is some reliable way to keep track of a "stack" of parentIDs, we'll have problems
    // when more than one avatar does parenting grabs on things.  This script tries to work
    // around this with two associative arrays: previousParentID and previousParentJointIndex.  If
    // (1) avatar-A does a parenting grab on something, and then (2) avatar-B takes it, and (3) avatar-A
    // releases it and then (4) avatar-B releases it, then avatar-B will set the parent back to
    // avatar-A's hand.  Avatar-A is no longer grabbing it, so it will end up triggering avatar-A's
    // checkForUnexpectedChildren which will put it back to wherever it was when avatar-A initially grabbed it.
    // this will work most of the time, unless avatar-A crashes or logs out while avatar-B is grabbing the
    // entity.  This can also happen when a single avatar passes something from hand to hand.
    this.previousParentID = {};
    this.previousParentJointIndex = {};
    this.previouslyUnhooked = {};

    this.shouldScale = false;
    this.isScalingAvatar = false;

    // handPosition is where the avatar's hand appears to be, in-world.
    this.getHandPosition = function () {
        if (this.hand === RIGHT_HAND) {
            return MyAvatar.getRightPalmPosition();
        } else {
            return MyAvatar.getLeftPalmPosition();
        }
    };
    this.getHandRotation = function () {
        if (this.hand === RIGHT_HAND) {
            return MyAvatar.getRightPalmRotation();
        } else {
            return MyAvatar.getLeftPalmRotation();
        }
    };

    this.handToController = function() {
        return (hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
    };

    this.actionID = null; // action this script created...
    this.grabbedThingID = null; // on this entity.
    this.grabbedOverlay = null;
    this.state = STATE_OFF;
    this.pointer = null; // entity-id of line object

    this.triggerValue = 0; // rolling average of trigger value
    this.triggerClicked = false;
    this.rawTriggerValue = 0;
    this.rawSecondaryValue = 0;
    this.rawThumbValue = 0;

    // for visualizations
    this.overlayLine = null;
    this.searchSphere = null;
    this.otherGrabbingLine = null;

    this.otherGrabbingUUID = null;

    this.waitForTriggerRelease = false;

    // how far from camera to search intersection?
    var DEFAULT_SEARCH_SPHERE_DISTANCE = 1000;
    this.intersectionDistance = 0.0;
    this.searchSphereDistance = DEFAULT_SEARCH_SPHERE_DISTANCE;

    this.ignoreIK = false;
    this.offsetPosition = Vec3.ZERO;
    this.offsetRotation = Quat.IDENTITY;

    this.lastPickTime = 0;
    this.lastUnequipCheckTime = 0;

    this.equipOverlayInfoSetMap = {};

    this.tabletStabbed = false;
    this.tabletStabbedPos2D = null;
    this.tabletStabbedPos3D = null;

    this.useFingerInsteadOfStylus = false;
    this.fingerPointing = false;

    // initialize stylus tip
    var DEFAULT_STYLUS_TIP = {
        position: {x: 0, y: 0, z: 0},
        orientation: {x: 0, y: 0, z: 0, w: 0},
        rotation: {x: 0, y: 0, z: 0, w: 0},
        velocity: {x: 0, y: 0, z: 0},
        valid: false
    };
    this.stylusTip = DEFAULT_STYLUS_TIP;

    var _this = this;

    var suppressedIn2D = [STATE_OFF, STATE_SEARCHING];
    this.ignoreInput = function() {
        // We've made the decision to use 'this' for new code, even though it is fragile,
        // in order to keep/ the code uniform without making any no-op line changes.
        return (-1 !== suppressedIn2D.indexOf(this.state)) && isIn2DMode();
    };

    this.updateStylusTip = function() {
        if (this.useFingerInsteadOfStylus) {
            this.stylusTip = getFingerWorldLocation(this.hand);
        } else {
            this.stylusTip = getControllerWorldLocation(this.handToController(), true);

            // translate tip forward according to constant.
            var TIP_OFFSET = {x: 0, y: WEB_STYLUS_LENGTH - WEB_TOUCH_Y_OFFSET, z: 0};
            this.stylusTip.position = Vec3.sum(this.stylusTip.position, Vec3.multiplyQbyV(this.stylusTip.orientation, TIP_OFFSET));
        }

        // compute tip velocity from hand controller motion, it is more accurate then computing it from previous positions.
        var pose = Controller.getPoseValue(this.handToController());
        if (pose.valid) {
            var worldControllerPos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation));
            var worldControllerLinearVel = Vec3.multiplyQbyV(MyAvatar.orientation, pose.velocity);
            var worldControllerAngularVel = Vec3.multiplyQbyV(MyAvatar.orientation, pose.angularVelocity);
            var tipVelocity = Vec3.sum(worldControllerLinearVel, Vec3.cross(worldControllerAngularVel, Vec3.subtract(this.stylusTip.position, worldControllerPos)));
            this.stylusTip.velocity = tipVelocity;
        } else {
            this.stylusTip.velocity = {x: 0, y: 0, z: 0};
        }
    };

    this.update = function(deltaTime, timestamp) {
        this.updateSmoothedTrigger();
        this.maybeScaleMyAvatar();

        this.updateStylusTip();

        var DEFAULT_USE_FINGER_AS_STYLUS = false;
        var USE_FINGER_AS_STYLUS = Settings.getValue("preferAvatarFingerOverStylus");
        if (USE_FINGER_AS_STYLUS === "") {
            USE_FINGER_AS_STYLUS = DEFAULT_USE_FINGER_AS_STYLUS;
        }
        if (USE_FINGER_AS_STYLUS && MyAvatar.getJointIndex("LeftHandIndex4") !== -1) {
            this.useFingerInsteadOfStylus = true;
        } else {
            this.useFingerInsteadOfStylus = false;
        }

        if (this.ignoreInput()) {

            // Most hand input is disabled, because we are interacting with the 2d hud.
            // However, we still should check for collisions of the stylus with the web overlay.
            this.processStylus();
            this.turnOffVisualizations();
            return;
        }

        if (CONTROLLER_STATE_MACHINE[this.state]) {
            var updateMethodName = CONTROLLER_STATE_MACHINE[this.state].updateMethod;
            var updateMethod = this[updateMethodName];
            if (updateMethod) {
                updateMethod.call(this, deltaTime, timestamp);
            } else {
                print("WARNING: could not find updateMethod for state " + stateToName(this.state));
            }
        } else {
            print("WARNING: could not find state " + this.state + " in state machine");
        }
    };

    this.callEntityMethodOnGrabbed = function(entityMethodName) {
        if (this.grabbedIsOverlay) {
            return;
        }
        var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
        Entities.callEntityMethod(this.grabbedThingID, entityMethodName, args);
    };

    this.setState = function(newState, reason) {
        if ((isInEditMode() && this.grabbedThingID !== HMD.tabletID) &&
            (newState !== STATE_OFF &&
             newState !== STATE_SEARCHING &&
             newState !== STATE_STYLUS_TOUCHING &&
             newState !== STATE_OVERLAY_LASER_TOUCHING)) {
            return;
        }
        setGrabCommunications((newState === STATE_DISTANCE_HOLDING) ||
                              (newState === STATE_DISTANCE_ROTATING) ||
                              (newState === STATE_NEAR_GRABBING));
        if (WANT_DEBUG || WANT_DEBUG_STATE) {
            var oldStateName = stateToName(this.state);
            var newStateName = stateToName(newState);
            print("STATE (" + this.hand + "): " + this.state + "-" + newStateName +
                  " <-- " + oldStateName + ", reason = " + reason);
        }

        // exit the old state
        if (CONTROLLER_STATE_MACHINE[this.state]) {
            var exitMethodName = CONTROLLER_STATE_MACHINE[this.state].exitMethod;
            var exitMethod = this[exitMethodName];
            if (exitMethod) {
                exitMethod.call(this);
            }
        } else {
            print("WARNING: could not find state " + this.state + " in state machine");
        }

        this.state = newState;

        // enter the new state
        if (CONTROLLER_STATE_MACHINE[newState]) {
            var enterMethodName = CONTROLLER_STATE_MACHINE[newState].enterMethod;
            var enterMethod = this[enterMethodName];
            if (enterMethod) {
                enterMethod.call(this);
            }
        } else {
            print("WARNING: could not find newState " + newState + " in state machine");
        }
    };

    this.grabPointSphereOn = function() {
        if (!SHOW_GRAB_POINT_SPHERE) {
            return;
        }

        if (!this.grabPointSphere) {
            this.grabPointSphere = Overlays.addOverlay("sphere", {
                name: "grabPointSphere",
                localPosition: getGrabPointSphereOffset(this.handToController()),
                localRotation: { x: 0, y: 0, z: 0, w: 1 },
                dimensions: GRAB_POINT_SPHERE_RADIUS * 2,
                color: GRAB_POINT_SPHERE_COLOR,
                alpha: GRAB_POINT_SPHERE_ALPHA,
                solid: true,
                visible: true,
                ignoreRayIntersection: true,
                drawInFront: false,
                parentID: AVATAR_SELF_ID,
                parentJointIndex: this.controllerJointIndex
            });
        }
    };

    this.grabPointSphereOff = function() {
        if (this.grabPointSphere) {
            Overlays.deleteOverlay(this.grabPointSphere);
            this.grabPointSphere = null;
        }
    };

    this.searchSphereOn = function(location, size, color) {

        var rotation = Quat.lookAt(location, Camera.getPosition(), Vec3.UP);
        var brightColor = colorPow(color, 0.06);
        if (this.searchSphere === null) {
            var sphereProperties = {
                name: "searchSphere",
                position: location,
                rotation: rotation,
                outerRadius: size * 1.2,
                innerColor: brightColor,
                outerColor: color,
                innerAlpha: 0.9,
                outerAlpha: 0.0,
                solid: true,
                ignoreRayIntersection: true,
                drawInFront: true, // Even when burried inside of something, show it.
                visible: true
            };
            this.searchSphere = Overlays.addOverlay("circle3d", sphereProperties);
        } else {
            Overlays.editOverlay(this.searchSphere, {
                position: location,
                rotation: rotation,
                innerColor: brightColor,
                outerColor: color,
                innerAlpha: 1.0,
                outerAlpha: 0.0,
                outerRadius: size * 1.2,
                visible: true,
                ignoreRayIntersection: true
            });
        }
    };

    this.showStylus = function() {
        if (this.stylus) {
            return;
        }

        var stylusProperties = {
            name: "stylus",
            url: Script.resourcesPath() + "meshes/tablet-stylus-fat.fbx",
            loadPriority: 10.0,
            localPosition: Vec3.sum({ x: 0.0,
                                      y: WEB_TOUCH_Y_OFFSET,
                                      z: 0.0 },
                                    getGrabPointSphereOffset(this.handToController())),
            localRotation: Quat.fromVec3Degrees({ x: -90, y: 0, z: 0 }),
            dimensions: { x: 0.01, y: 0.01, z: WEB_STYLUS_LENGTH },
            solid: true,
            visible: true,
            ignoreRayIntersection: true,
            drawInFront: false,
            parentID: AVATAR_SELF_ID,
            parentJointIndex: MyAvatar.getJointIndex(this.hand === RIGHT_HAND ?
                                                     "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" :
                                                     "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND")
        };
        this.stylus = Overlays.addOverlay("model", stylusProperties);
    };

    this.hideStylus = function() {
        if (!this.stylus) {
            return;
        }
        Overlays.deleteOverlay(this.stylus);
        this.stylus = null;
    };

    this.overlayLineOn = function(closePoint, farPoint, color, farParentID) {
        if (this.overlayLine === null) {
            var lineProperties = {
                name: "line",
                glow: 1.0,
                lineWidth: 5,
                start: closePoint,
                end: farPoint,
                color: color,
                ignoreRayIntersection: true, // always ignore this
                drawInFront: true, // Even when burried inside of something, show it.
                visible: true,
                alpha: 1,
                parentID: AVATAR_SELF_ID,
                parentJointIndex: MyAvatar.getJointIndex(this.hand === RIGHT_HAND ?
                                                         "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" :
                                                         "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND"),
                endParentID: farParentID
            };
            this.overlayLine = Overlays.addOverlay("line3d", lineProperties);

        } else {
            if (farParentID && farParentID != NULL_UUID) {
                Overlays.editOverlay(this.overlayLine, {
                    color: color,
                        endParentID: farParentID
                });
            } else {
                Overlays.editOverlay(this.overlayLine, {
                    length: Vec3.distance(farPoint, closePoint),
                    color: color,
                    endParentID: farParentID
                });
            }
        }
    };

    this.searchIndicatorOn = function(distantPickRay) {
        var handPosition = distantPickRay.origin;
        var SEARCH_SPHERE_SIZE = 0.011;
        var SEARCH_SPHERE_FOLLOW_RATE = 0.50;

        if (this.intersectionDistance > 0) {
            //  If we hit something with our pick ray, move the search sphere toward that distance
            this.searchSphereDistance = this.searchSphereDistance * SEARCH_SPHERE_FOLLOW_RATE +
                this.intersectionDistance * (1.0 - SEARCH_SPHERE_FOLLOW_RATE);
        }

        var searchSphereLocation = Vec3.sum(distantPickRay.origin,
                                            Vec3.multiply(distantPickRay.direction, this.searchSphereDistance));
        this.searchSphereOn(searchSphereLocation, SEARCH_SPHERE_SIZE * this.searchSphereDistance,
                            (this.triggerSmoothedGrab() || this.secondarySqueezed()) ?
                            COLORS_GRAB_SEARCHING_FULL_SQUEEZE :
                            COLORS_GRAB_SEARCHING_HALF_SQUEEZE);
        if (PICK_WITH_HAND_RAY) {
            this.overlayLineOn(handPosition, searchSphereLocation,
                               (this.triggerSmoothedGrab() || this.secondarySqueezed()) ?
                               COLORS_GRAB_SEARCHING_FULL_SQUEEZE :
                               COLORS_GRAB_SEARCHING_HALF_SQUEEZE);
        }
    };

    // Turns off indicators used for searching. Overlay line and sphere.
    this.searchIndicatorOff = function() {
        this.searchSphereOff();
        if (PICK_WITH_HAND_RAY) {
            this.overlayLineOff();
        }
    };

    this.otherGrabbingLineOn = function(avatarPosition, entityPosition, color) {
        if (this.otherGrabbingLine === null) {
            var lineProperties = {
                lineWidth: 5,
                start: avatarPosition,
                end: entityPosition,
                color: color,
                glow: 1.0,
                ignoreRayIntersection: true,
                drawInFront: true,
                visible: true,
                alpha: 1
            };
            this.otherGrabbingLine = Overlays.addOverlay("line3d", lineProperties);
        } else {
            Overlays.editOverlay(this.otherGrabbingLine, {
                start: avatarPosition,
                end: entityPosition,
                color: color
            });
        }
    };

    this.evalLightWorldTransform = function(modelPos, modelRot) {

        var MODEL_LIGHT_POSITION = {
            x: 0,
            y: -0.3,
            z: 0
        };

        var MODEL_LIGHT_ROTATION = Quat.angleAxis(-90, {
            x: 1,
            y: 0,
            z: 0
        });

        return {
            p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
            q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
        };
    };

    this.lineOff = function() {
        if (this.pointer !== null) {
            Entities.deleteEntity(this.pointer);
        }
        this.pointer = null;
    };

    this.overlayLineOff = function() {
        if (this.overlayLine !== null) {
            Overlays.deleteOverlay(this.overlayLine);
        }
        this.overlayLine = null;
    };

    this.searchSphereOff = function() {
        if (this.searchSphere !== null) {
            Overlays.deleteOverlay(this.searchSphere);
            this.searchSphere = null;
            this.searchSphereDistance = DEFAULT_SEARCH_SPHERE_DISTANCE;
            this.intersectionDistance = 0.0;
        }
    };

    this.otherGrabbingLineOff = function() {
        if (this.otherGrabbingLine !== null) {
            Overlays.deleteOverlay(this.otherGrabbingLine);
        }
        this.otherGrabbingLine = null;
    };

    this.turnOffVisualizations = function() {
        this.overlayLineOff();
        this.grabPointSphereOff();
        this.lineOff();
        this.searchSphereOff();
        this.otherGrabbingLineOff();
        restore2DMode();
    };

    this.triggerPress = function(value) {
        _this.rawTriggerValue = value;
    };

    this.triggerClick = function(value) {
        _this.triggerClicked = value;
    };

    this.secondaryPress = function(value) {
        _this.rawSecondaryValue = value;
    };

    this.updateSmoothedTrigger = function() {
        var triggerValue = this.rawTriggerValue;
        // smooth out trigger value
        this.triggerValue = (this.triggerValue * TRIGGER_SMOOTH_RATIO) +
            (triggerValue * (1.0 - TRIGGER_SMOOTH_RATIO));
    };

    this.triggerSmoothedGrab = function() {
        return this.triggerClicked;
    };

    this.triggerSmoothedSqueezed = function() {
        return this.triggerValue > TRIGGER_ON_VALUE;
    };

    this.triggerSmoothedReleased = function() {
        return this.triggerValue < TRIGGER_OFF_VALUE;
    };

    this.secondarySqueezed = function() {
        return _this.rawSecondaryValue > BUMPER_ON_VALUE;
    };

    this.secondaryReleased = function() {
        return _this.rawSecondaryValue < BUMPER_ON_VALUE;
    };

    // this.triggerOrsecondarySqueezed = function () {
    //     return triggerSmoothedSqueezed() || secondarySqueezed();
    // }

    // this.triggerAndSecondaryReleased = function () {
    //     return triggerSmoothedReleased() && secondaryReleased();
    // }

    this.thumbPress = function(value) {
        _this.rawThumbValue = value;
    };

    this.thumbPressed = function() {
        return _this.rawThumbValue > THUMB_ON_VALUE;
    };

    this.thumbReleased = function() {
        return _this.rawThumbValue < THUMB_ON_VALUE;
    };

    this.stealTouchFocus = function(stylusTarget) {
        // send hover events to target
        // record the entity or overlay we are hovering over.
        if ((stylusTarget.entityID === this.getOtherHandController().hoverEntity) ||
            (stylusTarget.overlayID === this.getOtherHandController().hoverOverlay)) {
            this.getOtherHandController().relinquishTouchFocus();
        }
        this.requestTouchFocus(stylusTarget);
    };

    this.requestTouchFocus = function(stylusTarget) {

        // send hover events to target if we can.
        // record the entity or overlay we are hovering over.
        if (stylusTarget.entityID && stylusTarget.entityID !== this.hoverEntity && stylusTarget.entityID !== this.getOtherHandController().hoverEntity) {
            this.hoverEntity = stylusTarget.entityID;
            sendHoverEnterEventToStylusTarget(this.hand, stylusTarget);
        } else if (stylusTarget.overlayID && stylusTarget.overlayID !== this.hoverOverlay && stylusTarget.overlayID !== this.getOtherHandController().hoverOverlay) {
            this.hoverOverlay = stylusTarget.overlayID;
            sendHoverEnterEventToStylusTarget(this.hand, stylusTarget);
        }
    };

    this.hasTouchFocus = function(stylusTarget) {
        return ((stylusTarget.entityID && stylusTarget.entityID === this.hoverEntity) ||
                (stylusTarget.overlayID && stylusTarget.overlayID === this.hoverOverlay));
    };

    this.relinquishTouchFocus = function() {

        // send hover leave event.
        var pointerEvent = { type: "Move", id: this.hand + 1 };
        if (this.hoverEntity) {
            Entities.sendHoverLeaveEntity(this.hoverEntity, pointerEvent);
            this.hoverEntity = null;
        } else if (this.hoverOverlay) {
            Overlays.sendMouseMoveOnOverlay(this.hoverOverlay, pointerEvent);
            Overlays.sendHoverOverOverlay(this.hoverOverlay, pointerEvent);
            Overlays.sendHoverLeaveOverlay(this.hoverOverlay, pointerEvent);
            this.hoverOverlay = null;
        }
    };

    this.pointFinger = function(value) {
        var HIFI_POINT_INDEX_MESSAGE_CHANNEL = "Hifi-Point-Index";
        if (this.fingerPointing !== value) {
            var message;
            if (this.hand === RIGHT_HAND) {
                message = { pointRightIndex: value };
            } else {
                message = { pointLeftIndex: value };
            }
            Messages.sendMessage(HIFI_POINT_INDEX_MESSAGE_CHANNEL, JSON.stringify(message), true);
            this.fingerPointing = value;
        }
    };

    this.processStylus = function() {
        if (!this.stylusTip.valid) {
            this.pointFinger(false);
            this.hideStylus();
            return;
        }

        if (this.useFingerInsteadOfStylus) {
            this.hideStylus();
        }

        var tipPosition = this.stylusTip.position;

        // build list of stylus targets, near the stylusTip
        var stylusTargets = [];
        var candidateEntities = Entities.findEntities(tipPosition, WEB_DISPLAY_STYLUS_DISTANCE);
        entityPropertiesCache.addEntities(candidateEntities);
        var i, props, stylusTarget;
        for (i = 0; i < candidateEntities.length; i++) {
            props = entityPropertiesCache.getProps(candidateEntities[i]);
            if (props && (props.type === "Web" || this.isTablet(candidateEntities[i]))) {
                stylusTarget = calculateStylusTargetFromEntity(this.stylusTip, candidateEntities[i]);
                if (stylusTarget) {
                    stylusTargets.push(stylusTarget);
                }
            }
        }

        // add the tabletScreen, if it is valid
        if (HMD.tabletScreenID && HMD.tabletScreenID !== NULL_UUID && Overlays.getProperty(HMD.tabletScreenID, "visible")) {
            stylusTarget = calculateStylusTargetFromOverlay(this.stylusTip, HMD.tabletScreenID);
            if (stylusTarget) {
                stylusTargets.push(stylusTarget);
            }
        }

        // add the tablet home button.
        if (HMD.homeButtonID && HMD.homeButtonID !== NULL_UUID && Overlays.getProperty(HMD.homeButtonID, "visible")) {
            stylusTarget = calculateStylusTargetFromOverlay(this.stylusTip, HMD.homeButtonID);
            if (stylusTarget) {
                stylusTargets.push(stylusTarget);
            }
        }

        var TABLET_MIN_HOVER_DISTANCE = 0.01;
        var TABLET_MAX_HOVER_DISTANCE = 0.1;
        var TABLET_MIN_TOUCH_DISTANCE = -0.05;
        var TABLET_MAX_TOUCH_DISTANCE = TABLET_MIN_HOVER_DISTANCE;
        var EDGE_BORDER = 0.075;

        var hysteresisOffset = 0.0;
        if (this.isNearStylusTarget) {
            hysteresisOffset = 0.05;
        }

        this.isNearStylusTarget = isNearStylusTarget(stylusTargets, EDGE_BORDER + hysteresisOffset,
                                                     TABLET_MIN_TOUCH_DISTANCE - hysteresisOffset, WEB_DISPLAY_STYLUS_DISTANCE + hysteresisOffset);

        if (this.isNearStylusTarget) {
            if (!this.useFingerInsteadOfStylus) {
                this.showStylus();
            } else {
                this.pointFinger(true);
            }
        } else {
            this.hideStylus();
            this.pointFinger(false);
        }

        var nearestStylusTarget = calculateNearestStylusTarget(stylusTargets);

        if (nearestStylusTarget && nearestStylusTarget.distance > TABLET_MIN_TOUCH_DISTANCE &&
            nearestStylusTarget.distance < TABLET_MAX_HOVER_DISTANCE) {

            this.requestTouchFocus(nearestStylusTarget);

            if (!stylusTargetHasKeyboardFocus(nearestStylusTarget)) {
                setKeyboardFocusOnStylusTarget(nearestStylusTarget);
            }

            if (this.hasTouchFocus(nearestStylusTarget)) {
                sendHoverOverEventToStylusTarget(this.hand, nearestStylusTarget);
            }

            // filter out presses when tip is moving away from tablet.
            // ensure that stylus is within bounding box by checking normalizedPosition
            if (nearestStylusTarget.valid && nearestStylusTarget.distance > TABLET_MIN_TOUCH_DISTANCE &&
                nearestStylusTarget.distance < TABLET_MAX_TOUCH_DISTANCE && Vec3.dot(this.stylusTip.velocity, nearestStylusTarget.normal) < 0 &&
                nearestStylusTarget.normalizedPosition.x >= 0 && nearestStylusTarget.normalizedPosition.x <= 1 &&
                nearestStylusTarget.normalizedPosition.y >= 0 && nearestStylusTarget.normalizedPosition.y <= 1) {

                var name;
                if (nearestStylusTarget.entityID) {
                    name = entityPropertiesCache.getProps(nearestStylusTarget.entityID).name;
                    this.stylusTarget = nearestStylusTarget;
                    this.setState(STATE_STYLUS_TOUCHING, "begin touching entity '" + name + "'");
                } else if (nearestStylusTarget.overlayID) {
                    name = Overlays.getProperty(nearestStylusTarget.overlayID, "name");
                    this.stylusTarget = nearestStylusTarget;
                    this.setState(STATE_STYLUS_TOUCHING, "begin touching overlay '" + name + "'");
                }
            }
        } else {
            this.relinquishTouchFocus();
        }

        this.homeButtonTouched = false;
    };

    this.off = function(deltaTime, timestamp) {

        this.controllerJointIndex = getControllerJointIndex(this.hand);
        this.checkForUnexpectedChildren();

        if (this.editTriggered) {
            this.editTriggered = false;
        }

        if (this.triggerSmoothedReleased() && this.secondaryReleased()) {
            this.waitForTriggerRelease = false;
        }
        if (!this.waitForTriggerRelease && (this.triggerSmoothedSqueezed() || this.secondarySqueezed())) {
            this.lastPickTime = 0;
            this.startingHandRotation = getControllerWorldLocation(this.handToController(), true).orientation;
            this.searchStartTime = Date.now();
            this.setState(STATE_SEARCHING, "trigger squeeze detected");
            return;
        }

        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var worldHandPosition = controllerLocation.position;

        var candidateEntities = Entities.findEntities(worldHandPosition, MAX_EQUIP_HOTSPOT_RADIUS);
        entityPropertiesCache.addEntities(candidateEntities);
        var potentialEquipHotspot = this.chooseBestEquipHotspot(candidateEntities);
        if (!this.waitForTriggerRelease) {
            this.updateEquipHaptics(potentialEquipHotspot, worldHandPosition);
        }

        var nearEquipHotspots = this.chooseNearEquipHotspots(candidateEntities, EQUIP_HOTSPOT_RENDER_RADIUS);
        equipHotspotBuddy.updateHotspots(nearEquipHotspots, timestamp);
        if (potentialEquipHotspot) {
            equipHotspotBuddy.highlightHotspot(potentialEquipHotspot);
        }

        // when the grab-point enters a grabable entity, give a haptic pulse
        candidateEntities = Entities.findEntities(worldHandPosition, NEAR_GRAB_RADIUS);
        var grabbableEntities = candidateEntities.filter(function(entity) {
            return _this.entityIsNearGrabbable(entity, worldHandPosition, NEAR_GRAB_MAX_DISTANCE);
        });
        if (grabbableEntities.length > 0) {
            if (!this.grabPointIntersectsEntity) {
                // don't do haptic pulse for tablet
                var nonTabletEntities = grabbableEntities.filter(function(entityID) {
                    return entityID != HMD.tabletID && entityID != HMD.homeButtonID;
                });
                if (nonTabletEntities.length > 0) {
                    Controller.triggerHapticPulse(1, 20, this.hand);
                }
                this.grabPointIntersectsEntity = true;
                this.grabPointSphereOn();
            }
        } else {
            this.grabPointIntersectsEntity = false;
            this.grabPointSphereOff();
        }

        this.processStylus();

        if (isInEditMode() && !this.isNearStylusTarget && HMD.isHandControllerAvailable()) {
            // Always showing lasers while in edit mode and hands/stylus is not active.

            var rayPickInfo = this.calcRayPickInfo(this.hand);
            if (rayPickInfo.isValid) {
                this.intersectionDistance = (rayPickInfo.entityID || rayPickInfo.overlayID) ? rayPickInfo.distance : 0;
                this.searchIndicatorOn(rayPickInfo.searchRay);
            } else {
                this.searchIndicatorOff();
            }
        } else {
            this.searchIndicatorOff();
        }
    };

    this.handleLaserOnHomeButton = function(rayPickInfo) {
        if (rayPickInfo.overlayID && this.triggerSmoothedGrab()) {
            var homeButton = rayPickInfo.overlayID;
            var hmdHomeButton = HMD.homeButtonID;
            if (homeButton === hmdHomeButton) {
                if (this.homeButtonTouched === false) {
                    this.homeButtonTouched = true;
                    Controller.triggerHapticPulse(HAPTIC_LASER_UI_STRENGTH, HAPTIC_LASER_UI_DURATION, this.hand);
                    Messages.sendLocalMessage("home", homeButton);
                }
            } else {
                this.homeButtonTouched = false;
            }
        } else {
            this.homeButtonTouched = false;
        }
    };

    this.clearEquipHaptics = function() {
        this.prevPotentialEquipHotspot = null;
    };

    this.updateEquipHaptics = function(potentialEquipHotspot, currentLocation) {
        if (potentialEquipHotspot && !this.prevPotentialEquipHotspot ||
            !potentialEquipHotspot && this.prevPotentialEquipHotspot) {
            Controller.triggerHapticPulse(HAPTIC_TEXTURE_STRENGTH, HAPTIC_TEXTURE_DURATION, this.hand);
            this.lastHapticPulseLocation = currentLocation;
        } else if (potentialEquipHotspot &&
                   Vec3.distance(this.lastHapticPulseLocation, currentLocation) > HAPTIC_TEXTURE_DISTANCE) {
            Controller.triggerHapticPulse(HAPTIC_TEXTURE_STRENGTH, HAPTIC_TEXTURE_DURATION, this.hand);
            this.lastHapticPulseLocation = currentLocation;
        }
        this.prevPotentialEquipHotspot = potentialEquipHotspot;
    };

    // Performs ray pick test from the hand controller into the world
    // @param {number} which hand to use, RIGHT_HAND or LEFT_HAND
    // @param {object} if set, use this as as the pick ray, expects origin, direction, and length fields.
    // @returns {object} returns object with two keys entityID and distance
    //
    this.calcRayPickInfo = function(hand, pickRayOverride) {

        var pickRay;
        var valid = true
        if (pickRayOverride) {
            pickRay = pickRayOverride;
        } else {
            var controllerLocation = getControllerWorldLocation(this.handToController(), true);
            var worldHandPosition = controllerLocation.position;
            var worldHandRotation = controllerLocation.orientation;
            valid = !(worldHandPosition === undefined);

            pickRay = {
                origin: PICK_WITH_HAND_RAY ? worldHandPosition : Camera.position,
                direction: PICK_WITH_HAND_RAY ? Quat.getUp(worldHandRotation) : Vec3.mix(Quat.getUp(worldHandRotation),
                                                                                         Quat.getFront(Camera.orientation),
                                                                                         HAND_HEAD_MIX_RATIO),
                length: PICK_MAX_DISTANCE
            };
        }

        var result = {
            entityID: null,
            overlayID: null,
            searchRay: pickRay,
            distance: PICK_MAX_DISTANCE,
            isValid: valid
        };

        var now = Date.now();
        if (now - this.lastPickTime < MSECS_PER_SEC / PICKS_PER_SECOND_PER_HAND) {
            return result;
        }
        this.lastPickTime = now;

        var intersection;
        if (USE_BLACKLIST === true && blacklist.length !== 0) {
            intersection = findRayIntersection(pickRay, true, [], blacklist, true);
        } else {
            intersection = findRayIntersection(pickRay, true, [], [], true);
        }

        if (intersection.intersects) {
            return {
                entityID: intersection.entityID,
                overlayID: intersection.overlayID,
                searchRay: pickRay,
                distance: Vec3.distance(pickRay.origin, intersection.intersection),
                intersection: intersection.intersection,
                normal: intersection.surfaceNormal,
                properties: intersection.properties
            };
        } else {
            return result;
        }
    };

    this.entityWantsTrigger = function(entityID) {
        var grabbableProps = entityPropertiesCache.getGrabbableProps(entityID);
        return grabbableProps && grabbableProps.wantsTrigger;
    };

    // returns a list of all equip-hotspots assosiated with this entity.
    // @param {UUID} entityID
    // @returns {Object[]} array of objects with the following fields.
    //      * key {string} a string that can be used to uniquely identify this hotspot
    //      * entityID {UUID}
    //      * localPosition {Vec3} position of the hotspot in object space.
    //      * worldPosition {vec3} position of the hotspot in world space.
    //      * radius {number} radius of equip hotspot
    //      * joints {Object} keys are joint names values are arrays of two elements:
    //        offset position {Vec3} and offset rotation {Quat}, both are in the coordinate system of the joint.
    //      * modelURL {string} url for model to use instead of default sphere.
    //      * modelScale {Vec3} scale factor for model
    this.collectEquipHotspots = function(entityID) {
        var result = [];
        var props = entityPropertiesCache.getProps(entityID);
        var entityXform = new Xform(props.rotation, props.position);
        var equipHotspotsProps = entityPropertiesCache.getEquipHotspotsProps(entityID);
        if (equipHotspotsProps && equipHotspotsProps.length > 0) {
            var i, length = equipHotspotsProps.length;
            for (i = 0; i < length; i++) {
                var hotspot = equipHotspotsProps[i];
                if (hotspot.position && hotspot.radius && hotspot.joints) {
                    result.push({
                        key: entityID.toString() + i.toString(),
                        entityID: entityID,
                        localPosition: hotspot.position,
                        worldPosition: entityXform.xformPoint(hotspot.position),
                        radius: hotspot.radius,
                        joints: hotspot.joints,
                        modelURL: hotspot.modelURL,
                        modelScale: hotspot.modelScale
                    });
                }
            }
        } else {
            var wearableProps = entityPropertiesCache.getWearableProps(entityID);
            if (wearableProps && wearableProps.joints) {
                result.push({
                    key: entityID.toString() + "0",
                    entityID: entityID,
                    localPosition: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    worldPosition: entityXform.pos,
                    radius: EQUIP_RADIUS,
                    joints: wearableProps.joints,
                    modelURL: null,
                    modelScale: null
                });
            }
        }
        return result;
    };

    this.hotspotIsEquippable = function(hotspot) {
        var props = entityPropertiesCache.getProps(hotspot.entityID);
        var debug = (WANT_DEBUG_SEARCH_NAME && props.name === WANT_DEBUG_SEARCH_NAME);

        var otherHandControllerState = this.getOtherHandController().state;
        var okToEquipFromOtherHand = ((otherHandControllerState === STATE_NEAR_GRABBING ||
                                       otherHandControllerState === STATE_DISTANCE_HOLDING ||
                                       otherHandControllerState === STATE_DISTANCE_ROTATING) &&
                                      this.getOtherHandController().grabbedThingID === hotspot.entityID);
        var hasParent = true;
        if (props.parentID === NULL_UUID) {
            hasParent = false;
        }
        if ((hasParent || entityHasActions(hotspot.entityID)) && !okToEquipFromOtherHand) {
            if (debug) {
                print("equip is skipping '" + props.name + "': grabbed by someone else");
            }
            return false;
        }

        return true;
    };
    this.entityIsCloneable = function(entityID) {
        var entityProps = entityPropertiesCache.getGrabbableProps(entityID);
        var props = entityPropertiesCache.getProps(entityID);
        if (!props) {
            return false;
        }

        if (entityProps.hasOwnProperty("cloneable")) {
            return entityProps.cloneable;
        }
        return false;
    };
    this.entityIsGrabbable = function(entityID) {
        var grabbableProps = entityPropertiesCache.getGrabbableProps(entityID);
        var props = entityPropertiesCache.getProps(entityID);
        if (!props) {
            return false;
        }
        var debug = (WANT_DEBUG_SEARCH_NAME && props.name === WANT_DEBUG_SEARCH_NAME);
        var grabbable = propsArePhysical(props);
        if (grabbableProps.hasOwnProperty("grabbable")) {
            grabbable = grabbableProps.grabbable;
        }

        if (!grabbable && !grabbableProps.wantsTrigger) {
            if (debug) {
                print("grab is skipping '" + props.name + "': not grabbable.");
            }
            return false;
        }
        if (FORBIDDEN_GRAB_TYPES.indexOf(props.type) >= 0) {
            if (debug) {
                print("grab is skipping '" + props.name + "': forbidden entity type.");
            }
            return false;
        }
        if (props.locked && !grabbableProps.wantsTrigger) {
            if (debug) {
                print("grab is skipping '" + props.name + "': locked and not triggerable.");
            }
            return false;
        }
        if (FORBIDDEN_GRAB_NAMES.indexOf(props.name) >= 0) {
            if (debug) {
                print("grab is skipping '" + props.name + "': forbidden name.");
            }
            return false;
        }

        return true;
    };

    this.entityIsDistanceGrabbable = function(entityID, handPosition) {
        if (!this.entityIsGrabbable(entityID)) {
            return false;
        }

        var props = entityPropertiesCache.getProps(entityID);
        var distance = Vec3.distance(props.position, handPosition);
        var debug = (WANT_DEBUG_SEARCH_NAME && props.name === WANT_DEBUG_SEARCH_NAME);

        // we can't distance-grab non-physical
        var isPhysical = propsArePhysical(props);
        if (!isPhysical) {
            if (debug) {
                print("distance grab is skipping '" + props.name + "': not physical");
            }
            return false;
        }

        if (distance > PICK_MAX_DISTANCE) {
            // too far away, don't grab
            if (debug) {
                print("distance grab is skipping '" + props.name + "': too far away.");
            }
            return false;
        }

        this.otherGrabbingUUID = entityIsGrabbedByOther(entityID);
        if (this.otherGrabbingUUID !== null) {
            // don't distance grab something that is already grabbed.
            if (debug) {
                print("distance grab is skipping '" + props.name + "': already grabbed by another.");
            }
            return false;
        }

        return true;
    };

    this.entityIsNearGrabbable = function(entityID, handPosition, maxDistance) {

        if (!this.entityIsCloneable(entityID) && !this.entityIsGrabbable(entityID)) {
            return false;
        }

        var props = entityPropertiesCache.getProps(entityID);
        var distance = Vec3.distance(props.position, handPosition);
        var debug = (WANT_DEBUG_SEARCH_NAME && props.name === WANT_DEBUG_SEARCH_NAME);

        if (distance > maxDistance) {
            // too far away, don't grab
            if (debug) {
                print(" grab is skipping '" + props.name + "': too far away.");
            }
            return false;
        }

        return true;
    };

    this.entityIsFarToNearGrabbable = function (objectPosition, handPosition, maxDistance) {
        var distanceToObjectFromHand = Vec3.length(Vec3.subtract(handPosition, objectPosition));

        if (distanceToObjectFromHand > maxDistance) {
          return false;
        }
        return true;
    };

    this.chooseNearEquipHotspots = function(candidateEntities, distance) {
        var equippableHotspots = flatten(candidateEntities.map(function(entityID) {
            return _this.collectEquipHotspots(entityID);
        })).filter(function(hotspot) {
            return (_this.hotspotIsEquippable(hotspot) &&
                    Vec3.distance(hotspot.worldPosition, getControllerWorldLocation(_this.handToController(), true).position) <
                    hotspot.radius + distance);
        });
        return equippableHotspots;
    };

    this.chooseBestEquipHotspot = function(candidateEntities) {
        var DISTANCE = 0;
        var equippableHotspots = this.chooseNearEquipHotspots(candidateEntities, DISTANCE);
        var _this = this;
        if (equippableHotspots.length > 0) {
            // sort by distance
            equippableHotspots.sort(function(a, b) {
                var handControllerLocation = getControllerWorldLocation(_this.handToController(), true);
                var aDistance = Vec3.distance(a.worldPosition, handControllerLocation.position);
                var bDistance = Vec3.distance(b.worldPosition, handControllerLocation.position);
                return aDistance - bDistance;
            });
            return equippableHotspots[0];
        } else {
            return null;
        }
    };

    this.chooseNearEquipHotspotsForFarToNearEquip = function(candidateEntities, distance) {
        var equippableHotspots = flatten(candidateEntities.map(function(entityID) {
                return _this.collectEquipHotspots(entityID);
            })).filter(function(hotspot) {
                return (Vec3.distance(hotspot.worldPosition, getControllerWorldLocation(_this.handToController(), true).position) <
                    hotspot.radius + distance);
            });
            return equippableHotspots;
    };

    this.chooseBestEquipHotspotForFarToNearEquip = function(candidateEntities) {
        var DISTANCE = 1;
        var equippableHotspots = this.chooseNearEquipHotspotsForFarToNearEquip(candidateEntities, DISTANCE);
        var _this = this;
        if (equippableHotspots.length > 0) {
            // sort by distance
            equippableHotspots.sort(function(a, b) {
                var handControllerLocation = getControllerWorldLocation(_this.handToController(), true);
                var aDistance = Vec3.distance(a.worldPosition, handControllerLocation.position);
                var bDistance = Vec3.distance(b.worldPosition, handControllerLocation.position);
                return aDistance - bDistance;
            });
            return equippableHotspots[0];
        } else {
            return null;
        }
    };

    this.searchEnter = function() {
        mostRecentSearchingHand = this.hand;
        var rayPickInfo = this.calcRayPickInfo(this.hand);
        if (rayPickInfo.entityID || rayPickInfo.overlayID) {
            this.intersectionDistance = rayPickInfo.distance;
            this.searchSphereDistance = this.intersectionDistance;
        }
    };

    this.search = function(deltaTime, timestamp) {
        var _this = this;
        var name;
        var FAR_SEARCH_DELAY = 0;  //  msecs before search beam appears

        var farSearching =  this.triggerSmoothedSqueezed() && (Date.now() - this.searchStartTime > FAR_SEARCH_DELAY);

        this.grabbedThingID = null;
        this.grabbedOverlay = null;
        this.isInitialGrab = false;
        this.preparingHoldRelease = false;

        this.checkForUnexpectedChildren();

        if ((this.triggerSmoothedReleased() && this.secondaryReleased())) {
            this.grabbedThingID = null;
            this.setState(STATE_OFF, "trigger released");
            return;
        }

        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var handPosition = controllerLocation.position;

        var rayPickInfo = this.calcRayPickInfo(this.hand);

        if (rayPickInfo.entityID) {
            entityPropertiesCache.addEntity(rayPickInfo.entityID);
        }

        if (rayPickInfo.entityID && entitiesWithHoverOverlays.indexOf(rayPickInfo.entityID) == -1) {
            entitiesWithHoverOverlays.forEach(function (element) {
                HoverOverlay.destroyHoverOverlay(element);
            });
            entitiesWithHoverOverlays = [];
            HoverOverlay.createHoverOverlay(rayPickInfo.entityID);
            entitiesWithHoverOverlays.push(rayPickInfo.entityID);
        }

        var candidateHotSpotEntities = Entities.findEntities(handPosition, MAX_EQUIP_HOTSPOT_RADIUS);
        entityPropertiesCache.addEntities(candidateHotSpotEntities);

        var potentialEquipHotspot = this.chooseBestEquipHotspot(candidateHotSpotEntities);
        if (potentialEquipHotspot) {
            if ((this.triggerSmoothedGrab() || this.secondarySqueezed()) && holdEnabled) {
                this.grabbedHotspot = potentialEquipHotspot;
                this.grabbedThingID = potentialEquipHotspot.entityID;
                this.grabbedIsOverlay = false;
                this.setState(STATE_HOLD, "equipping '" + entityPropertiesCache.getProps(this.grabbedThingID).name + "'");

                return;
            }
        }

        var candidateEntities = Entities.findEntities(handPosition, NEAR_GRAB_RADIUS);
        var grabbableEntities = candidateEntities.filter(function(entity) {
            return _this.entityIsNearGrabbable(entity, handPosition, NEAR_GRAB_MAX_DISTANCE);
        });

        var candidateOverlays = Overlays.findOverlays(handPosition, NEAR_GRAB_RADIUS);
        var grabbableOverlays = candidateOverlays.filter(function(overlayID) {
            return Overlays.getProperty(overlayID, "grabbable");
        });

        if (rayPickInfo.entityID) {
            this.intersectionDistance = rayPickInfo.distance;
            if (this.entityIsGrabbable(rayPickInfo.entityID) && rayPickInfo.distance < NEAR_GRAB_PICK_RADIUS) {
                grabbableEntities.push(rayPickInfo.entityID);
            }
        } else if (rayPickInfo.overlayID) {
            this.intersectionDistance = rayPickInfo.distance;
        } else {
            this.intersectionDistance = 0;
        }

        if (grabbableOverlays.length > 0) {
            grabbableOverlays.sort(function(a, b) {
                var aPosition = Overlays.getProperty(a, "position");
                var aDistance = Vec3.distance(aPosition, handPosition);
                var bPosition = Overlays.getProperty(b, "position");
                var bDistance = Vec3.distance(bPosition, handPosition);
                return aDistance - bDistance;
            });
            this.grabbedThingID = grabbableOverlays[0];
            this.grabbedIsOverlay = true;
            if ((this.triggerSmoothedGrab() || this.secondarySqueezed()) && nearGrabEnabled) {
                this.setState(STATE_NEAR_GRABBING, "near grab overlay '" +
                              Overlays.getProperty(this.grabbedThingID, "name") + "'");
                return;
            }
        }

        var entity;
        if (grabbableEntities.length > 0) {
            // sort by distance
            grabbableEntities.sort(function(a, b) {
                var aDistance = Vec3.distance(entityPropertiesCache.getProps(a).position, handPosition);
                var bDistance = Vec3.distance(entityPropertiesCache.getProps(b).position, handPosition);
                return aDistance - bDistance;
            });
            entity = grabbableEntities[0];
            if (!isInEditMode() || entity == HMD.tabletID) { // tablet is grabbable, even when editing
                name = entityPropertiesCache.getProps(entity).name;
                this.grabbedThingID = entity;
                this.grabbedIsOverlay = false;
                if (this.entityWantsTrigger(entity)) {
                    if (this.triggerSmoothedGrab()) {
                        this.setState(STATE_NEAR_TRIGGER, "near trigger '" + name + "'");
                        return;
                    }
                } else {
                    //  If near something grabbable, grab it!
                    if ((this.triggerSmoothedGrab() || this.secondarySqueezed()) && nearGrabEnabled) {
                        this.setState(STATE_NEAR_GRABBING, "near grab entity '" + name + "'");
                        return;
                    }
                }
            }
        }

        if (rayPickInfo.distance >= WEB_STYLUS_LENGTH / 2.0 + WEB_TOUCH_Y_OFFSET) {
            this.handleLaserOnHomeButton(rayPickInfo);
            if (this.handleLaserOnWebEntity(rayPickInfo)) {
                return;
            }
            if (this.handleLaserOnWebOverlay(rayPickInfo)) {
                return;
            }
        }

        if (isInEditMode()) {
            this.searchIndicatorOn(rayPickInfo.searchRay);
            if (this.triggerSmoothedGrab()) {
                if (!this.editTriggered){
                    if (rayPickInfo.entityID) {
                        Messages.sendLocalMessage("entityToolUpdates", JSON.stringify({
                            method: "selectEntity",
                            entityID: rayPickInfo.entityID
                        }));
                    } else if (rayPickInfo.overlayID) {
                        Messages.sendLocalMessage("entityToolUpdates", JSON.stringify({
                            method: "selectOverlay",
                            overlayID: rayPickInfo.overlayID
                        }));
                    }
                }
                this.editTriggered = true;
            }
            Reticle.setVisible(false);
            return;
        }

        if (rayPickInfo.entityID) {
            entity = rayPickInfo.entityID;
            name = entityPropertiesCache.getProps(entity).name;
            if (this.entityWantsTrigger(entity)) {
                if (this.triggerSmoothedGrab()) {
                    this.grabbedThingID = entity;
                    this.grabbedIsOverlay = false;
                    this.setState(STATE_FAR_TRIGGER, "far trigger '" + name + "'");
                    return;
                } else {
                    // potentialFarTriggerEntity = entity;
                }
                this.otherGrabbingLineOff();
            } else if (this.entityIsDistanceGrabbable(rayPickInfo.entityID, handPosition)) {
                if (this.triggerSmoothedGrab() && !isEditing() && farGrabEnabled && farSearching) {
                    this.grabbedThingID = entity;
                    this.grabbedIsOverlay = false;
                    this.grabbedDistance = rayPickInfo.distance;
                    if (this.getOtherHandController().state === STATE_DISTANCE_HOLDING) {
                        this.setState(STATE_DISTANCE_ROTATING, "distance rotate '" + name + "'");
                    } else {
                        this.setState(STATE_DISTANCE_HOLDING, "distance hold '" + name + "'");
                    }
                    return;
                } else {
                    // potentialFarGrabEntity = entity;
                }
                this.otherGrabbingLineOff();
            } else if (this.otherGrabbingUUID !== null) {
                if (this.triggerSmoothedGrab() && !isEditing() && farGrabEnabled && farSearching) {
                    var avatar = AvatarList.getAvatar(this.otherGrabbingUUID);
                    var IN_FRONT_OF_AVATAR = { x: 0, y: 0.2, z: 0.4 };  // Up from hips and in front of avatar.
                    var startPosition = Vec3.sum(avatar.position, Vec3.multiplyQbyV(avatar.rotation, IN_FRONT_OF_AVATAR));
                    var rayHitProps = entityPropertiesCache.getProps(rayPickInfo.entityID);
                    var finishPisition = Vec3.sum(rayHitProps.position,  // Entity's centroid.
                        Vec3.multiplyQbyV(rayHitProps.rotation,
                        Vec3.multiplyVbyV(rayHitProps.dimensions,
                        Vec3.subtract(DEFAULT_REGISTRATION_POINT, rayHitProps.registrationPoint))));
                    this.otherGrabbingLineOn(startPosition, finishPisition, COLORS_GRAB_DISTANCE_HOLD);
                } else {
                    this.otherGrabbingLineOff();
                }
            } else {
                this.otherGrabbingLineOff();
            }
        } else {
            this.otherGrabbingLineOff();
        }

        this.updateEquipHaptics(potentialEquipHotspot, handPosition);

        var nearEquipHotspots = this.chooseNearEquipHotspots(candidateEntities, EQUIP_HOTSPOT_RENDER_RADIUS);
        equipHotspotBuddy.updateHotspots(nearEquipHotspots, timestamp);
        if (potentialEquipHotspot) {
            equipHotspotBuddy.highlightHotspot(potentialEquipHotspot);
        }

        if (farGrabEnabled && farSearching) {
            this.searchIndicatorOn(rayPickInfo.searchRay);
        }
        Reticle.setVisible(false);
    };

    this.isTablet = function (entityID) {
        if (entityID === HMD.tabletID) {
            return true;
        }
        return false;
    };

    this.handleLaserOnWebEntity = function (rayPickInfo) {
        var pointerEvent;

        if (rayPickInfo.entityID && Entities.wantsHandControllerPointerEvents(rayPickInfo.entityID)) {
            var entity = rayPickInfo.entityID;
            var name = entityPropertiesCache.getProps(entity).name;

            if (Entities.keyboardFocusEntity != entity) {
                Overlays.keyboardFocusOverlay = 0;
                Entities.keyboardFocusEntity = entity;

                pointerEvent = {
                    type: "Move",
                    id: this.hand + 1, // 0 is reserved for hardware mouse
                    pos2D: projectOntoEntityXYPlane(entity, rayPickInfo.intersection),
                    pos3D: rayPickInfo.intersection,
                     normal: rayPickInfo.normal,
                    direction: rayPickInfo.searchRay.direction,
                    button: "None"
                };

                this.hoverEntity = entity;
                Entities.sendHoverEnterEntity(entity, pointerEvent);
            }

            // send mouse events for button highlights and tooltips.
            if (this.hand == mostRecentSearchingHand ||
                (this.hand !== mostRecentSearchingHand &&
                 this.getOtherHandController().state !== STATE_SEARCHING &&
                 this.getOtherHandController().state !== STATE_STYLUS_TOUCHING &&
                 this.getOtherHandController().state !== STATE_ENTITY_LASER_TOUCHING &&
                 this.getOtherHandController().state !== STATE_OVERLAY_LASER_TOUCHING)) {

                // most recently searching hand has priority over other hand, for the purposes of button highlighting.
                pointerEvent = {
                    type: "Move",
                    id: this.hand + 1, // 0 is reserved for hardware mouse
                    pos2D: projectOntoEntityXYPlane(entity, rayPickInfo.intersection),
                    pos3D: rayPickInfo.intersection,
                    normal: rayPickInfo.normal,
                    direction: rayPickInfo.searchRay.direction,
                    button: "None"
                };

                Entities.sendMouseMoveOnEntity(entity, pointerEvent);
                Entities.sendHoverOverEntity(entity, pointerEvent);
            }

            if (this.triggerSmoothedGrab()) {
                this.grabbedThingID = entity;
                this.grabbedIsOverlay = false;
                this.setState(STATE_ENTITY_LASER_TOUCHING, "begin touching entity '" + name + "'");
                return true;
            }

        } else if (this.hoverEntity) {
            pointerEvent = {
                type: "Move",
                id: this.hand + 1
            };
            Entities.sendHoverLeaveEntity(this.hoverEntity, pointerEvent);
            this.hoverEntity = null;
        }

        return false;
    };

    this.handleLaserOnWebOverlay = function (rayPickInfo) {
        var pointerEvent;
        if (rayPickInfo.overlayID) {
            var overlay = rayPickInfo.overlayID;
            if (Overlays.getProperty(overlay, "type") != "web3d") {
                return false;
            }
            if (Overlays.keyboardFocusOverlay != overlay) {
                Entities.keyboardFocusEntity = null;
                Overlays.keyboardFocusOverlay = overlay;

                pointerEvent = {
                    type: "Move",
                    id: HARDWARE_MOUSE_ID,
                    pos2D: projectOntoOverlayXYPlane(overlay, rayPickInfo.intersection),
                    pos3D: rayPickInfo.intersection,
                    normal: rayPickInfo.normal,
                    direction: rayPickInfo.searchRay.direction,
                    button: "None"
                };

                this.hoverOverlay = overlay;
                Overlays.sendHoverEnterOverlay(overlay, pointerEvent);
            }

            // Send mouse events for button highlights and tooltips.
            if (this.hand == mostRecentSearchingHand ||
                (this.hand !== mostRecentSearchingHand &&
                 this.getOtherHandController().state !== STATE_SEARCHING &&
                 this.getOtherHandController().state !== STATE_STYLUS_TOUCHING &&
                 this.getOtherHandController().state !== STATE_ENTITY_LASER_TOUCHING &&
                 this.getOtherHandController().state !== STATE_OVERLAY_LASER_TOUCHING)) {

                // most recently searching hand has priority over other hand, for the purposes of button highlighting.
                pointerEvent = {
                    type: "Move",
                    id: HARDWARE_MOUSE_ID,
                    pos2D: projectOntoOverlayXYPlane(overlay, rayPickInfo.intersection),
                    pos3D: rayPickInfo.intersection,
                    normal: rayPickInfo.normal,
                    direction: rayPickInfo.searchRay.direction,
                    button: "None"
                };

                Overlays.sendMouseMoveOnOverlay(overlay, pointerEvent);
                Overlays.sendHoverOverOverlay(overlay, pointerEvent);
            }

            if (this.triggerSmoothedGrab()) {
                this.grabbedOverlay = overlay;
                this.setState(STATE_OVERLAY_LASER_TOUCHING, "begin touching overlay '" + overlay + "'");
                return true;
            }

        } else if (this.hoverOverlay) {
            pointerEvent = {
                type: "Move",
                id: HARDWARE_MOUSE_ID
            };
            Overlays.sendHoverLeaveOverlay(this.hoverOverlay, pointerEvent);
            this.hoverOverlay = null;
        }

        return false;
    };

    this.distanceGrabTimescale = function(mass, distance) {
        var timeScale = DISTANCE_HOLDING_ACTION_TIMEFRAME * mass /
            DISTANCE_HOLDING_UNITY_MASS * distance /
            DISTANCE_HOLDING_UNITY_DISTANCE;
        if (timeScale < DISTANCE_HOLDING_ACTION_TIMEFRAME) {
            timeScale = DISTANCE_HOLDING_ACTION_TIMEFRAME;
        }
        return timeScale;
    };

    this.getMass = function(dimensions, density) {
        return (dimensions.x * dimensions.y * dimensions.z) * density;
    };

    this.ensureDynamic = function () {
        // if we distance hold something and keep it very still before releasing it, it ends up
        // non-dynamic in bullet.  If it's too still, give it a little bounce so it will fall.
        var props = Entities.getEntityProperties(this.grabbedThingID, ["velocity", "dynamic", "parentID"]);
        if (props.dynamic && props.parentID == NULL_UUID) {
            var velocity = props.velocity;
            if (Vec3.length(velocity) < 0.05) { // see EntityMotionState.cpp DYNAMIC_LINEAR_VELOCITY_THRESHOLD
                velocity = { x: 0.0, y: 0.2, z: 0.0 };
                Entities.editEntity(this.grabbedThingID, { velocity: velocity });
            }
        }
    };

    this.distanceHoldingEnter = function() {
        this.clearEquipHaptics();
        this.grabPointSphereOff();

        this.shouldScale = false;

        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var worldControllerPosition = controllerLocation.position;
        var worldControllerRotation = controllerLocation.orientation;

        // transform the position into room space
        var worldToSensorMat = Mat4.inverse(MyAvatar.getSensorToWorldMatrix());
        var roomControllerPosition = Mat4.transformPoint(worldToSensorMat, worldControllerPosition);

        var grabbedProperties = Entities.getEntityProperties(this.grabbedThingID, GRABBABLE_PROPERTIES);
        var now = Date.now();

        // add the action and initialize some variables
        this.currentObjectPosition = grabbedProperties.position;
        this.currentObjectRotation = grabbedProperties.rotation;
        this.currentObjectTime = now;
        this.currentCameraOrientation = Camera.orientation;

        this.grabRadius = this.grabbedDistance;
        this.grabRadialVelocity = 0.0;

        // offset between controller vector at the grab radius and the entity position
        var targetPosition = Vec3.multiply(this.grabRadius, Quat.getUp(worldControllerRotation));
        targetPosition = Vec3.sum(targetPosition, worldControllerPosition);
        this.offsetPosition = Vec3.subtract(this.currentObjectPosition, targetPosition);

        // compute a constant based on the initial conditions which we use below to exaggerate hand motion
        // onto the held object
        this.radiusScalar = Math.log(this.grabRadius + 1.0);
        if (this.radiusScalar < 1.0) {
            this.radiusScalar = 1.0;
        }

        // compute the mass for the purpose of energy and how quickly to move object
        this.mass = this.getMass(grabbedProperties.dimensions, grabbedProperties.density);
        var distanceToObject = Vec3.length(Vec3.subtract(MyAvatar.position, grabbedProperties.position));
        var timeScale = this.distanceGrabTimescale(this.mass, distanceToObject);
        this.linearTimeScale = timeScale;
        this.actionID = NULL_UUID;
        this.actionID = Entities.addAction("far-grab", this.grabbedThingID, {
            targetPosition: this.currentObjectPosition,
            linearTimeScale: timeScale,
            targetRotation: this.currentObjectRotation,
            angularTimeScale: timeScale,
            tag: getTag(),
            ttl: ACTION_TTL
        });
        if (this.actionID === NULL_UUID) {
            this.actionID = null;
        }
        this.actionTimeout = now + (ACTION_TTL * MSECS_PER_SEC);

        if (this.actionID !== null) {
            this.callEntityMethodOnGrabbed("startDistanceGrab");
        }

        Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);
        this.turnOffVisualizations();
        this.previousRoomControllerPosition = roomControllerPosition;
    };

    this.distanceHolding = function(deltaTime, timestamp) {

        if (!this.triggerClicked) {
            this.callEntityMethodOnGrabbed("releaseGrab");
            this.ensureDynamic();
            this.setState(STATE_OFF, "trigger released");
            if (this.getOtherHandController().state === STATE_DISTANCE_ROTATING) {
                this.getOtherHandController().setState(STATE_SEARCHING, "trigger released on holding controller");
                // Can't set state of other controller to STATE_DISTANCE_HOLDING because then either:
                // (a) The entity would jump to line up with the formerly rotating controller's orientation, or
                // (b) The grab beam would need an orientation offset to the controller's true orientation.
                // Neither of these options is good, so instead set STATE_SEARCHING and subsequently let the formerly distance
                // rotating controller start distance holding the entity if it happens to be pointing at the entity.
            }
            return;
        }

        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var worldControllerPosition = controllerLocation.position;
        var worldControllerRotation = controllerLocation.orientation;

        // also transform the position into room space
        var worldToSensorMat = Mat4.inverse(MyAvatar.getSensorToWorldMatrix());
        var roomControllerPosition = Mat4.transformPoint(worldToSensorMat, worldControllerPosition);

        var grabbedProperties = Entities.getEntityProperties(this.grabbedThingID, GRABBABLE_PROPERTIES);
        var now = Date.now();
        var deltaObjectTime = (now - this.currentObjectTime) / MSECS_PER_SEC; // convert to seconds
        this.currentObjectTime = now;

        // the action was set up when this.distanceHolding was called.  update the targets.
        var radius = Vec3.distance(this.currentObjectPosition, worldControllerPosition) *
            this.radiusScalar * DISTANCE_HOLDING_RADIUS_FACTOR;
        if (radius < 1.0) {
            radius = 1.0;
        }

        var roomHandDelta = Vec3.subtract(roomControllerPosition, this.previousRoomControllerPosition);
        var worldHandDelta = Mat4.transformVector(MyAvatar.getSensorToWorldMatrix(), roomHandDelta);
        var handMoved = Vec3.multiply(worldHandDelta, radius);
        this.currentObjectPosition = Vec3.sum(this.currentObjectPosition, handMoved);

        this.callEntityMethodOnGrabbed("continueDistantGrab");

        var defaultMoveWithHeadData = {
            disableMoveWithHead: false
        };

        //  Update radialVelocity
        var lastVelocity = Vec3.multiply(worldHandDelta, 1.0 / deltaObjectTime);
        var delta = Vec3.normalize(Vec3.subtract(grabbedProperties.position, worldControllerPosition));
        var newRadialVelocity = Vec3.dot(lastVelocity, delta);

        var VELOCITY_AVERAGING_TIME = 0.016;
        var blendFactor = deltaObjectTime / VELOCITY_AVERAGING_TIME;
        if (blendFactor < 0.0) {
            blendFactor = 0.0;
        } else if (blendFactor > 1.0) {
            blendFactor = 1.0;
        }
        this.grabRadialVelocity = blendFactor * newRadialVelocity + (1.0 - blendFactor) * this.grabRadialVelocity;

        var RADIAL_GRAB_AMPLIFIER = 10.0;
        if (Math.abs(this.grabRadialVelocity) > 0.0) {
            this.grabRadius = this.grabRadius + (this.grabRadialVelocity * deltaObjectTime *
                                                 this.grabRadius * RADIAL_GRAB_AMPLIFIER);
        }

        // don't let grabRadius go all the way to zero, because it can't come back from that
        var MINIMUM_GRAB_RADIUS = 0.1;
        if (this.grabRadius < MINIMUM_GRAB_RADIUS) {
            this.grabRadius = MINIMUM_GRAB_RADIUS;
        }
        var newTargetPosition = Vec3.multiply(this.grabRadius, Quat.getUp(worldControllerRotation));
        newTargetPosition = Vec3.sum(newTargetPosition, worldControllerPosition);
        newTargetPosition = Vec3.sum(newTargetPosition, this.offsetPosition);
        var objectToAvatar = Vec3.subtract(this.currentObjectPosition, MyAvatar.position);
        var handControllerData = getEntityCustomData('handControllerKey', this.grabbedThingID, defaultMoveWithHeadData);
        if (handControllerData.disableMoveWithHead !== true) {
            // mix in head motion
            if (MOVE_WITH_HEAD) {
                var objDistance = Vec3.length(objectToAvatar);
                var before = Vec3.multiplyQbyV(this.currentCameraOrientation, {
                    x: 0.0,
                    y: 0.0,
                    z: objDistance
                });
                var after = Vec3.multiplyQbyV(Camera.orientation, {
                    x: 0.0,
                    y: 0.0,
                    z: objDistance
                });
                var change = Vec3.multiply(Vec3.subtract(before, after), HAND_HEAD_MIX_RATIO);
                this.currentCameraOrientation = Camera.orientation;
                this.currentObjectPosition = Vec3.sum(this.currentObjectPosition, change);
            }
        }

        this.maybeScale(grabbedProperties);

        // visualizations
        var rayPickInfo = this.calcRayPickInfo(this.hand);
        this.overlayLineOn(rayPickInfo.searchRay.origin,
                           Vec3.subtract(grabbedProperties.position, this.offsetPosition),
                           COLORS_GRAB_DISTANCE_HOLD,
                           this.grabbedThingID);

        var distanceToObject = Vec3.length(Vec3.subtract(MyAvatar.position, this.currentObjectPosition));

        var candidateHotSpotEntities =
            Entities.findEntities(controllerLocation.position,MAX_FAR_TO_NEAR_EQUIP_HOTSPOT_RADIUS);
        entityPropertiesCache.addEntities(candidateHotSpotEntities);

        var potentialEquipHotspot = this.chooseBestEquipHotspotForFarToNearEquip(candidateHotSpotEntities);
        if (potentialEquipHotspot && (potentialEquipHotspot.entityID == this.grabbedThingID)) {
            if ((this.triggerSmoothedGrab() || this.secondarySqueezed()) && holdEnabled) {
                this.grabbedHotspot = potentialEquipHotspot;
                this.grabbedThingID = potentialEquipHotspot.entityID;
                this.grabbedIsOverlay = false;

                Entities.deleteAction(this.grabbedThingID, this.actionID);
                this.actionID = null;

                this.setState(STATE_HOLD, "equipping '" + entityPropertiesCache.getProps(this.grabbedThingID).name + "'");
                return;
             }
        }
        var rayPositionOnEntity = Vec3.subtract(grabbedProperties.position, this.offsetPosition);
        //Far to Near Grab: If object is draw by user inside FAR_TO_NEAR_GRAB_MAX_DISTANCE, grab it
        if (this.entityIsFarToNearGrabbable(rayPositionOnEntity,
                                            controllerLocation.position,
                                            FAR_TO_NEAR_GRAB_MAX_DISTANCE)) {
            this.farToNearGrab = true;

            Entities.deleteAction(this.grabbedThingID, this.actionID);
            this.actionID = null;

            this.setState(STATE_NEAR_GRABBING , "near grab entity '" + this.grabbedThingID + "'");
            return;
        }

        this.linearTimeScale = (this.linearTimeScale / 2);
        if (this.linearTimeScale <= DISTANCE_HOLDING_ACTION_TIMEFRAME) {
            this.linearTimeScale = DISTANCE_HOLDING_ACTION_TIMEFRAME;
        }
        var success = Entities.updateAction(this.grabbedThingID, this.actionID, {
            targetPosition: newTargetPosition,
            linearTimeScale: this.linearTimeScale,
            targetRotation: this.currentObjectRotation,
            angularTimeScale: this.distanceGrabTimescale(this.mass, distanceToObject),
            ttl: ACTION_TTL
        });
        if (success) {
            this.actionTimeout = now + (ACTION_TTL * MSECS_PER_SEC);
        } else {
            print("continueDistanceHolding -- updateAction failed");
        }

        this.previousRoomControllerPosition = roomControllerPosition;
    };

    this.distanceRotatingEnter = function() {
        this.clearEquipHaptics();
        this.grabPointSphereOff();

        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var worldControllerPosition = controllerLocation.position;
        var worldControllerRotation = controllerLocation.orientation;

        var grabbedProperties = Entities.getEntityProperties(this.grabbedThingID, GRABBABLE_PROPERTIES);
        this.currentObjectPosition = grabbedProperties.position;
        this.grabRadius = this.grabbedDistance;

        // Offset between controller vector at the grab radius and the entity position.
        var targetPosition = Vec3.multiply(this.grabRadius, Quat.getUp(worldControllerRotation));
        targetPosition = Vec3.sum(targetPosition, worldControllerPosition);
        this.offsetPosition = Vec3.subtract(this.currentObjectPosition, targetPosition);

        // Initial controller rotation.
        this.previousWorldControllerRotation = worldControllerRotation;

        Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);
        this.turnOffVisualizations();
    };

    this.distanceRotating = function(deltaTime, timestamp) {

        if (!this.triggerClicked) {
            this.callEntityMethodOnGrabbed("releaseGrab");
            this.ensureDynamic();
            this.setState(STATE_OFF, "trigger released");
            return;
        }

        var grabbedProperties = Entities.getEntityProperties(this.grabbedThingID, GRABBABLE_PROPERTIES);

        // Delta rotation of grabbing controller since last update.
        var worldControllerRotation = getControllerWorldLocation(this.handToController(), true).orientation;
        var controllerRotationDelta = Quat.multiply(worldControllerRotation, Quat.inverse(this.previousWorldControllerRotation));

        // Rotate entity by twice the delta rotation.
        controllerRotationDelta = Quat.multiply(controllerRotationDelta, controllerRotationDelta);

        // Perform the rotation in the translation controller's action update.
        this.getOtherHandController().currentObjectRotation = Quat.multiply(controllerRotationDelta,
            this.getOtherHandController().currentObjectRotation);

        // Rotate about the translation controller's target position.
        this.offsetPosition = Vec3.multiplyQbyV(controllerRotationDelta, this.offsetPosition);
        this.getOtherHandController().offsetPosition = Vec3.multiplyQbyV(controllerRotationDelta,
            this.getOtherHandController().offsetPosition);

        var rayPickInfo = this.calcRayPickInfo(this.hand);
        this.overlayLineOn(rayPickInfo.searchRay.origin, Vec3.subtract(grabbedProperties.position, this.offsetPosition),
            COLORS_GRAB_DISTANCE_HOLD, this.grabbedThingID);

        this.previousWorldControllerRotation = worldControllerRotation;
    };

    this.setupHoldAction = function() {
        this.actionID = Entities.addAction("hold", this.grabbedThingID, {
            hand: this.hand === RIGHT_HAND ? "right" : "left",
            timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
            relativePosition: this.offsetPosition,
            relativeRotation: this.offsetRotation,
            ttl: ACTION_TTL,
            kinematic: this.kinematicGrab,
            kinematicSetVelocity: true,
            ignoreIK: this.ignoreIK
        });
        if (this.actionID === NULL_UUID) {
            this.actionID = null;
            return false;
        }
        var now = Date.now();
        this.actionTimeout = now + (ACTION_TTL * MSECS_PER_SEC);
        return true;
    };

    this.projectVectorAlongAxis = function(position, axisStart, axisEnd) {
        var aPrime = Vec3.subtract(position, axisStart);
        var bPrime = Vec3.subtract(axisEnd, axisStart);
        var bPrimeMagnitude = Vec3.length(bPrime);
        var dotProduct = Vec3.dot(aPrime, bPrime);
        var scalar = dotProduct / bPrimeMagnitude;
        if (scalar < 0) {
            scalar = 0;
        }
        if (scalar > 1) {
            scalar = 1;
        }
        var projection = Vec3.sum(axisStart, Vec3.multiply(scalar, Vec3.normalize(bPrime)));
        return projection;
    };

    this.dropGestureReset = function() {
        this.prevHandIsUpsideDown = false;
    };

    this.dropGestureProcess = function(deltaTime) {
        var worldHandRotation = getControllerWorldLocation(this.handToController(), true).orientation;
        var localHandUpAxis = this.hand === RIGHT_HAND ? {
            x: 1,
            y: 0,
            z: 0
        } : {
            x: -1,
            y: 0,
            z: 0
        };
        var worldHandUpAxis = Vec3.multiplyQbyV(worldHandRotation, localHandUpAxis);
        var DOWN = {
            x: 0,
            y: -1,
            z: 0
        };

        var DROP_ANGLE = Math.PI / 3;
        var HYSTERESIS_FACTOR = 1.1;
        var ROTATION_ENTER_THRESHOLD = Math.cos(DROP_ANGLE);
        var ROTATION_EXIT_THRESHOLD = Math.cos(DROP_ANGLE * HYSTERESIS_FACTOR);
        var rotationThreshold = this.prevHandIsUpsideDown ? ROTATION_EXIT_THRESHOLD : ROTATION_ENTER_THRESHOLD;

        var handIsUpsideDown = false;
        if (Vec3.dot(worldHandUpAxis, DOWN) > rotationThreshold) {
            handIsUpsideDown = true;
        }

        if (handIsUpsideDown != this.prevHandIsUpsideDown) {
            this.prevHandIsUpsideDown = handIsUpsideDown;
            Controller.triggerHapticPulse(HAPTIC_DEQUIP_STRENGTH, HAPTIC_DEQUIP_DURATION, this.hand);
        }

        return handIsUpsideDown;
    };

    this.nearGrabbingEnter = function() {
        this.grabPointSphereOff();
        this.lineOff();
        this.overlayLineOff();
        this.searchSphereOff();
        this.otherGrabbingLineOff();

        this.dropGestureReset();
        this.clearEquipHaptics();

        this.shouldScale = false;

        Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

        var grabbedProperties;
        if (this.grabbedIsOverlay) {
            grabbedProperties = {
                position: Overlays.getProperty(this.grabbedThingID, "position"),
                rotation: Overlays.getProperty(this.grabbedThingID, "rotation"),
                parentID: Overlays.getProperty(this.grabbedThingID, "parentID"),
                parentJointIndex: Overlays.getProperty(this.grabbedThingID, "parentJointIndex"),
                dynamic: false,
                shapeType: "none"
            };
            this.ignoreIK = true;
        } else {
            grabbedProperties = Entities.getEntityProperties(this.grabbedThingID, GRABBABLE_PROPERTIES);
            var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedThingID, DEFAULT_GRABBABLE_DATA);
            if (FORCE_IGNORE_IK) {
                this.ignoreIK = true;
            } else {
                this.ignoreIK = (grabbableData.ignoreIK !== undefined) ? grabbableData.ignoreIK : true;
            }

            this.kinematicGrab = (grabbableData.kinematic !== undefined) ? grabbableData.kinematic : NEAR_GRABBING_KINEMATIC;
        }

        var handRotation;
        var handPosition;
        if (this.ignoreIK) {
            var controllerLocation = getControllerWorldLocation(this.handToController(), false);
            handRotation = controllerLocation.orientation;
            handPosition = controllerLocation.position;
        } else {
            handRotation = this.getHandRotation();
            handPosition = this.getHandPosition();
        }

        var hasPresetPosition = false;
        if (this.state == STATE_HOLD && this.grabbedHotspot) {
            // if an object is "equipped" and has a predefined offset, use it.
            var offsets = USE_ATTACH_POINT_SETTINGS && getAttachPointForHotspotFromSettings(this.grabbedHotspot, this.hand);
            if (offsets) {
                this.offsetPosition = offsets[0];
                this.offsetRotation = offsets[1];
                hasPresetPosition = true;
            } else {
                var handJointName = this.hand === RIGHT_HAND ? "RightHand" : "LeftHand";
                if (this.grabbedHotspot.joints[handJointName]) {
                    this.offsetPosition = this.grabbedHotspot.joints[handJointName][0];
                    this.offsetRotation = this.grabbedHotspot.joints[handJointName][1];
                    hasPresetPosition = true;
                }
            }
        } else {
            var objectRotation = grabbedProperties.rotation;
            this.offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

            var currentObjectPosition = grabbedProperties.position;
            var offset = Vec3.subtract(currentObjectPosition, handPosition);
            this.offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, this.offsetRotation)), offset);
        }

        // This boolean is used to check if the object that is grabbed has just been cloned
        // It is only set true, if the object that is grabbed creates a new clone.
        var isClone = false;
        var isPhysical = propsArePhysical(grabbedProperties) ||
            (!this.grabbedIsOverlay && entityHasActions(this.grabbedThingID));
        if (isPhysical && this.state == STATE_NEAR_GRABBING && grabbedProperties.parentID === NULL_UUID) {
            // grab entity via action
            if (!this.setupHoldAction()) {
                return;
            }
            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: this.grabbedThingID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
        } else {
            // grab entity via parenting
            this.actionID = null;
            var handJointIndex;
            if (this.ignoreIK) {
                handJointIndex = this.controllerJointIndex;
            } else {
                handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            }

            var reparentProps = {
                parentID: AVATAR_SELF_ID,
                parentJointIndex: handJointIndex,
                velocity: {x: 0, y: 0, z: 0},
                angularVelocity: {x: 0, y: 0, z: 0}
            };
            if (hasPresetPosition) {
                reparentProps.localPosition = this.offsetPosition;
                reparentProps.localRotation = this.offsetRotation;
            }

            if (this.grabbedIsOverlay) {
                Overlays.editOverlay(this.grabbedThingID, reparentProps);
            } else {
                if (grabbedProperties.userData.length > 0) {
                    try{
                        var userData = JSON.parse(grabbedProperties.userData);
                        var grabInfo = userData.grabbableKey;
                        if (grabInfo && grabInfo.cloneable) {
                            var worldEntities = Entities.findEntities(MyAvatar.position, 50);
                            var count = 0;
                            worldEntities.forEach(function(item) {
                                var itemWE = Entities.getEntityProperties(item, ["name"]);
                                if (itemWE.name.indexOf('-clone-' + grabbedProperties.id) !== -1) {
                                    count++;
                                }
                            });

                            var limit = grabInfo.cloneLimit ? grabInfo.cloneLimit : 0;
                            if (count >= limit && limit !== 0) {
                                return;
                            }

                            var cloneableProps = Entities.getEntityProperties(grabbedProperties.id);
                            cloneableProps.name = cloneableProps.name + '-clone-' + grabbedProperties.id;
                            var lifetime = grabInfo.cloneLifetime ? grabInfo.cloneLifetime : 300;
                            var dynamic = grabInfo.cloneDynamic ? grabInfo.cloneDynamic : false;
                            var cUserData = Object.assign({}, userData);
                            var cProperties = Object.assign({}, cloneableProps);
                            isClone = true;

                            delete cUserData.grabbableKey.cloneLifetime;
                            delete cUserData.grabbableKey.cloneable;
                            delete cUserData.grabbableKey.cloneDynamic;
                            delete cUserData.grabbableKey.cloneLimit;
                            delete cProperties.id;

                            cProperties.dynamic = dynamic;
                            cProperties.locked = false;
                            cUserData.grabbableKey.triggerable = true;
                            cUserData.grabbableKey.grabbable = true;
                            cProperties.lifetime = lifetime;
                            cProperties.userData = JSON.stringify(cUserData);
                            var cloneID = Entities.addEntity(cProperties);
                            this.grabbedThingID = cloneID;
                            grabbedProperties = Entities.getEntityProperties(cloneID);
                        }
                    }catch(e) {}
                }
                Entities.editEntity(this.grabbedThingID, reparentProps);
            }

            if (this.thisHandIsParent(grabbedProperties)) {
                // this should never happen, but if it does, don't set previous parent to be this hand.
                // this.previousParentID[this.grabbedThingID] = NULL;
                // this.previousParentJointIndex[this.grabbedThingID] = -1;
            } else {
                this.previousParentID[this.grabbedThingID] = grabbedProperties.parentID;
                this.previousParentJointIndex[this.grabbedThingID] = grabbedProperties.parentJointIndex;
            }
            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'equip',
                grabbedEntity: this.grabbedThingID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
        }

        if (!this.grabbedIsOverlay) {
            Entities.editEntity(this.grabbedThingID, {
                velocity: { x: 0, y: 0, z: 0 },
                angularVelocity: { x: 0, y: 0, z: 0 },
                // dynamic: false
            });
        }

        var _this = this;
        /*
         * Setting context for function that is either called via timer or directly, depending if
         * if the object in question is a clone. If it is a clone, we need to make sure that the intial equipment event
         * is called correctly, as these just freshly created entity may not have completely initialized.
        */
        var grabEquipCheck = function () {
            if (_this.state == STATE_NEAR_GRABBING) {
                _this.callEntityMethodOnGrabbed("startNearGrab");
            } else { // this.state == STATE_HOLD
                _this.callEntityMethodOnGrabbed("startEquip");
            }

            // don't block teleport raypick with equipped entity
            Messages.sendMessage('Hifi-Teleport-Ignore-Add', _this.grabbedThingID);

            _this.currentHandControllerTipPosition =
                (_this.hand === RIGHT_HAND) ? MyAvatar.rightHandTipPosition : MyAvatar.leftHandTipPosition;
            _this.currentObjectTime = Date.now();

            _this.currentObjectPosition = grabbedProperties.position;
            _this.currentObjectRotation = grabbedProperties.rotation;
            _this.currentVelocity = ZERO_VEC;
            _this.currentAngularVelocity = ZERO_VEC;

            _this.prevDropDetected = false;
        };

        if (isClone) {
            // 100 ms seems to be sufficient time to force the check even occur after the object has been initialized.
            Script.setTimeout(grabEquipCheck, 100);
        } else {
            grabEquipCheck();
        }
    };

    this.nearGrabbing = function(deltaTime, timestamp) {
        this.grabPointSphereOff();

        var ttl = ACTION_TTL;

        if (this.farToNearGrab) {
            if(!this.triggerClicked){
               this.farToNearGrab = false;
            }
        }

        if (this.state == STATE_NEAR_GRABBING && (!this.triggerClicked && this.secondaryReleased())) {
            this.callEntityMethodOnGrabbed("releaseGrab");
            this.setState(STATE_OFF, "trigger released");
            return;
        }

        if (this.state == STATE_HOLD) {

            if (this.secondarySqueezed()) {
                // this.secondaryReleased() will always be true when not depressed
                // so we cannot simply rely on that for release - ensure that the
                // trigger was first "prepared" by being pushed in before the release
                this.preparingHoldRelease = true;
            }

            if (this.preparingHoldRelease && this.secondaryReleased()) {
                // we have an equipped object and the secondary trigger was released
                // short-circuit the other checks and release it
                this.preparingHoldRelease = false;
                this.callEntityMethodOnGrabbed("releaseEquip");
                this.setState(STATE_OFF, "equipping ended via secondary press");
                return;
            }

            var dropDetected = this.dropGestureProcess(deltaTime);

            if (this.triggerSmoothedReleased()) {
                this.waitForTriggerRelease = false;
            }

            if (dropDetected && this.prevDropDetected != dropDetected) {
                this.waitForTriggerRelease = true;
            }

            // highlight the grabbed hotspot when the dropGesture is detected.
            if (dropDetected) {
                entityPropertiesCache.addEntity(this.grabbedHotspot.entityID);
                equipHotspotBuddy.updateHotspot(this.grabbedHotspot, timestamp);
                equipHotspotBuddy.highlightHotspot(this.grabbedHotspot);
            }

            if (dropDetected && !this.waitForTriggerRelease && this.triggerSmoothedGrab()) {
                // store the offset attach points into preferences.
                if (USE_ATTACH_POINT_SETTINGS && this.grabbedHotspot && this.grabbedThingID) {
                    var prefprops = Entities.getEntityProperties(this.grabbedThingID, ["localPosition", "localRotation"]);
                    if (prefprops && prefprops.localPosition && prefprops.localRotation) {
                        storeAttachPointForHotspotInSettings(this.grabbedHotspot, this.hand,
                                                             prefprops.localPosition, prefprops.localRotation);
                    }
                }

                var grabbedEntity = this.grabbedThingID;
                this.release();
                this.grabbedThingID = grabbedEntity;
                this.setState(STATE_NEAR_GRABBING, "drop gesture detected");
                return;
            }
            this.prevDropDetected = dropDetected;
        }

        var props;
        if (this.grabbedIsOverlay) {
            props = {
                localPosition: Overlays.getProperty(this.grabbedThingID, "localPosition"),
                parentID: Overlays.getProperty(this.grabbedThingID, "parentID"),
                parentJointIndex: Overlays.getProperty(this.grabbedThingID, "parentJointIndex"),
                position: Overlays.getProperty(this.grabbedThingID, "position"),
                rotation: Overlays.getProperty(this.grabbedThingID, "rotation"),
                dimensions: Overlays.getProperty(this.grabbedThingID, "dimensions"),
                registrationPoint: { x: 0.5, y: 0.5, z: 0.5 }
            };
        } else {
            props = Entities.getEntityProperties(this.grabbedThingID, ["localPosition", "parentID", "parentJointIndex",
                                                                      "position", "rotation", "dimensions",
                                                                      "registrationPoint"]);
        }
        if (!props.position) {
            // server may have reset, taking our equipped entity with it.  move back to "off" state
            this.callEntityMethodOnGrabbed("releaseGrab");
            this.setState(STATE_OFF, "entity has no position property");
            return;
        }

        if (this.state == STATE_NEAR_GRABBING && this.actionID === null && !this.thisHandIsParent(props)) {
            // someone took it from us or otherwise edited the parentID.  end the grab.  We don't do this
            // for equipped things so that they can be adjusted while equipped.
            this.callEntityMethodOnGrabbed("releaseGrab");
            this.grabbedThingID = null;
            this.setState(STATE_OFF, "someone took it");
            return;
        }

        var now = Date.now();
        if (this.state == STATE_HOLD && now - this.lastUnequipCheckTime > MSECS_PER_SEC * CHECK_TOO_FAR_UNEQUIP_TIME) {
            this.lastUnequipCheckTime = now;

            if (props.parentID == AVATAR_SELF_ID) {
                var handPosition;
                if (this.ignoreIK) {
                    handPosition = getControllerWorldLocation(this.handToController(), false).position;
                } else {
                    handPosition = this.getHandPosition();
                }

                var TEAR_AWAY_DISTANCE = 0.1;
                var dist = distanceBetweenPointAndEntityBoundingBox(handPosition, props);
                if (dist > TEAR_AWAY_DISTANCE) {
                    this.autoUnequipCounter += deltaTime;
                } else {
                    this.autoUnequipCounter = 0;
                }

                if (this.autoUnequipCounter > 0.25) {
                        // for whatever reason, the held/equipped entity has been pulled away.  ungrab or unequip.
                    print("handControllerGrab -- autoreleasing held or equipped item because it is far from hand." +
                        props.parentID + ", dist = " + dist);

                    if (this.state == STATE_NEAR_GRABBING) {
                        this.callEntityMethodOnGrabbed("releaseGrab");
                    } else { // this.state == STATE_HOLD
                        this.callEntityMethodOnGrabbed("releaseEquip");
                    }
                    this.setState(STATE_OFF, "held object too far away");
                    return;
                }
            }
        }

        // Keep track of the fingertip velocity to impart when we release the object.
        // Note that the idea of using a constant 'tip' velocity regardless of the
        // object's actual held offset is an idea intended to make it easier to throw things:
        // Because we might catch something or transfer it between hands without a good idea
        // of it's actual offset, let's try imparting a velocity which is at a fixed radius
        // from the palm.

        var handControllerPosition = (this.hand === RIGHT_HAND) ? MyAvatar.rightHandPosition : MyAvatar.leftHandPosition;

        var deltaObjectTime = (now - this.currentObjectTime) / MSECS_PER_SEC; // convert to seconds

        if (deltaObjectTime > 0.0) {
            var worldDeltaPosition = Vec3.subtract(props.position, this.currentObjectPosition);

            var previousEulers = Quat.safeEulerAngles(this.currentObjectRotation);
            var newEulers = Quat.safeEulerAngles(props.rotation);
            var worldDeltaRotation = Vec3.subtract(newEulers, previousEulers);

            this.currentVelocity = Vec3.multiply(worldDeltaPosition, 1.0 / deltaObjectTime);
            this.currentAngularVelocity = Vec3.multiply(worldDeltaRotation, Math.PI / (deltaObjectTime * 180.0));

            this.currentObjectPosition = props.position;
            this.currentObjectRotation = props.rotation;
        }

        this.currentHandControllerTipPosition = handControllerPosition;
        this.currentObjectTime = now;

        if (this.state === STATE_HOLD) {
            this.callEntityMethodOnGrabbed("continueEquip");
        }
        if (this.state == STATE_NEAR_GRABBING) {
            this.callEntityMethodOnGrabbed("continueNearGrab");
        }

        if (this.state == STATE_NEAR_GRABBING) {
            this.maybeScale(props);
        }

        if (this.actionID && this.actionTimeout - now < ACTION_TTL_REFRESH * MSECS_PER_SEC) {
            // if less than a 5 seconds left, refresh the actions ttl
            var success = Entities.updateAction(this.grabbedThingID, this.actionID, {
                hand: this.hand === RIGHT_HAND ? "right" : "left",
                timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
                relativePosition: this.offsetPosition,
                relativeRotation: this.offsetRotation,
                ttl: ttl,
                kinematic: this.kinematicGrab,
                kinematicSetVelocity: true,
                ignoreIK: this.ignoreIK
            });
            if (success) {
               this.actionTimeout = now + (ttl * MSECS_PER_SEC);
            } else {
                print("continueNearGrabbing -- updateAction failed");
                Entities.deleteAction(this.grabbedThingID, this.actionID);
                this.setupHoldAction();
            }
        }
    };

    this.maybeScale = function(props) {
        if (!objectScalingEnabled || this.isTablet(this.grabbedThingID) || this.grabbedIsOverlay) {
            return;
        }

        if (!this.shouldScale) {
            //  If both secondary triggers squeezed, and the non-holding hand is empty, start scaling
            if (this.secondarySqueezed() &&
                this.getOtherHandController().secondarySqueezed() &&
                this.grabbedThingID && this.getOtherHandController().grabbedThingID &&
                this.grabbedThingID == this.getOtherHandController().grabbedThingID) {
                this.scalingStartDistance = Vec3.length(Vec3.subtract(this.getHandPosition(),
                                                                      this.getOtherHandController().getHandPosition()));
                this.scalingStartDimensions = props.dimensions;
                this.shouldScale = true;
            }
        } else if (!this.secondarySqueezed() || !this.getOtherHandController().secondarySqueezed()) {
            this.shouldScale = false;
        }
        if (this.shouldScale) {
            var scalingCurrentDistance = Vec3.length(Vec3.subtract(this.getHandPosition(),
                                                                   this.getOtherHandController().getHandPosition()));
            var currentRescale = scalingCurrentDistance / this.scalingStartDistance;
            var newDimensions = Vec3.multiply(currentRescale, this.scalingStartDimensions);
            Entities.editEntity(this.grabbedThingID, { dimensions: newDimensions });
        }
    };

    this.maybeScaleMyAvatar = function() {
        if (!myAvatarScalingEnabled || this.shouldScale || this.hand === LEFT_HAND) {
            //  If scaling disabled, or if we are currently scaling an entity, don't scale avatar
            //  and only rescale avatar for one hand (so we're not doing it twice)
            return;
        }

        // Only scale avatar if both triggers and grips are squeezed
        var tryingToScale = this.secondarySqueezed() && this.getOtherHandController().secondarySqueezed() &&
                            this.triggerSmoothedSqueezed() && this.getOtherHandController().triggerSmoothedSqueezed();


        if (!this.isScalingAvatar) {
            //  If both secondary triggers squeezed, start scaling
            if (tryingToScale) {
                this.scalingStartDistance = Vec3.length(Vec3.subtract(this.getHandPosition(),
                                                                      this.getOtherHandController().getHandPosition()));
                this.scalingStartAvatarScale = MyAvatar.scale;
                this.isScalingAvatar = true;
            }
        } else if (!tryingToScale) {
            this.isScalingAvatar = false;
        }
        if (this.isScalingAvatar) {
            var scalingCurrentDistance = Vec3.length(Vec3.subtract(this.getHandPosition(),
                                                                   this.getOtherHandController().getHandPosition()));
            var newAvatarScale = (scalingCurrentDistance / this.scalingStartDistance) * this.scalingStartAvatarScale;
            MyAvatar.scale = newAvatarScale;
        }
    };

    this.nearTriggerEnter = function() {
        this.clearEquipHaptics();
        this.grabPointSphereOff();
        Controller.triggerShortHapticPulse(1.0, this.hand);
        this.callEntityMethodOnGrabbed("startNearTrigger");
    };

    this.farTriggerEnter = function() {
        this.clearEquipHaptics();
        this.grabPointSphereOff();
        this.callEntityMethodOnGrabbed("startFarTrigger");
    };

    this.nearTrigger = function(deltaTime, timestamp) {
        if (this.triggerSmoothedReleased()) {
            this.callEntityMethodOnGrabbed("stopNearTrigger");
            this.grabbedThingID = null;
            this.setState(STATE_OFF, "trigger released");
            return;
        }
        this.callEntityMethodOnGrabbed("continueNearTrigger");
    };

    this.farTrigger = function(deltaTime, timestamp) {
        if (this.triggerSmoothedReleased()) {
            this.callEntityMethodOnGrabbed("stopFarTrigger");
            this.grabbedThingID = null;
            this.setState(STATE_OFF, "trigger released");
            return;
        }

        var pickRay = {
            origin: getControllerWorldLocation(this.handToController(), false).position,
            direction: Quat.getUp(getControllerWorldLocation(this.handToController(), false).orientation)
        };

        var now = Date.now();
        if (now - this.lastPickTime > MSECS_PER_SEC / PICKS_PER_SECOND_PER_HAND) {
            var intersection = findRayIntersection(pickRay, true, [], [], true);
            if (intersection.accurate || intersection.overlayID) {
                this.lastPickTime = now;
                if (intersection.entityID != this.grabbedThingID) {
                    this.callEntityMethodOnGrabbed("stopFarTrigger");
                    this.grabbedThingID = null;
                    this.setState(STATE_OFF, "laser moved off of entity");
                    return;
                }
                if (intersection.intersects) {
                    this.intersectionDistance = Vec3.distance(pickRay.origin, intersection.intersection);
                }
                if (farGrabEnabled) {
                    this.searchIndicatorOn(pickRay);
                }
            }
        }

        this.callEntityMethodOnGrabbed("continueFarTrigger");
    };

    this.offEnter = function() {
        // Reuse the existing search distance if lasers were active since
        // they will be shown in OFF state while in edit mode.
        var existingSearchDistance = this.searchSphereDistance;
        this.release();

        if (isInEditMode()) {
            this.searchSphereDistance = existingSearchDistance;
        }
    };

    this.entityLaserTouchingEnter = function() {
        // test for intersection between controller laser and web entity plane.
        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var intersectInfo = handLaserIntersectEntity(this.grabbedThingID, controllerLocation);
        if (intersectInfo) {
            var pointerEvent = {
                type: "Press",
                id: this.hand + 1, // 0 is reserved for hardware mouse
                pos2D: projectOntoEntityXYPlane(this.grabbedThingID, intersectInfo.point),
                pos3D: intersectInfo.point,
                normal: intersectInfo.normal,
                direction: intersectInfo.searchRay.direction,
                button: "Primary",
                isPrimaryHeld: true
            };

            Entities.sendMousePressOnEntity(this.grabbedThingID, pointerEvent);
            Entities.sendClickDownOnEntity(this.grabbedThingID, pointerEvent);

            this.touchingEnterTimer = 0;
            this.touchingEnterPointerEvent = pointerEvent;
            this.touchingEnterPointerEvent.button = "None";
            this.deadspotExpired = false;

            var LASER_PRESS_TO_MOVE_DEADSPOT_ANGLE = 0.026; // radians ~ 1.2 degrees
            this.deadspotRadius = Math.tan(LASER_PRESS_TO_MOVE_DEADSPOT_ANGLE) * intersectInfo.distance;  // dead spot radius in meters
        }

        Controller.triggerHapticPulse(HAPTIC_LASER_UI_STRENGTH, HAPTIC_LASER_UI_DURATION, this.hand);
    };

    this.entityLaserTouchingExit = function() {
        // test for intersection between controller laser and web entity plane.
        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var intersectInfo = handLaserIntersectEntity(this.grabbedThingID, controllerLocation);
        if (intersectInfo) {
            var pointerEvent;
            if (this.deadspotExpired) {
                pointerEvent = {
                    type: "Release",
                    id: this.hand + 1, // 0 is reserved for hardware mouse
                    pos2D: projectOntoEntityXYPlane(this.grabbedThingID, intersectInfo.point),
                    pos3D: intersectInfo.point,
                    normal: intersectInfo.normal,
                    direction: intersectInfo.searchRay.direction,
                    button: "Primary"
                };
            } else {
                pointerEvent = this.touchingEnterPointerEvent;
                pointerEvent.type = "Release";
                pointerEvent.button = "Primary";
                pointerEvent.isPrimaryHeld = false;
            }

            Entities.sendMouseReleaseOnEntity(this.grabbedThingID, pointerEvent);
            Entities.sendClickReleaseOnEntity(this.grabbedThingID, pointerEvent);
            Entities.sendHoverLeaveEntity(this.grabbedThingID, pointerEvent);
        }
        this.grabbedThingID = null;
        this.grabbedOverlay = null;
    };

    this.entityLaserTouching = function(dt) {

        this.touchingEnterTimer += dt;

        entityPropertiesCache.addEntity(this.grabbedThingID);

        if (this.state == STATE_ENTITY_LASER_TOUCHING && !this.triggerSmoothedGrab()) {  // AJT:
            this.setState(STATE_OFF, "released trigger");
            return;
        }

        // test for intersection between controller laser and web entity plane.
        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var intersectInfo = handLaserIntersectEntity(this.grabbedThingID, controllerLocation);
        if (intersectInfo) {

            if (Entities.keyboardFocusEntity != this.grabbedThingID) {
                Overlays.keyboardFocusOverlay = 0;
                Entities.keyboardFocusEntity = this.grabbedThingID;
            }

            var pointerEvent = {
                type: "Move",
                id: this.hand + 1, // 0 is reserved for hardware mouse
                pos2D: projectOntoEntityXYPlane(this.grabbedThingID, intersectInfo.point),
                pos3D: intersectInfo.point,
                normal: intersectInfo.normal,
                direction: intersectInfo.searchRay.direction,
                button: "NoButtons",
                isPrimaryHeld: true
            };

            var POINTER_PRESS_TO_MOVE_DELAY = 0.25; // seconds
            if (this.deadspotExpired || this.touchingEnterTimer > POINTER_PRESS_TO_MOVE_DELAY ||
                Vec3.distance(intersectInfo.point, this.touchingEnterPointerEvent.pos3D) > this.deadspotRadius) {
                Entities.sendMouseMoveOnEntity(this.grabbedThingID, pointerEvent);
                Entities.sendHoldingClickOnEntity(this.grabbedThingID, pointerEvent);
                this.deadspotExpired = true;
            }

            this.intersectionDistance = intersectInfo.distance;
            if (this.state == STATE_ENTITY_LASER_TOUCHING) {
                this.searchIndicatorOn(intersectInfo.searchRay);
            }
            Reticle.setVisible(false);
        } else {
            this.grabbedThingID = null;
            this.setState(STATE_OFF, "grabbed entity was destroyed");
            return;
        }
    };

    this.overlayLaserTouchingEnter = function () {
        // Test for intersection between controller laser and Web overlay plane.
        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var intersectInfo = handLaserIntersectOverlay(this.grabbedOverlay, controllerLocation);
        if (intersectInfo) {
            var pointerEvent = {
                type: "Press",
                id: this.hand + 1,
                pos2D: projectOntoOverlayXYPlane(this.grabbedOverlay, intersectInfo.point),
                pos3D: intersectInfo.point,
                normal: intersectInfo.normal,
                direction: intersectInfo.searchRay.direction,
                button: "Primary",
                isPrimaryHeld: true
            };

            Overlays.sendMousePressOnOverlay(this.grabbedOverlay, pointerEvent);

            this.touchingEnterTimer = 0;
            this.touchingEnterPointerEvent = pointerEvent;
            this.touchingEnterPointerEvent.button = "None";
            this.deadspotExpired = false;

            var LASER_PRESS_TO_MOVE_DEADSPOT_ANGLE = 0.026; // radians ~ 1.2 degrees
            this.deadspotRadius = Math.tan(LASER_PRESS_TO_MOVE_DEADSPOT_ANGLE) * intersectInfo.distance;  // dead spot radius in meters
        }

        Controller.triggerHapticPulse(HAPTIC_LASER_UI_STRENGTH, HAPTIC_LASER_UI_DURATION, this.hand);
    };

    this.overlayLaserTouchingExit = function () {
        // Test for intersection between controller laser and Web overlay plane.
        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var intersectInfo = handLaserIntersectOverlay(this.grabbedOverlay, controllerLocation);
        if (intersectInfo) {
            var pointerEvent;

            var pos2D;
            var pos3D;
            if (this.tabletStabbed) {
                // Some people like to jam the stylus a long ways into the tablet when clicking on a button.
                // They almost always move out of the deadzone when they do this.  We detect if the stylus
                // has gone far through the tablet and suppress any further faux mouse events until the
                // stylus is withdrawn.  Once it has withdrawn, we do a release click wherever the stylus was
                // when it was pushed into the tablet.
                this.tabletStabbed = false;
                pos2D = this.tabletStabbedPos2D;
                pos3D = this.tabletStabbedPos3D;
            } else {
                pos2D = projectOntoOverlayXYPlane(this.grabbedOverlay, intersectInfo.point);
                pos3D = intersectInfo.point;
            }

            if (this.deadspotExpired) {
                pointerEvent = {
                    type: "Release",
                    id: this.hand + 1,
                    pos2D: pos2D,
                    pos3D: pos3D,
                    normal: intersectInfo.normal,
                    direction: intersectInfo.searchRay.direction,
                    button: "Primary"
                };
            } else {
                pointerEvent = this.touchingEnterPointerEvent;
                pointerEvent.type = "Release";
                pointerEvent.button = "Primary";
                pointerEvent.isPrimaryHeld = false;
            }

            Overlays.sendMouseReleaseOnOverlay(this.grabbedOverlay, pointerEvent);
            Overlays.sendHoverLeaveOverlay(this.grabbedOverlay, pointerEvent);
        }
        this.grabbedThingID = null;
        this.grabbedOverlay = null;
    };

    this.overlayLaserTouching = function (dt) {
        this.touchingEnterTimer += dt;

        if (this.state == STATE_OVERLAY_LASER_TOUCHING && !this.triggerSmoothedGrab()) {
            this.setState(STATE_OFF, "released trigger");
            return;
        }

        // Test for intersection between controller laser and Web overlay plane.
        var controllerLocation = getControllerWorldLocation(this.handToController(), true);
        var intersectInfo = handLaserIntersectOverlay(this.grabbedOverlay, controllerLocation);
        if (intersectInfo) {

            var pos2D = projectOntoOverlayXYPlane(this.grabbedOverlay, intersectInfo.point);
            var pos3D = intersectInfo.point;

            if (Overlays.keyboardFocusOverlay != this.grabbedOverlay) {
                Entities.keyboardFocusEntity = null;
                Overlays.keyboardFocusOverlay = this.grabbedOverlay;
            }

            var pointerEvent = {
                type: "Move",
                id: this.hand + 1,
                pos2D: pos2D,
                pos3D: pos3D,
                normal: intersectInfo.normal,
                direction: intersectInfo.searchRay.direction,
                button: "NoButtons",
                isPrimaryHeld: true
            };

            var POINTER_PRESS_TO_MOVE_DELAY = 0.25; // seconds
            if (this.deadspotExpired || this.touchingEnterTimer > POINTER_PRESS_TO_MOVE_DELAY ||
                Vec3.distance(intersectInfo.point, this.touchingEnterPointerEvent.pos3D) > this.deadspotRadius) {
                Overlays.sendMouseMoveOnOverlay(this.grabbedOverlay, pointerEvent);
                this.deadspotExpired = true;
            }

            this.intersectionDistance = intersectInfo.distance;
            if (this.state == STATE_OVERLAY_LASER_TOUCHING) {
                this.searchIndicatorOn(intersectInfo.searchRay);
            }
            Reticle.setVisible(false);
        } else {
            this.grabbedThingID = null;
            this.setState(STATE_OFF, "grabbed overlay was destroyed");
            return;
        }
    };

    this.stylusTouchingEnter = function () {
        this.stealTouchFocus(this.stylusTarget);
        sendTouchStartEventToStylusTarget(this.hand, this.stylusTarget);
        Controller.triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION, this.hand);

        this.touchingEnterTimer = 0;
        this.touchingEnterStylusTarget = this.stylusTarget;
        this.deadspotExpired = false;

        var TOUCH_PRESS_TO_MOVE_DEADSPOT = 0.0381;
        this.deadspotRadius = TOUCH_PRESS_TO_MOVE_DEADSPOT;
    };

    this.stylusTouchingExit = function () {

        if (this.stylusTarget === undefined) {
            return;
        }

        // special case to handle home button.
        if (this.stylusTarget.overlayID === HMD.homeButtonID) {
            Messages.sendLocalMessage("home", this.stylusTarget.overlayID);
        }

        // send press event
        if (this.deadspotExpired) {
            sendTouchEndEventToStylusTarget(this.hand, this.stylusTarget);
        } else {
            sendTouchEndEventToStylusTarget(this.hand, this.touchingEnterStylusTarget);
        }
    };

    this.stylusTouching = function (dt) {

        this.touchingEnterTimer += dt;

        if (this.stylusTarget.entityID) {
            entityPropertiesCache.addEntity(this.stylusTarget.entityID);
            this.stylusTarget = calculateStylusTargetFromEntity(this.stylusTip, this.stylusTarget.entityID);
        } else if (this.stylusTarget.overlayID) {
            this.stylusTarget = calculateStylusTargetFromOverlay(this.stylusTip, this.stylusTarget.overlayID);
        }

        var TABLET_MIN_TOUCH_DISTANCE = -0.1;
        var TABLET_MAX_TOUCH_DISTANCE = 0.01;

        if (this.stylusTarget) {
            if (this.stylusTarget.distance > TABLET_MIN_TOUCH_DISTANCE &&
                this.stylusTarget.distance < TABLET_MAX_TOUCH_DISTANCE) {
                var POINTER_PRESS_TO_MOVE_DELAY = 0.33; // seconds
                if (this.deadspotExpired || this.touchingEnterTimer > POINTER_PRESS_TO_MOVE_DELAY ||
                    distance2D(this.stylusTarget.position2D,
                               this.touchingEnterStylusTarget.position2D) > this.deadspotRadius) {
                    sendTouchMoveEventToStylusTarget(this.hand, this.stylusTarget);
                    this.deadspotExpired = true;
                }
            } else {
                this.setState(STATE_OFF, "hand moved away from touch surface");
            }
        } else {
            this.setState(STATE_OFF, "touch surface was destroyed");
        }
    };

    this.release = function() {
        this.turnOffVisualizations();

        entitiesWithHoverOverlays.forEach(function (element) {
            HoverOverlay.destroyHoverOverlay(element);
        });
        entitiesWithHoverOverlays = [];

        if (this.grabbedThingID !== null) {

            Messages.sendMessage('Hifi-Teleport-Ignore-Remove', this.grabbedThingID);

            if (this.state === STATE_HOLD) {
                this.callEntityMethodOnGrabbed("releaseEquip");
            }

            // Make a small release haptic pulse if we really were holding something
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);
            if (this.actionID !== null) {
                Entities.deleteAction(this.grabbedThingID, this.actionID);
            } else {
                // no action, so it's a parenting grab
                if (this.previousParentID[this.grabbedThingID] === NULL_UUID) {
                    if (this.grabbedIsOverlay) {
                        Overlays.editOverlay(this.grabbedThingID, {
                            parentID: NULL_UUID,
                            parentJointIndex: -1
                        });
                    } else {
                        Entities.editEntity(this.grabbedThingID, {
                            parentID: this.previousParentID[this.grabbedThingID],
                            parentJointIndex: this.previousParentJointIndex[this.grabbedThingID]
                        });
                        this.ensureDynamic();
                    }
                } else {
                    if (this.grabbedIsOverlay) {
                        Overlays.editOverlay(this.grabbedThingID, {
                            parentID: this.previousParentID[this.grabbedThingID],
                            parentJointIndex: this.previousParentJointIndex[this.grabbedThingID],
                        });
                    } else {
                        // we're putting this back as a child of some other parent, so zero its velocity
                        Entities.editEntity(this.grabbedThingID, {
                            parentID: this.previousParentID[this.grabbedThingID],
                            parentJointIndex: this.previousParentJointIndex[this.grabbedThingID],
                            velocity: {x: 0, y: 0, z: 0},
                            angularVelocity: {x: 0, y: 0, z: 0}
                        });
                    }
                }
            }

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'release',
                grabbedEntity: this.grabbedThingID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
        }

        this.actionID = null;
        this.grabbedThingID = null;
        this.grabbedOverlay = null;
        this.grabbedHotspot = null;

        if (this.triggerSmoothedGrab() || this.secondarySqueezed()) {
            this.waitForTriggerRelease = true;
        }
    };

    this.cleanup = function() {
        this.release();
        this.grabPointSphereOff();
        this.hideStylus();
        this.overlayLineOff();
    };

    this.thisHandIsParent = function(props) {
        if (props.parentID !== MyAvatar.sessionUUID && props.parentID !== AVATAR_SELF_ID) {
            return false;
        }

        var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
        if (props.parentJointIndex == handJointIndex) {
            return true;
        }

        var controllerJointIndex = this.controllerJointIndex;
        if (props.parentJointIndex == controllerJointIndex) {
            return true;
        }

        var controllerCRJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ?
                                                            "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" :
                                                            "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND");
        if (props.parentJointIndex == controllerCRJointIndex) {
            return true;
        }

        return false;
    };

    this.checkForUnexpectedChildren = function() {
        var _this = this;
        // sometimes things can get parented to a hand and this script is unaware.  Search for such entities and
        // unhook them.

        // find children of avatar's hand joint
        var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
        var children = Entities.getChildrenIDsOfJoint(MyAvatar.sessionUUID, handJointIndex);
        children = children.concat(Entities.getChildrenIDsOfJoint(AVATAR_SELF_ID, handJointIndex));

        // find children of faux controller joint
        var controllerJointIndex = this.controllerJointIndex;
        children = children.concat(Entities.getChildrenIDsOfJoint(MyAvatar.sessionUUID, controllerJointIndex));
        children = children.concat(Entities.getChildrenIDsOfJoint(AVATAR_SELF_ID, controllerJointIndex));

        // find children of faux camera-relative controller joint
        var controllerCRJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ?
                                                            "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" :
                                                            "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND");
        children = children.concat(Entities.getChildrenIDsOfJoint(MyAvatar.sessionUUID, controllerCRJointIndex));
        children = children.concat(Entities.getChildrenIDsOfJoint(AVATAR_SELF_ID, controllerCRJointIndex));

        children.forEach(function(childID) {
            if (childID !== _this.stylus &&
                childID !== _this.overlayLine) {
                // we appear to be holding something and this script isn't in a state that would be holding something.
                // unhook it.  if we previously took note of this entity's parent, put it back where it was.  This
                // works around some problems that happen when more than one hand or avatar is passing something around.
                var childType = Entities.getNestableType(childID);
                if (_this.previousParentID[childID]) {
                    var previousParentID = _this.previousParentID[childID];
                    var previousParentJointIndex = _this.previousParentJointIndex[childID];

                    // The main flaw with keeping track of previous parantage in individual scripts is:
                    // (1) A grabs something (2) B takes it from A (3) A takes it from B (4) A releases it
                    // now A and B will take turns passing it back to the other.  Detect this and stop the loop here...
                    var UNHOOK_LOOP_DETECT_MS = 200;
                    var now = Date.now();
                    if (_this.previouslyUnhooked[childID]) {
                        if (now - _this.previouslyUnhooked[childID] < UNHOOK_LOOP_DETECT_MS) {
                            previousParentID = NULL_UUID;
                            previousParentJointIndex = -1;
                        }
                    }
                    _this.previouslyUnhooked[childID] = now;

                    if (childType == "overlay" && Overlays.getProperty(childID, "grabbable")) {
                        // only auto-unhook overlays that were flagged as grabbable.  this avoids unhooking overlays
                        // used in tutorial.
                        Overlays.editOverlay(childID, {
                            parentID: previousParentID,
                            parentJointIndex: previousParentJointIndex
                        });
                    }
                    if (childType == "entity") {
                        Entities.editEntity(childID, {
                            parentID: previousParentID,
                            parentJointIndex: previousParentJointIndex
                        });
                    }

                } else {
                    if (childType == "entity") {
                        Entities.editEntity(childID, { parentID: NULL_UUID });
                    } else if (childType == "overlay") {
                        if (Overlays.getProperty(childID, "grabbable")) {
                            Overlays.editOverlay(childID, { parentID: NULL_UUID });
                        }
                    }
                }
            }
        });
    };

    this.getOtherHandController = function() {
        return (this.hand === RIGHT_HAND) ? leftController : rightController;
    };
}

var rightController = new MyController(RIGHT_HAND);
var leftController = new MyController(LEFT_HAND);

var MAPPING_NAME = "com.highfidelity.handControllerGrab";

var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from([Controller.Standard.RT]).peek().to(rightController.triggerPress);
mapping.from([Controller.Standard.RTClick]).peek().to(rightController.triggerClick);

mapping.from([Controller.Standard.LT]).peek().to(leftController.triggerPress);
mapping.from([Controller.Standard.LTClick]).peek().to(leftController.triggerClick);

mapping.from([Controller.Standard.RB]).peek().to(rightController.secondaryPress);
mapping.from([Controller.Standard.LB]).peek().to(leftController.secondaryPress);
mapping.from([Controller.Standard.LeftGrip]).peek().to(leftController.secondaryPress);
mapping.from([Controller.Standard.RightGrip]).peek().to(rightController.secondaryPress);

mapping.from([Controller.Standard.LeftPrimaryThumb]).peek().to(leftController.thumbPress);
mapping.from([Controller.Standard.RightPrimaryThumb]).peek().to(rightController.thumbPress);

Controller.enableMapping(MAPPING_NAME);

function handleMenuEvent(menuItem) {
    if (menuItem === "Show Grab Sphere") {
        SHOW_GRAB_POINT_SPHERE = Menu.isOptionChecked("Show Grab Sphere");
    }
}

Menu.addMenuItem({ menuName: "Developer", menuItemName: "Show Grab Sphere", isCheckable: true, isChecked: false });
Menu.menuItemEvent.connect(handleMenuEvent);

// the section below allows the grab script to listen for messages
// that disable either one or both hands.  useful for two handed items
var handToDisable = 'none';

function update(deltaTime) {
    var timestamp = Date.now();

    if (handToDisable !== LEFT_HAND && handToDisable !== 'both') {
        leftController.update(deltaTime, timestamp);
    } else {
        leftController.release();
    }
    if (handToDisable !== RIGHT_HAND && handToDisable !== 'both') {
        rightController.update(deltaTime, timestamp);
    } else {
        rightController.release();
    }
    equipHotspotBuddy.update(deltaTime, timestamp);
    entityPropertiesCache.update();
}

Messages.subscribe('Hifi-Grab-Disable');
Messages.subscribe('Hifi-Hand-Disabler');
Messages.subscribe('Hifi-Hand-Grab');
Messages.subscribe('Hifi-Hand-RayPick-Blacklist');
Messages.subscribe('Hifi-Object-Manipulation');
Messages.subscribe('Hifi-Hand-Drop');

var handleHandMessages = function(channel, message, sender) {
    var data;
    if (sender === MyAvatar.sessionUUID) {
        if (channel === 'Hifi-Hand-Disabler') {
            if (message === 'left') {
                handToDisable = LEFT_HAND;
                leftController.turnOffVisualizations();
            }
            if (message === 'right') {
                handToDisable = RIGHT_HAND;
                rightController.turnOffVisualizations();
            }
            if (message === 'both' || message === 'none') {
                if (message === 'both') {
                    rightController.turnOffVisualizations();
                    leftController.turnOffVisualizations();

                }
                handToDisable = message;
            }
        } else if (channel === 'Hifi-Grab-Disable') {
            data = JSON.parse(message);
            if (data.holdEnabled !== undefined) {
                print("holdEnabled: ", data.holdEnabled);
                holdEnabled = data.holdEnabled;
            }
            if (data.nearGrabEnabled !== undefined) {
                print("nearGrabEnabled: ", data.nearGrabEnabled);
                nearGrabEnabled = data.nearGrabEnabled;
            }
            if (data.farGrabEnabled !== undefined) {
                print("farGrabEnabled: ", data.farGrabEnabled);
                farGrabEnabled = data.farGrabEnabled;
            }
            if (data.myAvatarScalingEnabled !== undefined) {
                print("myAvatarScalingEnabled: ", data.myAvatarScalingEnabled);
                myAvatarScalingEnabled = data.myAvatarScalingEnabled;
            }
            if (data.objectScalingEnabled !== undefined) {
                print("objectScalingEnabled: ", data.objectScalingEnabled);
                objectScalingEnabled = data.objectScalingEnabled;
            }
        } else if (channel === 'Hifi-Hand-Grab') {
            try {
                data = JSON.parse(message);
                var selectedController = (data.hand === 'left') ? leftController : rightController;
                var hotspotIndex = data.hotspotIndex !== undefined ? parseInt(data.hotspotIndex) : 0;
                selectedController.release();
                var wearableEntity = data.entityID;
                entityPropertiesCache.addEntity(wearableEntity);
                selectedController.grabbedThingID = wearableEntity;
                var hotspots = selectedController.collectEquipHotspots(selectedController.grabbedThingID);
                if (hotspots.length > 0) {
                    if (hotspotIndex >= hotspots.length) {
                        hotspotIndex = 0;
                    }
                    selectedController.grabbedHotspot = hotspots[hotspotIndex];
                }
                selectedController.setState(STATE_HOLD, "Hifi-Hand-Grab msg received");
                selectedController.nearGrabbingEnter();

            } catch (e) {
                print("WARNING: handControllerGrab.js -- error parsing Hifi-Hand-Grab message: " + message);
            }

        } else if (channel === 'Hifi-Hand-RayPick-Blacklist') {
            try {
                data = JSON.parse(message);
                var action = data.action;
                var id = data.id;
                var index = blacklist.indexOf(id);

                if (action === 'add' && index === -1) {
                    blacklist.push(id);
                }
                if (action === 'remove') {
                    if (index > -1) {
                        blacklist.splice(index, 1);
                    }
                }

            } catch (e) {
                print("WARNING: handControllerGrab.js -- error parsing Hifi-Hand-RayPick-Blacklist message: " + message);
            }
        } else if (channel === 'Hifi-Hand-Drop') {
            if (message === 'left') {
                leftController.release();
            } else if (message === 'right') {
                rightController.release();
            } else if (message === 'both') {
                leftController.release();
                rightController.release();
            }
        }
    }
};

Messages.messageReceived.connect(handleHandMessages);

var TARGET_UPDATE_HZ = 60; // 50hz good enough, but we're using update
var BASIC_TIMER_INTERVAL_MS = 1000 / TARGET_UPDATE_HZ;
var lastInterval = Date.now();

var intervalCount = 0;
var totalDelta = 0;
var totalVariance = 0;
var highVarianceCount = 0;
var veryhighVarianceCount = 0;
var updateTotalWork = 0;

var UPDATE_PERFORMANCE_DEBUGGING = false;

var updateWrapper = function () {

    intervalCount++;
    var thisInterval = Date.now();
    var deltaTimeMsec = thisInterval - lastInterval;
    var deltaTime = deltaTimeMsec / 1000;
    lastInterval = thisInterval;

    totalDelta += deltaTimeMsec;

    var variance = Math.abs(deltaTimeMsec - BASIC_TIMER_INTERVAL_MS);
    totalVariance += variance;

    if (variance > 1) {
        highVarianceCount++;
    }

    if (variance > 5) {
        veryhighVarianceCount++;
    }

    // will call update for both hands
    var preWork = Date.now();
    update(deltaTime);
    var postWork = Date.now();
    var workDelta = postWork - preWork;
    updateTotalWork += workDelta;

    if (intervalCount == 100) {

        if (UPDATE_PERFORMANCE_DEBUGGING) {
            print("handControllerGrab.js -- For " + intervalCount + " samples average= " +
                  totalDelta/intervalCount + " ms" +
                  " average variance:" + totalVariance/intervalCount + " ms" +
                  " high variance count:" + highVarianceCount + " [ " + (highVarianceCount/intervalCount) * 100 + "% ] " +
                  " VERY high variance count:" + veryhighVarianceCount +
                  " [ " + (veryhighVarianceCount/intervalCount) * 100 + "% ] " +
                  " average work:" + updateTotalWork/intervalCount + " ms");
        }

        intervalCount = 0;
        totalDelta = 0;
        totalVariance = 0;
        highVarianceCount = 0;
        veryhighVarianceCount = 0;
        updateTotalWork = 0;
    }

    Script.setTimeout(updateWrapper, UPDATE_SLEEP_MS);
};

Script.setTimeout(updateWrapper, UPDATE_SLEEP_MS);
function cleanup() {
    Menu.removeMenuItem("Developer", "Show Grab Sphere");
    rightController.cleanup();
    leftController.cleanup();
    Controller.disableMapping(MAPPING_NAME);
    Reticle.setVisible(true);
}

Script.scriptEnding.connect(cleanup);

}()); // END LOCAL_SCOPE
