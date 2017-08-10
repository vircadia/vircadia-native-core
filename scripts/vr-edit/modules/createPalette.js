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
        palettePanelOverlay,
        highlightOverlay,
        paletteItemOverlays = [],

        LEFT_HAND = 0,

        controlJointName,

        CANVAS_SIZE = { x: 0.21, y: 0.13 },
        PALETTE_ROOT_POSITION = { x: -CANVAS_SIZE.x / 2, y: 0.15, z: 0.11 },
        PALETTE_ROOT_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 180, z: 180 }),
        lateralOffset,

        PALETTE_ORIGIN_PROPERTIES = {
            dimensions: { x: 0.005, y: 0.005, z: 0.005 },
            localPosition: PALETTE_ROOT_POSITION,
            localRotation: PALETTE_ROOT_ROTATION,
            color: { red: 255, blue: 0, green: 0 },
            alpha: 1.0,
            parentID: Uuid.SELF,
            ignoreRayIntersection: true,
            visible: false
        },

        PALETTE_PANEL_PROPERTIES = {
            dimensions: { x: CANVAS_SIZE.x, y: CANVAS_SIZE.y, z: 0.001 },
            localPosition: { x: CANVAS_SIZE.x / 2, y: CANVAS_SIZE.y / 2, z: 0 },
            localRotation: Quat.ZERO,
            color: { red: 192, green: 192, blue: 192 },
            alpha: 0.3,
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

        PALETTE_ITEMS = [
            {
                overlay: {
                    type: "cube",
                    properties: {
                        dimensions: { x: 0.03, y: 0.03, z: 0.03 },
                        localPosition: { x: 0.02, y: 0.02, z: 0.0 },
                        localRotation: Quat.ZERO,
                        color: { red: 240, green: 0, blue: 0 },
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: false,
                        visible: true
                    }
                },
                entity: {
                    type: "Box",
                    dimensions: { x: 0.2, y: 0.2, z: 0.2 },
                    color: { red: 192, green: 192, blue: 192 }
                }
            },
            {
                overlay: {
                    type: "shape",
                    properties: {
                        shape: "Cylinder",
                        dimensions: { x: 0.03, y: 0.03, z: 0.03 },
                        localPosition: { x: 0.06, y: 0.02, z: 0.0 },
                        localRotation: Quat.fromVec3Degrees({ x: -90, y: 0, z: 0 }),
                        color: { red: 240, green: 0, blue: 0 },
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: false,
                        visible: true
                    }
                },
                entity: {
                    type: "Shape",
                    shape: "Cylinder",
                    dimensions: { x: 0.2, y: 0.2, z: 0.2 },
                    color: { red: 192, green: 192, blue: 192 }
                }
            },
            {
                overlay: {
                    type: "shape",
                    properties: {
                        shape: "Cone",
                        dimensions: { x: 0.03, y: 0.03, z: 0.03 },
                        localPosition: { x: 0.10, y: 0.02, z: 0.0 },
                        localRotation: Quat.fromVec3Degrees({ x: -90, y: 0, z: 0 }),
                        color: { red: 240, green: 0, blue: 0 },
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: false,
                        visible: true
                    }
                },
                entity: {
                    type: "Shape",
                    shape: "Cone",
                    dimensions: { x: 0.2, y: 0.2, z: 0.2 },
                    color: { red: 192, green: 192, blue: 192 }
                }
            },
            {
                overlay: {
                    type: "sphere",
                    properties: {
                        dimensions: { x: 0.03, y: 0.03, z: 0.03 },
                        localPosition: { x: 0.14, y: 0.02, z: 0.0 },
                        localRotation: Quat.ZERO,
                        color: { red: 240, green: 0, blue: 0 },
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: false,
                        visible: true
                    }
                },
                entity: {
                    type: "Sphere",
                    dimensions: { x: 0.2, y: 0.2, z: 0.2 },
                    color: { red: 192, green: 192, blue: 192 }
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
        lateralOffset = side === LEFT_HAND ? -0.01 : 0.01;
    }

    setHand(side);

    function getEntityIDs() {
        return [palettePanelOverlay].concat(paletteItemOverlays);
    }

    function update(intersectionOverlayID) {
        var itemIndex,
            isTriggerClicked,
            properties,
            PRESS_DELTA = { x: 0, y: 0, z: 0.01 },
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
                localPosition: PALETTE_ITEMS[pressedItem].overlay.properties.localPosition
            });
            pressedItem = NONE;
        }

        // Press item and create new entity.
        isTriggerClicked = controlHand.triggerClicked();
        if (highlightedItem !== NONE && pressedItem === NONE && isTriggerClicked && !wasTriggerClicked) {
            Overlays.editOverlay(paletteItemOverlays[itemIndex], {
                localPosition: Vec3.sum(PALETTE_ITEMS[itemIndex].overlay.properties.localPosition, PRESS_DELTA)
            });
            pressedItem = itemIndex;

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
        properties.localPosition = Vec3.sum(PALETTE_ROOT_POSITION, { x: lateralOffset, y: 0, z: 0 });
        paletteOriginOverlay = Overlays.addOverlay("sphere", properties);

        // Create palette.
        properties = Object.clone(PALETTE_PANEL_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        palettePanelOverlay = Overlays.addOverlay("cube", properties);
        for (i = 0, length = PALETTE_ITEMS.length; i < length; i += 1) {
            properties = Object.clone(PALETTE_ITEMS[i].overlay.properties);
            properties.parentID = paletteOriginOverlay;
            paletteItemOverlays[i] = Overlays.addOverlay(PALETTE_ITEMS[i].overlay.type, properties);
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
