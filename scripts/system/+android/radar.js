"use strict";
//
// radar.js
// scripts/system/+android/
//
// Created by Cristian Duarte & Gabriel Calero on 31 Jan 2018
// Copyright 2018 High Fidelity, Inc.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or
// http://www.apache.org/licenses/LICENSE-2.0.html
//

var radarModeInterface = {};

var logEnabled = false;
function printd(str) {
    if (logEnabled) {
        print("[radar.js] " + str);
    }
}

var radar = false;
var radarHeight = 10; // camera position meters above the avatar
var tablet;

var RADAR_CAMERA_OFFSET = -1; // 1 meter below the avatar
var ABOVE_GROUND_DROP = 2;
var MOVE_BY = 1;

// Swipe/Drag vars
var PINCH_INCREMENT_FIRST = 0.4; // 0.1 meters zoom in - out
var PINCH_INCREMENT = 0.4; // 0.1 meters zoom in - out
var RADAR_HEIGHT_MAX_PLUS_AVATAR = 80;
var RADAR_HEIGHT_MIN_PLUS_AVATAR = 2;
var RADAR_CAMERA_DISTANCE_TO_ICONS = 1.5; // Icons are near the camera to prevent the LOD manager dismissing them
var RADAR_ICONS_APPARENT_DISTANCE_TO_AVATAR_BASE = 1; // How much above the avatar base should the icon appear
var AVATAR_DISPLAY_NAME_HEIGHT = 106;
var AVATAR_DISPLAY_NAME_CHAR_WIDTH = 48;
var AVATAR_DISPLAY_NAME_FONT_SIZE = 50;
var lastDragAt;
var lastDeltaDrag;

var uniqueColor;

function moveTo(position) {
    if (radar) {
        MyAvatar.position = position;
        Camera.position = Vec3.sum(MyAvatar.position, {
            x : 0,
            y : radarHeight,
            z : 0
        });
    }
}

function keyPressEvent(event) {
    if (radar) {
        switch (event.text) {
        case "UP":
            moveTo(Vec3.sum(MyAvatar.position, {
                x : 0.0,
                y : 0,
                z : -1 * MOVE_BY
            }));
            break;
        case "DOWN":
            moveTo(Vec3.sum(MyAvatar.position, {
                x : 0,
                y : 0,
                z : MOVE_BY
            }));
            break;
        case "LEFT":
            moveTo(Vec3.sum(MyAvatar.position, {
                x : -1 * MOVE_BY,
                y : 0,
                z : 0
            }));
            break;
        case "RIGHT":
            moveTo(Vec3.sum(MyAvatar.position, {
                x : MOVE_BY,
                y : 0,
                z : 0
            }));
            break;
        }
    }
}

function actionOnObjectFromEvent(event) {
    var rayIntersection = findRayIntersection(Camera.computePickRay(event.x,
            event.y));
    if (rayIntersection && rayIntersection.intersects
            && rayIntersection.overlayID) {
        printd("found overlayID touched " + rayIntersection.overlayID);
        if (entitiesByOverlayID[rayIntersection.overlayID]) {
            var entity = Entities.getEntityProperties(
                    entitiesByOverlayID[rayIntersection.overlayID],
                    [ "sourceUrl" ]);
            App.openUrl(entity.sourceUrl);
            return true;
        }
    }
    if (rayIntersection && rayIntersection.intersects
            && rayIntersection.entityID && rayIntersection.properties) {
        printd("found " + rayIntersection.entityID + " of type "
                + rayIntersection.properties.type);
        if (rayIntersection.properties.type == "Web") {
            printd("found web element to "
                    + rayIntersection.properties.sourceUrl);
            App.openUrl(rayIntersection.properties.sourceUrl);
            return true;
        }
    }
    return false;
}

function mousePress(event) {
    mousePressOrTouchEnd(event);
}

function mousePressOrTouchEnd(event) {
    if (radar) {
        if (actionOnObjectFromEvent(event)) {
            return;
        }
    }
}

function toggleRadarMode() {
    if (radar) {
        endRadar();
    } else {
        startRadar();
    }
}

function fakeDoubleTap(event) {
    // CLD - temporarily disable toggling mode through double tap
    // * As we have a new UI for toggling between modes, it may be discarded
    // completely in the future.
    // toggleRadarMode();
    teleporter.dragTeleportUpdate(event);
    teleporter.dragTeleportRelease(event);
}

var DOUBLE_TAP_TIME = 300;
var fakeDoubleTapStart = Date.now();
var touchEndCount = 0;

/*
 * Counts touchEnds and if there were 2 in the DOUBLE_TAP_TIME lapse, it
 * triggers a fakeDoubleTap and returns true. Otherwise, returns false (no
 * double tap yet)
 */
function analyzeDoubleTap(event) {
    var fakeDoubleTapEnd = Date.now();
    var elapsed = fakeDoubleTapEnd - fakeDoubleTapStart;
    if (elapsed > DOUBLE_TAP_TIME) {
        touchEndCount = 0;
    }

    // if this is our first "up" then record time so we can
    // later determine if second "up" is a double tap
    if (touchEndCount == 0) {
        fakeDoubleTapStart = Date.now();
    }
    touchEndCount++;

    if (touchEndCount >= 2) {
        var fakeDoubleTapEnd = Date.now();
        var elapsed = fakeDoubleTapEnd - fakeDoubleTapStart;
        printd("-- fakeDoubleTapEnd:" + fakeDoubleTapEnd + "-- elapsed:"
                + elapsed)
        if (elapsed <= DOUBLE_TAP_TIME) {
            touchEndCount = 0;
            fakeDoubleTap(event);
            return true; // don't do the normal touch end processing
        }

        touchEndCount = 0;
    }
    return false;
}

function touchEnd(event) {
    printd("touchEnd received " + JSON.stringify(event));
    // Clean up touch variables
    lastDragAt = null;
    lastDeltaDrag = null;
    touchStartingCoordinates = null; // maybe in special cases it should be
                                        // setup later?
    startedDraggingCamera = false;
    prevTouchPinchRadius = null;
    draggingCamera = false;

    if (movingCamera) {
        // if camera was indeed moving, we should not further process, it was
        // just dragging
        movingCamera = false;
        dragModeFunc = null;
        return;
    }

    // teleport release analysis
    if (teleporter && teleporter.dragTeleportUpdate == dragModeFunc) {
        teleporter.dragTeleportRelease(event);
        dragModeFunc = null;
        return;
    }
    dragModeFunc = null;

    // if pinching or moving is still detected, cancel
    if (event.isPinching) {
        printd("touchEnd fail because isPinching");
        return;
    }
    if (event.isPinchOpening) {
        printd("touchEnd fail because isPinchingOpening");
        return;
    }
    if (event.isMoved) {
        printd("touchEnd fail because isMoved");
        return;
    }

    if (analyzeDoubleTap(event))
        return; // double tap detected, finish

    if (radar) {
        mousePressOrTouchEnd(event);
    }
}

/**
 * Polyfill for sign(x)
 */
if (!Math.sign) {
    Math.sign = function(x) {
        // If x is NaN, the result is NaN.
        // If x is -0, the result is -0.
        // If x is +0, the result is +0.
        // If x is negative and not -0, the result is -1.
        // If x is positive and not +0, the result is +1.
        x = +x; // convert to a number
        if (x === 0 || isNaN(x)) {
            return Number(x);
        }
        return x > 0 ? 1 : -1;
    };
}

/*******************************************************************************
 * Line and Plane intersection methods
 ******************************************************************************/

/**
 * findLinePlaneIntersection Given points p {x: y: z:} and q that define a line,
 * and the plane of formula ax+by+cz+d = 0, returns the intersection point or
 * null if none.
 */
function findLinePlaneIntersection(p, q, a, b, c, d) {
    return findLinePlaneIntersectionCoords(p.x, p.y, p.z, q.x, q.y, q.z, a, b,
            c, d);
}

/**
 * findLineToHeightIntersection Given points p {x: y: z:} and q that define a
 * line, and a planeY value that defines a plane paralel to 'the floor' xz
 * plane, returns the intersection to that plane or null if none.
 */
function findLineToHeightIntersection(p, q, planeY) {
    return findLinePlaneIntersection(p, q, 0, 1, 0, -planeY);
}

/**
 * findLinePlaneIntersectionCoords (to avoid requiring unnecessary
 * instantiation) Given points p with px py pz and q that define a line, and the
 * plane of formula ax+by+cz+d = 0, returns the intersection point or null if
 * none.
 */
function findLinePlaneIntersectionCoords(px, py, pz, qx, qy, qz, a, b, c, d) {
    var tDenom = a * (qx - px) + b * (qy - py) + c * (qz - pz);
    if (tDenom == 0)
        return null;

    var t = -(a * px + b * py + c * pz + d) / tDenom;

    return {
        x : (px + t * (qx - px)),
        y : (py + t * (qy - py)),
        z : (pz + t * (qz - pz))
    };
}

/**
 * findLineToHeightIntersection Given points p with px py pz and q that define a
 * line, and a planeY value that defines a plane paralel to 'the floor' xz
 * plane, returns the intersection to that plane or null if none.
 */
function findLineToHeightIntersectionCoords(px, py, pz, qx, qy, qz, planeY) {
    return findLinePlaneIntersectionCoords(px, py, pz, qx, qy, qz, 0, 1, 0,
            -planeY);
}

function findRayIntersection(pickRay) {
    // Check 3D overlays and entities. Argument is an object with origin and
    // direction.
    var result = Overlays.findRayIntersection(pickRay);
    if (!result.intersects) {
        result = Entities.findRayIntersection(pickRay, true);
    }
    return result;
}

/**
 * Given a 2d point (x,y) this function returns the intersection (x, y, z) of
 * the computedPickRay for that point with the plane y = py
 */
function computePointAtPlaneY(x, y, py) {
    var ray = Camera.computePickRay(x, y);
    var p1 = ray.origin;
    var p2 = Vec3.sum(p1, Vec3.multiply(ray.direction, 1));
    return findLineToHeightIntersectionCoords(p1.x, p1.y, p1.z, p2.x, p2.y,
            p2.z, py);
}

/*******************************************************************************
 * 
 ******************************************************************************/

var touchStartingCoordinates = null;

var KEEP_PRESSED_FOR_TELEPORT_MODE_TIME = 750;
var touchBeginTime;

function touchBegin(event) {
    var coords = {
        x : event.x,
        y : event.y
    };
    touchStartingCoordinates = coords;
    touchBeginTime = Date.now();
}

var startedDraggingCamera = false; // first time
var draggingCamera = false; // is trying
var movingCamera = false; // definitive

var MIN_DRAG_DISTANCE_TO_CONSIDER = 100; // distance by axis, not real
                                            // distance

var prevTouchPinchRadius = null;

function pinchUpdate(event) {
    if (!event.isMoved)
        return;
    if (event.radius <= 0)
        return;

    // pinch management
    var avatarY = MyAvatar.position.y;
    var pinchIncrement;
    if (!!prevTouchPinchRadius) {
        // no prev value
        pinchIncrement = PINCH_INCREMENT
                * Math.abs(event.radius - prevTouchPinchRadius) * 0.1;
    } else {
        pinchIncrement = PINCH_INCREMENT_FIRST;
    }

    if (event.isPinching) {
        if (radarHeight + pinchIncrement > RADAR_HEIGHT_MAX_PLUS_AVATAR
                + avatarY) {
            radarHeight = RADAR_HEIGHT_MAX_PLUS_AVATAR + avatarY;
        } else {
            radarHeight += pinchIncrement;
        }
    } else if (event.isPinchOpening) {
        if (radarHeight - pinchIncrement < RADAR_HEIGHT_MIN_PLUS_AVATAR
                + avatarY) {
            radarHeight = RADAR_HEIGHT_MIN_PLUS_AVATAR + avatarY;
        } else {
            radarHeight -= pinchIncrement;
        }
    }
    var deltaHeight = avatarY + radarHeight - Camera.position.y;
    Camera.position = Vec3.sum(Camera.position, {
        x : 0,
        y : deltaHeight,
        z : 0
    });
    if (!draggingCamera) {
        startedDraggingCamera = true;
        draggingCamera = true;
    }

    prevTouchPinchRadius = event.radius;
}

function isInsideSquare(coords0, coords1, halfside) {
    return Math.abs(coords0.x - coords1.x) <= halfside
            && Math.abs(coords0.y - coords1.y) <= halfside;
}

function dragScrollUpdate(event) {
    if (!event.isMoved)
        return;

    // drag management
    var pickRay = Camera.computePickRay(event.x, event.y);
    var dragAt = Vec3.sum(pickRay.origin, Vec3.multiply(pickRay.direction,
            radarHeight));

    if (lastDragAt === undefined || lastDragAt === null) {
        lastDragAt = dragAt;
        return;
    }

    var deltaDrag = {
        x : (lastDragAt.x - dragAt.x),
        y : 0,
        z : (lastDragAt.z - dragAt.z)
    };

    lastDragAt = dragAt;
    if (lastDeltaDrag === undefined || lastDeltaDrag === null) {
        lastDeltaDrag = deltaDrag;
        return;
    }

    if (!draggingCamera) {
        startedDraggingCamera = true;
        draggingCamera = true;
    } else {
        if (!movingCamera) {
            if (!isInsideSquare(touchStartingCoordinates, event,
                    MIN_DRAG_DISTANCE_TO_CONSIDER)) {
                movingCamera = true;
            }
        }

        if (movingCamera) {
            if (Math.sign(deltaDrag.x) == Math.sign(lastDeltaDrag.x)
                    && Math.sign(deltaDrag.z) == Math.sign(lastDeltaDrag.z)) {
                // Process movement if direction of the movement is the same
                // than the previous frame
                // process delta
                var moveCameraTo = Vec3.sum(Camera.position, deltaDrag);
                // move camera
                Camera.position = moveCameraTo;
            } else {
                // Do not move camera if it's changing direction in this case,
                // wait until the next direction confirmation..
            }
            lastDeltaDrag = deltaDrag; // save last
        }
    }
}

/*******************************************************************************
 * Teleport feature
 ******************************************************************************/

function Teleporter() {

    var SURFACE_DETECTION_FOR_TELEPORT = true; // true if uses teleport.js
                                                // similar logic to detect
                                                // surfaces. false if uses plain
                                                // teleport to avatar same
                                                // height.

    var TELEPORT_TARGET_MODEL_URL = Script
            .resolvePath("../assets/models/teleport-destination.fbx");
    var TELEPORT_TOO_CLOSE_MODEL_URL = Script
            .resolvePath("../assets/models/teleport-cancel.fbx");

    var TELEPORT_MODEL_DEFAULT_DIMENSIONS = {
        x : 0.10,
        y : 0.00001,
        z : 0.10
    };

    var teleportOverlay = Overlays.addOverlay("model", {
        url : TELEPORT_TARGET_MODEL_URL,
        dimensions : TELEPORT_MODEL_DEFAULT_DIMENSIONS,
        orientation : Quat.fromPitchYawRollDegrees(0, 180, 0),
        visible : false
    });

    var teleportCancelOverlay = Overlays.addOverlay("model", {
        url : TELEPORT_TOO_CLOSE_MODEL_URL,
        dimensions : TELEPORT_MODEL_DEFAULT_DIMENSIONS,
        orientation : Quat.fromPitchYawRollDegrees(0, 180, 0),
        visible : false
    });

    var TELEPORT_COLOR = {
        red : 0,
        green : 255,
        blue : 255
    };
    var TELEPORT_CANCEL_COLOR = {
        red : 255,
        green : 255,
        blue : 0
    };

    var teleportLine = Overlays.addOverlay("line3d", {
        start : {
            x : 0,
            y : 0,
            z : 0
        },
        end : {
            x : 0,
            y : 0,
            z : 0
        },
        color : TELEPORT_COLOR,
        alpha : 1,
        lineWidth : 2,
        dashed : false,
        visible : false
    });

    // code from teleport.js
    var TELEPORT_TARGET = {
        NONE : 'none', // Not currently targetting anything
        INVISIBLE : 'invisible', // The current target is an invvsible
                                    // surface
        INVALID : 'invalid', // The current target is invalid (wall, ceiling,
                                // etc.)
        SURFACE : 'surface', // The current target is a valid surface
        SEAT : 'seat', // The current target is a seat
    }

    var TELEPORT_CANCEL_RANGE = 1;
    var teleportTargetType = TELEPORT_TARGET.NONE;

    function parseJSON(json) {
        try {
            return JSON.parse(json);
        } catch (e) {
            return undefined;
        }
    }

    /*
     * Enhanced with intersection with terrain instead of using current avatar y
     * position if SURFACE_DETECTION_FOR_TELEPORT is true
     */
    function computeDestination(touchEventPos, avatarPosition, cameraPosition,
            radarH) {
        if (SURFACE_DETECTION_FOR_TELEPORT) {
            var pickRay = Camera.computePickRay(touchEventPos.x,
                    touchEventPos.y);
            printd("newTeleportDetect - pickRay " + JSON.stringify(pickRay));
            var destination = Entities.findRayIntersection(pickRay, true, [],
                    [], false, true);
            printd("newTeleportDetect - destination "
                    + JSON.stringify(destination));
            return destination;
        } else {
            var pickRay = Camera.computePickRay(touchEventPos.x,
                    touchEventPos.y);
            var pointingAt = Vec3.sum(pickRay.origin, Vec3.multiply(
                    pickRay.direction, radarH));
            var destination = {
                x : pointingAt.x,
                y : avatarPosition.y,
                z : pointingAt.z
            };
            return destination;
        }
    }

    function renderTeleportOverlays(destination) {
        var overlayPosition = findLineToHeightIntersection(destination,
                Camera.position, Camera.position.y
                        - RADAR_CAMERA_DISTANCE_TO_ICONS);
        printd("[newTeleport] TELEPORT ! render overlay at "
                + JSON.stringify(overlayPosition));

        // CLD note Oct 11, 2017
        // Version of teleport.js 3c109f294f88ba7573bd1221f907f2605893c509
        // doesn't allow invisible surfaces, let's allow it for now
        if (teleportTargetType == TELEPORT_TARGET.SURFACE
                || teleportTargetType == TELEPORT_TARGET.INVISIBLE) {
            Overlays.editOverlay(teleportOverlay, {
                visible : true,
                position : overlayPosition
            });
            Overlays.editOverlay(teleportCancelOverlay, {
                visible : false
            });
            Overlays.editOverlay(teleportLine, {
                start : MyAvatar.position,
                end : destination,
                color : TELEPORT_COLOR,
                visible : true
            });
        } else if (teleportTargetType == TELEPORT_TARGET.INVALID) {
            Overlays.editOverlay(teleportOverlay, {
                visible : false
            });
            Overlays.editOverlay(teleportCancelOverlay, {
                visible : true,
                position : overlayPosition
            });
            Overlays.editOverlay(teleportLine, {
                start : MyAvatar.position,
                end : destination,
                color : TELEPORT_CANCEL_COLOR,
                visible : true
            });
        } else { // TELEPORT_TARGET:NONE?
            Overlays.editOverlay(teleportOverlay, {
                visible : false
            });
            Overlays.editOverlay(teleportCancelOverlay, {
                visible : false
            });
            Overlays.editOverlay(teleportLine, {
                visible : false
            });
        }
    }

    var BORDER_DISTANCE_PX = 100;
    var border_top = 0;
    var border_left = 0;
    var border_right = Window.innerWidth;
    var border_bottom = Window.innerHeight;

    function moveOnBorders(event) {
        var xDelta = 0;
        var zDelta = 0;

        if (event.y <= border_top + BORDER_DISTANCE_PX) {
            zDelta = -0.1;
        } else if (event.y >= border_bottom - BORDER_DISTANCE_PX) {
            zDelta = 0.1;
        }
        if (event.x <= border_left + BORDER_DISTANCE_PX) {
            xDelta = -0.1;
        } else if (event.x >= border_right - BORDER_DISTANCE_PX) {
            xDelta = 0.1;
        }
        if (xDelta == 0 && zDelta == 0) {
            draggingCamera = false;
            return;
        }

        Camera.position = Vec3.sum(Camera.position, {
            x : xDelta,
            y : 0,
            z : zDelta
        });
        draggingCamera = true;
    }

    // When determininig whether you can teleport to a location, the normal of
    // the
    // point that is being intersected with is looked at. If this normal is more
    // than MAX_ANGLE_FROM_UP_TO_TELEPORT degrees from <0, 1, 0> (straight up),
    // then
    // you can't teleport there.
    const MAX_ANGLE_FROM_UP_TO_TELEPORT = 70;
    function getTeleportTargetType(intersection) {
        if (SURFACE_DETECTION_FOR_TELEPORT) {
            if (!intersection.intersects) {
                return TELEPORT_TARGET.NONE;
            }
            var props = Entities.getEntityProperties(intersection.entityID, [
                    'userData', 'visible' ]);
            var data = parseJSON(props.userData);
            if (data !== undefined && data.seat !== undefined) {
                return TELEPORT_TARGET.SEAT;
            }

            if (!props.visible) {
                return TELEPORT_TARGET.INVISIBLE;
            }

            var surfaceNormal = intersection.surfaceNormal;
            var adj = Math.sqrt(surfaceNormal.x * surfaceNormal.x
                    + surfaceNormal.z * surfaceNormal.z);
            var angleUp = Math.atan2(surfaceNormal.y, adj) * (180 / Math.PI);

            if (angleUp < (90 - MAX_ANGLE_FROM_UP_TO_TELEPORT)
                    || angleUp > (90 + MAX_ANGLE_FROM_UP_TO_TELEPORT)
                    || Vec3.distance(MyAvatar.position,
                            intersection.intersection) <= TELEPORT_CANCEL_RANGE) {
                return TELEPORT_TARGET.INVALID;
            } else {
                return TELEPORT_TARGET.SURFACE;
            }
        } else {
            var destination = intersection;
            if (Vec3.distance(MyAvatar.position, destination) <= TELEPORT_CANCEL_RANGE) {
                return TELEPORT_TARGET.INVALID;
            } else {
                return TELEPORT_TARGET.SURFACE;
            }
        }
    }
    ;

    function moveToFromEvent(event) {
        var destination = computeDestination(event, MyAvatar.position,
                Camera.position, radarHeight);
        moveTo(SURFACE_DETECTION_FOR_TELEPORT ? Vec3.sum(
                destination.intersection, {
                    y : 1
                }) : destination);
        return true;
    }

    return {
        dragTeleportBegin : function(event) {
            printd("[newTeleport] TELEPORT began");
            var overlayDimensions = entityIconModelDimensions();
            // var destination = computeDestination(event, MyAvatar.position,
            // Camera.position, radarHeight);
            // Dimension teleport and cancel overlays (not show them yet)
            Overlays.editOverlay(teleportOverlay, {
                dimensions : overlayDimensions
            });
            Overlays.editOverlay(teleportCancelOverlay, {
                dimensions : overlayDimensions
            });
            // Position line
            Overlays.editOverlay(teleportLine, {
                visible : true,
                start : 0,
                end : 0
            });
        },

        dragTeleportUpdate : function(event) {
            // if in border, move camera
            moveOnBorders(event);

            var destination = computeDestination(event, MyAvatar.position,
                    Camera.position, radarHeight);

            teleportTargetType = getTeleportTargetType(destination);
            renderTeleportOverlays(SURFACE_DETECTION_FOR_TELEPORT ? destination.intersection
                    : destination);
        },

        dragTeleportRelease : function(event) {
            printd("[newTeleport] TELEPORT released at "
                    + JSON.stringify(event));
            // CLD note Oct 11, 2017
            // Version of teleport.js 3c109f294f88ba7573bd1221f907f2605893c509
            // doesn't allow invisible surfaces, let's allow it for now
            if (teleportTargetType == TELEPORT_TARGET.SURFACE
                    || teleportTargetType == TELEPORT_TARGET.INVISIBLE) {
                moveToFromEvent(event);
            }
            teleportTargetType = TELEPORT_TARGET.NONE;

            Overlays.editOverlay(teleportOverlay, {
                visible : false
            });
            Overlays.editOverlay(teleportLine, {
                visible : false
            });
            Overlays.editOverlay(teleportCancelOverlay, {
                visible : false
            });
        }
    };

}

var teleporter = Teleporter();

/*******************************************************************************
 * 
 ******************************************************************************/

var dragModeFunc = null; // by default is nothing

function oneFingerTouchUpdate(event) {
    if (dragModeFunc) {
        dragModeFunc(event);
    } else {
        if (!isInsideSquare(touchStartingCoordinates, event,
                MIN_DRAG_DISTANCE_TO_CONSIDER)) {
            dragModeFunc = dragScrollUpdate;
            dragModeFunc(event);
        } else {
            var now = Date.now(); // check time
            if (now - touchBeginTime >= KEEP_PRESSED_FOR_TELEPORT_MODE_TIME) {
                teleporter.dragTeleportBegin(event);
                dragModeFunc = teleporter.dragTeleportUpdate;
                dragModeFunc(event);
            } else {
                // not defined yet, let's wait for time or movement to happen
            }
        }
    }
}

function touchUpdate(event) {
    if (event.isPinching || event.isPinchOpening) {
        pinchUpdate(event);
    } else {
        oneFingerTouchUpdate(event);
    }
}

/*******************************************************************************
 * Avatar cache structure for showing avatars markers
 ******************************************************************************/

// by QUuid
var avatarsData = {};
var avatarsIcons = []; // a parallel list of icons (overlays) to easily run
                        // through
var avatarsNames = []; // a parallel list of names (overlays) to easily run
                        // through

function getAvatarIconForUser(uid) {
    var color = uniqueColor.getColor(uid);
    if (color.charAt(0) == '#') {
        color = color.substring(1, color.length);
    }
    // FIXME: this is a temporary solution until we can use circle3d with
    // lineWidth
    return Script.resolvePath("assets/images/circle-" + color + ".svg");
}

var avatarIconDimensionsVal = {
    x : 0,
    y : 0,
    z : 0.00001
};
function avatarIconPlaneDimensions() {
    // given the current height, give a size
    var xy = -0.003531 * radarHeight + 0.1;
    avatarIconDimensionsVal.x = Math.abs(xy);
    avatarIconDimensionsVal.y = Math.abs(xy);
    // reuse object
    return avatarIconDimensionsVal;
}

function currentOverlayIconForAvatar(QUuid) {
    if (avatarsData[QUuid] != undefined) {
        return avatarsData[QUuid].icon;
    } else {
        return null;
    }
}

function currentOverlayNameForAvatar(QUuid) {
    if (avatarsData[QUuid] != undefined) {
        return avatarsData[QUuid].name;
    } else {
        return null;
    }
}

function saveAvatarData(QUuid) {
    if (QUuid == null)
        return;
    var avat = AvatarList.getAvatar(QUuid);
    printd("avatar added save avatar " + QUuid);

    if (!avat)
        return;

    if (avatarsData[QUuid] != undefined) {
        avatarsData[QUuid].position = avat.position;
    } else {
        var avatarIcon = Overlays.addOverlay("circle3d", {
            color: uniqueColor.convertHexToRGB(uniqueColor.getColor(QUuid)),
            dimensions: ICON_ENTITY_DEFAULT_DIMENSIONS,
            rotation: Quat.fromPitchYawRollDegrees(90, 0, 0),
            innerRadius: 1.8,
            outerRadius: 2,
            isSolid: true,
            visible: false
        });

        var needRefresh = !avat || !avat.displayName;
        var displayName = avat && avat.displayName ? avat.displayName
                : "Unknown";
        var textWidth = displayName.length * AVATAR_DISPLAY_NAME_CHAR_WIDTH;
        var avatarName = Overlays.addOverlay("text", {
            width: textWidth,
            height: AVATAR_DISPLAY_NAME_HEIGHT,
            color: { red: 255, green: 255, blue: 255},
            backgroundAlpha: 0.0,
            textRaiseColor: { red: 0, green: 0, blue: 0},
            font: {size: AVATAR_DISPLAY_NAME_FONT_SIZE, bold: true},
            visible: false,
            text: displayName,
            textAlignCenter: true
        });
        avatarsIcons.push(avatarIcon);
        avatarsNames.push(avatarName);
        avatarsData[QUuid] = {
            position : avat.position,
            icon : avatarIcon,
            name : avatarName,
            textWidth : textWidth,
            needRefresh : needRefresh
        };
    }
}

function removeAvatarData(QUuid) {
    if (QUuid == null)
        return;

    var itsOverlay = currentOverlayIconForAvatar(QUuid);
    if (itsOverlay != null) {
        Overlays.deleteOverlay(itsOverlay);
    }
    var itsNameOverlay = currentOverlayNameForAvatar(QUuid);
    if (itsNameOverlay != null) {
        Overlays.deleteOverlay(itsNameOverlay);
    }

    var idx = avatarsIcons.indexOf(itsOverlay);
    avatarsIcons.splice(idx, 1);
    idx = avatarsNames.indexOf(itsNameOverlay);
    avatarsNames.splice(idx, 1);

    delete avatarsData[QUuid];
}

function saveAllOthersAvatarsData() {
    var avatarIds = AvatarList.getAvatarIdentifiers();
    var len = avatarIds.length;
    for (var i = 0; i < len; i++) {
        if (avatarIds[i]) {
            saveAvatarData(avatarIds[i]);
        }
    }
}

function avatarAdded(QUuid) {
    printd("avatar added " + QUuid);// + " at " +
                                    // JSON.stringify(AvatarList.getAvatar(QUuid).position));
    saveAvatarData(QUuid);
}

function avatarRemoved(QUuid) {
    printd("avatar removed " + QUuid);
    removeAvatarData(QUuid);
}

/*******************************************************************************
 * Avatar Icon/Markers rendering
 ******************************************************************************/
var myAvatarIcon;
var myAvatarName;
function distanceForCameraHeight(h) {
    if (h < 30) return 1;
    if (h < 40) return 2;
    if (h < 50) return 2.5;
    return 5;
}
function renderMyAvatarIcon() {
    var commonY = Camera.position.y - distanceForCameraHeight(Camera.position.y);
    var iconPos = findLineToHeightIntersectionCoords(   MyAvatar.position.x,
                                                        MyAvatar.position.y + RADAR_ICONS_APPARENT_DISTANCE_TO_AVATAR_BASE,
                                                        MyAvatar.position.z,
                                                        Camera.position.x, Camera.position.y, Camera.position.z,
                                                        commonY);
    if (!iconPos) { printd("avatarmy icon pos null"); return;}
    var iconDimensions = avatarIconPlaneDimensions();

    var avatarPos = MyAvatar.position;
    var cameraPos = Camera.position;
    var borderPoints = [
            computePointAtPlaneY(0, 0, commonY),
            computePointAtPlaneY(Window.innerWidth, Window.innerHeight, commonY) ];

    var p1 = findLineToHeightIntersectionCoords(avatarPos.x, avatarPos.y,
            avatarPos.z, cameraPos.x, cameraPos.y, cameraPos.z, commonY);
    var x = (p1.x - borderPoints[0].x) * (Window.innerWidth)
            / (borderPoints[1].x - borderPoints[0].x);
    var y = (p1.z - borderPoints[0].z) * (Window.innerHeight)
            / (borderPoints[1].z - borderPoints[0].z);

    if (!myAvatarIcon && MyAvatar.SELF_ID) {
       myAvatarIcon =  Overlays.addOverlay("circle3d", {
            color: uniqueColor.convertHexToRGB(uniqueColor.getColor(MyAvatar.SELF_ID)),
            dimensions: ICON_ENTITY_DEFAULT_DIMENSIONS,
            rotation: Quat.fromPitchYawRollDegrees(90, 0, 0),
            innerRadius: 1.8,
            outerRadius: 2,
            isSolid: true,
            visible: false
        });
    }

    if (!myAvatarName) {
        myAvatarName = Overlays.addOverlay("text", {
                        width: 100,
                        height: AVATAR_DISPLAY_NAME_HEIGHT,
                        textAlignCenter: true,
                        color: { red: 255, green: 255, blue: 255},
                        backgroundAlpha: 0.0,
                        font: {size: AVATAR_DISPLAY_NAME_FONT_SIZE, bold: true},
                        textRaiseColor: { red: 0, green: 0, blue: 0},
                        visible: false,
                        text: "Me"
                       });
    }

    if (myAvatarIcon) {
        Overlays.editOverlay(myAvatarIcon, {
            visible : true,
            dimensions : iconDimensions,
            position : iconPos
        });

    }
    var textSize = (14 + (iconDimensions.y - 0.03) * 15 / 0.06);

    Overlays.editOverlay(myAvatarName, {
        visible : true,
        x : x - 18 + (iconDimensions.y - 0.03) * 2 / 0.06,
        y : y + iconDimensions.y * 550,
        font : {
            size : textSize,
            bold : true
        },
    });

}

function hideAllAvatarIcons() {
    var len = avatarsIcons.length;
    for (var i = 0; i < len; i++) {
        Overlays.editOverlay(avatarsIcons[i], {
            visible : false
        });
    }
    len = avatarsNames.length;
    for (var j = 0; j < len; j++) {
        Overlays.editOverlay(avatarsNames[j], {
            visible : false
        });
    }
    if (myAvatarIcon) {
        Overlays.editOverlay(myAvatarIcon, {
            visible : false
        });
    }
    Overlays.editOverlay(myAvatarName, {
        visible : false
    })
}

function renderAllOthersAvatarIcons() {
    var avatarPos;
    var iconDimensions = avatarIconPlaneDimensions();
    var commonY = Camera.position.y - distanceForCameraHeight(Camera.position.y);
    var borderPoints = [
            computePointAtPlaneY(0, 0, commonY),
            computePointAtPlaneY(Window.innerWidth, Window.innerHeight, commonY) ];

    for ( var QUuid in avatarsData) {
        if (avatarsData.hasOwnProperty(QUuid)) {
            if (AvatarList.getAvatar(QUuid) != null) {
                avatarPos = AvatarList.getAvatar(QUuid).position;

                var cameraPos = Camera.position;
                var p1 = findLineToHeightIntersectionCoords(avatarPos.x, avatarPos.y, avatarPos.z, 
                                                    cameraPos.x, cameraPos.y, cameraPos.z,
                                                    commonY);

                var x = (p1.x - borderPoints[0].x) * (Window.innerWidth) / (borderPoints[1].x - borderPoints[0].x);
                var y = (p1.z - borderPoints[0].z) * (Window.innerHeight) / (borderPoints[1].z - borderPoints[0].z);

                if (avatarsData[QUuid].icon != undefined) {
                    var iconPos = findLineToHeightIntersectionCoords(   avatarPos.x, avatarPos.y + RADAR_ICONS_APPARENT_DISTANCE_TO_AVATAR_BASE, avatarPos.z,
                                                                        Camera.position.x, Camera.position.y, Camera.position.z,
                                                                        commonY);
                    if (!iconPos) { print ("avatar icon pos bad for " + QUuid); continue; }
                    if (avatarsData[QUuid].needRefresh) {
                        var avat = AvatarList.getAvatar(QUuid);
                        if (avat && avat.displayName) {
                            Overlays.editOverlay(avatarsData[QUuid].name, {
                                width : avat.displayName.length
                                        * AVATAR_DISPLAY_NAME_CHAR_WIDTH,
                                text : avat.displayName,
                                textAlignCenter : true
                            });
                            avatarsData[QUuid].needRefresh = false;
                        }
                    }
                    var textSize = (14 + (iconDimensions.y - 0.03) * 15 / 0.06);
                    Overlays.editOverlay(avatarsData[QUuid].icon, {
                        visible : true,
                        dimensions : iconDimensions,
                        position : iconPos
                    });
                    Overlays.editOverlay(avatarsData[QUuid].name, {
                        visible : true,
                        x : x - avatarsData[QUuid].textWidth * 0.5,
                        y : y + iconDimensions.y * 550,
                        font : {
                            size : textSize,
                            bold : true
                        }
                    });
                }
            }
        }
    }
}

function entityAdded(entityID) {
    printd("Entity added " + entityID);
    var props = Entities.getEntityProperties(entityID, [ "type" ]);
    printd("Entity added " + entityID + " PROPS " + JSON.stringify(props));
    if (props && props.type == "Web") {
        printd("Entity Web added " + entityID);
        saveEntityData(entityID, true);
    }
}

function entityRemoved(entityID) {
    printd("Entity removed " + entityID);
    var props = Entities.getEntityProperties(entityID, [ "type" ]);
    if (props && props.type == "Web") {
        print("Entity Web removed " + entityID);
        removeEntityData(entityID);
    }
}

/*******************************************************************************
 * Entities (to remark) cache structure for showing entities markers
 ******************************************************************************/

var entitiesData = {}; // by entityID
var entitiesByOverlayID = {}; // by overlayID
var entitiesIcons = []; // a parallel list of icons (overlays) to easily run
                        // through

var ICON_ENTITY_WEB_MODEL_URL = Script.resolvePath("../assets/images/web.svg");
var ICON_ENTITY_IMG_MODEL_URL = Script
        .resolvePath("../assets/models/teleport-cancel.fbx"); // FIXME - use
                                                                // correct
                                                                // model&texture
var ICON_ENTITY_DEFAULT_DIMENSIONS = {
    x : 0.10,
    y : 0.00001,
    z : 0.10
};

var entityIconModelDimensionsVal = {
    x : 0,
    y : 0.00001,
    z : 0
};
function entityIconModelDimensions() {
    // given the current height, give a size
    var xz = -0.002831 * radarHeight + 0.1;
    entityIconModelDimensionsVal.x = xz;
    entityIconModelDimensionsVal.z = xz;
    // reuse object
    return entityIconModelDimensionsVal;
}
/*
 * entityIconPlaneDimensions: similar to entityIconModelDimensions but using xy
 * plane
 */
function entityIconPlaneDimensions() {
    var dim = entityIconModelDimensions();
    var z = dim.z;
    dim.z = dim.y;
    dim.y = z;
    return dim;
}

function currentOverlayForEntity(QUuid) {
    if (entitiesData[QUuid] != undefined) {
        return entitiesData[QUuid].icon;
    } else {
        return null;
    }
}

function saveEntityData(QUuid, planar) {
    if (QUuid == null)
        return;
    var entity = Entities.getEntityProperties(QUuid, [ "position" ]);
    printd("entity added save entity " + QUuid);
    if (entitiesData[QUuid] != undefined) {
        entitiesData[QUuid].position = entity.position;
    } else {
        var entityIcon = Overlays.addOverlay("image3d", {
            subImage : {
                x : 0,
                y : 0,
                width : 150,
                height : 150
            },
            url : ICON_ENTITY_WEB_MODEL_URL,
            dimensions : ICON_ENTITY_DEFAULT_DIMENSIONS,
            visible : false,
            ignoreRayIntersection : false,
            orientation : Quat.fromPitchYawRollDegrees(-90, 0, 0)
        });

        entitiesIcons.push(entityIcon);
        entitiesData[QUuid] = {
            position : entity.position,
            icon : entityIcon
        };
        entitiesByOverlayID[entityIcon] = QUuid;
    }
}

function removeEntityData(QUuid) {
    if (QUuid == null)
        return;

    var itsOverlay = currentOverlayForEntity(QUuid);
    if (itsOverlay != null) {
        Overlays.deleteOverlay(itsOverlay);
        delete entitiesByOverlayID[itsOverlay];
    }
    var idx = entitiesIcons.indexOf(itsOverlay);
    entitiesIcons.splice(idx, 1);

    delete entitiesData[QUuid];
}

/*******************************************************************************
 * Entities to remark Icon/Markers rendering
 ******************************************************************************/

function hideAllEntitiesIcons() {
    var len = entitiesIcons.length;
    for (var i = 0; i < len; i++) {
        Overlays.editOverlay(entitiesIcons[i], {
            visible : false
        });
    }
}

function renderAllEntitiesIcons() {
    var entityPos;
    var entityProps;
    var iconDimensions = entityIconModelDimensions();
    var planeDimensions = entityIconPlaneDimensions(); // plane overlays uses
                                                        // xy instead of xz
    for ( var QUuid in entitiesData) {
        if (entitiesData.hasOwnProperty(QUuid)) {
            entityProps = Entities.getEntityProperties(QUuid, [ "position",
                    "visible" ]);
            if (entityProps != null) {
                entityPos = entityProps.position;
                if (entitiesData[QUuid].icon != undefined && entityPos) {
                    var iconPos = findLineToHeightIntersectionCoords(
                            entityPos.x,
                            entityPos.y
                                    + RADAR_ICONS_APPARENT_DISTANCE_TO_AVATAR_BASE,
                            entityPos.z, Camera.position.x, Camera.position.y,
                            Camera.position.z, Camera.position.y
                                    - RADAR_CAMERA_DISTANCE_TO_ICONS);
                    if (!iconPos) {
                        printd("entity icon pos bad for " + QUuid);
                        continue;
                    }
                    var dimensions = entitiesData[QUuid].planar ? planeDimensions
                            : iconDimensions;
                    Overlays.editOverlay(entitiesData[QUuid].icon, {
                        visible : entityProps.visible,
                        dimensions : dimensions,
                        position : iconPos
                    });
                }
            }
        }
    }
}

/*******************************************************************************
 * 
 ******************************************************************************/

function startRadar() {
    printd("avatar added my avatar is  " + MyAvatar.sessionUUID);
    saveAllOthersAvatarsData();
    Camera.mode = "independent";

    Camera.position = Vec3.sum(MyAvatar.position, {
        x : 0,
        y : radarHeight,
        z : 0
    });
    Camera.orientation = Quat.fromPitchYawRollDegrees(-90, 0, 0);
    radar = true;

    Controller.setVPadEnabled(false); // this was done before in CompositeExtra in the DisplayPlugin (Checking for camera not independent, not radar mode)

    connectRadarModeEvents();
}

function endRadar() {
    printd("-- endRadar");
    Camera.mode = "first person";
    radar = false;

    Controller.setVPadEnabled(true);

    disconnectRadarModeEvents();
    hideAllEntitiesIcons();
    hideAllAvatarIcons();
}

function onRadarModeClicked() {
    startRadar();
}

function onMyViewModeClicked() {
    endRadar();
}

radarModeInterface.startRadarMode = function() {
    startRadar();
};

radarModeInterface.endRadarMode = function() {
    endRadar();
};

radarModeInterface.init = function() {
    init();
}

radarModeInterface.setUniqueColor = function(c) {
    uniqueColor = c;
};

module.exports = radarModeInterface;

function updateRadar() {
    // Update avatar icons
    if (startedDraggingCamera) {
        hideAllAvatarIcons();
        hideAllEntitiesIcons();
        startedDraggingCamera = false;
    } else if (!draggingCamera) {
        renderMyAvatarIcon();
        renderAllOthersAvatarIcons();
        renderAllEntitiesIcons();
    }
}

function valueIfDefined(value) {
    return value !== undefined ? value : "";
}

function entitiesAnalysis() {
    var ids = Entities.findEntitiesInFrustum(Camera.frustum);
    var entities = [];
    for (var i = 0; i < ids.length; i++) {
        var id = ids[i];
        var properties = Entities.getEntityProperties(id);
        entities.push({
            id : id,
            name : properties.name,
            type : properties.type,
            url : properties.type == "Model" ? properties.modelURL : "",
            sourceUrl : properties.sourceUrl,
            locked : properties.locked,
            visible : properties.visible,
            drawCalls : valueIfDefined(properties.renderInfo.drawCalls),
            hasScript : properties.script !== ""
        });
    }
}

function connectRadarModeEvents() {
    Script.update.connect(updateRadar); // 60Hz loop
    Controller.keyPressEvent.connect(keyPressEvent);
    Controller.mousePressEvent.connect(mousePress); // single click/touch
    Controller.touchUpdateEvent.connect(touchUpdate);
    MyAvatar.positionGoneTo.connect(positionGoneTo);
}

function positionGoneTo() {
    Camera.position = Vec3.sum(MyAvatar.position, {
        x : 0,
        y : radarHeight,
        z : 0
    });
}

function disconnectRadarModeEvents() {
    Script.update.disconnect(updateRadar);
    Controller.keyPressEvent.disconnect(keyPressEvent);
    Controller.mousePressEvent.disconnect(mousePress);
    Controller.touchUpdateEvent.disconnect(touchUpdate);
    MyAvatar.positionGoneTo.disconnect(positionGoneTo);
}

function init() {
    tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

    Controller.touchBeginEvent.connect(touchBegin);
    Controller.touchEndEvent.connect(touchEnd);

    AvatarList.avatarAddedEvent.connect(avatarAdded);
    AvatarList.avatarRemovedEvent.connect(avatarRemoved);

    Entities.addingEntity.connect(entityAdded);
    Entities.deletingEntity.connect(entityRemoved);
}
