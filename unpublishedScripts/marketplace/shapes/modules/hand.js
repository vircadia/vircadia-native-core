//
//  hand.js
//
//  Created by David Rowe on 21 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Hand:true */

Hand = function (side) {

    "use strict";

    // Hand controller input.
    var handController,
        controllerTrigger,
        controllerTriggerClicked,
        controllerGrip,

        isGripClicked = false,
        isGripClickedHandled = false,
        GRIP_ON_VALUE = 0.99,
        GRIP_OFF_VALUE = 0.95,

        isTriggerPressed,
        isTriggerClicked,
        TRIGGER_ON_VALUE = 0.15, // Per controllerDispatcherUtils.js.
        TRIGGER_OFF_VALUE = 0.1, // Per controllerDispatcherUtils.js.
        TRIGGER_CLICKED_VALUE = 1.0,

        NEAR_GRAB_RADIUS = 0.05, // Different from controllerDispatcherUtils.js.
        NEAR_HOVER_RADIUS = 0.025,

        LEFT_HAND = 0,
        HALF_TREE_SCALE = 16384,

        handPose,
        handPosition,
        handOrientation,
        palmPosition,

        handleOverlayIDs = [],
        intersection = {};

    if (!(this instanceof Hand)) {
        return new Hand(side);
    }

    if (side === LEFT_HAND) {
        handController = Controller.Standard.LeftHand;
        controllerTrigger = Controller.Standard.LT;
        controllerTriggerClicked = Controller.Standard.LTClick;
        controllerGrip = Controller.Standard.LeftGrip;
    } else {
        handController = Controller.Standard.RightHand;
        controllerTrigger = Controller.Standard.RT;
        controllerTriggerClicked = Controller.Standard.RTClick;
        controllerGrip = Controller.Standard.RightGrip;
    }

    function setHandleOverlays(overlayIDs) {
        handleOverlayIDs = overlayIDs;
    }

    function valid() {
        return handPose.valid;
    }

    function position() {
        return handPosition;
    }

    function orientation() {
        return handOrientation;
    }

    function getPalmPosition() {
        return palmPosition;
    }

    function triggerPressed() {
        return isTriggerPressed;
    }

    function triggerClicked() {
        return isTriggerClicked;
    }

    function gripClicked() {
        return isGripClicked;
    }

    function setGripClickedHandled() {
        isGripClicked = false;
        isGripClickedHandled = true;
    }

    function getIntersection() {
        return intersection;
    }

    function getNearGrabRadius() {
        return NEAR_GRAB_RADIUS;
    }

    function update() {
        var gripValue,
            overlayID,
            overlayIDs,
            overlayDistance,
            intersectionPosition,
            distance,
            entityID,
            entityIDs,
            entitySize,
            size,
            i,
            length;


        // Hand pose.
        handPose = Controller.getPoseValue(handController);
        if (!handPose.valid) {
            intersection = {};
            return;
        }
        handPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, handPose.translation), MyAvatar.position);
        handOrientation = Quat.multiply(MyAvatar.orientation, handPose.rotation);

        // Controller trigger.
        isTriggerPressed = Controller.getValue(controllerTrigger) > (isTriggerPressed
            ? TRIGGER_OFF_VALUE : TRIGGER_ON_VALUE);
        isTriggerClicked = Controller.getValue(controllerTriggerClicked) === TRIGGER_CLICKED_VALUE;

        // Controller grip.
        gripValue = Controller.getValue(controllerGrip);
        if (isGripClicked) {
            isGripClicked = gripValue > GRIP_OFF_VALUE;
        } else {
            isGripClicked = gripValue > GRIP_ON_VALUE;
        }
        // Grip clicked may be being handled by UI.
        if (isGripClicked) {
            isGripClicked = !isGripClickedHandled;
        } else {
            isGripClickedHandled = false;
        }

        // Hand-overlay intersection, if any handle overlays.
        overlayID = null;
        palmPosition = side === LEFT_HAND ? MyAvatar.getLeftPalmPosition() : MyAvatar.getRightPalmPosition();
        if (handleOverlayIDs.length > 0) {
            overlayIDs = Overlays.findOverlays(palmPosition, NEAR_HOVER_RADIUS);
            if (overlayIDs.length > 0) {
                // Typically, there will be only one overlay; optimize for that case.
                overlayID = overlayIDs[0];
                if (overlayIDs.length > 1) {
                    // Find closest overlay.
                    overlayDistance = Vec3.length(Vec3.subtract(Overlays.getProperty(overlayID, "position"), palmPosition));
                    for (i = 1, length = overlayIDs.length; i < length; i++) {
                        distance =
                            Vec3.length(Vec3.subtract(Overlays.getProperty(overlayIDs[i], "position"), palmPosition));
                        if (distance > overlayDistance) {
                            overlayID = overlayIDs[i];
                            overlayDistance = distance;
                        }
                    }
                }
            }
            if (overlayID && handleOverlayIDs.indexOf(overlayID) === -1) {
                overlayID = null;
            }
            if (overlayID) {
                intersectionPosition = Overlays.getProperty(overlayID, "position");
            }
        }

        // Hand-entity intersection, if any editable, if overlay not intersected.
        entityID = null;
        if (overlayID === null) {
            // palmPosition is set above.
            entityIDs = Entities.findEntities(palmPosition, NEAR_GRAB_RADIUS);
            if (entityIDs.length > 0) {
                // Typically, there will be only one entity; optimize for that case.
                if (Entities.hasEditableRoot(entityIDs[0])) {
                    entityID = entityIDs[0];
                }
                if (entityIDs.length > 1) {
                    // Find smallest, editable entity.
                    entitySize = HALF_TREE_SCALE;
                    for (i = 0, length = entityIDs.length; i < length; i++) {
                        if (Entities.hasEditableRoot(entityIDs[i])) {
                            size = Vec3.length(Entities.getEntityProperties(entityIDs[i], "dimensions").dimensions);
                            if (size < entitySize) {
                                entityID = entityIDs[i];
                                entitySize = size;
                            }
                        }
                    }
                }
            }
            if (entityID) {
                intersectionPosition = Entities.getEntityProperties(entityID, "position").position;
                if (Vec3.distance(palmPosition, intersectionPosition) > NEAR_GRAB_RADIUS) {
                    intersectionPosition = Vec3.sum(palmPosition,
                        Vec3.multiply(NEAR_GRAB_RADIUS, Vec3.normalize(Vec3.subtract(intersectionPosition, palmPosition))));
                }
            }
        }

        intersection = {
            intersects: overlayID !== null || entityID !== null,
            overlayID: overlayID,
            entityID: entityID,
            handIntersected: overlayID !== null || entityID !== null,
            editableEntity: entityID !== null,
            intersection: intersectionPosition
        };
    }

    function clear() {
        // Nothing to do.
    }

    function destroy() {
        // Nothing to do.
    }

    return {
        setHandleOverlays: setHandleOverlays,
        valid: valid,
        position: position,
        orientation: orientation,
        palmPosition: getPalmPosition,
        triggerPressed: triggerPressed,
        triggerClicked: triggerClicked,
        gripClicked: gripClicked,
        setGripClickedHandled: setGripClickedHandled,
        intersection: getIntersection,
        getNearGrabRadius: getNearGrabRadius,
        update: update,
        clear: clear,
        destroy: destroy
    };
};

Hand.prototype = {};
