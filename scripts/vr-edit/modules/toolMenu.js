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

ToolMenu = function (side, scaleModeChangedCallback) {
    // Tool menu displayed on top of forearm.

    "use strict";

    var panelEntity,
        buttonEntity,
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

        PANEL_ENTITY_PROPERTIES = {
            type: "Box",
            dimensions: { x: 0.1, y: 0.2, z: 0.01 },
            color: { red: 192, green: 192, blue: 192 },
            parentID: AVATAR_SELF_ID,
            registrationPoint: { x: 0, y: 0.0, z: 0.0 },
            localRotation: ROOT_ROTATION,
            ignoreRayIntersection: false,
            lifetime: 3600,
            visible: true
        },

        BUTTON_ENTITY_PROPERTIES = {
            type: "Box",
            dimensions: { x: 0.03, y: 0.03, z: 0.01 },
            registrationPoint: { x: 0, y: 0.0, z: 0.0 },
            localPosition: { x: 0.005, y: 0.005, z: -0.005 },  // Relative to the root panel entity.
            color: { red: 240, green: 0, blue: 0 },
            ignoreRayIntersection: false,
            lifetime: 3600,
            visible: true
        },

        BUTTON_HIGHLIGHT_PROPERTIES = {
            dimensions: { x: 0.034, y: 0.034, z: 0.001 },
            color: { red: 240, green: 240, blue: 0 },
            alpha: 0.8,
            localPosition: { x: 0.0155, y: 0.0155, z: 0.003 },
            localRotation: ZERO_ROTATION,
            solid: false,
            drawInFront: true,
            ignoreRayIntersection: true,
            visible: false
        },

        isDisplaying = false,
        isHighlightingButton = false,
        isButtonPressed = false,

        SCALE_MODE_DIRECT = 0,
        SCALE_MODE_HANDLES = 1,

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
        return [panelEntity, buttonEntity];
    }

    function update(intersectionEntityID) {
        // Highlight button.
        if (intersectionEntityID === buttonEntity !== isHighlightingButton) {
            isHighlightingButton = !isHighlightingButton;
            Overlays.editOverlay(buttonHighlightOverlay, { visible: isHighlightingButton });
        }

        // Button click.
        if (isHighlightingButton && controlHand.triggerClicked() !== isButtonPressed) {
            isButtonPressed = controlHand.triggerClicked();
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
        PANEL_ENTITY_PROPERTIES.parentJointIndex = forearmJointIndex;
        PANEL_ENTITY_PROPERTIES.localPosition = rootOffset;

        // Create menu items.
        panelEntity = Entities.addEntity(PANEL_ENTITY_PROPERTIES, true);
        BUTTON_ENTITY_PROPERTIES.parentID = panelEntity;
        buttonEntity = Entities.addEntity(BUTTON_ENTITY_PROPERTIES, true);

        // Prepare highlight overlay.
        BUTTON_HIGHLIGHT_PROPERTIES.parentID = buttonEntity;
        buttonHighlightOverlay = Overlays.addOverlay("cube", BUTTON_HIGHLIGHT_PROPERTIES);

        isDisplaying = true;
    }

    function clear() {
        // Deletes menu entities.
        if (!isDisplaying) {
            return;
        }

        Entities.deleteEntity(buttonEntity);
        Entities.deleteEntity(panelEntity);
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
