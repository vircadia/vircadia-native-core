"use strict";

//  touchEventUtils.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND,
   enableDispatcherModule, disableDispatcherModule, makeRunningValues,
   Messages, Quat, Vec3, getControllerWorldLocation, makeDispatcherModuleParameters, Overlays, controllerDispatcher.ZERO_VEC,
   HMD, INCHES_TO_METERS, DEFAULT_REGISTRATION_POINT, Settings, getGrabPointSphereOffset
*/

var controllerDispatcher = Script.require("/~/system/libraries/controllerDispatcherUtils.js");
function touchTargetHasKeyboardFocus(touchTarget) {
    if (touchTarget.entityID && touchTarget.entityID !== Uuid.NULL) {
        return Entities.keyboardFocusEntity === touchTarget.entityID;
    } else if (touchTarget.overlayID && touchTarget.overlayID !== Uuid.NULL) {
        return Overlays.keyboardFocusOverlay === touchTarget.overlayID;
    }
}

function setKeyboardFocusOnTouchTarget(touchTarget) {
    if (touchTarget.entityID && touchTarget.entityID !== Uuid.NULL &&
        Entities.wantsHandControllerPointerEvents(touchTarget.entityID)) {
        Overlays.keyboardFocusOverlay = Uuid.NULL;
        Entities.keyboardFocusEntity = touchTarget.entityID;
    } else if (touchTarget.overlayID && touchTarget.overlayID !== Uuid.NULL) {
        Overlays.keyboardFocusOverlay = touchTarget.overlayID;
        Entities.keyboardFocusEntity = Uuid.NULL;
    }
}

function sendHoverEnterEventToTouchTarget(hand, touchTarget) {
    var pointerEvent = {
        type: "Move",
        id: hand + 1, // 0 is reserved for hardware mouse
        pos2D: touchTarget.position2D,
        pos3D: touchTarget.position,
        normal: touchTarget.normal,
        direction: Vec3.subtract(controllerDispatcher.ZERO_VEC, touchTarget.normal),
        button: "None"
    };

    if (touchTarget.entityID && touchTarget.entityID !== Uuid.NULL) {
        Entities.sendHoverEnterEntity(touchTarget.entityID, pointerEvent);
    } else if (touchTarget.overlayID && touchTarget.overlayID !== Uuid.NULL) {
        Overlays.sendHoverEnterOverlay(touchTarget.overlayID, pointerEvent);
    }
}

function sendHoverOverEventToTouchTarget(hand, touchTarget) {
    var pointerEvent = {
        type: "Move",
        id: hand + 1, // 0 is reserved for hardware mouse
        pos2D: touchTarget.position2D,
        pos3D: touchTarget.position,
        normal: touchTarget.normal,
        direction: Vec3.subtract(controllerDispatcher.ZERO_VEC, touchTarget.normal),
        button: "None"
    };

    if (touchTarget.entityID && touchTarget.entityID !== Uuid.NULL) {
        Entities.sendMouseMoveOnEntity(touchTarget.entityID, pointerEvent);
        Entities.sendHoverOverEntity(touchTarget.entityID, pointerEvent);
    } else if (touchTarget.overlayID && touchTarget.overlayID !== Uuid.NULL) {
        Overlays.sendMouseMoveOnOverlay(touchTarget.overlayID, pointerEvent);
        Overlays.sendHoverOverOverlay(touchTarget.overlayID, pointerEvent);
    }
}

function sendTouchStartEventToTouchTarget(hand, touchTarget) {
    var pointerEvent = {
        type: "Press",
        id: hand + 1, // 0 is reserved for hardware mouse
        pos2D: touchTarget.position2D,
        pos3D: touchTarget.position,
        normal: touchTarget.normal,
        direction: Vec3.subtract(controllerDispatcher.ZERO_VEC, touchTarget.normal),
        button: "Primary",
        isPrimaryHeld: true
    };

    if (touchTarget.entityID && touchTarget.entityID !== Uuid.NULL) {
        Entities.sendMousePressOnEntity(touchTarget.entityID, pointerEvent);
        Entities.sendClickDownOnEntity(touchTarget.entityID, pointerEvent);
    } else if (touchTarget.overlayID && touchTarget.overlayID !== Uuid.NULL) {
        Overlays.sendMousePressOnOverlay(touchTarget.overlayID, pointerEvent);
    }
}

function sendTouchEndEventToTouchTarget(hand, touchTarget) {
    var pointerEvent = {
        type: "Release",
        id: hand + 1, // 0 is reserved for hardware mouse
        pos2D: touchTarget.position2D,
        pos3D: touchTarget.position,
        normal: touchTarget.normal,
        direction: Vec3.subtract(controllerDispatcher.ZERO_VEC, touchTarget.normal),
        button: "Primary"
    };

    if (touchTarget.entityID && touchTarget.entityID !== Uuid.NULL) {
        Entities.sendMouseReleaseOnEntity(touchTarget.entityID, pointerEvent);
        Entities.sendClickReleaseOnEntity(touchTarget.entityID, pointerEvent);
        Entities.sendHoverLeaveEntity(touchTarget.entityID, pointerEvent);
    } else if (touchTarget.overlayID && touchTarget.overlayID !== Uuid.NULL) {
        Overlays.sendMouseReleaseOnOverlay(touchTarget.overlayID, pointerEvent);
    }
}

function sendTouchMoveEventToTouchTarget(hand, touchTarget) {
    var pointerEvent = {
        type: "Move",
        id: hand + 1, // 0 is reserved for hardware mouse
        pos2D: touchTarget.position2D,
        pos3D: touchTarget.position,
        normal: touchTarget.normal,
        direction: Vec3.subtract(controllerDispatcher.ZERO_VEC, touchTarget.normal),
        button: "Primary",
        isPrimaryHeld: true
    };

    if (touchTarget.entityID && touchTarget.entityID !== Uuid.NULL) {
        Entities.sendMouseMoveOnEntity(touchTarget.entityID, pointerEvent);
        Entities.sendHoldingClickOnEntity(touchTarget.entityID, pointerEvent);
    } else if (touchTarget.overlayID && touchTarget.overlayID !== Uuid.NULL) {
        Overlays.sendMouseMoveOnOverlay(touchTarget.overlayID, pointerEvent);
    }
}

function composeTouchTargetFromIntersection(intersection) {
    var isEntity = (intersection.type === Picks.INTERSECTED_ENTITY);
    var objectID = intersection.objectID;
    var worldPos = intersection.intersection;
    var props = null;
    if (isEntity) {
        props = Entities.getProperties(intersection.objectID);
    }

    var position2D =(isEntity ? controllerDispatcher.projectOntoEntityXYPlane(objectID, worldPos, props) :
        controllerDispatcher.projectOntoOverlayXYPlane(objectID, worldPos));
    return {
        entityID: isEntity ? objectID : null,
        overlayID: isEntity ? null : objectID,
        distance: intersection.distance,
        position: worldPos,
        position2D: position2D,
        normal: intersection.surfaceNormal
    };
}

// will return undefined if overlayID does not exist.
function calculateTouchTargetFromOverlay(touchTip, overlayID) {
    var overlayPosition = Overlays.getProperty(overlayID, "position");
    if (overlayPosition === undefined) {
        return;
    }

    // project touchTip onto overlay plane.
    var overlayRotation = Overlays.getProperty(overlayID, "rotation");
    if (overlayRotation === undefined) {
        return;
    }
    var normal = Vec3.multiplyQbyV(overlayRotation, {x: 0, y: 0, z: 1});
    var distance = Vec3.dot(Vec3.subtract(touchTip.position, overlayPosition), normal);
    var position = Vec3.subtract(touchTip.position, Vec3.multiply(normal, distance));

    // calclulate normalized position
    var invRot = Quat.inverse(overlayRotation);
    var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(position, overlayPosition));

    var dimensions = Overlays.getProperty(overlayID, "dimensions");
    if (dimensions === undefined) {
        return;
    }
    dimensions.z = 0.01; // we are projecting onto the XY plane of the overlay, so ignore the z dimension
    var invDimensions = { x: 1 / dimensions.x, y: 1 / dimensions.y, z: 1 / dimensions.z };
    var normalizedPosition = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), DEFAULT_REGISTRATION_POINT);

    // 2D position on overlay plane in meters, relative to the bounding box upper-left hand corner.
    var position2D = {
        x: normalizedPosition.x * dimensions.x,
        y: (1 - normalizedPosition.y) * dimensions.y // flip y-axis
    };

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

// will return undefined if entity does not exist.
function calculateTouchTargetFromEntity(touchTip, props) {
    if (props.rotation === undefined) {
        // if rotation is missing from props object, then this entity has probably been deleted.
        return;
    }

    // project touch tip onto entity plane.
    var normal = Vec3.multiplyQbyV(props.rotation, {x: 0, y: 0, z: 1});
    Vec3.multiplyQbyV(props.rotation, {x: 0, y: 1, z: 0});
    var distance = Vec3.dot(Vec3.subtract(touchTip.position, props.position), normal);
    var position = Vec3.subtract(touchTip.position, Vec3.multiply(normal, distance));

    // generate normalized coordinates
    var invRot = Quat.inverse(props.rotation);
    var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(position, props.position));
    var invDimensions = { x: 1 / props.dimensions.x, y: 1 / props.dimensions.y, z: 1 / props.dimensions.z };
    var normalizedPosition = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), props.registrationPoint);

    // 2D position on entity plane in meters, relative to the bounding box upper-left hand corner.
    var position2D = {
        x: normalizedPosition.x * props.dimensions.x,
        y: (1 - normalizedPosition.y) * props.dimensions.y // flip y-axis
    };

    return {
        entityID: props.id,
        entityProps: props,
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

module.exports = {
    calculateTouchTargetFromEntity: calculateTouchTargetFromEntity,
    calculateTouchTargetFromOverlay: calculateTouchTargetFromOverlay,
    touchTargetHasKeyboardFocus: touchTargetHasKeyboardFocus,
    setKeyboardFocusOnTouchTarget: setKeyboardFocusOnTouchTarget,
    sendHoverEnterEventToTouchTarget: sendHoverEnterEventToTouchTarget,
    sendHoverOverEventToTouchTarget: sendHoverOverEventToTouchTarget,
    sendTouchStartEventToTouchTarget: sendTouchStartEventToTouchTarget,
    sendTouchEndEventToTouchTarget: sendTouchEndEventToTouchTarget,
    sendTouchMoveEventToTouchTarget: sendTouchMoveEventToTouchTarget,
    composeTouchTargetFromIntersection: composeTouchTargetFromIntersection
};
