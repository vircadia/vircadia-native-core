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

ToolMenu = function (side, leftInputs, rightInputs, setAppScaleWithHandlesCallback) {
    // Tool menu displayed on top of forearm.

    "use strict";

    var SCALE_MODE_DIRECT = 0,
        SCALE_MODE_HANDLES = 1,
        scaleMode = SCALE_MODE_DIRECT,
        SCALE_MODE_DIRECT_COLOR = { red: 240, green: 240, blue: 0 },
        SCALE_MODE_HANDLES_COLOR = { red: 0, green: 240, blue: 240 },

        menuOriginOverlay,
        menuPanelOverlay,
        buttonOverlay,
        buttonHighlightOverlay,

        paletteOriginOverlay,
        palettePanelOverlay,
        cubeOverlay,
        cubeHighlightOverlay,

        LEFT_HAND = 0,
        AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}",

        HAND_JOINT_NAME = side === LEFT_HAND ? "LeftHand" : "RightHand",

        CANVAS_SIZE = { x: 0.21, y: 0.13 },
        LATERAL_OFFSET = side === LEFT_HAND ? -0.01 : 0.01,

        PANEL_ROOT_POSITION = { x: CANVAS_SIZE.x / 2 + LATERAL_OFFSET, y: 0.15, z: -0.03 },
        PANEL_ROOT_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 0, z: 180 }),

        PALETTE_ROOT_POSITION = { x: -CANVAS_SIZE.x / 2 + LATERAL_OFFSET, y: 0.15, z: 0.09 },
        PALETTE_ROOT_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 180, z: 180 }),

        ZERO_ROTATION = Quat.fromVec3Radians(Vec3.ZERO),

        PANEL_ORIGIN_PROPERTIES = {
            dimensions: { x: 0.005, y: 0.005, z: 0.005 },
            localPosition: PANEL_ROOT_POSITION,
            localRotation: PANEL_ROOT_ROTATION,
            color: { red: 255, blue: 0, green: 0 },
            alpha: 1.0,
            parentID: AVATAR_SELF_ID,
            ignoreRayIntersection: true,
            visible: false
        },

        MENU_PANEL_PROPERTIES = {
            dimensions: { x: CANVAS_SIZE.x, y: CANVAS_SIZE.y, z: 0.01 },
            localPosition: { x: CANVAS_SIZE.x / 2, y: CANVAS_SIZE.y / 2, z: 0.005 },
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
            localPosition: { x: 0.02, y: 0.02, z: -0.002 },
            localRotation: ZERO_ROTATION,
            color: { red: 240, green: 240, blue: 0 },
            alpha: 0.8,
            solid: false,
            drawInFront: true,
            ignoreRayIntersection: true,
            visible: false
        },

        PALETTE_ORIGIN_PROPERTIES = {
            dimensions: { x: 0.005, y: 0.005, z: 0.005 },
            localPosition: PALETTE_ROOT_POSITION,
            localRotation: PALETTE_ROOT_ROTATION,
            color: { red: 255, blue: 0, green: 0 },
            alpha: 1.0,
            parentID: AVATAR_SELF_ID,
            ignoreRayIntersection: true,
            visible: false
        },

        PALETTE_PANEL_PROPERTIES = {
            dimensions: { x: CANVAS_SIZE.x, y: CANVAS_SIZE.y, z: 0.001 },
            localPosition: { x: CANVAS_SIZE.x / 2, y: CANVAS_SIZE.y / 2, z: 0 },
            localRotation: ZERO_ROTATION,
            color: { red: 192, green: 192, blue: 192 },
            alpha: 0.3,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        CUBE_PROPERTIES = {
            dimensions: { x: 0.03, y: 0.03, z: 0.03 },
            localPosition: { x: 0.02, y: 0.02, z: 0.0 },
            localRotation: ZERO_ROTATION,
            color: { red: 240, green: 0, blue: 0 },
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        CUBE_HIGHLIGHT_PROPERTIES = {
            dimensions: { x: 0.034, y: 0.034, z: 0.034 },
            localPosition: { x: 0.02, y: 0.02, z: 0.0 },
            localRotation: ZERO_ROTATION,
            color: { red: 240, green: 240, blue: 0 },
            alpha: 0.8,
            solid: false,
            drawInFront: true,
            ignoreRayIntersection: true,
            visible: false
        },

        CUBE_ENTITY_PROPERTIES = {
            type: "Box",
            dimensions: { x: 0.2, y: 0.2, z: 0.2 },
            color: { red: 192, green: 192, blue: 192 }
        },

        isDisplaying = false,

        isHighlightingButton = false,
        isButtonPressed = false,
        isHighlightingCube = false,
        isCubePressed = false,

        // References.
        controlHand;


    if (!this instanceof ToolMenu) {
        return new ToolMenu();
    }

    controlHand = side === LEFT_HAND ? rightInputs.hand() : leftInputs.hand();

    function setHand(uiSide) {
        side = uiSide;
        controlHand = side === LEFT_HAND ? rightInputs.hand() : leftInputs.hand();

        if (isDisplaying) {
            // TODO: Move UI to other hand.
        }
    }

    function getEntityIDs() {
        return [menuPanelOverlay, buttonOverlay, palettePanelOverlay, cubeOverlay];
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
                    color: scaleMode === SCALE_MODE_DIRECT ? SCALE_MODE_DIRECT_COLOR : SCALE_MODE_HANDLES_COLOR,
                    localPosition: Vec3.sum(BUTTON_PROPERTIES.localPosition, { x: 0, y: 0, z: 0.004 })
                });
                setAppScaleWithHandlesCallback(scaleMode === SCALE_MODE_HANDLES);
            } else {
                Overlays.editOverlay(buttonOverlay, {
                    localPosition: BUTTON_PROPERTIES.localPosition
                });
            }
        }

        // Highlight cube.
        if (intersectionOverlayID === cubeOverlay !== isHighlightingCube) {
            isHighlightingCube = !isHighlightingCube;
            Overlays.editOverlay(cubeHighlightOverlay, { visible: isHighlightingCube });
        }

        // Cube click.
        if (isHighlightingCube && controlHand.triggerClicked() !== isCubePressed) {
            isCubePressed = controlHand.triggerClicked();

            if (isCubePressed) {
                Overlays.editOverlay(cubeOverlay, {
                    localPosition: Vec3.sum(BUTTON_PROPERTIES.localPosition, { x: 0, y: 0, z: 0.01 })
                });
                CUBE_ENTITY_PROPERTIES.position =
                    Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.2, z: -1.0 }));
                CUBE_ENTITY_PROPERTIES.rotation = MyAvatar.orientation;
                Entities.addEntity(CUBE_ENTITY_PROPERTIES);
            } else {
                Overlays.editOverlay(cubeOverlay, {
                    localPosition: BUTTON_PROPERTIES.localPosition
                });
            }
        }
    }

    function display() {
        // Creates and shows menu entities.
        var handJointIndex;

        if (isDisplaying) {
            return;
        }

        // Joint index.
        handJointIndex = MyAvatar.getJointIndex(HAND_JOINT_NAME);
        if (handJointIndex === -1) {
            // Don't display if joint isn't available (yet) to attach to.
            // User can clear this condition by toggling the app off and back on once avatar finishes loading.
            // TODO: Log error.
            return;
        }

        // Calculate position to put menu.
        PANEL_ORIGIN_PROPERTIES.parentJointIndex = handJointIndex;
        menuOriginOverlay = Overlays.addOverlay("sphere", PANEL_ORIGIN_PROPERTIES);

        // Create menu items.
        MENU_PANEL_PROPERTIES.parentID = menuOriginOverlay;
        menuPanelOverlay = Overlays.addOverlay("cube", MENU_PANEL_PROPERTIES);
        BUTTON_PROPERTIES.parentID = menuOriginOverlay;
        buttonOverlay = Overlays.addOverlay("cube", BUTTON_PROPERTIES);

        // Prepare button highlight overlay.
        BUTTON_HIGHLIGHT_PROPERTIES.parentID = menuOriginOverlay;
        buttonHighlightOverlay = Overlays.addOverlay("cube", BUTTON_HIGHLIGHT_PROPERTIES);

        // Calculate position to put palette.
        PALETTE_ORIGIN_PROPERTIES.parentJointIndex = handJointIndex;
        paletteOriginOverlay = Overlays.addOverlay("sphere", PALETTE_ORIGIN_PROPERTIES);

        // Create palette items.
        PALETTE_PANEL_PROPERTIES.parentID = paletteOriginOverlay;
        palettePanelOverlay = Overlays.addOverlay("cube", PALETTE_PANEL_PROPERTIES);
        CUBE_PROPERTIES.parentID = paletteOriginOverlay;
        cubeOverlay = Overlays.addOverlay("cube", CUBE_PROPERTIES);

        // Prepare cube highlight overlay.
        CUBE_HIGHLIGHT_PROPERTIES.parentID = paletteOriginOverlay;
        cubeHighlightOverlay = Overlays.addOverlay("cube", CUBE_HIGHLIGHT_PROPERTIES);

        isDisplaying = true;
    }

    function clear() {
        // Deletes menu entities.
        if (!isDisplaying) {
            return;
        }

        Overlays.deleteOverlay(cubeHighlightOverlay);
        Overlays.deleteOverlay(cubeOverlay);
        Overlays.deleteOverlay(palettePanelOverlay);
        Overlays.deleteOverlay(paletteOriginOverlay);

        Overlays.deleteOverlay(buttonHighlightOverlay);
        Overlays.deleteOverlay(buttonOverlay);
        Overlays.deleteOverlay(menuPanelOverlay);
        Overlays.deleteOverlay(menuOriginOverlay);
        isDisplaying = false;
    }

    function destroy() {
        clear();
    }

    return {
        setHand: setHand,
        entityIDs: getEntityIDs,
        update: update,
        display: display,
        clear: clear,
        destroy: destroy
    };
};

ToolMenu.prototype = {};
