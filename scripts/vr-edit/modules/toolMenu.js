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

ToolMenu = function (side, leftInputs, rightInputs, doCallback) {
    // Tool menu displayed on top of forearm.

    "use strict";

    var attachmentJointName,

        menuOriginOverlay,
        menuPanelOverlay,

        menuOverlays = [],

        optionsOverlays = [],
        optionsOverlaysIDs = [],  // Text ids (names) of options overlays.
        optionsEnabled = [],
        optionsSettings = {},

        highlightOverlay,

        LEFT_HAND = 0,

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
            parentID: Uuid.SELF,
            ignoreRayIntersection: true,
            visible: false
        },

        MENU_PANEL_PROPERTIES = {
            dimensions: { x: CANVAS_SIZE.x, y: CANVAS_SIZE.y, z: 0.01 },
            localPosition: { x: CANVAS_SIZE.x / 2, y: CANVAS_SIZE.y / 2, z: 0.005 },
            localRotation: Quat.ZERO,
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
                    localRotation: Quat.ZERO,
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
                    localRotation: Quat.ZERO,
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                }
            },
            "label": {
                overlay: "text3d",
                properties: {
                    dimensions: { x: 0.03, y: 0.0075 },
                    localPosition: { x: 0.0, y: 0.0, z: -0.005 },
                    localRotation: Quat.fromVec3Degrees({ x: 0, y: 180, z: 180 }),
                    topMargin: 0,
                    leftMargin: 0,
                    color: { red: 128, green: 128, blue: 128 },
                    alpha: 1.0,
                    lineHeight: 0.007,
                    backgroundAlpha: 0,
                    ignoreRayIntersection: true,
                    isFacingAvatar: false,
                    drawInFront: true,
                    visible: true
                }
            },
            "circle": {
                overlay: "circle3d",
                properties: {
                    size: 0.015,
                    localPosition: { x: 0.0, y: 0.0, z: -0.01 },
                    localRotation: Quat.fromVec3Degrees({ x: 0, y: 180, z: 180 }),
                    color: { red: 128, green: 128, blue: 128 },
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: true,
                    visible: true
                }
            }
        },

        OPTONS_PANELS = {
            groupOptions: [
                {
                    id: "groupOptionsPanel",
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
                        color: { red: 200, green: 200, blue: 200 }
                    },
                    label: " GROUP",
                    enabledColor: { red: 64, green: 240, blue: 64 },
                    callback: {
                        method: "groupButton"
                    }
                },
                {
                    id: "ungroupButton",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.07, y: 0.03, z: 0.01 },
                        localPosition: { x: 0, y: 0.025, z: -0.005 },
                        color: { red: 200, green: 200, blue: 200 }
                    },
                    label: "UNGROUP",
                    enabledColor: { red: 240, green: 64, blue: 64 },
                    callback: {
                        method: "ungroupButton"
                    }
                }
            ],
            colorOptions: [
                {
                    id: "colorOptionsPanel",
                    type: "panel",
                    properties: {
                        localPosition: { x: 0.055, y: 0.0, z: -0.005 }
                    }
                },
                {
                    id: "colorSwatch1",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.02, y: 0.02, z: 0.01 },
                        localPosition: { x: -0.035, y: 0.02, z: -0.005 },
                        color: { red: 255, green: 0, blue: 0 }
                    },
                    command: {
                        method: "setCurrentColor",
                        parameter: "colorSwatch1.color"
                    },
                    callback: {
                        method: "setColor",
                        parameter: "colorSwatch1.color"
                    }
                },
                {
                    id: "colorSwatch2",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.02, y: 0.02, z: 0.01 },
                        localPosition: { x: -0.01, y: 0.02, z: -0.005 },
                        color: { red: 0, green: 255, blue: 0 }
                    },
                    command: {
                        method: "setCurrentColor",
                        parameter: "colorSwatch2.color"
                    },
                    callback: {
                        method: "setColor",
                        parameter: "colorSwatch2.color"
                    }
                },
                {
                    id: "colorSwatch3",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.02, y: 0.02, z: 0.01 },
                        localPosition: { x: -0.035, y: 0.045, z: -0.005 },
                        color: { red: 0, green: 0, blue: 255 }
                    },
                    command: {
                        method: "setCurrentColor",
                        parameter: "colorSwatch3.color"
                    },
                    callback: {
                        method: "setColor",
                        parameter: "colorSwatch3.color"
                    }
                },
                {
                    id: "colorSwatch4",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.02, y: 0.02, z: 0.01 },
                        localPosition: { x: -0.01, y: 0.045, z: -0.005 },
                        color: { red: 255, green: 255, blue: 255 }
                    },
                    command: {
                        method: "setCurrentColor",
                        parameter: "colorSwatch4.color"
                    },
                    callback: {
                        method: "setColor",
                        parameter: "colorSwatch4.color"
                    }
                },
                {
                    id: "currentColor",
                    type: "circle",
                    properties: {
                        localPosition: { x: 0.025, y: 0.0325, z: -0.007 }
                    },
                    setting: {
                        key: "VREdit.colorTool.currentColor",
                        property: "color",
                        defaultValue: { red: 128, green: 128, blue: 128 }
                    }
                }
            ]
        },

        GROUP_BUTTON_INDEX = 1,
        UNGROUP_BUTTON_INDEX = 2,

        MENU_ITEMS = [
            {
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
                label: "  SCALE",
                callback: {
                    method: "scaleTool"
                }
            },
            {
                id: "cloneButton",
                type: "button",
                properties: {
                    localPosition: { x: 0.022, y: -0.04, z: -0.005 },
                    color: { red: 240, green: 240, blue: 0 }
                },
                label: "  CLONE",
                callback: {
                    method: "cloneTool"
                }
            },
            {
                id: "groupButton",
                type: "button",
                properties: {
                    localPosition: { x: -0.022, y: 0.0, z: -0.005 },
                    color: { red: 220, green: 60, blue: 220 }
                },
                label: " GROUP",
                toolOptions: "groupOptions",
                callback: {
                    method: "groupTool"
                }
            },
            {
                id: "colorButton",
                type: "button",
                properties: {
                    localPosition: { x: 0.022, y: 0.0, z: -0.005 },
                    color: { red: 220, green: 220, blue: 220 }
                },
                label: "  COLOR",
                toolOptions: "colorOptions",
                callback: {
                    method: "colorTool",
                    parameter: "currentColor.color"
                }
            },
            {
                id: "deleteButton",
                type: "button",
                properties: {
                    localPosition: { x: 0.022, y: 0.04, z: -0.005 },
                    color: { red: 240, green: 60, blue: 60 }
                },
                label: " DELETE",
                callback: {
                    method: "deleteTool"
                }
            }
        ],

        HIGHLIGHT_PROPERTIES = {
            xDelta: 0.004,
            yDelta: 0.004,
            zDimension: 0.001,
            properties: {
                localPosition: { x: 0, y: 0, z: -0.003 },
                localRotation: Quat.ZERO,
                color: { red: 255, green: 255, blue: 0 },
                alpha: 0.8,
                solid: false,
                drawInFront: true,
                ignoreRayIntersection: true,
                visible: false
            }
        },

        NONE = -1,

        optionsItems,
        intersectionOverlays,
        intersectionEnabled,
        highlightedItem,
        highlightedSource,
        isHighlightingButton,
        pressedItem,
        pressedSource,
        isButtonPressed,
        isGroupButtonEnabled,
        isUngroupButtonEnabled,

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
            value,
            i,
            length;

        // Close current panel, if any.
        for (i = 0, length = optionsOverlays.length; i < length; i += 1) {
            Overlays.deleteOverlay(optionsOverlays[i]);
        }
        optionsOverlays = [];

        optionsOverlaysIDs = [];
        optionsEnabled = [];
        optionsItems = null;

        // Open specified panel, if any.
        if (toolOptions) {
            optionsItems = OPTONS_PANELS[toolOptions];
            parentID = menuPanelOverlay;  // Menu panel parents to background panel.
            for (i = 0, length = optionsItems.length; i < length; i += 1) {
                properties = Object.clone(UI_ELEMENTS[optionsItems[i].type].properties);
                properties = Object.merge(properties, optionsItems[i].properties);
                properties.parentID = parentID;
                if (optionsItems[i].setting) {
                    optionsSettings[optionsItems[i].id] = { key: optionsItems[i].setting.key };
                    value = Settings.getValue(optionsItems[i].setting.key);
                    if (value === "") {
                        value = optionsItems[i].setting.defaultValue;
                    }
                    properties[optionsItems[i].setting.property] = value;
                    if (optionsItems[i].setting.callback) {
                        doCallback(optionsItems[i].setting.callback.method, value);
                    }
                }
                optionsOverlays.push(Overlays.addOverlay(UI_ELEMENTS[optionsItems[i].type].overlay, properties));
                optionsOverlaysIDs.push(optionsItems[i].id);
                if (optionsItems[i].label) {
                    properties = Object.clone(UI_ELEMENTS.label.properties);
                    properties.text = optionsItems[i].label;
                    properties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    Overlays.addOverlay(UI_ELEMENTS.label.overlay, properties);
                }
                parentID = optionsOverlays[0];  // Menu buttons parent to menu panel.
                optionsEnabled.push(true);
            }
        }

        // Special handling for Group options.
        if (toolOptions === "groupOptions") {
            optionsEnabled[GROUP_BUTTON_INDEX] = false;
            optionsEnabled[UNGROUP_BUTTON_INDEX] = false;
        }
    }

    function clearTool() {
        openOptions();
    }

    function evaluateParameter(parameter) {
        var parameters,
            overlayID,
            overlayProperty;

        parameters = parameter.split(".");
        overlayID = parameters[0];
        overlayProperty = parameters[1];

        return Overlays.getProperty(optionsOverlays[optionsOverlaysIDs.indexOf(overlayID)], overlayProperty);
    }

    function doCommand(command, parameter) {
        switch (command) {
        case "setCurrentColor":
            Overlays.editOverlay(optionsOverlays[optionsOverlaysIDs.indexOf("currentColor")], {
                color: parameter
            });
            if (optionsSettings.currentColor) {
                Settings.setValue(optionsSettings.currentColor.key, parameter);
            }
            break;
        default:
            // TODO: Log error.
        }

    }

    function update(intersectionOverlayID, groupsCount, entitiesCount) {
        var intersectedItem = -1,
            intersectionItems,
            parentProperties,
            BUTTON_PRESS_DELTA = 0.004,
            parameterValue,
            enableGroupButton,
            enableUngroupButton;

        // Intersection details.
        if (intersectionOverlayID) {
            intersectedItem = menuOverlays.indexOf(intersectionOverlayID);
            if (intersectedItem !== -1) {
                intersectionItems = MENU_ITEMS;
                intersectionOverlays = menuOverlays;
                intersectionEnabled = null;
            } else {
                intersectedItem = optionsOverlays.indexOf(intersectionOverlayID);
                if (intersectedItem !== -1) {
                    intersectionItems = optionsItems;
                    intersectionOverlays = optionsOverlays;
                    intersectionEnabled = optionsEnabled;
                }
            }
        }
        if (!intersectionOverlays) {
            return;
        }

        // Highlight clickable item.
        if (intersectedItem !== highlightedItem || intersectionOverlays !== highlightedSource) {
            if (intersectedItem !== -1 && intersectionItems[intersectedItem].callback !== undefined) {
                // Highlight new button. (The existence of a callback infers that the item is a button.)
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
                if (intersectionItems) {
                    Overlays.editOverlay(intersectionOverlays[pressedItem], {
                        localPosition: intersectionItems[pressedItem].properties.localPosition
                    });
                }
                pressedItem = NONE;
            }
            isButtonPressed = isHighlightingButton && controlHand.triggerClicked();
            if (isButtonPressed && (intersectionEnabled === null || intersectionEnabled[intersectedItem])) {
                // Press new button.
                Overlays.editOverlay(intersectionOverlays[intersectedItem], {
                    localPosition: Vec3.sum(intersectionItems[intersectedItem].properties.localPosition,
                        { x: 0, y: 0, z: BUTTON_PRESS_DELTA })
                });
                pressedItem = intersectedItem;
                pressedSource = intersectionOverlays;

                // Button press actions.
                if (intersectionOverlays === menuOverlays) {
                    openOptions(intersectionItems[intersectedItem].toolOptions);
                }
                if (intersectionItems[intersectedItem].command) {
                    if (intersectionItems[intersectedItem].command.parameter) {
                        parameterValue = evaluateParameter(intersectionItems[intersectedItem].command.parameter);
                    }
                    doCommand(intersectionItems[intersectedItem].command.method, parameterValue);
                }
                if (intersectionItems[intersectedItem].callback) {
                    if (intersectionItems[intersectedItem].callback.parameter) {
                        parameterValue = evaluateParameter(intersectionItems[intersectedItem].callback.parameter);
                    }
                    doCallback(intersectionItems[intersectedItem].callback.method, parameterValue);
                }
            }
        }

        // Special handling for Group options.
        if (optionsItems && optionsItems === OPTONS_PANELS.groupOptions) {
            enableGroupButton = groupsCount > 1;
            if (enableGroupButton !== isGroupButtonEnabled) {
                isGroupButtonEnabled = enableGroupButton;
                Overlays.editOverlay(optionsOverlays[GROUP_BUTTON_INDEX], {
                    color: isGroupButtonEnabled
                        ? OPTONS_PANELS.groupOptions[GROUP_BUTTON_INDEX].enabledColor
                        : OPTONS_PANELS.groupOptions[GROUP_BUTTON_INDEX].properties.color
                });
                optionsEnabled[GROUP_BUTTON_INDEX] = enableGroupButton;
            }

            enableUngroupButton = groupsCount === 1 && entitiesCount > 1;
            if (enableUngroupButton !== isUngroupButtonEnabled) {
                isUngroupButtonEnabled = enableUngroupButton;
                Overlays.editOverlay(optionsOverlays[UNGROUP_BUTTON_INDEX], {
                    color: isUngroupButtonEnabled
                        ? OPTONS_PANELS.groupOptions[UNGROUP_BUTTON_INDEX].enabledColor
                        : OPTONS_PANELS.groupOptions[UNGROUP_BUTTON_INDEX].properties.color
                });
                optionsEnabled[UNGROUP_BUTTON_INDEX] = enableUngroupButton;
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
            if (MENU_ITEMS[i].label) {
                properties = Object.clone(UI_ELEMENTS.label.properties);
                properties.text = MENU_ITEMS[i].label;
                properties.parentID = menuOverlays[menuOverlays.length - 1];
                Overlays.addOverlay(UI_ELEMENTS.label.overlay, properties);
            }
            parentID = menuOverlays[0];  // Menu buttons parent to menu panel.
        }

        // Prepare highlight overlay.
        properties = Object.clone(HIGHLIGHT_PROPERTIES);
        properties.parentID = menuOriginOverlay;
        highlightOverlay = Overlays.addOverlay("cube", properties);

        // Initial values.
        optionsItems = null;
        intersectionOverlays = null;
        intersectionEnabled = null;
        highlightedItem = NONE;
        highlightedSource = null;
        isHighlightingButton = false;
        pressedItem = NONE;
        pressedSource = null;
        isButtonPressed = false;
        isGroupButtonEnabled = false;
        isUngroupButtonEnabled = false;

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

        for (i = 0, length = menuOverlays.length; i < length; i += 1) {
            Overlays.deleteOverlay(menuOverlays[i]);
        }
        menuOverlays = [];

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
