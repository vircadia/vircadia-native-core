//
//  toolMenu.js
//
//  Created by David Rowe on 22 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global App, ToolMenu */

ToolMenu = function (side, leftInputs, rightInputs, uiCommandCallback) {
    // Tool menu displayed on top of forearm.

    "use strict";

    var attachmentJointName,

        menuOriginOverlay,
        menuPanelOverlay,

        menuOverlays = [],

        optionsOverlays = [],
        optionsOverlaysIDs = [],  // Text ids (names) of options overlays.
        optionsOverlaysLabels = [],  // Overlay IDs of labels for optionsOverlays.
        optionsSliderData = [],  // Uses same index values as optionsOverlays.
        optionsEnabled = [],
        optionsSettings = {},

        highlightOverlay,

        LEFT_HAND = 0,

        CANVAS_SIZE = { x: 0.22, y: 0.13 },
        PANEL_ORIGIN_POSITION = { x: -0.005 - CANVAS_SIZE.x / 2, y: 0.15, z: -CANVAS_SIZE.x / 2 },
        PANEL_ROOT_ROTATION = Quat.fromVec3Degrees({ x: 0, y: -90, z: 180 }),
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

        NO_SWATCH_COLOR = { red: 128, green: 128, blue: 128 },

        UI_BASE_COLOR = { red: 64, green: 64, blue: 64 },
        UI_HIGHLIGHT_COLOR = { red: 100, green: 240, blue: 100 },

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
            "toggleButton": {
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.03, y: 0.03, z: 0.01 },
                    localRotation: Quat.ZERO,
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                },
                onColor: UI_HIGHLIGHT_COLOR,
                offColor: UI_BASE_COLOR
            },
            "swatch": {
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.03, y: 0.03, z: 0.01 },
                    localRotation: Quat.ZERO,
                    color: NO_SWATCH_COLOR,
                    alpha: 1.0,
                    solid: false,  // False indicates "no swatch color assigned"
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
                    color: { red: 240, green: 240, blue: 240 },
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
                    size: 0.01,
                    localPosition: { x: 0.0, y: 0.0, z: -0.01 },
                    localRotation: Quat.fromVec3Degrees({ x: 0, y: 180, z: 180 }),
                    color: { red: 128, green: 128, blue: 128 },
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: true,
                    visible: true
                }
            },
            "image": {
                overlay: "image3d",
                properties: {
                    dimensions: { x: 0.1, y: 0.1 },
                    localPosition: { x: 0, y: 0, z: 0 },
                    localRotation: Quat.ZERO,
                    color: { red: 255, green: 255, blue: 255 },
                    alpha: 1.0,
                    ignoreRayIntersection: true,
                    isFacingAvatar: false,
                    visible: true
                }
            },
            "barSlider": {  // Values range between 0.0 and 1.0.
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.02, y: 0.1, z: 0.01 },
                    localRotation: Quat.ZERO,
                    color: { red: 128, green: 128, blue: 128 },
                    alpha: 0.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                }
            },
            "barSliderValue": {
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.02, y: 0.03, z: 0.01 },
                    localPosition: { x: 0, y: 0.035, z: 0 },
                    localRotation: Quat.ZERO,
                    color: UI_HIGHLIGHT_COLOR,
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: true,
                    visible: true
                }
            },
            "barSliderRemainder": {
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.02, y: 0.07, z: 0.01 },
                    localPosition: { x: 0, y: -0.015, z: 0 },
                    localRotation: Quat.ZERO,
                    color: UI_BASE_COLOR,
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: true,
                    visible: true
                }
            },
            "imageSlider": {  // Values range between 0.0 and 1.0.
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.01, y: 0.05, z: 0.01 },
                    localRotation: Quat.ZERO,
                    color: { red: 128, green: 128, blue: 128 },
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                },
                useBaseColor: false,
                imageURL: null,
                imageOverlayURL: null
            },
            "sliderPointer": {
                overlay: "shape",
                properties: {
                    shape: "Cone",
                    dimensions: { x: 0.005, y: 0.005, z: 0.005 },
                    localRotation: Quat.fromVec3Degrees({ x: 0, y: 0, z: -90 }),
                    color: { red: 180, green: 180, blue: 180 },
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: true,
                    visible: true
                }
            },
            "picklist": {
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.06, y: 0.02, z: 0.01 },
                    localRotation: Quat.ZERO,
                    color: UI_BASE_COLOR,
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                }
            },
            "picklistItem": {
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.06, y: 0.02, z: 0.01 },
                    localPosition: Vec3.ZERO,
                    localRotation: Quat.ZERO,
                    color: { red: 100, green: 100, blue: 100 },
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: false
                }
            }
        },

        BUTTON_UI_ELEMENTS = ["button", "toggleButton", "swatch"],
        BUTTON_PRESS_DELTA = { x: 0, y: 0, z: 0.004 },

        SLIDER_UI_ELEMENTS = ["barSlider", "imageSlider"],
        SLIDER_RAISE_DELTA = { x: 0, y: 0, z: 0.004 },
        MIN_BAR_SLIDER_DIMENSION = 0.0001,  // Avoid visual artifact for 0 slider values.

        PICKLIST_UI_ELEMENTS = ["picklist", "picklistItem"],
        PICKLIST_RAISE_DELTA = { x: 0, y: 0, z: 0.004 },

        PHYSICS_SLIDER_PRESETS = {
            // Slider values in the range 0.0 to 1.0.
            // Note: Damping values give the desired linear and angular damping values but friction values are a somewhat out,
            // especially for the balloon.
            presetDefault:    { gravity: 0.5,      bounce: 0.5,  damping: 0.5,      density: 0.5      },
            presetLead:       { gravity: 0.5,      bounce: 0.0,  damping: 0.5,      density: 1.0      },
            presetWood:       { gravity: 0.5,      bounce: 0.4,  damping: 0.5,      density: 0.5      },
            presetIce:        { gravity: 0.5,      bounce: 0.99, damping: 0.151004, density: 0.349485 },
            presetRubber:     { gravity: 0.5,      bounce: 0.99, damping: 0.5,      density: 0.5      },
            presetCotton:     { gravity: 0.587303, bounce: 0.0,  damping: 0.931878, density: 0.0      },
            presetTumbleweed: { gravity: 0.595893, bounce: 0.7,  damping: 0.5,      density: 0.0      },
            presetZeroG:      { gravity: 0.596844, bounce: 0.5,  damping: 0.5,      density: 0.5      },
            presetBalloon:    { gravity: 0.606313, bounce: 0.99, damping: 0.151004, density: 0.0      }
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
                    id: "colorSlider",
                    type: "imageSlider",
                    properties: {
                        localPosition: { x: 0.025, y: -0.025, z: -0.005 },
                        color: { red: 255, green: 0, blue: 0 }
                    },
                    useBaseColor: true,
                    imageURL: "../assets/slider-white.png",
                    imageOverlayURL: "../assets/slider-white-alpha.png",
                    callback: {
                        method: "setSliderValue"
                    }
                },
                {
                    id: "colorSwatch1",
                    type: "swatch",
                    properties: {
                        dimensions: { x: 0.02, y: 0.02, z: 0.01 },
                        localPosition: { x: -0.035, y: 0.02, z: -0.005 }
                    },
                    setting: {
                        key: "VREdit.colorTool.swatch1Color",
                        property: "color",
                        defaultValue: { red: 0, green: 255, blue: 0 }
                    },
                    command: {
                        method: "setColorPerSwatch"
                    },
                    clear: {
                        method: "clearSwatch"
                    }
                },
                {
                    id: "colorSwatch2",
                    type: "swatch",
                    properties: {
                        dimensions: { x: 0.02, y: 0.02, z: 0.01 },
                        localPosition: { x: -0.01, y: 0.02, z: -0.005 }
                    },
                    setting: {
                        key: "VREdit.colorTool.swatch2Color",
                        property: "color",
                        defaultValue: { red: 0, green: 0, blue: 255 }
                    },
                    command: {
                        method: "setColorPerSwatch"
                    },
                    clear: {
                        method: "clearSwatch"
                    }
                },
                {
                    id: "colorSwatch3",
                    type: "swatch",
                    properties: {
                        dimensions: { x: 0.02, y: 0.02, z: 0.01 },
                        localPosition: { x: -0.035, y: 0.045, z: -0.005 }
                    },
                    setting: {
                        key: "VREdit.colorTool.swatch3Color",
                        property: "color"
                        // Default to empty swatch.
                    },
                    command: {
                        method: "setColorPerSwatch"
                    },
                    clear: {
                        method: "clearSwatch"
                    }
                },
                {
                    id: "colorSwatch4",
                    type: "swatch",
                    properties: {
                        dimensions: { x: 0.02, y: 0.02, z: 0.01 },
                        localPosition: { x: -0.01, y: 0.045, z: -0.005 }
                    },
                    setting: {
                        key: "VREdit.colorTool.swatch4Color",
                        property: "color"
                        // Default to empty swatch.
                    },
                    command: {
                        method: "setColorPerSwatch"
                    },
                    clear: {
                        method: "clearSwatch"
                    }
                },
                {
                    id: "currentColor",
                    type: "circle",
                    properties: {
                        localPosition: { x: 0.025, y: 0.02, z: -0.007 }
                    },
                    setting: {
                        key: "VREdit.colorTool.currentColor",
                        property: "color",
                        defaultValue: { red: 128, green: 128, blue: 128 }
                    }
                },
                {
                    id: "pickColor",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.04, y: 0.02, z: 0.01 },
                        localPosition: { x: 0.025, y: 0.045, z: -0.005 },
                        color: { red: 255, green: 255, blue: 255 }
                    },
                    label: "    PICK",
                    callback: {
                        method: "pickColorTool"
                    }
                }
            ],
            physicsOptions: [
                {
                    id: "physicsOptionsPanel",
                    type: "panel",
                    properties: {
                        localPosition: { x: 0.055, y: 0.0, z: -0.005 }
                    }
                },

                {
                    id: "propertiesLabel",
                    type: "label",
                    properties: {
                        text: "PROPERTIES",
                        lineHeight: 0.0045,
                        localPosition: { x: -0.031, y: -0.0475, z: -0.0075}
                    }
                },
                {
                    id: "gravityToggle",
                    type: "toggleButton",
                    properties: {
                        localPosition: { x: -0.0325, y: -0.03, z: -0.005 },
                        dimensions: { x: 0.03, y: 0.02, z: 0.01 }
                    },
                    label: "GRAVITY",
                    setting: {
                        key: "VREdit.physicsTool.gravityOn",
                        defaultValue: false,
                        callback: "setGravityOn"
                    },
                    command: {
                        method: "setGravityOn"
                    }
                },
                {
                    id: "grabToggle",
                    type: "toggleButton",
                    properties: {
                        localPosition: { x: -0.0325, y: -0.005, z: -0.005 },
                        dimensions: { x: 0.03, y: 0.02, z: 0.01 }
                    },
                    label: "  GRAB",
                    setting: {
                        key: "VREdit.physicsTool.grabOn",
                        defaultValue: false,
                        callback: "setGrabOn"
                    },
                    command: {
                        method: "setGrabOn"
                    }
                },
                {
                    id: "collideToggle",
                    type: "toggleButton",
                    properties: {
                        localPosition: { x: -0.0325, y: 0.02, z: -0.005 },
                        dimensions: { x: 0.03, y: 0.02, z: 0.01 }
                    },
                    label: "COLLIDE",
                    setting: {
                        key: "VREdit.physicsTool.collideOn",
                        defaultValue: false,
                        callback: "setCollideOn"
                    },
                    command: {
                        method: "setCollideOn"
                    }
                },

                {
                    id: "propertiesLabel",
                    type: "label",
                    properties: {
                        text: "PRESETS",
                        lineHeight: 0.0045,
                        localPosition: { x: 0.002, y: -0.0475, z: -0.0075 }
                    }
                },
                {
                    id: "presets",
                    type: "picklist",
                    properties: {
                        localPosition: { x: 0.016, y: -0.03, z: -0.005 },
                        dimensions: { x: 0.06, y: 0.02, z: 0.01 }
                    },
                    label: "DEFAULT",
                    setting: {
                        key: "VREdit.physicsTool.presetLabel"
                    },
                    command: {
                        method: "togglePhysicsPresets"
                    },
                    items: [
                        "presetDefault",
                        "presetLead",
                        "presetWood",
                        "presetIce",
                        "presetRubber",
                        "presetCotton",
                        "presetTumbleweed",
                        "presetZeroG",
                        "presetBalloon"
                    ]
                },
                {
                    id: "presetDefault",
                    type: "picklistItem",
                    label: "DEFAULT",
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetLead",
                    type: "picklistItem",
                    label: "LEAD",
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetWood",
                    type: "picklistItem",
                    label: "WOOD",
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetIce",
                    type: "picklistItem",
                    label: "ICE",
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetRubber",
                    type: "picklistItem",
                    label: "RUBBER",
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetCotton",
                    type: "picklistItem",
                    label: "COTTON",
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetTumbleweed",
                    type: "picklistItem",
                    label: "TUMBLEWEED",
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetZeroG",
                    type: "picklistItem",
                    label: "ZERO-G",
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetBalloon",
                    type: "picklistItem",
                    label: "BALLOON",
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "gravitySlider",
                    type: "barSlider",
                    properties: {
                        localPosition: { x: -0.007, y: 0.016, z: -0.005 },
                        dimensions: { x: 0.014, y: 0.06, z: 0.01 }
                    },
                    setting: {
                        key: "VREdit.physicsTool.gravity",
                        defaultValue: 0.5,
                        callback: "setGravity"
                    },
                    command: {
                        method: "setGravity"
                    }
                },
                {
                    id: "gravityLabel",
                    type: "label",
                    properties: {
                        text: "GRAVITY",
                        lineHeight: 0.0045,
                        localPosition: { x: -0.003, y: 0.052, z: -0.0075 }
                    }
                },
                {
                    id: "bounceSlider",
                    type: "barSlider",
                    properties: {
                        localPosition: { x: 0.009, y: 0.016, z: -0.005 },
                        dimensions: { x: 0.014, y: 0.06, z: 0.01 }
                    },
                    setting: {
                        key: "VREdit.physicsTool.bounce",
                        defaultValue: 0.5,
                        callback: "setBounce"
                    },
                    command: {
                        method: "setBounce"
                    }
                },
                {
                    id: "bounceLabel",
                    type: "label",
                    properties: {
                        text: "BOUNCE",
                        lineHeight: 0.0045,
                        localPosition: { x: 0.015, y: 0.057, z: -0.0075 }
                    }
                },
                {
                    id: "dampingSlider",
                    type: "barSlider",
                    properties: {
                        localPosition: { x: 0.024, y: 0.016, z: -0.005 },
                        dimensions: { x: 0.014, y: 0.06, z: 0.01 }
                    },
                    setting: {
                        key: "VREdit.physicsTool.damping",
                        defaultValue: 0.5,
                        callback: "setDamping"
                    },
                    command: {
                        method: "setDamping"
                    }
                },
                {
                    id: "dampingLabel",
                    type: "label",
                    properties: {
                        text: "DAMPING",
                        lineHeight: 0.0045,
                        localPosition: { x: 0.030, y: 0.052, z: -0.0075 }
                    }
                },
                {
                    id: "densitySlider",
                    type: "barSlider",
                    properties: {
                        localPosition: { x: 0.039, y: 0.016, z: -0.005 },
                        dimensions: { x: 0.014, y: 0.06, z: 0.01 }
                    },
                    setting: {
                        key: "VREdit.physicsTool.density",
                        defaultValue: 0.5,
                        callback: "setDensity"
                    },
                    command: {
                        method: "setDensity"
                    }
                },
                {
                    id: "densityLabel",
                    type: "label",
                    properties: {
                        text: "DENSITY",
                        lineHeight: 0.0045,
                        localPosition: { x: 0.045, y: 0.057, z: -0.0075 }
                    }
                }
            ]
        },

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
                id: "physicsButton",
                type: "button",
                properties: {
                    localPosition: { x: -0.022, y: 0.04, z: -0.005 },
                    color: { red: 60, green: 60, blue: 240 }
                },
                label: "PHYSICS",
                toolOptions: "physicsOptions",
                callback: {
                    method: "physicsTool"
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
        highlightedItems,
        highlightedSource,
        isHighlightingButton,
        isHighlightingSlider,
        isHighlightingPicklist,
        isPicklistOpen,
        pressedItem = null,
        pressedSource,
        isButtonPressed,
        isPicklistPressed,
        isPicklistItemPressed,
        isGripClicked,

        isGroupButtonEnabled,
        isUngroupButtonEnabled,
        groupButtonIndex,
        ungroupButtonIndex,

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

    function closeOptions() {
        var i,
            length;

        Overlays.editOverlay(highlightOverlay, {
            parentID: menuOriginOverlay
        });
        for (i = 0, length = optionsOverlays.length; i < length; i += 1) {
            Overlays.deleteOverlay(optionsOverlays[i]);
        }
        optionsOverlays = [];

        optionsOverlaysIDs = [];
        optionsOverlaysLabels = [];
        optionsSliderData = [];
        optionsEnabled = [];
        optionsItems = null;

        isPicklistOpen = false;
    }

    function openOptions(toolOptions) {
        var properties,
            childProperties,
            auxiliaryProperties,
            parentID,
            value,
            imageOffset,
            IMAGE_OFFSET = 0.0005,
            id,
            i,
            length;

        // Close current panel, if any.
        closeOptions();

        // TODO: Remove once all tools have an options panel.
        if (OPTONS_PANELS[toolOptions] === undefined) {
            return;
        }

        // Open specified panel.
        optionsItems = OPTONS_PANELS[toolOptions];
        parentID = menuPanelOverlay;  // Menu panel parents to background panel.
        for (i = 0, length = optionsItems.length; i < length; i += 1) {
            properties = Object.clone(UI_ELEMENTS[optionsItems[i].type].properties);
            if (optionsItems[i].properties) {
                properties = Object.merge(properties, optionsItems[i].properties);
            }
            properties.parentID = parentID;
            if (properties.url) {
                properties.url = Script.resolvePath(properties.url);
            }
            if (optionsItems[i].setting) {
                optionsSettings[optionsItems[i].id] = { key: optionsItems[i].setting.key };
                value = Settings.getValue(optionsItems[i].setting.key);
                if (value === "" && optionsItems[i].setting.defaultValue !== undefined) {
                    value = optionsItems[i].setting.defaultValue;
                }
                if (value !== "") {
                    properties[optionsItems[i].setting.property] = value;
                    if (optionsItems[i].type === "swatch") {
                        // Special case for when swatch color is defined.
                        properties.solid = true;
                    }
                    if (optionsItems[i].type === "toggleButton") {
                        // Store value in optionsSettings rather than using overlay property.
                        optionsSettings[optionsItems[i].id].value = value;
                        properties.color = value
                            ? UI_ELEMENTS[optionsItems[i].type].onColor
                            : UI_ELEMENTS[optionsItems[i].type].offColor;
                    }
                    if (optionsItems[i].type === "barSlider") {
                        // Store value in optionsSettings rather than using overlay property.
                        optionsSettings[optionsItems[i].id].value = value;
                    }
                    if (optionsItems[i].type === "picklist") {
                        optionsSettings[optionsItems[i].id].value = value;
                        optionsItems[i].label = value;
                    }
                    if (optionsItems[i].setting.callback) {
                        uiCommandCallback(optionsItems[i].setting.callback, value);
                    }
                }
            }
            optionsOverlays.push(Overlays.addOverlay(UI_ELEMENTS[optionsItems[i].type].overlay, properties));
            optionsOverlaysIDs.push(optionsItems[i].id);
            if (optionsItems[i].label) {
                properties = Object.clone(UI_ELEMENTS.label.properties);
                properties.text = optionsItems[i].label;
                properties.parentID = optionsOverlays[optionsOverlays.length - 1];
                properties.visible = optionsItems[i].type !== "picklistItem";
                id = Overlays.addOverlay(UI_ELEMENTS.label.overlay, properties);
                optionsOverlaysLabels[i] = id;
            }

            if (optionsItems[i].type === "barSlider") {
                optionsSliderData[i] = {};
                auxiliaryProperties = Object.clone(UI_ELEMENTS.barSliderValue.properties);
                auxiliaryProperties.localPosition = { x: 0, y: (0.5 - value / 2) * properties.dimensions.y, z: 0 };
                auxiliaryProperties.dimensions = {
                    x: properties.dimensions.x,
                    y: Math.max(value * properties.dimensions.y, MIN_BAR_SLIDER_DIMENSION),
                    z: properties.dimensions.z
                };
                auxiliaryProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                optionsSliderData[i].value = Overlays.addOverlay(UI_ELEMENTS.barSliderValue.overlay,
                    auxiliaryProperties);
                auxiliaryProperties = Object.clone(UI_ELEMENTS.barSliderRemainder.properties);
                auxiliaryProperties.localPosition = { x: 0, y: (-0.5 + (1.0 - value) / 2) * properties.dimensions.y, z: 0 };
                auxiliaryProperties.dimensions = {
                    x: properties.dimensions.x,
                    y: Math.max((1.0 - value) * properties.dimensions.y, MIN_BAR_SLIDER_DIMENSION),
                    z: properties.dimensions.z
                };
                auxiliaryProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                optionsSliderData[i].remainder = Overlays.addOverlay(UI_ELEMENTS.barSliderRemainder.overlay,
                    auxiliaryProperties);
            }

            if (optionsItems[i].type === "imageSlider") {
                imageOffset = 0.0;

                // Primary image.
                if (optionsItems[i].imageURL) {
                    childProperties = Object.clone(UI_ELEMENTS.image.properties);
                    childProperties.url = Script.resolvePath(optionsItems[i].imageURL);
                    delete childProperties.dimensions;
                    childProperties.scale = properties.dimensions.y;
                    imageOffset += IMAGE_OFFSET;
                    childProperties.emissive = true;
                    if (optionsItems[i].useBaseColor) {
                        childProperties.color = properties.color;
                    }
                    childProperties.localPosition = { x: 0, y: 0, z: -properties.dimensions.z / 2 - imageOffset };
                    childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                }

                // Overlay image.
                if (optionsItems[i].imageOverlayURL) {
                    childProperties = Object.clone(UI_ELEMENTS.image.properties);
                    childProperties.url = Script.resolvePath(optionsItems[i].imageOverlayURL);
                    childProperties.drawInFront = true;  // TODO: Work-around for rendering bug; remove when bug fixed.
                    delete childProperties.dimensions;
                    childProperties.scale = properties.dimensions.y;
                    imageOffset += IMAGE_OFFSET;
                    childProperties.localPosition = { x: 0, y: 0, z: -properties.dimensions.z / 2 - imageOffset };
                    childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                }

                // Value pointers.
                optionsSliderData[i] = {};
                optionsSliderData[i].offset =
                    { x: -properties.dimensions.x / 2, y: 0, z: -properties.dimensions.z / 2 - imageOffset };
                auxiliaryProperties = Object.clone(UI_ELEMENTS.sliderPointer.properties);
                auxiliaryProperties.localPosition = optionsSliderData[i].offset;
                auxiliaryProperties.drawInFront = true;  // TODO: Accommodate work-around above; remove when bug fixed.
                auxiliaryProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                optionsSliderData[i].value = Overlays.addOverlay(UI_ELEMENTS.sliderPointer.overlay,
                    auxiliaryProperties);
                auxiliaryProperties.localPosition = { x: 0, y: properties.dimensions.x, z: 0 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 180 });
                auxiliaryProperties.parentID = optionsSliderData[i].value;
                Overlays.addOverlay(UI_ELEMENTS.sliderPointer.overlay, auxiliaryProperties);
            }
            parentID = optionsOverlays[0];  // Menu buttons parent to menu panel.
            optionsEnabled.push(true);
        }

        // Special handling for Group options.
        if (toolOptions === "groupOptions") {
            optionsEnabled[groupButtonIndex] = false;
            optionsEnabled[ungroupButtonIndex] = false;
        }
    }

    function clearTool() {
        closeOptions();
    }

    function setPresetsLabelToCustom() {
        var CUSTOM = "CUSTOM";
        if (optionsSettings.presets.value !== CUSTOM) {
            optionsSettings.presets.value = CUSTOM;
            Overlays.editOverlay(optionsOverlaysLabels[optionsOverlaysIDs.indexOf("presets")], {
                text: CUSTOM
            });
            Settings.setValue(optionsSettings.presets.key, CUSTOM);
        }
    }

    function setBarSliderValue(item, fraction) {
        var overlayDimensions,
            otherFraction;

        overlayDimensions = optionsItems[item].properties.dimensions;
        if (overlayDimensions === undefined) {
            overlayDimensions = UI_ELEMENTS.barSlider.properties.dimensions;
        }

        otherFraction = 1.0 - fraction;

        Overlays.editOverlay(optionsSliderData[item].value, {
            localPosition: { x: 0, y: (0.5 - fraction / 2) * overlayDimensions.y, z: 0 },
            dimensions: {
                x: overlayDimensions.x,
                y: Math.max(fraction * overlayDimensions.y, MIN_BAR_SLIDER_DIMENSION),
                z: overlayDimensions.z
            }
        });
        Overlays.editOverlay(optionsSliderData[item].remainder, {
            localPosition: { x: 0, y: (-0.5 + otherFraction / 2) * overlayDimensions.y, z: 0 },
            dimensions: {
                x: overlayDimensions.x,
                y: Math.max(otherFraction * overlayDimensions.y, MIN_BAR_SLIDER_DIMENSION),
                z: overlayDimensions.z
            }
        });
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
        var index,
            hasColor,
            value,
            items,
            parentID,
            label,
            values,
            i,
            length;

        switch (command) {

        case "setColorPerSwatch":
            index = optionsOverlaysIDs.indexOf(parameter);
            hasColor = Overlays.getProperty(optionsOverlays[index], "solid");
            if (hasColor) {
                value = Overlays.getProperty(optionsOverlays[index], "color");
                Overlays.editOverlay(optionsOverlays[optionsOverlaysIDs.indexOf("currentColor")], {
                    color: value
                });
                if (optionsSettings.currentColor) {
                    Settings.setValue(optionsSettings.currentColor.key, value);
                }
                uiCommandCallback("setColor", value);
            } else {
                // Swatch has no color; set swatch color to current fill color.
                value = Overlays.getProperty(optionsOverlays[optionsOverlaysIDs.indexOf("currentColor")], "color");
                Overlays.editOverlay(optionsOverlays[index], {
                    color: value,
                    solid: true
                });
                if (optionsSettings[parameter]) {
                    Settings.setValue(optionsSettings[parameter].key, value);
                }
            }
            break;

        case "setColorFromPick":
            Overlays.editOverlay(optionsOverlays[optionsOverlaysIDs.indexOf("currentColor")], {
                color: parameter
            });
            if (optionsSettings.currentColor) {
                Settings.setValue(optionsSettings.currentColor.key, parameter);
            }
            break;

        case "setGravityOn":
        case "setGrabOn":
        case "setCollideOn":
            value = !optionsSettings[parameter].value;
            optionsSettings[parameter].value = value;
            Settings.setValue(optionsSettings[parameter].key, value);
            index = optionsOverlaysIDs.indexOf(parameter);
            Overlays.editOverlay(optionsOverlays[index], {
                color: value ? UI_ELEMENTS[optionsItems[index].type].onColor : UI_ELEMENTS[optionsItems[index].type].offColor
            });
            uiCommandCallback(command, value);
            break;

        case "togglePhysicsPresets":
            if (isPicklistOpen) {
                // Close picklist.
                index = optionsOverlaysIDs.indexOf(parameter);

                // Lower picklist.
                Overlays.editOverlay(optionsOverlays[index], {
                    localPosition: optionsItems[index].properties.localPosition
                });

                // Hide options.
                items = optionsItems[index].items;
                for (i = 0, length = items.length; i < length; i += 1) {
                    index = optionsOverlaysIDs.indexOf(items[i]);
                    Overlays.editOverlay(optionsOverlays[index], {
                        localPosition: Vec3.ZERO,
                        visible: false
                    });
                    Overlays.editOverlay(optionsOverlaysLabels[index], {
                        visible: false
                    });
                }
            }

            isPicklistOpen = !isPicklistOpen;

            if (isPicklistOpen) {
                // Open picklist.
                index = optionsOverlaysIDs.indexOf(parameter);
                parentID = optionsOverlays[index];

                // Raise picklist.
                Overlays.editOverlay(parentID, {
                    localPosition: Vec3.subtract(optionsItems[index].properties.localPosition, PICKLIST_RAISE_DELTA)
                });

                // Show options.
                items = optionsItems[index].items;
                for (i = 0, length = items.length; i < length; i += 1) {
                    index = optionsOverlaysIDs.indexOf(items[i]);
                    Overlays.editOverlay(optionsOverlays[index], {
                        parentID: parentID,
                        localPosition: { x: 0, y: (i + 1) * -UI_ELEMENTS.picklistItem.properties.dimensions.y, z: 0 },
                        visible: true
                    });
                    Overlays.editOverlay(optionsOverlaysLabels[index], {
                        visible: true
                    });
                }
            }
            break;

        case "pickPhysicsPreset":
            // Close picklist.
            doCommand("togglePhysicsPresets", "presets");

            // Update picklist label.
            label = optionsItems[optionsOverlaysIDs.indexOf(parameter)].label;
            optionsSettings.presets.value = label;
            Overlays.editOverlay(optionsOverlaysLabels[optionsOverlaysIDs.indexOf("presets")], {
                text: label
            });
            Settings.setValue(optionsSettings.presets.key, label);

            // Update sliders.
            values = PHYSICS_SLIDER_PRESETS[parameter];
            setBarSliderValue(optionsOverlaysIDs.indexOf("gravitySlider"), values.gravity);
            Settings.setValue(optionsSettings.gravitySlider.key, values.gravity);
            uiCommandCallback("setGravity", values.gravity);
            setBarSliderValue(optionsOverlaysIDs.indexOf("bounceSlider"), values.bounce);
            Settings.setValue(optionsSettings.bounceSlider.key, values.bounce);
            uiCommandCallback("setBounce", values.bounce);
            setBarSliderValue(optionsOverlaysIDs.indexOf("dampingSlider"), values.damping);
            Settings.setValue(optionsSettings.dampingSlider.key, values.damping);
            uiCommandCallback("setDamping", values.damping);
            setBarSliderValue(optionsOverlaysIDs.indexOf("densitySlider"), values.density);
            Settings.setValue(optionsSettings.densitySlider.key, values.density);
            uiCommandCallback("setDensity", values.density);

            break;

        case "setGravity":
            setPresetsLabelToCustom();
            Settings.setValue(optionsSettings.gravitySlider.key, parameter);
            uiCommandCallback("setGravity", parameter);
            break;
        case "setBounce":
            setPresetsLabelToCustom();
            Settings.setValue(optionsSettings.bounceSlider.key, parameter);
            uiCommandCallback("setBounce", parameter);
            break;
        case "setDamping":
            setPresetsLabelToCustom();
            Settings.setValue(optionsSettings.dampingSlider.key, parameter);
            uiCommandCallback("setDamping", parameter);
            break;
        case "setDensity":
            setPresetsLabelToCustom();
            Settings.setValue(optionsSettings.densitySlider.key, parameter);
            uiCommandCallback("setDensity", parameter);
            break;

        default:
            App.log(side, "ERROR: ToolMenu: Unexpected command! " + command);
        }
    }

    function doGripClicked(command, parameter) {
        var overlayID;
        switch (command) {
        case "clearSwatch":
            overlayID = optionsOverlaysIDs.indexOf(parameter);
            Overlays.editOverlay(optionsOverlays[overlayID], {
                color: NO_SWATCH_COLOR,
                solid: false
            });
            if (optionsSettings[parameter]) {
                Settings.setValue(optionsSettings[parameter].key, null);  // Deleted settings value.
            }
            break;
        default:
            App.log(side, "ERROR: ToolMenu: Unexpected command! " + command);
        }
    }

    function adjustSliderFraction(fraction) {
        // Makes slider values achieve and saturate at 0.0 and 1.0.
        return Math.min(1.0, Math.max(0.0, fraction * 1.01 - 0.005));
    }

    function update(intersection, groupsCount, entitiesCount) {
        var intersectedItem = NONE,
            intersectionItems,
            parentProperties,
            localPosition,
            parameter,
            parameterValue,
            enableGroupButton,
            enableUngroupButton,
            sliderProperties,
            overlayDimensions,
            basePoint,
            fraction;

        // Intersection details.
        if (intersection.overlayID) {
            intersectedItem = menuOverlays.indexOf(intersection.overlayID);
            if (intersectedItem !== NONE) {
                intersectionItems = MENU_ITEMS;
                intersectionOverlays = menuOverlays;
                intersectionEnabled = null;
            } else {
                intersectedItem = optionsOverlays.indexOf(intersection.overlayID);
                if (intersectedItem !== NONE) {
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
            if (intersectedItem !== NONE && (intersectionItems[intersectedItem].command !== undefined
                    || intersectionItems[intersectedItem].callback !== undefined)) {
                // Highlight new button. (The existence of a command or callback infers that the item is a button.)
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
                // Lower old slider.
                if (isHighlightingSlider) {
                    localPosition = highlightedItems[highlightedItem].properties.localPosition;
                    Overlays.editOverlay(highlightedSource[highlightedItem], {
                        localPosition: localPosition
                    });
                }
                // Update status variables.
                highlightedItem = intersectedItem;
                highlightedItems = intersectionItems;
                isHighlightingButton = BUTTON_UI_ELEMENTS.indexOf(intersectionItems[highlightedItem].type) !== NONE;
                isHighlightingSlider = SLIDER_UI_ELEMENTS.indexOf(intersectionItems[highlightedItem].type) !== NONE;
                isHighlightingPicklist = PICKLIST_UI_ELEMENTS.indexOf(intersectionItems[highlightedItem].type) !== NONE;
                // Raise new slider.
                if (isHighlightingSlider) {
                    localPosition = intersectionItems[highlightedItem].properties.localPosition;
                    Overlays.editOverlay(intersectionOverlays[highlightedItem], {
                        localPosition: Vec3.subtract(localPosition, SLIDER_RAISE_DELTA)
                    });
                }
            } else if (highlightedItem !== NONE) {
                // Un-highlight previous button.
                Overlays.editOverlay(highlightOverlay, {
                    visible: false
                });
                // Lower slider.
                if (isHighlightingSlider) {
                    localPosition = highlightedItems[highlightedItem].properties.localPosition;
                    Overlays.editOverlay(highlightedSource[highlightedItem], {
                        localPosition: localPosition
                    });
                }
                // Update status variables.
                highlightedItem = NONE;
                isHighlightingButton = false;
                isHighlightingSlider = false;
                isHighlightingPicklist = false;
            }
            highlightedSource = intersectionOverlays;
        }

        // Press/unpress button.
        if ((pressedItem && intersectedItem !== pressedItem.index) || intersectionOverlays !== pressedSource
                || controlHand.triggerClicked() !== isButtonPressed) {
            if (pressedItem) {
                // Unpress previous button.
                Overlays.editOverlay(intersectionOverlays[pressedItem.index], {
                    localPosition: pressedItem.localPosition
                });
                pressedItem = null;
            }
            isButtonPressed = isHighlightingButton && controlHand.triggerClicked();
            if (isButtonPressed && (intersectionEnabled === null || intersectionEnabled[intersectedItem])) {
                // Press new button.
                localPosition = intersectionItems[intersectedItem].properties.localPosition;
                Overlays.editOverlay(intersectionOverlays[intersectedItem], {
                    localPosition: Vec3.sum(localPosition, BUTTON_PRESS_DELTA)
                });
                pressedSource = intersectionOverlays;
                pressedItem = {
                    index: intersectedItem,
                    localPosition: localPosition
                };

                // Button press actions.
                if (intersectionOverlays === menuOverlays) {
                    openOptions(intersectionItems[intersectedItem].toolOptions);
                }
                if (intersectionItems[intersectedItem].command) {
                    parameter = intersectionItems[intersectedItem].id;
                    doCommand(intersectionItems[intersectedItem].command.method, parameter);
                }
                if (intersectionItems[intersectedItem].callback) {
                    if (intersectionItems[intersectedItem].callback.parameter) {
                        parameterValue = evaluateParameter(intersectionItems[intersectedItem].callback.parameter);
                    }
                    uiCommandCallback(intersectionItems[intersectedItem].callback.method, parameterValue);
                }
            }
        }

        // Picklist update.
        if (intersectionItems && ((intersectionItems[intersectedItem].type === "picklist"
                && controlHand.triggerClicked() !== isPicklistPressed)
                    || (intersectionItems[intersectedItem].type !== "picklist" && isPicklistPressed))) {
            isPicklistPressed = isHighlightingPicklist && controlHand.triggerClicked();
            if (isPicklistPressed) {
                doCommand(intersectionItems[intersectedItem].command.method, intersectionItems[intersectedItem].id);
            }
        }
        if (intersectionItems && ((intersectionItems[intersectedItem].type === "picklistItem"
                && controlHand.triggerClicked() !== isPicklistItemPressed)
                    || (intersectionItems[intersectedItem].type !== "picklistItem" && isPicklistItemPressed))) {
            isPicklistItemPressed = isHighlightingPicklist && controlHand.triggerClicked();
            if (isPicklistItemPressed) {
                doCommand(intersectionItems[intersectedItem].command.method, intersectionItems[intersectedItem].id);
            }
        }
        if (intersectionItems && isPicklistOpen && controlHand.triggerClicked()
                && intersectionItems[intersectedItem].type !== "picklist"
                && intersectionItems[intersectedItem].type !== "picklistItem") {
            doCommand("togglePhysicsPresets", "presets");  //  TODO: This is a bit hacky.
        }

        // Grip click.
        if (controlHand.gripClicked() !== isGripClicked) {
            isGripClicked = !isGripClicked;
            if (isGripClicked && intersectionItems && intersectedItem && intersectionItems[intersectedItem].clear) {
                controlHand.setGripClickedHandled();
                parameter = intersectionItems[intersectedItem].id;
                doGripClicked(intersectionItems[intersectedItem].clear.method, parameter);
            }
        }

        // Bar slider update.
        if (intersectionItems && intersectionItems[intersectedItem].type === "barSlider" && controlHand.triggerClicked()) {
            sliderProperties = Overlays.getProperties(intersection.overlayID, ["position", "orientation"]);
            overlayDimensions = intersectionItems[intersectedItem].properties.dimensions;
            if (overlayDimensions === undefined) {
                overlayDimensions = UI_ELEMENTS.barSlider.properties.dimensions;
            }
            basePoint = Vec3.sum(sliderProperties.position,
                Vec3.multiplyQbyV(sliderProperties.orientation, { x: 0, y: overlayDimensions.y / 2, z: 0 }));
            fraction = Vec3.dot(Vec3.subtract(basePoint, intersection.intersection),
                Vec3.multiplyQbyV(sliderProperties.orientation, Vec3.UNIT_Y)) / overlayDimensions.y;
            fraction = adjustSliderFraction(fraction);
            setBarSliderValue(intersectedItem, fraction);
            if (intersectionItems[intersectedItem].command) {
                doCommand(intersectionItems[intersectedItem].command.method, fraction);
            }
        }

        // Image slider update.
        if (intersectionItems && intersectionItems[intersectedItem].type === "imageSlider" && controlHand.triggerClicked()) {
            sliderProperties = Overlays.getProperties(intersection.overlayID, ["position", "orientation"]);
            overlayDimensions = intersectionItems[intersectedItem].properties.dimensions;
            if (overlayDimensions === undefined) {
                overlayDimensions = UI_ELEMENTS.imageSlider.properties.dimensions;
            }
            basePoint = Vec3.sum(sliderProperties.position,
                Vec3.multiplyQbyV(sliderProperties.orientation, { x: 0, y: overlayDimensions.y / 2, z: 0 }));
            fraction = Vec3.dot(Vec3.subtract(basePoint, intersection.intersection),
                Vec3.multiplyQbyV(sliderProperties.orientation, Vec3.UNIT_Y)) / overlayDimensions.y;
            fraction = adjustSliderFraction(fraction);
            Overlays.editOverlay(optionsSliderData[intersectedItem].value, {
                localPosition: Vec3.sum(optionsSliderData[intersectedItem].offset,
                    { x: 0, y: (0.5 - fraction) * overlayDimensions.y, z: 0 })
            });
            if (intersectionItems[intersectedItem].callback) {
                uiCommandCallback(intersectionItems[intersectedItem].callback.method, fraction);
            }
        }

        // Special handling for Group options.
        if (optionsItems && optionsItems === OPTONS_PANELS.groupOptions) {
            enableGroupButton = groupsCount > 1;
            if (enableGroupButton !== isGroupButtonEnabled) {
                isGroupButtonEnabled = enableGroupButton;
                Overlays.editOverlay(optionsOverlays[groupButtonIndex], {
                    color: isGroupButtonEnabled
                        ? OPTONS_PANELS.groupOptions[groupButtonIndex].enabledColor
                        : OPTONS_PANELS.groupOptions[groupButtonIndex].properties.color
                });
                optionsEnabled[groupButtonIndex] = enableGroupButton;
            }

            enableUngroupButton = groupsCount === 1 && entitiesCount > 1;
            if (enableUngroupButton !== isUngroupButtonEnabled) {
                isUngroupButtonEnabled = enableUngroupButton;
                Overlays.editOverlay(optionsOverlays[ungroupButtonIndex], {
                    color: isUngroupButtonEnabled
                        ? OPTONS_PANELS.groupOptions[ungroupButtonIndex].enabledColor
                        : OPTONS_PANELS.groupOptions[ungroupButtonIndex].properties.color
                });
                optionsEnabled[ungroupButtonIndex] = enableUngroupButton;
            }
        }
    }

    function display() {
        // Creates and shows menu entities.
        var handJointIndex,
            properties,
            parentID,
            id,
            i,
            length;

        if (isDisplaying) {
            return;
        }

        // Joint index.
        handJointIndex = MyAvatar.getJointIndex(attachmentJointName);
        if (handJointIndex === NONE) {
            // Don't display if joint isn't available (yet) to attach to.
            // User can clear this condition by toggling the app off and back on once avatar finishes loading.
            App.log(side, "ERROR: ToolMenu: Hand joint index isn't available!");
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
        isHighlightingSlider = false;
        isHighlightingPicklist = false;
        isPicklistOpen = false;
        pressedItem = null;
        pressedSource = null;
        isButtonPressed = false;
        isPicklistPressed = false;
        isPicklistItemPressed = false;
        isGripClicked = false;
        isGroupButtonEnabled = false;
        isUngroupButtonEnabled = false;

        // Special handling for Group options.
        for (i = 0, length = OPTONS_PANELS.groupOptions.length; i < length; i += 1) {
            id = OPTONS_PANELS.groupOptions[i].id;
            if (id === "groupButton") {
                groupButtonIndex = i;
            }
            if (id === "ungroupButton") {
                ungroupButtonIndex = i;
            }
        }

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
            // Any auxiliary overlays parented to this overlay are automatically deleted.
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
        doCommand: doCommand,
        update: update,
        display: display,
        clear: clear,
        destroy: destroy
    };
};

ToolMenu.prototype = {};
