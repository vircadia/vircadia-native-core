//
//  createPalette.js
//
//  Created by David Rowe on 28 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global App, CreatePalette */

CreatePalette = function (side, leftInputs, rightInputs, uiCommandCallback) {
    // Tool menu displayed on top of forearm.

    "use strict";

    var paletteOriginOverlay,
        paletteHeaderOverlay,
        paletteHeaderBarOverlay,
        paletteTitleOverlay,
        palettePanelOverlay,
        highlightOverlay,
        paletteItemOverlays = [],
        paletteItemPositions = [],

        LEFT_HAND = 0,

        controlJointName,

        PALETTE_ORIGIN_POSITION = {
            x: 0,
            y: UIT.dimensions.handOffset,
            z: UIT.dimensions.canvasSeparation + UIT.dimensions.canvas.x / 2
        },
        PALETTE_ORIGIN_ROTATION = Quat.ZERO,
        paletteLateralOffset,

        PALETTE_ORIGIN_PROPERTIES = {
            dimensions: { x: 0.005, y: 0.005, z: 0.005 },
            localPosition: PALETTE_ORIGIN_POSITION,
            localRotation: PALETTE_ORIGIN_ROTATION,
            color: { red: 255, blue: 0, green: 0 },
            alpha: 1.0,
            parentID: Uuid.SELF,
            ignoreRayIntersection: true,
            visible: false
        },

        PALETTE_HEADER_PROPERTIES = {
            dimensions: UIT.dimensions.header,
            localPosition: {
                x: 0,
                y: UIT.dimensions.canvas.y / 2 - UIT.dimensions.header.y / 2,
                z: UIT.dimensions.header.z / 2
            },
            localRotation: Quat.ZERO,
            color: UIT.colors.baseGray,
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        PALETTE_HEADER_BAR_PROPERTIES = {
            dimensions: UIT.dimensions.headerBar,
            localPosition: {
                x: 0,
                y: UIT.dimensions.canvas.y / 2 - UIT.dimensions.header.y - UIT.dimensions.headerBar.y / 2,
                z: UIT.dimensions.headerBar.z / 2
            },
            localRotation: Quat.ZERO,
            color: UIT.colors.blueHighlight,
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        PALETTE_TITLE_PROPERTIES = {
            url: "../assets/create/create-heading.svg",
            scale: 0.0363,
            localPosition: { x: 0, y: 0, z: PALETTE_HEADER_PROPERTIES.dimensions.z / 2 + UIT.dimensions.imageOffset },
            localRotation: Quat.ZERO,
            color: UIT.colors.white,
            alpha: 1.0,
            emissive: true,
            ignoreRayIntersection: true,
            isFacingAvatar: false,
            visible: true
        },

        PALETTE_PANEL_PROPERTIES = {
            dimensions: UIT.dimensions.panel,
            localPosition: { x: 0, y: UIT.dimensions.panel.y / 2 - UIT.dimensions.canvas.y / 2, z: UIT.dimensions.panel.z / 2 },
            localRotation: Quat.ZERO,
            color: UIT.colors.baseGray,
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        HIGHLIGHT_PROPERTIES = {
            dimensions: { x: 0.034, y: 0.034, z: 0.034 },
            localPosition: { x: 0.02, y: 0.02, z: 0.0 },
            localRotation: Quat.ZERO,
            color: { red: 240, green: 240, blue: 0 },
            alpha: 0.8,
            solid: false,
            drawInFront: true,
            ignoreRayIntersection: true,
            visible: false
        },

        PALETTE_ENTITY_DIMENSIONS = { x: 0.024, y: 0.024, z: 0.024 },
        PALETTE_ENTITY_COLOR = UIT.colors.faintGray,
        ENTITY_CREATION_DIMENSIONS = { x: 0.2, y: 0.2, z: 0.2 },
        ENTITY_CREATION_COLOR = { red: 192, green: 192, blue: 192 },

        PALETTE_ITEMS = [
            {
                overlay: {
                    type: "cube",
                    properties: {
                        dimensions: PALETTE_ENTITY_DIMENSIONS,
                        localRotation: Quat.ZERO,
                        color: PALETTE_ENTITY_COLOR,
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: false,
                        visible: true
                    }
                },
                entity: {
                    type: "Box",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            },
            {
                overlay: {
                    type: "shape",
                    properties: {
                        shape: "Cylinder",
                        dimensions: PALETTE_ENTITY_DIMENSIONS,
                        localRotation: Quat.fromVec3Degrees({ x: -90, y: 0, z: 0 }),
                        color: PALETTE_ENTITY_COLOR,
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: false,
                        visible: true
                    }
                },
                entity: {
                    type: "Shape",
                    shape: "Cylinder",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            },
            {
                overlay: {
                    type: "shape",
                    properties: {
                        shape: "Cone",
                        dimensions: PALETTE_ENTITY_DIMENSIONS,
                        localRotation: Quat.fromVec3Degrees({ x: 90, y: 0, z: 0 }),
                        color: PALETTE_ENTITY_COLOR,
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: false,
                        visible: true
                    }
                },
                entity: {
                    type: "Shape",
                    shape: "Cone",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            },
            {
                overlay: {
                    type: "sphere",
                    properties: {
                        dimensions: PALETTE_ENTITY_DIMENSIONS,
                        localRotation: Quat.ZERO,
                        color: PALETTE_ENTITY_COLOR,
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: false,
                        visible: true
                    }
                },
                entity: {
                    type: "Sphere",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            }
        ],

        isDisplaying = false,

        NONE = -1,
        highlightedItem = NONE,
        pressedItem = NONE,
        wasTriggerClicked = false,

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
        paletteLateralOffset = side === LEFT_HAND ? -UIT.dimensions.handLateralOffset : UIT.dimensions.handLateralOffset;
    }

    setHand(side);

    function getEntityIDs() {
        return [palettePanelOverlay, paletteHeaderOverlay, paletteHeaderBarOverlay].concat(paletteItemOverlays);
    }

    function update(intersectionOverlayID) {
        var itemIndex,
            isTriggerClicked,
            properties,
            PRESS_DELTA = { x: 0, y: 0, z: -0.01 },
            CREATE_OFFSET = { x: 0, y: 0.05, z: -0.02 },
            INVERSE_HAND_BASIS_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 0, z: -90 });

        // Highlight cube.
        itemIndex = paletteItemOverlays.indexOf(intersectionOverlayID);
        if (itemIndex !== NONE) {
            if (highlightedItem !== itemIndex) {
                Overlays.editOverlay(highlightOverlay, {
                    parentID: intersectionOverlayID,
                    localPosition: Vec3.ZERO,
                    visible: true
                });
                highlightedItem = itemIndex;
            }
        } else {
            Overlays.editOverlay(highlightOverlay, {
                visible: false
            });
            highlightedItem = NONE;
        }

        // Unpress currently pressed item.
        if (pressedItem !== NONE && pressedItem !== itemIndex) {
            Overlays.editOverlay(paletteItemOverlays[pressedItem], {
                localPosition: paletteItemPositions[pressedItem]
            });
            pressedItem = NONE;
        }

        // Press item and create new entity.
        isTriggerClicked = controlHand.triggerClicked();
        if (highlightedItem !== NONE && pressedItem === NONE && isTriggerClicked && !wasTriggerClicked) {
            // Press item.
            Overlays.editOverlay(paletteItemOverlays[itemIndex], {
                localPosition: Vec3.sum(paletteItemPositions[itemIndex], PRESS_DELTA)
            });
            pressedItem = itemIndex;

            // Create entity.
            properties = Object.clone(PALETTE_ITEMS[itemIndex].entity);
            properties.position = Vec3.sum(controlHand.palmPosition(),
                Vec3.multiplyQbyV(controlHand.orientation(),
                    Vec3.sum({ x: 0, y: properties.dimensions.z / 2, z: 0 }, CREATE_OFFSET)));
            properties.rotation = Quat.multiply(controlHand.orientation(), INVERSE_HAND_BASIS_ROTATION);
            Entities.addEntity(properties);

            uiCommandCallback("autoGrab");
        }

        wasTriggerClicked = isTriggerClicked;
    }

    function itemPosition(index) {
        // Position relative to palette panel.
        var ITEMS_PER_ROW = 3,
            ROW_ZERO_Y_OFFSET = 0.0580,
            ROW_SPACING = 0.0560,
            COLUMN_ZERO_OFFSET = -0.08415,
            COLUMN_SPACING = 0.0561,
            row,
            column;

        row = Math.floor(index / ITEMS_PER_ROW);
        column = index % ITEMS_PER_ROW;

        return {
            x: COLUMN_ZERO_OFFSET + column * COLUMN_SPACING,
            y: ROW_ZERO_Y_OFFSET - row * ROW_SPACING,
            z: UIT.dimensions.panel.z + PALETTE_ENTITY_DIMENSIONS.z / 2
        };
    }

    function display() {
        // Creates and shows menu entities.
        var handJointIndex,
            properties,
            i,
            length;

        if (isDisplaying) {
            return;
        }

        // Joint index.
        handJointIndex = MyAvatar.getJointIndex(controlJointName);
        if (handJointIndex === NONE) {
            // Don't display if joint isn't available (yet) to attach to.
            // User can clear this condition by toggling the app off and back on once avatar finishes loading.
            App.log(side, "ERROR: CreatePalette: Hand joint index isn't available!");
            return;
        }

        // Calculate position to put palette.
        properties = Object.clone(PALETTE_ORIGIN_PROPERTIES);
        properties.parentJointIndex = handJointIndex;
        properties.localPosition = Vec3.sum(PALETTE_ORIGIN_POSITION, { x: paletteLateralOffset, y: 0, z: 0 });
        paletteOriginOverlay = Overlays.addOverlay("sphere", properties);

        // Header.
        properties = Object.clone(PALETTE_HEADER_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        paletteHeaderOverlay = Overlays.addOverlay("cube", properties);
        properties = Object.clone(PALETTE_HEADER_BAR_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        paletteHeaderBarOverlay = Overlays.addOverlay("cube", properties);
        properties = Object.clone(PALETTE_TITLE_PROPERTIES);
        properties.parentID = paletteHeaderOverlay;
        properties.url = Script.resolvePath(properties.url);
        paletteTitleOverlay = Overlays.addOverlay("image3d", properties);

        // Palette background.
        properties = Object.clone(PALETTE_PANEL_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        palettePanelOverlay = Overlays.addOverlay("cube", properties);

        // Palette items.
        for (i = 0, length = PALETTE_ITEMS.length; i < length; i += 1) {
            properties = Object.clone(PALETTE_ITEMS[i].overlay.properties);
            properties.parentID = palettePanelOverlay;
            properties.localPosition = itemPosition(i);
            paletteItemOverlays[i] = Overlays.addOverlay(PALETTE_ITEMS[i].overlay.type, properties);
            paletteItemPositions[i] = properties.localPosition;
        }

        // Prepare cube highlight overlay.
        properties = Object.clone(HIGHLIGHT_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        highlightOverlay = Overlays.addOverlay("cube", properties);

        isDisplaying = true;
    }

    function clear() {
        // Deletes menu entities.
        var i,
            length;

        if (!isDisplaying) {
            return;
        }

        Overlays.deleteOverlay(highlightOverlay);
        for (i = 0, length = paletteItemOverlays.length; i < length; i += 1) {
            Overlays.deleteOverlay(paletteItemOverlays[i]);
        }
        Overlays.deleteOverlay(palettePanelOverlay);
        Overlays.deleteOverlay(paletteTitleOverlay);
        Overlays.deleteOverlay(paletteHeaderBarOverlay);
        Overlays.deleteOverlay(paletteHeaderOverlay);
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
