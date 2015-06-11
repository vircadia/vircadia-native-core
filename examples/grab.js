//  grab.js
//  examples
//
//  Created by Eric Levin on May 1, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Grab's physically moveable entities with the mouse, by applying a spring force.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var MOVE_TIMESCALE = 0.1;
var INV_MOVE_TIMESCALE = 1.0 / MOVE_TIMESCALE;
var MAX_SOLID_ANGLE = 0.01; // objects that appear smaller than this can't be grabbed
var CLOSE_ENOUGH = 0.001;
var ZERO_VEC3 = { x: 0, y: 0, z: 0 };
var ANGULAR_DAMPING_RATE = 0.40;

// NOTE: to improve readability global variable names start with 'g'
var gIsGrabbing = false;
var gGrabbedEntity = null;
var gActionID = null;
var gEntityProperties;
var gStartPosition;
var gStartRotation;
var gCurrentPosition;
var gOriginalGravity = ZERO_VEC3;
var gPlaneNormal = ZERO_VEC3;

// gMaxGrabDistance is a function of the size of the object.
var gMaxGrabDistance;

// gGrabMode defines the degrees of freedom of the grab target positions
// relative to gGrabStartPosition options include:
//     xzPlane  (default)
//     verticalCylinder  (SHIFT)
//     rotate  (CONTROL)
// Modes to eventually support?:
//     xyPlane
//     yzPlane
//     polar
//     elevationAzimuth
var gGrabMode = "xzplane";

// gGrabOffset allows the user to grab an object off-center.  It points from ray's intersection
// with the move-plane to object center (at the moment the grab is initiated).  Future target positions
// are relative to the ray's intersection by the same offset.
var gGrabOffset = { x: 0, y: 0, z: 0 };

var gTargetPosition;
var gTargetRotation;
var gLiftKey = false; // SHIFT
var gRotateKey = false; // CONTROL

var gInitialMouse = { x: 0, y: 0 };
var gPreviousMouse = { x: 0, y: 0 };
var gMouseCursorLocation = { x: 0, y: 0 };
var gMouseAtRotateStart = { x: 0, y: 0 };

var gBeaconHeight = 0.10;

// var gAngularVelocity = ZERO_VEC3;

// TODO: play sounds again when we aren't leaking AudioInjector threads
// var grabSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/CloseClamp.wav");
// var releaseSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/ReleaseClamp.wav");
// var VOLUME = 0.0;

var gBeaconHeight = 0.10;
var BEACON_COLOR = {
    red: 200,
    green: 200,
    blue: 200
};
var BEACON_WIDTH = 2;


var gBeacon = Overlays.addOverlay("line3d", {
    color: BEACON_COLOR,
    alpha: 1,
    visible: false,
    lineWidth: BEACON_WIDTH
});

function updateDropLine(position) {
    Overlays.editOverlay(gBeacon, {
        visible: true,
        start: {
            x: position.x,
            y: position.y + gBeaconHeight,
            z: position.z
        },
        end: {
            x: position.x,
            y: position.y - gBeaconHeight,
            z: position.z
        }
    });
}

function mouseIntersectionWithPlane(pointOnPlane, planeNormal, event) {
    var cameraPosition = Camera.getPosition();
    var localPointOnPlane = Vec3.subtract(pointOnPlane, cameraPosition);
    var distanceFromPlane = Vec3.dot(localPointOnPlane, planeNormal);
    var MIN_DISTANCE_FROM_PLANE = 0.001;
    if (Math.abs(distanceFromPlane) < MIN_DISTANCE_FROM_PLANE) {
        // camera is touching the plane
        return pointOnPlane;
    }
    var pickRay = Camera.computePickRay(event.x, event.y);
    var dirDotNorm = Vec3.dot(pickRay.direction, planeNormal);
    var MIN_RAY_PLANE_DOT = 0.00001;

    var localIntersection;
    var useMaxForwardGrab = false;
    if (Math.abs(dirDotNorm) > MIN_RAY_PLANE_DOT) {
        var distanceToIntersection = distanceFromPlane / dirDotNorm;
        if (distanceToIntersection > 0 && distanceToIntersection < gMaxGrabDistance) {
            // ray points into the plane
            localIntersection = Vec3.multiply(pickRay.direction, distanceFromPlane / dirDotNorm);
        } else {
            // ray intersects BEHIND the camera or else very far away
            // so we clamp the grab point to be the maximum forward position
            useMaxForwardGrab = true;
        }
    } else {
        // ray points perpendicular to grab plane
        // so we map the grab point to the maximum forward position
        useMaxForwardGrab = true;
    }
    if (useMaxForwardGrab) {
        // we re-route the intersection to be in front at max distance.
        var rayDirection = Vec3.subtract(pickRay.direction, Vec3.multiply(planeNormal, dirDotNorm));
        rayDirection = Vec3.normalize(rayDirection);
        localIntersection = Vec3.multiply(rayDirection, gMaxGrabDistance);
        localIntersection = Vec3.sum(localIntersection, Vec3.multiply(planeNormal, distanceFromPlane));
    }
    var worldIntersection = Vec3.sum(cameraPosition, localIntersection);
    return worldIntersection;
}

function computeNewGrabPlane() {
    if (!gIsGrabbing) {
        return;
    }

    var maybeResetMousePosition = false;
    if (gGrabMode !== "rotate") {
        gMouseAtRotateStart = gMouseCursorLocation;
    } else {
        maybeResetMousePosition = true;
    }
    gGrabMode = "xzPlane";
    gPointOnPlane = gCurrentPosition;
    gPlaneNormal = { x: 0, y: 1, z: 0 };
    if (gLiftKey) {
        if (!gRotateKey) {
            gGrabMode = "verticalCylinder";
            // a new planeNormal will be computed each move
        }
    } else if (gRotateKey) {
        gGrabMode = "rotate";
    }

    gPointOnPlane = Vec3.subtract(gCurrentPosition, gGrabOffset);
    var xzOffset = Vec3.subtract(gPointOnPlane, Camera.getPosition());
    xzOffset.y = 0;
    gXzDistanceToGrab = Vec3.length(xzOffset);

    if (gGrabMode !== "rotate" && maybeResetMousePosition) {
        // we reset the mouse position whenever we stop rotating
        Window.setCursorPosition(gMouseAtRotateStart.x, gMouseAtRotateStart.y);
    }
}

function mousePressEvent(event) {
    if (!event.isLeftButton) {
        return;
    }
    gInitialMouse = {x: event.x, y: event.y };
    gPreviousMouse = {x: event.x, y: event.y };

    var pickRay = Camera.computePickRay(event.x, event.y);
    var pickResults = Entities.findRayIntersection(pickRay, true); // accurate picking
    if (!pickResults.intersects) {
        // didn't click on anything
        return;
    }

    if (!pickResults.properties.collisionsWillMove) {
        // only grab dynamic objects
        return;
    }

    var clickedEntity = pickResults.entityID;
    var entityProperties = Entities.getEntityProperties(clickedEntity)
    gStartPosition = entityProperties.position;
    gStartRotation = entityProperties.rotation;
    var cameraPosition = Camera.getPosition();

    gBeaconHeight = Vec3.length(entityProperties.dimensions);
    gMaxGrabDistance = gBeaconHeight / MAX_SOLID_ANGLE;
    if (Vec3.distance(gStartPosition, cameraPosition) > gMaxGrabDistance) {
        // don't allow grabs of things far away
        return;
    }

    Entities.editEntity(clickedEntity, { gravity: ZERO_VEC3 });
    gIsGrabbing = true;

    gGrabbedEntity = clickedEntity;
    gCurrentPosition = entityProperties.position;
    gOriginalGravity = entityProperties.gravity;
    gTargetPosition = gStartPosition;

    // compute the grab point
    var nearestPoint = Vec3.subtract(gStartPosition, cameraPosition);
    var distanceToGrab = Vec3.dot(nearestPoint, pickRay.direction);
    nearestPoint = Vec3.multiply(distanceToGrab, pickRay.direction);
    gPointOnPlane = Vec3.sum(cameraPosition, nearestPoint);

    // compute the grab offset
    gGrabOffset = Vec3.subtract(gStartPosition, gPointOnPlane);

    computeNewGrabPlane();

    updateDropLine(gStartPosition);

    // TODO: play sounds again when we aren't leaking AudioInjector threads
    //Audio.playSound(grabSound, { position: entityProperties.position, volume: VOLUME });
}

function mouseReleaseEvent() {
    if (gIsGrabbing) {
        if (Vec3.length(gOriginalGravity) != 0) {
            Entities.editEntity(gGrabbedEntity, { gravity: gOriginalGravity });
        }

        gIsGrabbing = false
        Entities.deleteAction(gGrabbedEntity, gActionID);
        gActionID = null;

        Overlays.editOverlay(gBeacon, { visible: false });

        // TODO: play sounds again when we aren't leaking AudioInjector threads
        //Audio.playSound(releaseSound, { position: entityProperties.position, volume: VOLUME });
    }
}

function mouseMoveEvent(event) {
    if (!gIsGrabbing) {
        return;
    }

    // see if something added/restored gravity
    var entityProperties = Entities.getEntityProperties(gGrabbedEntity);
    if (Vec3.length(entityProperties.gravity) != 0) {
        gOriginalGravity = entityProperties.gravity;
    }

    var actionArgs = {};

    if (gGrabMode === "rotate") {
        var deltaMouse = { x: 0, y: 0 };
        var dx = event.x - gInitialMouse.x;
        var dy = event.y - gInitialMouse.y;
        var orientation = Camera.getOrientation();
        var dragOffset = Vec3.multiply(dx, Quat.getRight(orientation));
        dragOffset = Vec3.sum(dragOffset, Vec3.multiply(-dy, Quat.getUp(orientation)));
        var axis = Vec3.cross(dragOffset, Quat.getFront(orientation));
        axis = Vec3.normalize(axis);
        var ROTATE_STRENGTH = 0.4; // magic number tuned by hand
        var angle = ROTATE_STRENGTH * Math.sqrt((dx * dx) + (dy * dy));
        var deltaQ = Quat.angleAxis(angle, axis);
        // var qZero = entityProperties.rotation;
        var qZero = gStartRotation;
        var qOne = Quat.multiply(deltaQ, qZero);
        actionArgs = {targetRotation: qOne, angularTimeScale: 0.1};
    } else {
        var newTargetPosition;
        if (gGrabMode === "verticalCylinder") {
            // for this mode we recompute the plane based on current Camera
            var planeNormal = Quat.getFront(Camera.getOrientation());
            planeNormal.y = 0;
            planeNormal = Vec3.normalize(planeNormal);
            var pointOnCylinder = Vec3.multiply(planeNormal, gXzDistanceToGrab);
            pointOnCylinder = Vec3.sum(Camera.getPosition(), pointOnCylinder);
            newTargetPosition = mouseIntersectionWithPlane(pointOnCylinder, planeNormal, event);
        } else {
            var cameraPosition = Camera.getPosition();
            newTargetPosition = mouseIntersectionWithPlane(gPointOnPlane, gPlaneNormal, event);
            var relativePosition = Vec3.subtract(newTargetPosition, cameraPosition);
            var distance = Vec3.length(relativePosition);
            if (distance > gMaxGrabDistance) {
                // clamp distance
                relativePosition = Vec3.multiply(relativePosition, gMaxGrabDistance / distance);
                newTargetPosition = Vec3.sum(relativePosition, cameraPosition);
            }
        }
        gTargetPosition = Vec3.sum(newTargetPosition, gGrabOffset);
        actionArgs = {targetPosition: gTargetPosition, linearTimeScale: 0.1};
    }
    gPreviousMouse = { x: event.x, y: event.y };
    gMouseCursorLocation = { x: Window.getCursorPositionX(), y: Window.getCursorPositionY() };

    if (!gActionID) {
        gActionID = Entities.addAction("spring", gGrabbedEntity, actionArgs);
    } else {
        Entities.updateAction(gGrabbedEntity, gActionID, actionArgs);
    }

    updateDropLine(gTargetPosition);
}

function keyReleaseEvent(event) {
    if (event.text === "SHIFT") {
        gLiftKey = false;
    }
    if (event.text === "CONTROL") {
        gRotateKey = false;
    }
    computeNewGrabPlane();
}

function keyPressEvent(event) {
    if (event.text === "SHIFT") {
        gLiftKey = true;
    }
    if (event.text === "CONTROL") {
        gRotateKey = true;
    }
    computeNewGrabPlane();
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
