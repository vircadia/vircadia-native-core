//
//  toolMenu.js
//
//  Created by David Rowe on 22 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global ToolMenu */

ToolMenu = function (side, setAppScaleWithHandlesCallback) {
    // Tool menu displayed on top of forearm.

    "use strict";

    var SCALE_MODE_DIRECT = 0,
        SCALE_MODE_HANDLES = 1,
        scaleMode = SCALE_MODE_DIRECT,
        SCALE_MODE_DIRECT_COLOR = { red: 240, green: 240, blue: 0 },
        SCALE_MODE_HANDLES_COLOR = { red: 0, green: 240, blue: 240 },

        originOverlay,
        panelOverlay,
        buttonOverlay,
        buttonHighlightOverlay,

        LEFT_HAND = 0,
        AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}",

        FOREARM_JOINT_NAME = side === LEFT_HAND ? "LeftForeArm" : "RightForeArm",
        HAND_JOINT_NAME = side === LEFT_HAND ? "LeftHand" : "RightHand",

        ROOT_X_OFFSET = side === LEFT_HAND ? -0.05 : 0.05,
        ROOT_Z_OFFSET = side === LEFT_HAND ? -0.05 : 0.05,
        ROOT_ROTATION = side === LEFT_HAND
            ? Quat.fromVec3Degrees({ x: 180, y: -90, z: 0 })
            : Quat.fromVec3Degrees({ x: 180, y: 90, z: 0 }),

        ZERO_ROTATION = Quat.fromVec3Radians(Vec3.ZERO),

        ORIGIN_PROPERTIES = {
            dimensions: { x: 0.005, y: 0.005, z: 0.005 },
            color: { red: 255, blue: 0, green: 0 },
            alpha: 1.0,
            parentID: AVATAR_SELF_ID,
            localRotation: ROOT_ROTATION,
            ignoreRayIntersection: true,
            visible: false
        },

        PANEL_PROPERTIES = {
            dimensions: { x: 0.1, y: 0.2, z: 0.01 },
            localPosition: { x: 0.05, y: 0.1, z: 0.005 },
            localRotation: ZERO_ROTATION,
            color: { red: 192, green: 192, blue: 192 },
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        BUTTON_PROPERTIES = {
            dimensions: { x: 0.03, y: 0.03, z: 0.01 },
            localPosition: { x: 0.02, y: 0.02, z: 0.0 },
            localRotation: ZERO_ROTATION,
            color: scaleMode === SCALE_MODE_DIRECT ? SCALE_MODE_DIRECT_COLOR : SCALE_MODE_HANDLES_COLOR,
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        BUTTON_HIGHLIGHT_PROPERTIES = {
            dimensions: { x: 0.034, y: 0.034, z: 0.001 },
            localPosition: { x: 0, y: 0, z: -0.002 },
            localRotation: ZERO_ROTATION,
            color: { red: 240, green: 240, blue: 0 },
            alpha: 0.8,
            solid: false,
            drawInFront: true,
            ignoreRayIntersection: true,
            visible: false
        },

        isDisplaying = false,
        isHighlightingButton = false,
        isButtonPressed = false,

        // References.
        leftInputs,
        rightInputs,
        controlHand;

    function setReferences(left, right) {
        leftInputs = left;
        rightInputs = right;
        controlHand = side === LEFT_HAND ? rightInputs.hand() : leftInputs.hand();
    }

    function setHand(uiSide) {
        side = uiSide;
        controlHand = side === LEFT_HAND ? rightInputs.hand() : leftInputs.hand();

        if (isDisplaying) {
            // TODO: Move UI to other hand.
        }
    }

    function getEntityIDs() {
        return [panelOverlay, buttonOverlay];
    }

    function update(intersectionOverlayID) {
        // Highlight button.
        if (intersectionOverlayID === buttonOverlay !== isHighlightingButton) {
            isHighlightingButton = !isHighlightingButton;
            Overlays.editOverlay(buttonHighlightOverlay, { visible: isHighlightingButton });
        }

        // Button click.
        if (isHighlightingButton && controlHand.triggerClicked() !== isButtonPressed) {
            isButtonPressed = controlHand.triggerClicked();

            if (isButtonPressed) {
                scaleMode = scaleMode === SCALE_MODE_DIRECT ? SCALE_MODE_HANDLES : SCALE_MODE_DIRECT;
                Overlays.editOverlay(buttonOverlay, {
                    color: scaleMode === SCALE_MODE_DIRECT ? SCALE_MODE_DIRECT_COLOR : SCALE_MODE_HANDLES_COLOR
                });
                setAppScaleWithHandlesCallback(scaleMode === SCALE_MODE_HANDLES);
            }
        }
    }

    function display() {
        // Creates and shows menu entities.
        var forearmJointIndex,
            handJointIndex,
            forearmLength,
            rootOffset;

        if (isDisplaying) {
            return;
        }

        // Joint indexes.
        forearmJointIndex = MyAvatar.getJointIndex(FOREARM_JOINT_NAME);
        handJointIndex = MyAvatar.getJointIndex(HAND_JOINT_NAME);
        if (forearmJointIndex === -1 || handJointIndex === -1) {
            // Don't display if joint isn't available (yet) to attach to.
            // User can clear this condition by toggling the app off and back on once avatar finishes loading.
            // TODO: Log error.
            return;
        }

        // Calculate position to put menu.
        forearmLength = Vec3.distance(MyAvatar.getJointPosition(forearmJointIndex), MyAvatar.getJointPosition(handJointIndex));
        rootOffset = { x: ROOT_X_OFFSET, y: forearmLength, z: ROOT_Z_OFFSET };
        ORIGIN_PROPERTIES.parentJointIndex = forearmJointIndex;
        ORIGIN_PROPERTIES.localPosition = rootOffset;
        originOverlay = Overlays.addOverlay("sphere", ORIGIN_PROPERTIES);

        // Create menu items.
        PANEL_PROPERTIES.parentID = originOverlay;
        panelOverlay = Overlays.addOverlay("cube", PANEL_PROPERTIES);
        BUTTON_PROPERTIES.parentID = originOverlay;
        buttonOverlay = Overlays.addOverlay("cube", BUTTON_PROPERTIES);

        // Prepare highlight overlay.
        BUTTON_HIGHLIGHT_PROPERTIES.parentID = buttonOverlay;
        buttonHighlightOverlay = Overlays.addOverlay("cube", BUTTON_HIGHLIGHT_PROPERTIES);

        isDisplaying = true;
    }

    function clear() {
        // Deletes menu entities.
        if (!isDisplaying) {
            return;
        }

        Overlays.deleteOverlay(buttonHighlightOverlay);
        Overlays.deleteOverlay(buttonOverlay);
        Overlays.deleteOverlay(panelOverlay);
        isDisplaying = false;
    }

    function destroy() {
        clear();
    }

    if (!this instanceof ToolMenu) {
        return new ToolMenu();
    }

    return {
        setReferences: setReferences,
        setHand: setHand,
        getEntityIDs: getEntityIDs,
        update: update,
        display: display,
        clear: clear,
        destroy: destroy
    };
};

ToolMenu.prototype = {};
