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

ToolMenu = function (side, leftInputs, rightInputs, commandCallback) {
    // Tool menu displayed on top of forearm.

    "use strict";

    var attachmentJointName,

        menuOriginOverlay,
        menuPanelOverlay,

        menuOverlays = [],
        menuCallbacks = [],
        optionsOverlays = [],
        optionsCallbacks = [],
        optionsItems,
        highlightOverlay,

        LEFT_HAND = 0,
        AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}",
        ZERO_ROTATION = Quat.fromVec3Radians(Vec3.ZERO),

        CANVAS_SIZE = { x: 0.22, y: 0.13 },
        PANEL_ORIGIN_POSITION = { x: CANVAS_SIZE.x / 2, y: 0.15, z: -0.04 },
        PANEL_ROOT_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 0, z: 180 }),
        panelLateralOffset,

        MENU_ORIGIN_PROPERTIES = {
            dimensions: { x: 0.005, y: 0.005, z: 0.005 },
            localPosition: PANEL_ORIGIN_POSITION,
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
            color: { red: 164, green: 164, blue: 164 },
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        UI_ELEMENTS = {
            "panel": {
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.10, y: 0.12, z: 0.01 },
                    localRotation: ZERO_ROTATION,
                    color: { red: 192, green: 192, blue: 192 },
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                }
            },
            "button": {
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.03, y: 0.03, z: 0.01 },
                    localRotation: ZERO_ROTATION,
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                }
            }
        },

        OPTONS_PANELS = {
            groupOptions: [
                {
                    // Background element
                    id: "toolsOptionsPanel",
                    type: "panel",
                    properties: {
                        localPosition: { x: 0.055, y: 0.0, z: -0.005 }
                    }
                },
                {
                    id: "groupButton",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.07, y: 0.03, z: 0.01 },
                        localPosition: { x: 0, y: -0.025, z: -0.005 },
                        color: { red: 64, green: 240, blue: 64 }
                    },
                    callback: "groupButton"
                },
                {
                    id: "ungroupButton",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.07, y: 0.03, z: 0.01 },
                        localPosition: { x: 0, y: 0.025, z: -0.005 },
                        color: { red: 240, green: 64, blue: 64 }
                    },
                    callback: "ungroupButton"
                }
            ]
        },

        MENU_ITEMS = [
            {
                // Background element
                id: "toolsMenuPanel",
                type: "panel",
                properties: {
                    localPosition: { x: -0.055, y: 0.0, z: -0.005 }
                }
            },
            {
                id: "scaleButton",
                type: "button",
                properties: {
                    localPosition: { x: -0.022, y: -0.04, z: -0.005 },
                    color: { red: 0, green: 240, blue: 240 }
                },
                callback: "scaleTool"
            },
            {
                id: "cloneButton",
                type: "button",
                properties: {
                    localPosition: { x: 0.022, y: -0.04, z: -0.005 },
                    color: { red: 240, green: 240, blue: 0 }
                },
                callback: "cloneTool"
            },
            {
                id: "groupButton",
                type: "button",
                properties: {
                    localPosition: { x: -0.022, y: 0.0, z: -0.005 },
                    color: { red: 220, green: 60, blue: 220 }
                },
                toolOptions: "groupOptions",
                callback: "groupTool"
            }
        ],

        HIGHLIGHT_PROPERTIES = {
            xDelta: 0.004,
            yDelta: 0.004,
            zDimension: 0.001,
            properties: {
                localPosition: { x: 0, y: 0, z: -0.003 },
                localRotation: ZERO_ROTATION,
                color: { red: 255, green: 255, blue: 0 },
                alpha: 0.8,
                solid: false,
                drawInFront: true,
                ignoreRayIntersection: true,
                visible: false
            }
        },

        NONE = -1,

        intersectionOverlays,
        intersectionCallbacks,
        intersectionProperties,
        highlightedItem,
        highlightedSource,
        isHighlightingButton,
        pressedItem,
        pressedSource,
        isButtonPressed,

        isDisplaying = false,

        // References.
        controlHand;


    if (!this instanceof ToolMenu) {
        return new ToolMenu();
    }

    controlHand = side === LEFT_HAND ? rightInputs.hand() : leftInputs.hand();

    function setHand(hand) {
        // Assumes UI is not displaying.
        side = hand;
        controlHand = side === LEFT_HAND ? rightInputs.hand() : leftInputs.hand();
        attachmentJointName = side === LEFT_HAND ? "LeftHand" : "RightHand";
        panelLateralOffset = side === LEFT_HAND ? -0.01 : 0.01;
    }

    setHand(side);

    function getEntityIDs() {
        return [menuPanelOverlay].concat(menuOverlays).concat(optionsOverlays);
    }

    function openOptions(toolOptions) {
        var properties,
            parentID,
            i,
            length;

        // Close current panel, if any.
        for (i = 0, length = optionsOverlays.length; i < length; i += 1) {
            Overlays.deleteOverlay(optionsOverlays[i]);
            optionsOverlays = [];
            optionsCallbacks = [];
        }

        // Open specified panel, if any.
        if (toolOptions) {
            optionsItems = OPTONS_PANELS[toolOptions];
            parentID = menuPanelOverlay;  // Menu panel parents to background panel.
            for (i = 0, length = optionsItems.length; i < length; i += 1) {
                properties = Object.clone(UI_ELEMENTS[optionsItems[i].type].properties);
                properties = Object.merge(properties, optionsItems[i].properties);
                properties.parentID = parentID;
                optionsOverlays.push(Overlays.addOverlay(UI_ELEMENTS[optionsItems[i].type].overlay, properties));
                parentID = optionsOverlays[0];  // Menu buttons parent to menu panel.
                optionsCallbacks.push(optionsItems[i].callback);
            }
        }
    }

    function clearTool() {
        openOptions();
    }

    function update(intersectionOverlayID) {
        var intersectedItem,
            parentProperties,
            BUTTON_PRESS_DELTA = 0.004;

        // Intersection details.
        if (intersectionOverlayID) {
            intersectedItem = menuOverlays.indexOf(intersectionOverlayID);
            if (intersectedItem !== -1) {
                intersectionOverlays = menuOverlays;
                intersectionCallbacks = menuCallbacks;
                intersectionProperties = MENU_ITEMS;
            } else {
                intersectedItem = optionsOverlays.indexOf(intersectionOverlayID);
                if (intersectedItem !== -1) {
                    intersectionOverlays = optionsOverlays;
                    intersectionCallbacks = optionsCallbacks;
                    intersectionProperties = optionsItems;
                }
            }
        }
        if (!intersectionOverlays) {
            return;
        }

        // Highlight clickable item.
        if (intersectedItem !== highlightedItem || intersectionOverlays !== highlightedSource) {
            if (intersectedItem !== -1 && intersectionCallbacks[intersectedItem] !== undefined) {
                // Highlight new button.
                parentProperties = Overlays.getProperties(intersectionOverlays[intersectedItem],
                    ["dimensions", "localPosition"]);
                Overlays.editOverlay(highlightOverlay, {
                    parentID: intersectionOverlays[intersectedItem],
                    dimensions: {
                        x: parentProperties.dimensions.x + HIGHLIGHT_PROPERTIES.xDelta,
                        y: parentProperties.dimensions.y + HIGHLIGHT_PROPERTIES.yDelta,
                        z: HIGHLIGHT_PROPERTIES.zDimension
                    },
                    localPosition: HIGHLIGHT_PROPERTIES.properties.localPosition,
                    localRotation: HIGHLIGHT_PROPERTIES.properties.localRotation,
                    color: HIGHLIGHT_PROPERTIES.properties.color,
                    visible: true
                });
                highlightedItem = intersectedItem;
                isHighlightingButton = true;
            } else if (highlightedItem !== NONE) {
                // Un-highlight previous button.
                Overlays.editOverlay(highlightOverlay, {
                    visible: false
                });
                highlightedItem = NONE;
                isHighlightingButton = false;
            }
            highlightedSource = intersectionOverlays;
        }

        // Press button.
        if (intersectedItem !== pressedItem || intersectionOverlays !== pressedSource
                || controlHand.triggerClicked() !== isButtonPressed) {
            if (pressedItem !== NONE) {
                // Unpress previous button.
                Overlays.editOverlay(intersectionOverlays[pressedItem], {
                    localPosition: intersectionProperties[pressedItem].properties.localPosition
                });
                pressedItem = NONE;
            }
            isButtonPressed = isHighlightingButton && controlHand.triggerClicked();
            if (isButtonPressed) {
                // Press new button.
                Overlays.editOverlay(intersectionOverlays[intersectedItem], {
                    localPosition: Vec3.sum(intersectionProperties[intersectedItem].properties.localPosition,
                        { x: 0, y: 0, z: BUTTON_PRESS_DELTA })
                });
                pressedItem = intersectedItem;
                pressedSource = intersectionOverlays;

                // Button press actions.
                if (intersectionOverlays === menuOverlays) {
                    openOptions(intersectionProperties[highlightedItem].toolOptions);
                }
                commandCallback(intersectionCallbacks[highlightedItem]);
            }
        }
    }

    function display() {
        // Creates and shows menu entities.
        var handJointIndex,
            properties,
            parentID,
            i,
            length;

        if (isDisplaying) {
            return;
        }

        // Joint index.
        handJointIndex = MyAvatar.getJointIndex(attachmentJointName);
        if (handJointIndex === -1) {
            // Don't display if joint isn't available (yet) to attach to.
            // User can clear this condition by toggling the app off and back on once avatar finishes loading.
            // TODO: Log error.
            return;
        }

        // Menu origin.
        properties = Object.clone(MENU_ORIGIN_PROPERTIES);
        properties.parentJointIndex = handJointIndex;
        properties.localPosition = Vec3.sum(properties.localPosition, { x: panelLateralOffset, y: 0, z: 0 });
        menuOriginOverlay = Overlays.addOverlay("sphere", properties);

        // Panel background.
        properties = Object.clone(MENU_PANEL_PROPERTIES);
        properties.parentID = menuOriginOverlay;
        menuPanelOverlay = Overlays.addOverlay("cube", properties);

        // Menu items.
        parentID = menuPanelOverlay;  // Menu panel parents to background panel.
        for (i = 0, length = MENU_ITEMS.length; i < length; i += 1) {
            properties = Object.clone(UI_ELEMENTS[MENU_ITEMS[i].type].properties);
            properties = Object.merge(properties, MENU_ITEMS[i].properties);
            properties.parentID = parentID;
            menuOverlays.push(Overlays.addOverlay(UI_ELEMENTS[MENU_ITEMS[i].type].overlay, properties));
            parentID = menuOverlays[0];  // Menu buttons parent to menu panel.
            menuCallbacks.push(MENU_ITEMS[i].callback);
        }

        // Prepare highlight overlay.
        properties = Object.clone(HIGHLIGHT_PROPERTIES);
        properties.parentID = menuOriginOverlay;
        highlightOverlay = Overlays.addOverlay("cube", properties);

        // Initial values.
        intersectionOverlays = null;
        intersectionCallbacks = null;
        intersectionProperties = null;
        highlightedItem = NONE;
        highlightedSource = null;
        isHighlightingButton = false;
        pressedItem = NONE;
        pressedSource = null;
        isButtonPressed = false;

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
        for (i = 0, length = optionsOverlays.length; i < length; i += 1) {
            Overlays.deleteOverlay(optionsOverlays[i]);
        }
        optionsOverlays = [];
        optionsCallbacks = [];

        for (i = 0, length = menuOverlays.length; i < length; i += 1) {
            Overlays.deleteOverlay(menuOverlays[i]);
        }
        menuOverlays = [];
        menuCallbacks = [];

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
        clearTool: clearTool,
        update: update,
        display: display,
        clear: clear,
        destroy: destroy
    };
};

ToolMenu.prototype = {};
