//
//  createPalette.js
//
//  Created by David Rowe on 28 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global CreatePalette */

CreatePalette = function (side, leftInputs, rightInputs) {
    // Tool menu displayed on top of forearm.

    "use strict";

    var paletteOriginOverlay,
        palettePanelOverlay,
        cubeOverlay,
        cubeHighlightOverlay,

        LEFT_HAND = 0,
        AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}",
        ZERO_ROTATION = Quat.fromVec3Radians(Vec3.ZERO),

        controlJointName,

        CANVAS_SIZE = { x: 0.21, y: 0.13 },
        PALETTE_ROOT_POSITION = { x: -CANVAS_SIZE.x / 2, y: 0.15, z: 0.09 },
        PALETTE_ROOT_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 180, z: 180 }),
        lateralOffset,

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

        isHighlightingCube = false,
        isCubePressed = false,

        // References.
        controlHand;


    if (!this instanceof CreatePalette) {
        return new CreatePalette();
    }

    function setHand(hand) {
        // Assumes UI is not displaying.
        side = hand;
        controlHand = side === LEFT_HAND ? rightInputs.hand() : leftInputs.hand();
        controlJointName = side === LEFT_HAND ? "LeftHand" : "RightHand";
        lateralOffset = side === LEFT_HAND ? -0.01 : 0.01;
    }

    setHand(side);

    function getEntityIDs() {
        return [palettePanelOverlay, cubeOverlay];
    }

    function update(intersectionOverlayID) {
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
                    localPosition: Vec3.sum(CUBE_PROPERTIES.localPosition, { x: 0, y: 0, z: 0.01 })
                });
                CUBE_ENTITY_PROPERTIES.position =
                    Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.2, z: -1.0 }));
                CUBE_ENTITY_PROPERTIES.rotation = MyAvatar.orientation;
                Entities.addEntity(CUBE_ENTITY_PROPERTIES);
            } else {
                Overlays.editOverlay(cubeOverlay, {
                    localPosition: CUBE_PROPERTIES.localPosition
                });
            }
        }
    }

    function display() {
        // Creates and shows menu entities.
        var handJointIndex,
            properties;

        if (isDisplaying) {
            return;
        }

        // Joint index.
        handJointIndex = MyAvatar.getJointIndex(controlJointName);
        if (handJointIndex === -1) {
            // Don't display if joint isn't available (yet) to attach to.
            // User can clear this condition by toggling the app off and back on once avatar finishes loading.
            // TODO: Log error.
            return;
        }

        // Calculate position to put palette.
        properties = Object.clone(PALETTE_ORIGIN_PROPERTIES);
        properties.parentJointIndex = handJointIndex;
        properties.localPosition = Vec3.sum(PALETTE_ROOT_POSITION, { x: lateralOffset, y: 0, z: 0 });
        paletteOriginOverlay = Overlays.addOverlay("sphere", properties);

        // Create palette items.
        properties = Object.clone(PALETTE_PANEL_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        palettePanelOverlay = Overlays.addOverlay("cube", properties);
        properties = Object.clone(CUBE_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        cubeOverlay = Overlays.addOverlay("cube", properties);

        // Prepare cube highlight overlay.
        properties = Object.clone(CUBE_HIGHLIGHT_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        cubeHighlightOverlay = Overlays.addOverlay("cube", properties);

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

CreatePalette.prototype = {};
