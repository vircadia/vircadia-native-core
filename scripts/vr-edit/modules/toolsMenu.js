//
//  toolsMenu.js
//
//  Created by David Rowe on 22 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global App, ToolsMenu */

ToolsMenu = function (side, leftInputs, rightInputs, uiCommandCallback) {
    // Tool menu displayed on top of forearm.

    "use strict";

    var attachmentJointName,

        menuOriginLocalPosition,
        menuOriginLocalRotation,

        menuOriginOverlay,
        menuHeaderOverlay,
        menuHeaderHeadingOverlay,
        menuHeaderBarOverlay,
        menuHeaderBackOverlay,
        menuHeaderTitleOverlay,
        menuHeaderIconOverlay,
        menuPanelOverlay,

        menuOverlays = [],
        menuHoverOverlays = [],

        optionsOverlays = [],
        optionsOverlaysIDs = [],  // Text ids (names) of options overlays.
        optionsOverlaysLabels = [],  // Overlay IDs of labels for optionsOverlays.
        optionsSliderData = [],  // Uses same index values as optionsOverlays.
        optionsColorData = [],  // Uses same index values as optionsOverlays.
        optionsEnabled = [],
        optionsSettings = {},

        highlightOverlay,

        LEFT_HAND = 0,
        PANEL_ORIGIN_POSITION_LEFT_HAND = {
            x: -UIT.dimensions.canvasSeparation - UIT.dimensions.canvas.x / 2,
            y: UIT.dimensions.handOffset,
            z: 0
        },
        PANEL_ORIGIN_POSITION_RIGHT_HAND = {
            x: UIT.dimensions.canvasSeparation + UIT.dimensions.canvas.x / 2,
            y: UIT.dimensions.handOffset,
            z: 0
        },
        PANEL_ORIGIN_ROTATION_LEFT_HAND = Quat.fromVec3Degrees({ x: 0, y: -90, z: 0 }),
        PANEL_ORIGIN_ROTATION_RIGHT_HAND = Quat.fromVec3Degrees({ x: 0, y: 90, z: 0 }),
        panelLateralOffset,

        MENU_ORIGIN_PROPERTIES = {
            dimensions: { x: 0.005, y: 0.005, z: 0.005 },
            color: { red: 255, blue: 0, green: 0 },
            alpha: 1.0,
            parentID: Uuid.SELF,
            ignoreRayIntersection: true,
            visible: false,
            displayInFront: true
        },

        MENU_HEADER_HOVER_OFFSET = { x: 0, y: 0, z: 0.0040 },

        MENU_HEADER_PROPERTIES = {
            dimensions: Vec3.sum(UIT.dimensions.header, MENU_HEADER_HOVER_OFFSET),  // Keep the laser on top when hover.
            localPosition: {
                x: 0,
                y: UIT.dimensions.canvas.y / 2 - UIT.dimensions.header.y / 2,
                z: UIT.dimensions.header.z / 2 + MENU_HEADER_HOVER_OFFSET.z / 2
            },
            localRotation: Quat.ZERO,
            alpha: 0.0,  // Invisible
            solid: true,
            ignoreRayIntersection: false,
            visible: true  // Catch laser intersections.
        },

        MENU_HEADER_HEADING_PROPERTIES = {
            dimensions:  UIT.dimensions.headerHeading,
            localPosition: { x: 0, y: UIT.dimensions.headerBar.y / 2, z: -MENU_HEADER_HOVER_OFFSET.z / 2 },
            localRotation: Quat.ZERO,
            color: UIT.colors.baseGray,
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: true,
            visible: true
        },

        MENU_HEADER_BAR_PROPERTIES = {
            dimensions: UIT.dimensions.headerBar,
            localPosition: { x: 0, y: -UIT.dimensions.headerHeading.y / 2 - UIT.dimensions.headerBar.y / 2, z: 0 },
            localRotation: Quat.ZERO,
            color: UIT.colors.greenHighlight,
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: true,
            visible: true
        },

        MENU_HEADER_BACK_PROPERTIES = {
            url: "../assets/tools/back-icon.svg",
            dimensions: { x: 0.0069, y: 0.0107 },
            localPosition: {
                x: -MENU_HEADER_HEADING_PROPERTIES.dimensions.x / 2 + 0.0118 + 0.0069 / 2,
                y: 0,
                z: MENU_HEADER_HEADING_PROPERTIES.dimensions.z / 2 + UIT.dimensions.imageOverlayOffset
            },
            localRotation: Quat.ZERO,
            color: UIT.colors.lightGrayText,
            alpha: 1.0,
            emissive: true,
            ignoreRayIntersection: true,
            isFacingAvatar: false,
            visible: true
        },

        MENU_HEADER_TITLE_PROPERTIES = {
            url: "../assets/tools/tools-heading.svg",
            scale: 0.0327,
            localPosition: {
                x: 0,
                y: 0,
                z: MENU_HEADER_HEADING_PROPERTIES.dimensions.z / 2 + UIT.dimensions.imageOverlayOffset
            },
            localRotation: Quat.ZERO,
            color: UIT.colors.white,
            alpha: 1.0,
            emissive: true,
            ignoreRayIntersection: true,
            isFacingAvatar: false,
            visible: true
        },

        MENU_HEADER_TITLE_BACK_URL = "../assets/tools/back-heading.svg",
        MENU_HEADER_TITLE_BACK_SCALE = 0.0256,

        MENU_HEADER_ICON_OFFSET = {
            // Default right center position for header tool icons.
            x: MENU_HEADER_HEADING_PROPERTIES.dimensions.x / 2 - 0.0118,
            y: 0,
            z: MENU_HEADER_HEADING_PROPERTIES.dimensions.z / 2 + UIT.dimensions.imageOverlayOffset
        },

        MENU_HEADER_ICON_PROPERTIES = {
            url: "../assets/tools/color-icon.svg",  // Initial value so that the overlay is initialized OK.
            dimensions: { x: 0.01, y: 0.01 },       // ""
            localPosition: Vec3.ZERO,               // ""
            localRotation: Quat.ZERO,
            color: UIT.colors.lightGrayText,
            alpha: 1.0,
            emissive: true,
            ignoreRayIntersection: true,
            isFacingAvatar: false,
            visible: false
        },

        MENU_PANEL_PROPERTIES = {
            dimensions: UIT.dimensions.panel,
            localPosition: { x: 0, y: UIT.dimensions.panel.y / 2 - UIT.dimensions.canvas.y / 2, z: UIT.dimensions.panel.z / 2 },
            localRotation: Quat.ZERO,
            color: UIT.colors.baseGray,
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        NO_SWATCH_COLOR = { red: 128, green: 128, blue: 128 },

        UI_BASE_COLOR = { red: 64, green: 64, blue: 64 },
        UI_HIGHLIGHT_COLOR = { red: 100, green: 240, blue: 100 },

        UI_ELEMENTS = {
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
            "menuButton": {
                overlay: "cube",  // Invisible cube for hit area.
                properties: {
                    dimensions: UIT.dimensions.itemCollisionZone,
                    localRotation: Quat.ZERO,
                    alpha: 0.0,  // Invisible.
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true  // So that laser intersects.
                },
                hoverButton: {
                    overlay: "shape",
                    properties: {
                        shape: "Cylinder",
                        dimensions: {
                            x: UIT.dimensions.menuButtonDimensions.x,
                            y: UIT.dimensions.menuButtonDimensions.z,
                            z: UIT.dimensions.menuButtonDimensions.y
                        },
                        localPosition: UIT.dimensions.menuButtonIconOffset,
                        localRotation: Quat.fromVec3Degrees({ x: 90, y: 0, z: -90 }),
                        color: UIT.colors.greenHighlight,
                        alpha: 1.0,
                        emissive: true,  // TODO: This has no effect.
                        solid: true,
                        ignoreRayIntersection: true,
                        visible: false
                    }
                },
                icon: {
                    // Relative to hoverButton.
                    type: "image",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.menuButtonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset,
                            z: 0
                        },
                        localRotation: Quat.fromVec3Degrees({ x: -90, y: 90, z: 0 }),
                        color: UIT.colors.lightGrayText
                    }
                },
                label: {
                    // Relative to menuButton.
                    type: "image",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.menuButtonLabelYOffset,
                            z: -UIT.dimensions.itemCollisionZone.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        color: UIT.colors.white
                    }
                },
                sublabel: {
                    // Relative to menuButton.
                    type: "image",
                    properties: {
                        url: "../assets/tools/tool-label.svg",
                        scale: 0.0152,
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.menuButtonSublabelYOffset,
                            z: -UIT.dimensions.itemCollisionZone.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        color: UIT.colors.lightGrayText
                    }
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
                    localPosition: { x: 0, y: 0, z: 0.005 },
                    localRotation: Quat.ZERO,
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
                    localPosition: { x: 0.0, y: 0.0, z: 0.01 },
                    localRotation: Quat.ZERO,
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
                    localPosition: { x: 0, y: 0, z: 0 },
                    localRotation: Quat.ZERO,
                    color: { red: 255, green: 255, blue: 255 },
                    alpha: 1.0,
                    emissive: true,
                    ignoreRayIntersection: true,
                    isFacingAvatar: false,
                    visible: true
                }
            },
            "sphere": {
                overlay: "sphere",
                properties: {
                    dimensions: { x: 0.01, y: 0.01, z: 0.01 },
                    localRotation: Quat.ZERO,
                    color: { red: 192, green: 192, blue: 192 },
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: true,
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
                    dimensions: { x: 0.01, y: 0.06, z: 0.01 },
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
            "colorCircle": {
                overlay: "shape",
                properties: {
                    shape: "Cylinder",
                    dimensions: { x: 0.06, y: 0.01, z: 0.06 },
                    localRotation: Quat.fromVec3Degrees({ x: 90, y: 0, z: -90 }),
                    color: { red: 128, green: 128, blue: 128 },
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                },
                imageURL: null,
                imageOverlayURL: null
            },
            "circlePointer": {
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

        BUTTON_UI_ELEMENTS = ["button", "menuButton", "toggleButton", "swatch"],
        BUTTON_PRESS_DELTA = { x: 0, y: 0, z: -0.004 },

        SLIDER_UI_ELEMENTS = ["barSlider", "imageSlider"],
        COLOR_CIRCLE_UI_ELEMENTS = ["colorCircle"],
        PICKLIST_UI_ELEMENTS = ["picklist", "picklistItem"],
        MENU_RAISE_DELTA = { x: 0, y: 0, z: 0.006 },
        ITEM_RAISE_DELTA = { x: 0, y: 0, z: 0.004 },

        MIN_BAR_SLIDER_DIMENSION = 0.0001,  // Avoid visual artifact for 0 slider values.

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
            scaleOptions: [
                {
                    id: "scaleFinishButton",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.07, y: 0.03, z: 0.01 },
                        localPosition: { x: 0, y: 0, z: 0.005 },
                        color: { red: 200, green: 200, blue: 200 }
                    },
                    label: "FINISH",
                    command: {
                        method: "clearTool"
                    }
                }
            ],
            cloneOptions: [
                {
                    id: "cloneFinishButton",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.07, y: 0.03, z: 0.01 },
                        localPosition: { x: 0, y: 0, z: 0.005 },
                        color: { red: 200, green: 200, blue: 200 }
                    },
                    label: "FINISH",
                    command: {
                        method: "clearTool"
                    }
                }
            ],
            groupOptions: [
                {
                    id: "groupButton",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.07, y: 0.03, z: 0.01 },
                        localPosition: { x: 0, y: 0.025, z: 0.005 },
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
                        localPosition: { x: 0, y: -0.025, z: 0.005 },
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
                    id: "colorCircle",
                    type: "colorCircle",
                    properties: {
                        localPosition: { x: -0.0125, y: 0.025, z: 0.005 }
                    },
                    imageURL: "../assets/color-circle.png",
                    imageOverlayURL: "../assets/color-circle-black.png",
                    command: {
                        method: "setColorPerCircle"
                    }
                },
                {
                    id: "colorSlider",
                    type: "imageSlider",
                    properties: {
                        localPosition: { x: 0.035, y: 0.025, z: 0.005 }
                    },
                    useBaseColor: true,
                    imageURL: "../assets/slider-white.png",
                    imageOverlayURL: "../assets/slider-v-alpha.png",
                    command: {
                        method: "setColorPerSlider"
                    }
                },
                {
                    id: "colorSwatch1",
                    type: "swatch",
                    properties: {
                        dimensions: { x: 0.02, y: 0.02, z: 0.01 },
                        localPosition: { x: -0.035, y: -0.02, z: 0.005 }
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
                        localPosition: { x: -0.01, y: -0.02, z: 0.005 }
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
                        localPosition: { x: -0.035, y: -0.045, z: 0.005 }
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
                        localPosition: { x: -0.01, y: -0.045, z: 0.005 }
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
                        localPosition: { x: 0.025, y: -0.02, z: 0.007 }
                    },
                    setting: {
                        key: "VREdit.colorTool.currentColor",
                        property: "color",
                        defaultValue: { red: 128, green: 128, blue: 128 },
                        command: "setPickColor"
                    }
                },
                {
                    id: "pickColor",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.04, y: 0.02, z: 0.01 },
                        localPosition: { x: 0.025, y: -0.045, z: 0.005 },
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
                    id: "propertiesLabel",
                    type: "label",
                    properties: {
                        text: "PROPERTIES",
                        lineHeight: 0.0045,
                        localPosition: { x: -0.031, y: 0.0475, z: 0.0075}
                    }
                },
                {
                    id: "gravityToggle",
                    type: "toggleButton",
                    properties: {
                        localPosition: { x: -0.0325, y: 0.03, z: 0.005 },
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
                        localPosition: { x: -0.0325, y: 0.005, z: 0.005 },
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
                        localPosition: { x: -0.0325, y: -0.02, z: 0.005 },
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
                        localPosition: { x: 0.002, y: 0.0475, z: 0.0075 }
                    }
                },
                {
                    id: "presets",
                    type: "picklist",
                    properties: {
                        localPosition: { x: 0.016, y: 0.03, z: 0.005 },
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
                        localPosition: { x: -0.007, y: -0.016, z: 0.005 },
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
                        localPosition: { x: -0.003, y: -0.052, z: 0.0075 }
                    }
                },
                {
                    id: "bounceSlider",
                    type: "barSlider",
                    properties: {
                        localPosition: { x: 0.009, y: -0.016, z: 0.005 },
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
                        localPosition: { x: 0.015, y: -0.057, z: 0.0075 }
                    }
                },
                {
                    id: "dampingSlider",
                    type: "barSlider",
                    properties: {
                        localPosition: { x: 0.024, y: -0.016, z: 0.005 },
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
                        localPosition: { x: 0.030, y: -0.052, z: 0.0075 }
                    }
                },
                {
                    id: "densitySlider",
                    type: "barSlider",
                    properties: {
                        localPosition: { x: 0.039, y: -0.016, z: 0.005 },
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
                        localPosition: { x: 0.045, y: -0.057, z: 0.0075 }
                    }
                }
            ],
            deleteOptions: [
                {
                    id: "deleteFinishButton",
                    type: "button",
                    properties: {
                        dimensions: { x: 0.07, y: 0.03, z: 0.01 },
                        localPosition: { x: 0, y: 0, z: 0.005 },
                        color: { red: 200, green: 200, blue: 200 }
                    },
                    label: "FINISH",
                    command: {
                        method: "clearTool"
                    }
                }
            ]
        },

        MENU_ITEM_XS = [-0.08415, -0.02805, 0.02805, 0.08415],
        MENU_ITEM_YS = [0.058, 0.002, -0.054],

        MENU_ITEMS = [
            {
                id: "colorButton",
                type: "menuButton",
                properties: {
                    localPosition: {
                        x: MENU_ITEM_XS[0],
                        y: MENU_ITEM_YS[0],
                        z: UIT.dimensions.panel.z / 2 + UI_ELEMENTS.menuButton.properties.dimensions.z / 2
                    }
                },
                icon: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/color-icon.svg",
                        dimensions: { x: 0.0165, y: 0.0187 }
                    },
                    headerOffset: { x: -0.00825, y: 0.0020, z: 0 }
                },
                label: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/color-label.svg",
                        scale: 0.0241
                    }
                },
                title: {
                    url: "../assets/tools/color-tool-heading.svg",
                    scale: 0.0631
                },
                toolOptions: "colorOptions",
                callback: {
                    method: "colorTool",
                    parameter: "currentColor.color"
                }
            },
            {
                id: "scaleButton",
                type: "menuButton",
                properties: {
                    localPosition: {
                        x: MENU_ITEM_XS[1],
                        y: MENU_ITEM_YS[0],
                        z: UIT.dimensions.panel.z / 2 + UI_ELEMENTS.menuButton.properties.dimensions.z / 2
                    }
                },
                icon: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/stretch-icon.svg",
                        dimensions: { x: 0.0167, y: 0.0167 }
                    },
                    headerOffset: { x: -0.00835, y: 0, z: 0 }
                },
                label: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/stretch-label.svg",
                        scale: 0.0311
                    }
                },
                title: {
                    url: "../assets/tools/stretch-tool-heading.svg",
                    scale: 0.0737
                },
                toolOptions: "scaleOptions",
                callback: {
                    method: "scaleTool"
                }
            },
            {
                id: "cloneButton",
                type: "menuButton",
                properties: {
                    localPosition: {
                        x: MENU_ITEM_XS[2],
                        y: MENU_ITEM_YS[0],
                        z: UIT.dimensions.panel.z / 2 + UI_ELEMENTS.menuButton.properties.dimensions.z / 2
                    }
                },
                icon: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/clone-icon.svg",
                        dimensions: { x: 0.0154, y: 0.0155 }
                    },
                    headerOffset: { x: -0.0077, y: 0, z: 0 }
                },
                label: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/clone-label.svg",
                        scale: 0.0231
                    }
                },
                title: {
                    url: "../assets/tools/clone-tool-heading.svg",
                    scale: 0.0621
                },
                toolOptions: "cloneOptions",
                callback: {
                    method: "cloneTool"
                }
            },
            {
                id: "groupButton",
                type: "menuButton",
                properties: {
                    localPosition: {
                        x: MENU_ITEM_XS[3],
                        y: MENU_ITEM_YS[0],
                        z: UIT.dimensions.panel.z / 2 + UI_ELEMENTS.menuButton.properties.dimensions.z / 2
                    }
                },
                icon: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/group-icon.svg",
                        dimensions: { x: 0.0161, y: 0.0114 }
                    },
                    headerOffset: { x: -0.00805, y: 0, z: 0 }
                },
                label: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/group-label.svg",
                        scale: 0.0250
                    }
                },
                title: {
                    url: "../assets/tools/group-tool-heading.svg",
                    scale: 0.0647
                },
                toolOptions: "groupOptions",
                callback: {
                    method: "groupTool"
                }
            },
            {
                id: "physicsButton",
                type: "menuButton",
                properties: {
                    localPosition: {
                        x: MENU_ITEM_XS[0],
                        y: MENU_ITEM_YS[1],
                        z: UIT.dimensions.panel.z / 2 + UI_ELEMENTS.menuButton.properties.dimensions.z / 2
                    }
                },
                icon: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/physics-icon.svg",
                        dimensions: { x: 0.0180, y: 0.0198 }
                    },
                    headerOffset: { x: -0.009, y: 0, z: 0 }
                },
                label: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/physics-label.svg",
                        scale: 0.0297
                    }
                },
                title: {
                    url: "../assets/tools/physics-tool-heading.svg",
                    scale: 0.0712
                },
                toolOptions: "physicsOptions",
                callback: {
                    method: "physicsTool"
                }
            },
            {
                id: "deleteButton",
                type: "menuButton",
                properties: {
                    localPosition: {
                        x: MENU_ITEM_XS[1],
                        y: MENU_ITEM_YS[1],
                        z: UIT.dimensions.panel.z / 2 + UI_ELEMENTS.menuButton.properties.dimensions.z / 2
                    }
                },
                icon: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/delete-icon.svg",
                        dimensions: { x: 0.0161, y: 0.0161 }
                    },
                    headerOffset: { x: -0.00805, y: 0, z: 0 }
                },
                label: {
                    type: "image",
                    properties: {
                        url: "../assets/tools/delete-label.svg",
                        scale: 0.0254
                    }
                },
                title: {
                    url: "../assets/tools/delete-tool-heading.svg",
                    scale: 0.0653
                },
                toolOptions: "deleteOptions",
                callback: {
                    method: "deleteTool"
                }
            }
        ],
        COLOR_TOOL = 0,  // Indexes of corresponding MENU_ITEMS item.
        SCALE_TOOL = 1,
        CLONE_TOOL = 2,
        GROUP_TOOL = 3,
        PHYSICS_TOOL = 4,
        DELETE_TOOL = 5,

        HIGHLIGHT_PROPERTIES = {
            xDelta: 0.004,
            yDelta: 0.004,
            zDimension: 0.001,
            properties: {
                localPosition: { x: 0, y: 0, z: 0.003 },
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
        isHighlightingMenuButton,
        isHighlightingSlider,
        isHighlightingColorCircle,
        isHighlightingPicklist,
        isPicklistOpen,
        pressedItem = null,
        pressedSource = null,
        isPicklistPressed,
        isPicklistItemPressed,
        isTriggerClicked,
        wasTriggerClicked,
        isGripClicked,

        isGroupButtonEnabled,
        isUngroupButtonEnabled,
        groupButtonIndex,
        ungroupButtonIndex,

        hsvControl = {
            hsv: { h: 0, s: 0, v: 0 },
            circle: {},
            slider: {}
        },

        isDisplaying = false,
        isOptionsOpen = false,
        isOptionsHeadingRaised = false,
        optionsHeadingURL,
        optionsHeadingScale,

        // References.
        controlHand,

        // Forward declarations.
        doCommand;


    if (!this instanceof ToolsMenu) {
        return new ToolsMenu();
    }

    controlHand = side === LEFT_HAND ? rightInputs.hand() : leftInputs.hand();

    function setHand(hand) {
        // Assumes UI is not displaying.
        side = hand;
        if (side === LEFT_HAND) {
            controlHand = rightInputs.hand();
            attachmentJointName = "LeftHand";
            panelLateralOffset = -UIT.dimensions.handLateralOffset;
            menuOriginLocalPosition = PANEL_ORIGIN_POSITION_LEFT_HAND;
            menuOriginLocalRotation = PANEL_ORIGIN_ROTATION_LEFT_HAND;
        } else {
            controlHand = leftInputs.hand();
            attachmentJointName = "RightHand";
            panelLateralOffset = UIT.dimensions.handLateralOffset;
            menuOriginLocalPosition = PANEL_ORIGIN_POSITION_RIGHT_HAND;
            menuOriginLocalRotation = PANEL_ORIGIN_ROTATION_RIGHT_HAND;
        }
    }

    setHand(side);

    function getEntityIDs() {
        return [menuPanelOverlay, menuHeaderOverlay].concat(menuOverlays).concat(optionsOverlays);
    }

    function getIconInfo(tool) {
        // Provides details of tool icon, label, and sublabel images for the specified tool.
        return {
            icon: MENU_ITEMS[tool].icon,
            label: MENU_ITEMS[tool].label,
            sublabel: UI_ELEMENTS.menuButton.sublabel
        };
    }

    function openMenu() {
        var properties,
            itemID,
            buttonID,
            i,
            length;

        // Update header.
        Overlays.editOverlay(menuHeaderBackOverlay, { visible: false });
        Overlays.editOverlay(menuHeaderTitleOverlay, {
            url: Script.resolvePath(MENU_HEADER_TITLE_PROPERTIES.url),
            scale: MENU_HEADER_TITLE_PROPERTIES.scale
        });
        Overlays.editOverlay(menuHeaderIconOverlay, { visible: false });

        // Display menu items.
        for (i = 0, length = MENU_ITEMS.length; i < length; i += 1) {
            properties = Object.clone(UI_ELEMENTS[MENU_ITEMS[i].type].properties);
            properties = Object.merge(properties, MENU_ITEMS[i].properties);
            properties.parentID = menuPanelOverlay;
            itemID = Overlays.addOverlay(UI_ELEMENTS[MENU_ITEMS[i].type].overlay, properties);
            menuOverlays[i] = itemID;

            if (MENU_ITEMS[i].label) {
                properties = Object.clone(UI_ELEMENTS.label.properties);
                properties.text = MENU_ITEMS[i].label;
                properties.parentID = itemID;
                Overlays.addOverlay(UI_ELEMENTS.label.overlay, properties);
            }

            if (MENU_ITEMS[i].type === "menuButton") {
                // Collision overlay.
                properties = Object.clone(UI_ELEMENTS.menuButton.hoverButton.properties);
                properties.parentID = itemID;
                buttonID = Overlays.addOverlay(UI_ELEMENTS.menuButton.hoverButton.overlay, properties);
                menuHoverOverlays[i] = buttonID;

                // Icon.
                properties = Object.clone(UI_ELEMENTS[UI_ELEMENTS.menuButton.icon.type].properties);
                properties = Object.merge(properties, UI_ELEMENTS.menuButton.icon.properties);
                properties = Object.merge(properties, MENU_ITEMS[i].icon.properties);
                properties.url = Script.resolvePath(properties.url);
                properties.parentID = buttonID;
                Overlays.addOverlay(UI_ELEMENTS[UI_ELEMENTS.menuButton.icon.type].overlay, properties);

                // Label.
                properties = Object.clone(UI_ELEMENTS[UI_ELEMENTS.menuButton.label.type].properties);
                properties = Object.merge(properties, UI_ELEMENTS.menuButton.label.properties);
                properties = Object.merge(properties, MENU_ITEMS[i].label.properties);
                properties.url = Script.resolvePath(properties.url);
                properties.parentID = itemID;
                Overlays.addOverlay(UI_ELEMENTS[UI_ELEMENTS.menuButton.label.type].overlay, properties);

                // Sublabel.
                properties = Object.clone(UI_ELEMENTS[UI_ELEMENTS.menuButton.sublabel.type].properties);
                properties = Object.merge(properties, UI_ELEMENTS.menuButton.sublabel.properties);
                properties.url = Script.resolvePath(properties.url);
                properties.parentID = itemID;
                Overlays.addOverlay(UI_ELEMENTS[UI_ELEMENTS.menuButton.sublabel.type].overlay, properties);
            }
        }
    }

    function closeMenu() {
        var i,
            length;

        Overlays.editOverlay(highlightOverlay, {
            parentID: menuOriginOverlay
        });

        for (i = 0, length = menuOverlays.length; i < length; i += 1) {
            Overlays.deleteOverlay(menuOverlays[i]);
        }

        menuOverlays = [];
        menuHoverOverlays = [];
        pressedItem = null;
    }

    function openOptions(menuItem) {
        var properties,
            childProperties,
            auxiliaryProperties,
            parentID,
            value,
            imageOffset,
            IMAGE_OFFSET = 0.0005,
            CIRCLE_CURSOR_GAP = 0.002,
            id,
            i,
            length;

        // Remove menu items.
        closeMenu();

        // Update header.
        optionsHeadingURL = Script.resolvePath(menuItem.title.url);
        optionsHeadingScale = menuItem.title.scale;
        Overlays.editOverlay(menuHeaderBackOverlay, { visible: true });
        Overlays.editOverlay(menuHeaderTitleOverlay, {
            url: optionsHeadingURL,
            scale: optionsHeadingScale
        });
        Overlays.editOverlay(menuHeaderIconOverlay, {
            url: Script.resolvePath(menuItem.icon.properties.url),
            dimensions: menuItem.icon.properties.dimensions,
            localPosition: Vec3.sum(MENU_HEADER_ICON_OFFSET, menuItem.icon.headerOffset),
            visible: true
        });

        // Open specified options panel.
        optionsItems = OPTONS_PANELS[menuItem.toolOptions];
        parentID = menuPanelOverlay;
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
                    if (optionsItems[i].setting.command) {
                        doCommand(optionsItems[i].setting.command, value);
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
                auxiliaryProperties.localPosition = { x: 0, y: (-0.5 + value / 2) * properties.dimensions.y, z: 0 };
                auxiliaryProperties.dimensions = {
                    x: properties.dimensions.x,
                    y: Math.max(value * properties.dimensions.y, MIN_BAR_SLIDER_DIMENSION),
                    z: properties.dimensions.z
                };
                auxiliaryProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                optionsSliderData[i].value = Overlays.addOverlay(UI_ELEMENTS.barSliderValue.overlay,
                    auxiliaryProperties);
                auxiliaryProperties = Object.clone(UI_ELEMENTS.barSliderRemainder.properties);
                auxiliaryProperties.localPosition = { x: 0, y: (0.5 - (1.0 - value) / 2) * properties.dimensions.y, z: 0 };
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
                    if (optionsItems[i].useBaseColor) {
                        childProperties.color = properties.color;
                    }
                    childProperties.localPosition = { x: 0, y: 0, z: properties.dimensions.z / 2 + imageOffset };
                    hsvControl.slider.localPosition = childProperties.localPosition;
                    childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    hsvControl.slider.colorOverlay = Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                    hsvControl.slider.length = properties.dimensions.y;
                }

                // Overlay image.
                if (optionsItems[i].imageOverlayURL) {
                    childProperties = Object.clone(UI_ELEMENTS.image.properties);
                    childProperties.url = Script.resolvePath(optionsItems[i].imageOverlayURL);
                    childProperties.drawInFront = true;  // TODO: Work-around for rendering bug; remove when bug fixed.
                    delete childProperties.dimensions;
                    childProperties.scale = properties.dimensions.y;
                    childProperties.emissive = false;
                    imageOffset += IMAGE_OFFSET;
                    childProperties.localPosition = { x: 0, y: 0, z: properties.dimensions.z / 2 + imageOffset };
                    childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                }

                // Value pointers.
                optionsSliderData[i] = {};
                optionsSliderData[i].offset =
                    { x: -properties.dimensions.x / 2, y: 0, z: properties.dimensions.z / 2 + imageOffset };
                auxiliaryProperties = Object.clone(UI_ELEMENTS.sliderPointer.properties);
                auxiliaryProperties.localPosition = optionsSliderData[i].offset;
                hsvControl.slider.localPosition = auxiliaryProperties.localPosition;
                auxiliaryProperties.drawInFront = true;  // TODO: Accommodate work-around above; remove when bug fixed.
                auxiliaryProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                optionsSliderData[i].value = Overlays.addOverlay(UI_ELEMENTS.sliderPointer.overlay, auxiliaryProperties);
                hsvControl.slider.pointerOverlay = optionsSliderData[i].value;
                auxiliaryProperties.localPosition = { x: 0, y: properties.dimensions.x, z: 0 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 180 });
                auxiliaryProperties.parentID = optionsSliderData[i].value;
                Overlays.addOverlay(UI_ELEMENTS.sliderPointer.overlay, auxiliaryProperties);
            }

            if (optionsItems[i].type === "colorCircle") {
                imageOffset = 0.0;

                // Primary image.
                if (optionsItems[i].imageURL) {
                    childProperties = Object.clone(UI_ELEMENTS.image.properties);
                    childProperties.url = Script.resolvePath(optionsItems[i].imageURL);
                    delete childProperties.dimensions;
                    childProperties.scale = 0.95 * properties.dimensions.x;  // TODO: Magic number.
                    imageOffset += IMAGE_OFFSET;
                    childProperties.localPosition = { x: 0, y: properties.dimensions.y / 2 + imageOffset, z: 0 };
                    childProperties.localRotation = Quat.fromVec3Degrees({ x: -90, y: 90, z: 0 });
                    childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                }

                // Overlay image.
                if (optionsItems[i].imageOverlayURL) {
                    childProperties = Object.clone(UI_ELEMENTS.image.properties);
                    childProperties.url = Script.resolvePath(optionsItems[i].imageOverlayURL);
                    childProperties.drawInFront = true;  // TODO: Work-around for rendering bug; remove when bug fixed.
                    delete childProperties.dimensions;
                    childProperties.scale = 0.95 * properties.dimensions.x;  // TODO: Magic number.
                    imageOffset += IMAGE_OFFSET;
                    childProperties.emissive = false;
                    childProperties.localPosition = { x: 0, y: properties.dimensions.y / 2 + imageOffset, z: 0 };
                    childProperties.localRotation = Quat.fromVec3Degrees({ x: 90, y: 90, z: 0 });
                    childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    childProperties.alpha = 0.0;
                    hsvControl.circle.overlay = Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                }

                // Value pointers.
                // Invisible sphere at target point with cones as decoration.
                optionsColorData[i] = {};
                optionsColorData[i].offset =
                    { x: 0, y: properties.dimensions.y / 2 + imageOffset, z: 0 };
                auxiliaryProperties = Object.clone(UI_ELEMENTS.sphere.properties);
                auxiliaryProperties.localPosition = optionsColorData[i].offset;
                auxiliaryProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                auxiliaryProperties.visible = false;
                optionsColorData[i].value = Overlays.addOverlay(UI_ELEMENTS.sphere.overlay, auxiliaryProperties);
                hsvControl.circle.radius = childProperties.scale / 2;
                hsvControl.circle.localPosition = auxiliaryProperties.localPosition;
                hsvControl.circle.cursorOverlay = optionsColorData[i].value;

                auxiliaryProperties = Object.clone(UI_ELEMENTS.circlePointer.properties);
                auxiliaryProperties.parentID = optionsColorData[i].value;
                auxiliaryProperties.drawInFront = true;  // TODO: Accommodate work-around above; remove when bug fixed.
                auxiliaryProperties.localPosition =
                    { x: -(auxiliaryProperties.dimensions.x + CIRCLE_CURSOR_GAP) / 2, y: 0, z: 0 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: 0, y: 90, z: -90 });
                Overlays.addOverlay(UI_ELEMENTS.circlePointer.overlay, auxiliaryProperties);
                auxiliaryProperties.localPosition =
                    { x: (auxiliaryProperties.dimensions.x + CIRCLE_CURSOR_GAP) / 2, y: 0, z: 0 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 90 });
                Overlays.addOverlay(UI_ELEMENTS.circlePointer.overlay, auxiliaryProperties);
                auxiliaryProperties.localPosition =
                    { x: 0, y: 0, z: -(auxiliaryProperties.dimensions.x + CIRCLE_CURSOR_GAP) / 2 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: 90, y: 0, z: 0 });
                Overlays.addOverlay(UI_ELEMENTS.circlePointer.overlay, auxiliaryProperties);
                auxiliaryProperties.localPosition =
                    { x: 0, y: 0, z: (auxiliaryProperties.dimensions.x + CIRCLE_CURSOR_GAP) / 2 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: -90, y: 0, z: 0 });
                Overlays.addOverlay(UI_ELEMENTS.circlePointer.overlay, auxiliaryProperties);
            }

            optionsEnabled.push(true);
        }

        // Special handling for Group options.
        if (menuItem.toolOptions === "groupOptions") {
            optionsEnabled[groupButtonIndex] = false;
            optionsEnabled[ungroupButtonIndex] = false;
        }

        isOptionsOpen = true;
    }

    function closeOptions() {
        var i,
            length;

        // Remove options items.
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
        optionsColorData = [];
        optionsEnabled = [];
        optionsItems = null;

        isPicklistOpen = false;

        pressedItem = null;

        isOptionsOpen = false;

        // Display menu items.
        openMenu();
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
            localPosition: { x: 0, y: (-0.5 + fraction / 2) * overlayDimensions.y, z: 0 },
            dimensions: {
                x: overlayDimensions.x,
                y: Math.max(fraction * overlayDimensions.y, MIN_BAR_SLIDER_DIMENSION),
                z: overlayDimensions.z
            }
        });
        Overlays.editOverlay(optionsSliderData[item].remainder, {
            localPosition: { x: 0, y: (0.5 - otherFraction / 2) * overlayDimensions.y, z: 0 },
            dimensions: {
                x: overlayDimensions.x,
                y: Math.max(otherFraction * overlayDimensions.y, MIN_BAR_SLIDER_DIMENSION),
                z: overlayDimensions.z
            }
        });
    }

    function hsvToRGB(hsv) {
        // https://en.wikipedia.org/wiki/HSL_and_HSV
        var c, h, x, rgb, m;

        c = hsv.v * hsv.s;
        h = hsv.h * 6.0;
        x = c * (1 - Math.abs(h % 2 - 1));
        if (0 <= h && h <= 1) {
            rgb = { red: c, green: x, blue: 0 };
        } else if (1 < h && h <= 2) {
            rgb = { red: x, green: c, blue: 0 };
        } else if (2 < h && h <= 3) {
            rgb = { red: 0, green: c, blue: x };
        } else if (3 < h && h <= 4) {
            rgb = { red: 0, green: x, blue: c };
        } else if (4 < h && h <= 5) {
            rgb = { red: x, green: 0, blue: c };
        } else {
            rgb = { red: c, green: 0, blue: x };
        }
        m = hsv.v - c;
        rgb = {
            red: Math.round((rgb.red + m) * 255),
            green: Math.round((rgb.green + m) * 255),
            blue: Math.round((rgb.blue + m) * 255)
        };
        return rgb;
    }

    function rgbToHSV(rgb) {
        // https://en.wikipedia.org/wiki/HSL_and_HSV
        var mMax, mMin, c, h, v, s;

        mMax = Math.max(rgb.red, rgb.green, rgb.blue);
        mMin = Math.min(rgb.red, rgb.green, rgb.blue);
        c = mMax - mMin;

        if (c === 0) {
            h = 0;
        } else if (mMax === rgb.red) {
            h = ((rgb.green - rgb.blue) / c) % 6;
        } else if (mMax === rgb.green) {
            h = (rgb.blue - rgb.red) / c + 2;
        } else {
            h = (rgb.red - rgb.green) / c + 4;
        }
        h = h / 6;
        v = mMax / 255;
        s = v === 0 ? 0 : c / mMax;
        return { h: h, s: s, v: v };
    }

    function updateColorCircle() {
        var theta, r, x, y;

        // V overlay alpha per v.
        Overlays.editOverlay(hsvControl.circle.overlay, { alpha: 1.0 - hsvControl.hsv.v });

        // Cursor position per h & s.
        theta = 2 * Math.PI * hsvControl.hsv.h;
        r = hsvControl.hsv.s * hsvControl.circle.radius;
        x = r * Math.cos(theta);
        y = r * Math.sin(theta);
        Overlays.editOverlay(hsvControl.circle.cursorOverlay, {
            // Coordinates based on rotate cylinder entity. TODO: Use FBX model instead of cylinder entity.
            localPosition: { x: -y, y: hsvControl.circle.localPosition.y, z: -x }
        });
    }

    function updateColorSlider() {
        // Base color per h & s.
        Overlays.editOverlay(hsvControl.slider.colorOverlay, {
            color: hsvToRGB({ h: hsvControl.hsv.h, s: hsvControl.hsv.s, v: 1.0 })
        });

        // Slider position per v.
        Overlays.editOverlay(hsvControl.slider.pointerOverlay, {
            localPosition: {
                x: hsvControl.slider.localPosition.x,
                y: (hsvControl.hsv.v - 0.5) * hsvControl.slider.length,
                z: hsvControl.slider.localPosition.z
            }
        });
    }

    function setColorPicker(rgb) {
        hsvControl.hsv = rgbToHSV(rgb);
        updateColorCircle();
        updateColorSlider();
    }

    function setCurrentColor(rgb) {
        Overlays.editOverlay(optionsOverlays[optionsOverlaysIDs.indexOf("currentColor")], {
            color: rgb
        });
        if (optionsSettings.currentColor) {
            Settings.setValue(optionsSettings.currentColor.key, rgb);
        }
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

    doCommand = function (command, parameter) {
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

        case "setPickColor":
            setColorPicker(parameter);
            break;

        case "setColorPerCircle":
            hsvControl.hsv.h = parameter.h;
            hsvControl.hsv.s = parameter.s;
            updateColorSlider();
            value = hsvToRGB(hsvControl.hsv);
            setCurrentColor(value);
            uiCommandCallback("setColor", value);
            break;

        case "setColorPerSlider":
            hsvControl.hsv.v = parameter;
            updateColorCircle();
            value = hsvToRGB(hsvControl.hsv);
            setCurrentColor(value);
            uiCommandCallback("setColor", value);
            break;

        case "setColorPerSwatch":
            index = optionsOverlaysIDs.indexOf(parameter);
            hasColor = Overlays.getProperty(optionsOverlays[index], "solid");
            if (hasColor) {
                value = Overlays.getProperty(optionsOverlays[index], "color");
                setCurrentColor(value);
                setColorPicker(value);
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
            setCurrentColor(parameter);
            setColorPicker(parameter);
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
                    localPosition: Vec3.sum(optionsItems[index].properties.localPosition, ITEM_RAISE_DELTA)
                });

                // Show options.
                items = optionsItems[index].items;
                for (i = 0, length = items.length; i < length; i += 1) {
                    index = optionsOverlaysIDs.indexOf(items[i]);
                    Overlays.editOverlay(optionsOverlays[index], {
                        parentID: parentID,
                        localPosition: { x: 0, y: (i + 1) * UI_ELEMENTS.picklistItem.properties.dimensions.y, z: 0 },
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

        case "closeOptions":
            closeOptions();
            break;

        case "clearTool":
            uiCommandCallback("clearTool");
            break;

        default:
            App.log(side, "ERROR: ToolsMenu: Unexpected command! " + command);
        }
    };

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
            App.log(side, "ERROR: ToolsMenu: Unexpected command! " + command);
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
            fraction,
            delta,
            radius,
            x,
            y,
            s,
            h;

        isTriggerClicked = controlHand.triggerClicked();

        // Handle heading.
        if (isOptionsOpen && intersection.overlayID === menuHeaderOverlay) {
            if (isTriggerClicked && !wasTriggerClicked) {
                // Lower and unhighlight heading; go back to Tools menu.
                Overlays.editOverlay(menuHeaderHeadingOverlay, {
                    localPosition: MENU_HEADER_HEADING_PROPERTIES.localPosition,
                    color: UIT.colors.baseGray,
                    emissive: false
                });
                isOptionsHeadingRaised = false;
                doCommand("clearTool");
            } else if (!isOptionsHeadingRaised) {
                // Hover heading.
                Overlays.editOverlay(menuHeaderHeadingOverlay, {
                    localPosition: Vec3.sum(MENU_HEADER_HEADING_PROPERTIES.localPosition, MENU_HEADER_HOVER_OFFSET),
                    color: UIT.colors.greenHighlight,
                    emissive: true  // TODO: This has no effect.
                });
                Overlays.editOverlay(menuHeaderTitleOverlay, {
                    url: Script.resolvePath(MENU_HEADER_TITLE_BACK_URL),
                    scale: MENU_HEADER_TITLE_BACK_SCALE
                });
                Overlays.editOverlay(menuHeaderIconOverlay, {
                    visible: false
                });
                isOptionsHeadingRaised = true;
            }
        } else {
            if (isOptionsHeadingRaised) {
                // Unhover heading.
                Overlays.editOverlay(menuHeaderHeadingOverlay, {
                    localPosition: MENU_HEADER_HEADING_PROPERTIES.localPosition,
                    color: UIT.colors.baseGray,
                    emissive: false
                });
                Overlays.editOverlay(menuHeaderTitleOverlay, {
                    url: optionsHeadingURL,
                    scale: optionsHeadingScale
                });
                Overlays.editOverlay(menuHeaderIconOverlay, {
                    visible: true
                });
                isOptionsHeadingRaised = false;
            }
        }

        // Intersection details for menus and options.
        intersectionOverlays = null;
        intersectionEnabled = null;
        if (intersection.overlayID) {
            intersectedItem = menuOverlays.indexOf(intersection.overlayID);
            if (intersectedItem !== NONE) {
                intersectionItems = MENU_ITEMS;
                intersectionOverlays = menuOverlays;
            } else {
                intersectedItem = optionsOverlays.indexOf(intersection.overlayID);
                if (intersectedItem !== NONE) {
                    intersectionItems = optionsItems;
                    intersectionOverlays = optionsOverlays;
                    intersectionEnabled = optionsEnabled;
                }
            }
        }

        // Highlight clickable item.
        if (intersectedItem !== highlightedItem || intersectionOverlays !== highlightedSource) {
            if (intersectedItem !== NONE && intersectionItems[intersectedItem] &&
                    (intersectionItems[intersectedItem].command !== undefined
                    || intersectionItems[intersectedItem].callback !== undefined)) {
                if (isHighlightingMenuButton) {
                    // Lower old menu button.
                    Overlays.editOverlay(menuHoverOverlays[highlightedItem], {
                        localPosition: UI_ELEMENTS.menuButton.hoverButton.properties.localPosition,
                        visible: false
                    });
                } else if (isHighlightingSlider || isHighlightingColorCircle) {
                    // Lower old slider or color circle.
                    Overlays.editOverlay(highlightedSource[highlightedItem], {
                        localPosition: highlightedItems[highlightedItem].properties.localPosition
                    });
                }
                // Update status variables.
                highlightedItem = intersectedItem;
                highlightedItems = intersectionItems;
                isHighlightingButton = BUTTON_UI_ELEMENTS.indexOf(intersectionItems[highlightedItem].type) !== NONE;
                isHighlightingMenuButton = intersectionItems[highlightedItem].type === "menuButton";
                isHighlightingSlider = SLIDER_UI_ELEMENTS.indexOf(intersectionItems[highlightedItem].type) !== NONE;
                isHighlightingColorCircle = COLOR_CIRCLE_UI_ELEMENTS.indexOf(intersectionItems[highlightedItem].type) !== NONE;
                isHighlightingPicklist = PICKLIST_UI_ELEMENTS.indexOf(intersectionItems[highlightedItem].type) !== NONE;
                if (isHighlightingMenuButton) {
                    // Raise new menu button.
                    Overlays.editOverlay(menuHoverOverlays[highlightedItem], {
                        localPosition: Vec3.sum(UI_ELEMENTS.menuButton.hoverButton.properties.localPosition, MENU_RAISE_DELTA),
                        visible: true
                    });
                } else if (isHighlightingSlider || isHighlightingColorCircle) {
                    // Raise new slider or color circle.
                    localPosition = intersectionItems[highlightedItem].properties.localPosition;
                    Overlays.editOverlay(intersectionOverlays[highlightedItem], {
                        localPosition: Vec3.sum(localPosition, ITEM_RAISE_DELTA)
                    });
                }
                // Highlight new item. (The existence of a command or callback infers that the item should be highlighted.)
                parentProperties = Overlays.getProperties(intersectionOverlays[intersectedItem],
                    ["dimensions", "localPosition"]);
                if (isHighlightingColorCircle) {
                    // Cylinder used has different coordinate system to other elements.
                    // TODO: Should be able to remove this special case when UI look is reworked.
                    Overlays.editOverlay(highlightOverlay, {
                        parentID: intersectionOverlays[intersectedItem],
                        dimensions: {
                            x: parentProperties.dimensions.x + HIGHLIGHT_PROPERTIES.xDelta,
                            y: parentProperties.dimensions.z + HIGHLIGHT_PROPERTIES.yDelta,
                            z: HIGHLIGHT_PROPERTIES.zDimension
                        },
                        localPosition: {
                            x: HIGHLIGHT_PROPERTIES.properties.localPosition.x,
                            y: HIGHLIGHT_PROPERTIES.properties.localPosition.z,
                            z: HIGHLIGHT_PROPERTIES.properties.localPosition.y
                        },
                        localRotation: Quat.fromVec3Degrees({ x: 90, y: 0, z: 0 }),
                        color: HIGHLIGHT_PROPERTIES.properties.color,
                        visible: true
                    });
                } else if (!isHighlightingMenuButton) {
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
                }
            } else if (highlightedItem !== NONE) {
                // Un-highlight previous button.
                Overlays.editOverlay(highlightOverlay, {
                    visible: false
                });
                if (isHighlightingMenuButton) {
                    // Lower menu button.
                    Overlays.editOverlay(menuHoverOverlays[highlightedItem], {
                        localPosition: UI_ELEMENTS.menuButton.hoverButton.properties.localPosition,
                        visible: false
                    });
                    // Lower slider or color circle.
                } else if (isHighlightingSlider || isHighlightingColorCircle) {
                    Overlays.editOverlay(highlightedSource[highlightedItem], {
                        localPosition: highlightedItems[highlightedItem].properties.localPosition
                    });
                }
                // Update status variables.
                highlightedItem = NONE;
                isHighlightingButton = false;
                isHighlightingMenuButton = false;
                isHighlightingSlider = false;
                isHighlightingColorCircle = false;
                isHighlightingPicklist = false;
            }
            highlightedSource = intersectionOverlays;
        }

        // Press/unpress button.
        if ((pressedItem && intersectedItem !== pressedItem.index) || intersectionOverlays !== pressedSource
                || isTriggerClicked !== (pressedItem !== null)) {
            if (pressedItem) {
                // Unpress previous button.
                Overlays.editOverlay(intersectionOverlays[pressedItem.index], {
                    localPosition: pressedItem.localPosition
                });
                pressedItem = null;
                pressedSource = null;
            }
            if (isHighlightingButton && (intersectionEnabled === null || intersectionEnabled[intersectedItem])
                    && isTriggerClicked && !wasTriggerClicked) {
                // Press new button.
                localPosition = intersectionItems[intersectedItem].properties.localPosition;
                if (!isHighlightingMenuButton) {
                    Overlays.editOverlay(intersectionOverlays[intersectedItem], {
                        localPosition: Vec3.sum(localPosition, BUTTON_PRESS_DELTA)
                    });
                }
                pressedSource = intersectionOverlays;
                pressedItem = {
                    index: intersectedItem,
                    localPosition: localPosition
                };

                // Button press actions.
                if (intersectionOverlays === menuOverlays) {
                    openOptions(intersectionItems[intersectedItem]);
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
                Vec3.multiplyQbyV(sliderProperties.orientation, { x: 0, y: -overlayDimensions.y / 2, z: 0 }));
            fraction = Vec3.dot(Vec3.subtract(intersection.intersection, basePoint),
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
                Vec3.multiplyQbyV(sliderProperties.orientation, { x: 0, y: -overlayDimensions.y / 2, z: 0 }));
            fraction = Vec3.dot(Vec3.subtract(intersection.intersection, basePoint),
                Vec3.multiplyQbyV(sliderProperties.orientation, Vec3.UNIT_Y)) / overlayDimensions.y;
            fraction = adjustSliderFraction(fraction);
            Overlays.editOverlay(optionsSliderData[intersectedItem].value, {
                localPosition: Vec3.sum(optionsSliderData[intersectedItem].offset,
                    { x: 0, y: (fraction - 0.5) * overlayDimensions.y, z: 0 })
            });
            if (intersectionItems[intersectedItem].command) {
                doCommand(intersectionItems[intersectedItem].command.method, fraction);
            }
        }

        // Color circle update.
        if (intersectionItems && intersectionItems[intersectedItem].type === "colorCircle" && controlHand.triggerClicked()) {
            sliderProperties = Overlays.getProperties(intersection.overlayID, ["position", "orientation"]);
            delta = Vec3.multiplyQbyV(Quat.inverse(sliderProperties.orientation),
                Vec3.subtract(intersection.intersection, sliderProperties.position));
            radius = Vec3.length(delta);
            if (radius > hsvControl.circle.radius) {
                delta = Vec3.multiply(hsvControl.circle.radius / radius, delta);
            }
            Overlays.editOverlay(optionsColorData[intersectedItem].value, {
                localPosition: Vec3.sum(optionsColorData[intersectedItem].offset,
                    { x: delta.x, y: 0, z: delta.z })
            });
            if (intersectionItems[intersectedItem].command) {
                // Cartesian planar coordinates.
                x = -delta.z;  // Coordinates based on rotate cylinder entity. TODO: Use FBX model instead of cylinder entity.
                y = -delta.x;  // ""
                s = Math.sqrt(x * x + y * y) / hsvControl.circle.radius;
                h = Math.atan2(y, x) / (2 * Math.PI);
                if (h < 0) {
                    h = h + 1;
                }
                doCommand(intersectionItems[intersectedItem].command.method, { h: h, s: s });
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

        wasTriggerClicked = isTriggerClicked;
    }

    function display() {
        // Creates and shows menu entities.
        var handJointIndex,
            properties,
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
            App.log(side, "ERROR: ToolsMenu: Hand joint index isn't available!");
            return;
        }

        // Menu origin.
        properties = Object.clone(MENU_ORIGIN_PROPERTIES);
        properties.parentJointIndex = handJointIndex;
        properties.localPosition = Vec3.sum(menuOriginLocalPosition, { x: panelLateralOffset, y: 0, z: 0 });
        properties.localRotation = menuOriginLocalRotation;
        menuOriginOverlay = Overlays.addOverlay("sphere", properties);

        // Header.
        properties = Object.clone(MENU_HEADER_PROPERTIES);
        properties.parentID = menuOriginOverlay;
        menuHeaderOverlay = Overlays.addOverlay("cube", properties);
        properties = Object.clone(MENU_HEADER_HEADING_PROPERTIES);
        properties.parentID = menuHeaderOverlay;
        menuHeaderHeadingOverlay = Overlays.addOverlay("cube", properties);
        properties = Object.clone(MENU_HEADER_BAR_PROPERTIES);
        properties.parentID = menuHeaderHeadingOverlay;
        menuHeaderBarOverlay = Overlays.addOverlay("cube", properties);

        // Heading content.
        properties = Object.clone(MENU_HEADER_BACK_PROPERTIES);
        properties.parentID = menuHeaderHeadingOverlay;
        properties.url = Script.resolvePath(properties.url);
        menuHeaderBackOverlay = Overlays.addOverlay("image3d", properties);
        properties = Object.clone(MENU_HEADER_TITLE_PROPERTIES);
        properties.parentID = menuHeaderHeadingOverlay;
        properties.url = Script.resolvePath(properties.url);
        menuHeaderTitleOverlay = Overlays.addOverlay("image3d", properties);
        properties = Object.clone(MENU_HEADER_ICON_PROPERTIES);
        properties.parentID = menuHeaderHeadingOverlay;
        properties.url = Script.resolvePath(properties.url);
        menuHeaderIconOverlay = Overlays.addOverlay("image3d", properties);

        // Panel background.
        properties = Object.clone(MENU_PANEL_PROPERTIES);
        properties.parentID = menuOriginOverlay;
        menuPanelOverlay = Overlays.addOverlay("cube", properties);

        // Menu items.
        openMenu();

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
        isHighlightingMenuButton = false;
        isHighlightingSlider = false;
        isHighlightingColorCircle = false;
        isHighlightingPicklist = false;
        isPicklistOpen = false;
        pressedItem = null;
        pressedSource = null;
        isPicklistPressed = false;
        isPicklistItemPressed = false;
        isTriggerClicked = false;
        wasTriggerClicked = false;
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
        if (!isDisplaying) {
            return;
        }
        Overlays.deleteOverlay(menuOriginOverlay);  // Automatically deletes all other overlays because they're children.
        menuOverlays = [];
        menuHoverOverlays = [];
        optionsOverlays = [];
        isDisplaying = false;
    }

    function destroy() {
        clear();
    }

    return {
        COLOR_TOOL: COLOR_TOOL,
        SCALE_TOOL: SCALE_TOOL,
        CLONE_TOOL: CLONE_TOOL,
        GROUP_TOOL: GROUP_TOOL,
        PHYSICS_TOOL: PHYSICS_TOOL,
        DELETE_TOOL: DELETE_TOOL,
        iconInfo: getIconInfo,
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

ToolsMenu.prototype = {};
