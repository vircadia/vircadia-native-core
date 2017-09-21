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
        menuIconOverlays = [],
        menuLabelOverlays = [],
        menuEnabled = [],

        optionsOverlays = [],
        optionsOverlaysIDs = [],  // Text ids (names) of options overlays.
        optionsOverlaysLabels = [],  // Overlay IDs of labels for optionsOverlays.
        optionsOverlaysSublabels = [],  // Overlay IDs of sublabels for optionsOverlays.
        optionsSliderData = [],  // Uses same index values as optionsOverlays.
        optionsColorData = [],  // Uses same index values as optionsOverlays.
        optionsExtraOverlays = [],
        optionsEnabled = [],
        optionsSettings = {
            //  <option item id>: { 
            //      key: <Settings key to store value in>
            //      value: <current value>
            //  }
            //  For reliable access, use the values from optionsSettings rather than doing Settings.getValue() - Settings values
            //  are not necessarily updated instantaneously.
        },
        optionsToggles = {},  // Cater for toggle buttons without a setting.

        swatchHighlightOverlay = null,

        staticOverlays = [],

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
            // Invisible box to catch laser intersections while menu heading and bar move inside.
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

        EMPTY_SWATCH_COLOR = UIT.colors.baseGrayShadow,
        SWATCH_HIGHLIGHT_DELTA = 0.0020,

        UI_ELEMENTS = {
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
                    },
                    highlightColor: UIT.colors.darkGray
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
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.menuButtonSublabelYOffset,
                            z: -UIT.dimensions.itemCollisionZone.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        color: UIT.colors.lightGrayText
                    }
                }
            },
            "button": {
                overlay: "cube",
                properties: {
                    dimensions: UIT.dimensions.buttonDimensions,
                    localRotation: Quat.ZERO,
                    color: UIT.colors.baseGrayShadow,
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                },
                label: {
                    // Relative to button.
                    localPosition: {
                        x: 0,
                        y: 0,
                        z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                    },
                    color: UIT.colors.white
                }
            },
            "toggleButton": {
                overlay: "cube",
                properties: {
                    dimensions: UIT.dimensions.buttonDimensions,
                    localRotation: Quat.ZERO,
                    color: UIT.colors.baseGrayShadow,
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                },
                onColor: UIT.colors.greenHighlight,
                offColor: UIT.colors.baseGrayShadow,
                onHoverColor: UIT.colors.greenShadow,
                offHoverColor: UIT.colors.darkGray,
                label: {
                    // Relative to toggleButton.
                    localPosition: {
                        x: 0,
                        y: 0,
                        z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                    },
                    color: UIT.colors.white
                },
                sublabel: {
                    // Relative to toggleButton.
                    localPosition: {
                        x: 0,
                        y: 0,
                        z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                    },
                    color: UIT.colors.lightGray
                }
                /*
                onSublabel: { }  // Optional properties to update sublabel with.
                offSublabel: { }  // Optional properties to update sublabel with.
                */
            },
            "swatch": {
                overlay: "shape",
                properties: {
                    shape: "Cylinder",
                    dimensions: { x: 0.0294, y: UIT.dimensions.buttonDimensions.z, z: 0.0294 },
                    localRotation: Quat.fromVec3Degrees({ x: 90, y: 0, z: -90 }),
                    color: EMPTY_SWATCH_COLOR,
                    emissive: true,  // TODO: This has no effect.
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                }
                // Must have a setting property in order to function property.
                // Setting property may optionally include a defaultValue.
                // A setting value of "" means that the swatch is unpopulated.
            },
            "swatchHighlight": {
                overlay: "shape",
                properties: {
                    shape: "Cylinder",
                    dimensions: {
                        x: 0.0294 + SWATCH_HIGHLIGHT_DELTA,
                        y: UIT.dimensions.buttonDimensions.z,
                        z: 0.0294 + SWATCH_HIGHLIGHT_DELTA
                    },
                    localRotation: Quat.ZERO,  // Relative to swatch.
                    localPosition: { x: 0, y: -0.0001, z: 0 },
                    color: UIT.colors.faintGray,
                    emissive: true,  // TODO: This has no effect.
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: true,
                    visible: false
                }

            },
            "square": {
                overlay: "cube",  // Emulate a 2D square with a cube.
                properties: {
                    localRotation: Quat.ZERO,
                    color: UIT.colors.baseGrayShadow,
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
                    alpha: 1.0,
                    emissive: true,
                    ignoreRayIntersection: true,
                    isFacingAvatar: false,
                    visible: true
                }
            },
            "horizontalRule": {
                overlay: "image3d",
                properties: {
                    url: "../assets/horizontal-rule.svg",
                    dimensions: { x: UIT.dimensions.panel.x - 2 * UIT.dimensions.leftMargin, y: 0.001 },
                    localRotation: Quat.ZERO,
                    color: UIT.colors.baseGrayShadow,
                    alpha: 1.0,
                    solid: true,
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
            "barSlider": {
                // Invisible cube to catch laser intersections; value and remainder entities move inside.
                // Values range between 0.0 and 1.0.
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.02, y: 0.1, z: 0.01 },
                    localRotation: Quat.ZERO,
                    alpha: 0.0,  // Invisible.
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true  // Catch laser intersections.
                },
                label: {
                    // Relative to barSlider.
                    color: UIT.colors.white
                },
                zeroIndicator: {
                    overlay: "image3d",
                    properties: {
                        url: "../assets/horizontal-rule.svg",
                        dimensions: { x: 0.02, y: 0.001 },
                        localRotation: Quat.ZERO,
                        color: UIT.colors.lightGrayText,
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: true,
                        isFacingAvatar: false,
                        visible: true
                    }
                },
                sliderValue: {
                    overlay: "cube",
                    properties: {
                        dimensions: { x: 0.02, y: 0.03, z: 0.01 },
                        localPosition: { x: 0, y: 0.035, z: 0 },
                        localRotation: Quat.ZERO,
                        color: UIT.colors.greenHighlight,
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: true,
                        visible: true
                    }
                },
                sliderRemainder: {
                    overlay: "cube",
                    properties: {
                        dimensions: { x: 0.02, y: 0.07, z: 0.01 },
                        localPosition: { x: 0, y: -0.015, z: 0 },
                        localRotation: Quat.ZERO,
                        color: UIT.colors.baseGrayShadow,
                        alpha: 1.0,
                        solid: true,
                        ignoreRayIntersection: true,
                        visible: true
                    }
                }
            },
            "imageSlider": {  // Values range between 0.0 and 1.0.
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.0160, y: 0.1229, z: UIT.dimensions.buttonDimensions.z },
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
                    dimensions: { x: 0.1229, y: UIT.dimensions.buttonDimensions.z, z: 0.1229 },
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
                    color: UIT.colors.baseGrayShadow,
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: true
                },
                label: {
                    // Relative to picklist.
                    color: UIT.colors.white
                }
            },
            "picklistItem": {  // Note: When using, declare before picklist item that it's being used in.
                overlay: "cube",
                properties: {
                    dimensions: { x: 0.1416, y: 0.0280, z: UIT.dimensions.buttonDimensions.z },
                    localPosition: Vec3.ZERO,
                    localRotation: Quat.ZERO,
                    color: UIT.colors.baseGrayShadow,
                    alpha: 1.0,
                    solid: true,
                    ignoreRayIntersection: false,
                    visible: false
                },
                label: {
                    // Relative to picklistItem.
                    color: UIT.colors.white
                }
            }
        },

        BUTTON_UI_ELEMENTS = ["button", "menuButton", "toggleButton", "swatch"],

        MENU_HOVER_DELTA = { x: 0, y: 0, z: 0.006 },
        OPTION_HOVER_DELTA = { x: 0, y: 0, z: 0.002 },
        PICKLIST_HOVER_DELTA = { x: 0, y: 0, z: 0.006 },
        BUTTON_PRESS_DELTA = { x: 0, y: 0, z: -0.002 },

        MIN_BAR_SLIDER_DIMENSION = 0.0001,  // Avoid visual artifact for 0 slider values.

        PHYSICS_SLIDER_PRESETS = {
            // Slider values in the range 0.0 to 1.0.
            // Note: Friction values give the desired linear and angular damping values but friction values are a somewhat out,
            // especially for the balloon.
            presetDefault:    { gravity: 0.5,      bounce: 0.5,  friction: 0.5,      density: 0.5      },
            presetLead:       { gravity: 0.5,      bounce: 0.0,  friction: 0.5,      density: 1.0      },
            presetWood:       { gravity: 0.5,      bounce: 0.4,  friction: 0.5,      density: 0.5      },
            presetIce:        { gravity: 0.5,      bounce: 0.99, friction: 0.151004, density: 0.349485 },
            presetRubber:     { gravity: 0.5,      bounce: 0.99, friction: 0.5,      density: 0.5      },
            presetCotton:     { gravity: 0.587303, bounce: 0.0,  friction: 0.931878, density: 0.0      },
            presetTumbleweed: { gravity: 0.595893, bounce: 0.7,  friction: 0.5,      density: 0.0      },
            presetZeroG:      { gravity: 0.596844, bounce: 0.5,  friction: 0.5,      density: 0.5      },
            presetBalloon:    { gravity: 0.606313, bounce: 0.99, friction: 0.151004, density: 0.0      }
        },

        OPTONS_PANELS = {
            colorOptions: [
                {
                    id: "colorSwatchesLabel",
                    type: "image",
                    properties: {
                        color: UIT.colors.white,
                        url: "../assets/tools/color/swatches-label.svg",
                        scale: 0.0345,
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0345 / 2,
                            y: UIT.dimensions.panel.y / 2 - UIT.dimensions.topMargin - 0.0047 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "colorRule1",
                    type: "horizontalRule",
                    properties: {
                        dimensions: { x: 0.0668, y: 0.001 },
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0668 / 2,
                            y: UIT.dimensions.panel.y / 2 - 0.0199,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "colorSwatch1",
                    type: "swatch",
                    properties: {
                        localPosition: {
                            x: -0.0935,
                            y: 0.0513,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    setting: {
                        key: "VREdit.colorTool.swatch1Color",
                        property: "color"
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
                        localPosition: {
                            x: -0.0561,
                            y: 0.0513,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    setting: {
                        key: "VREdit.colorTool.swatch2Color",
                        property: "color"
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
                        localPosition: {
                            x: -0.0935,
                            y: 0.0153,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    setting: {
                        key: "VREdit.colorTool.swatch3Color",
                        property: "color"
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
                        localPosition: {
                            x: -0.0561,
                            y: 0.0153,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    setting: {
                        key: "VREdit.colorTool.swatch4Color",
                        property: "color"
                    },
                    command: {
                        method: "setColorPerSwatch"
                    },
                    clear: {
                        method: "clearSwatch"
                    }
                },
                {
                    id: "colorSwatch5",
                    type: "swatch",
                    properties: {
                        localPosition: {
                            x: -0.0935,
                            y: -0.0207,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    setting: {
                        key: "VREdit.colorTool.swatch5Color",
                        property: "color"
                    },
                    command: {
                        method: "setColorPerSwatch"
                    },
                    clear: {
                        method: "clearSwatch"
                    }
                },
                {
                    id: "colorSwatch6",
                    type: "swatch",
                    properties: {
                        localPosition: {
                            x: -0.0561,
                            y: -0.0207,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    setting: {
                        key: "VREdit.colorTool.swatch6Color",
                        property: "color"
                    },
                    command: {
                        method: "setColorPerSwatch"
                    },
                    clear: {
                        method: "clearSwatch"
                    }
                },
                {
                    id: "colorRule2",
                    type: "horizontalRule",
                    properties: {
                        dimensions: { x: 0.0668, y: 0.001 },
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0668 / 2,
                            y: -UIT.dimensions.panel.y / 2 + 0.0481,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "colorCircle",
                    type: "colorCircle",
                    properties: {
                        localPosition: {
                            x: 0.04675,
                            y: 0.01655,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    imageURL: "../assets/tools/color/color-circle.png",
                    imageOverlayURL: "../assets/tools/color/color-circle-black.png",
                    command: {
                        method: "setColorPerCircle"
                    }
                },
                {
                    id: "colorSlider",
                    type: "imageSlider",
                    properties: {
                        localPosition: {
                            x: 0.04675,
                            y: -0.0620,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        },
                        localRotation: Quat.fromVec3Degrees({ x: 0, y: 0, z: -90 })
                    },
                    useBaseColor: true,
                    imageURL: "../assets/tools/color/slider-white.png",
                    // Alpha PNG created by overlaying two black-to-transparent gradients in order to achieve visual effect.
                    imageOverlayURL: "../assets/tools/color/slider-alpha.png",
                    command: {
                        method: "setColorPerSlider"
                    }
                },
                {
                    id: "colorRule3",
                    type: "horizontalRule",
                    properties: {
                        dimensions: { x: 0.1229, y: 0.001 },
                        localPosition: {
                            x: 0.04675,
                            y: -0.0781,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "pickColor",
                    type: "toggleButton",
                    properties: {
                        dimensions: { x: 0.0294, y: 0.0280, z: UIT.dimensions.buttonDimensions.z },
                        localPosition: {
                            x: -0.0935,
                            y: -0.064,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        url: "../assets/tools/color/pick-color-label.svg",
                        scale: 0.0120
                    },
                    command: {
                        method: "togglePickColor"
                    }
                },
                {
                    id: "currentColor",
                    type: "square",
                    properties: {
                        dimensions: { x: 0.0294, y: 0.0280, z: UIT.dimensions.imageOverlayOffset },
                        localPosition: {
                            x: -0.0561,
                            y: -0.064,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    setting: {
                        key: "VREdit.colorTool.currentColor",
                        property: "color",
                        defaultValue: { red: 255, green: 255, blue: 255 },
                        command: "setPickColor"
                    }
                }
            ],
            scaleOptions: [
                {
                    id: "stretchActionsLabel",
                    type: "image",
                    properties: {
                        color: UIT.colors.white,
                        url: "../assets/tools/common/actions-label.svg",
                        scale: 0.0276,
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0276 / 2,
                            y: UIT.dimensions.panel.y / 2 - UIT.dimensions.topMargin - 0.0047 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "stretchRule1",
                    type: "horizontalRule",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.panel.y / 2 - 0.0199,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "stretchFinishButton",
                    type: "button",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.panel.y / 2 - 0.0280 - UIT.dimensions.buttonDimensions.y / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        url: "../assets/tools/common/finish-label.svg",
                        scale: 0.0318
                    },
                    command: {
                        method: "clearTool"
                    }
                },
                {
                    id: "stretchRule2",
                    type: "horizontalRule",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.panel.y / 2 - 0.1197,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "stretchInfoIcon",
                    type: "image",
                    properties: {
                        url: "../assets/tools/common/info-icon.svg",
                        dimensions: { x: 0.0321, y: 0.0320 },
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0321 / 2,
                            y: -UIT.dimensions.panel.y / 2 + 0.0200 + 0.0320 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        color: UIT.colors.white  // Icon SVG is already lightGray color.
                    }
                },
                {
                    id: "stretchInfo",
                    type: "image",
                    properties: {
                        url: "../assets/tools/stretch/info-text.svg",
                        scale: 0.1340,
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + 0.0679 + 0.1340 / 2,
                            y: -UIT.dimensions.panel.y / 2 + 0.0200 + 0.0320 / 2,  // Center on info icon.
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        color: UIT.colors.white
                    }
                }
            ],
            cloneOptions: [
                {
                    id: "cloneActionsLabel",
                    type: "image",
                    properties: {
                        color: UIT.colors.white,
                        url: "../assets/tools/common/actions-label.svg",
                        scale: 0.0276,
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0276 / 2,
                            y: UIT.dimensions.panel.y / 2 - UIT.dimensions.topMargin - 0.0047 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "cloneRule1",
                    type: "horizontalRule",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.panel.y / 2 - 0.0199,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "cloneFinishButton",
                    type: "button",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.panel.y / 2 - 0.0280 - UIT.dimensions.buttonDimensions.y / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        url: "../assets/tools/common/finish-label.svg",
                        scale: 0.0318
                    },
                    command: {
                        method: "clearTool"
                    }
                }
            ],
            groupOptions: [
                {
                    id: "groupActionsLabel",
                    type: "image",
                    properties: {
                        color: UIT.colors.white,
                        url: "../assets/tools/common/actions-label.svg",
                        scale: 0.0276,
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0276 / 2,
                            y: UIT.dimensions.panel.y / 2 - UIT.dimensions.topMargin - 0.0047 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "groupRule1",
                    type: "horizontalRule",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.panel.y / 2 - 0.0199,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "groupButton",
                    type: "button",
                    properties: {
                        dimensions: {
                            x: UIT.dimensions.buttonDimensions.x,
                            y: 0.0680,
                            z: UIT.dimensions.buttonDimensions.z
                        },
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.panel.y / 2 - 0.0280 - 0.0680 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        },
                        color: UIT.colors.baseGrayShadow
                    },
                    enabledColor: UIT.colors.greenHighlight,
                    highlightColor: UIT.colors.greenShadow,
                    label: {
                        url: "../assets/tools/group/group-label.svg",
                        scale: 0.0351,
                        color: UIT.colors.baseGray
                    },
                    labelEnabledColor: UIT.colors.white,
                    callback: {
                        method: "groupButton"
                    }
                },
                {
                    id: "ungroupButton",
                    type: "button",
                    properties: {
                        dimensions: {
                            x: UIT.dimensions.buttonDimensions.x,
                            y: 0.0680,
                            z: UIT.dimensions.buttonDimensions.z
                        },
                        localPosition: {
                            x: 0,
                            y: -UIT.dimensions.panel.y / 2 + 0.0120 + 0.0680 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        },
                        color: UIT.colors.baseGrayShadow

                    },
                    enabledColor: UIT.colors.redHighlight,
                    highlightColor: UIT.colors.redAccent,
                    label: {
                        url: "../assets/tools/group/ungroup-label.svg",
                        scale: 0.0496,
                        color: UIT.colors.baseGray
                    },
                    labelEnabledColor: UIT.colors.white,
                    callback: {
                        method: "ungroupButton"
                    }
                }
            ],
            physicsOptions: [
                {
                    id: "physicsPropertiesLabel",
                    type: "image",
                    properties: {
                        color: UIT.colors.white,
                        url: "../assets/tools/physics/properties-label.svg",
                        scale: 0.0376,
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0376 / 2,
                            y: UIT.dimensions.panel.y / 2 - UIT.dimensions.topMargin - 0.0047 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "physicsRule1",
                    type: "horizontalRule",
                    properties: {
                        dimensions: { x: 0.0668, y: 0.001 },
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0668 / 2,
                            y: UIT.dimensions.panel.y / 2 - 0.0199,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "gravityToggle",
                    type: "toggleButton",
                    properties: {
                        dimensions: { x: 0.0668, y: 0.0280, z: UIT.dimensions.buttonDimensions.z },
                        localPosition: {
                            x: -0.0748,
                            y: 0.0480,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        localPosition: {
                            x: 0,
                            y: 0.0034,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        url: "../assets/tools/physics/buttons/gravity-label.svg",
                        scale: 0.0240,
                        color: UIT.colors.white
                    },
                    sublabel: {
                        localPosition: {
                            x: 0,
                            y: -0.0034,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        url: "../assets/tools/physics/buttons/off-label.svg",
                        scale: 0.0104,
                        color: UIT.colors.white  // SVG has gray color.
                    },
                    onSublabel: {
                        url: "../assets/tools/physics/buttons/on-label.svg",
                        scale: 0.0081
                    },
                    offSublabel: {
                        url: "../assets/tools/physics/buttons/off-label.svg",
                        scale: 0.0104
                    },
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
                    id: "collideToggle",
                    type: "toggleButton",
                    properties: {
                        dimensions: { x: 0.0668, y: 0.0280, z: UIT.dimensions.buttonDimensions.z },
                        localPosition: {
                            x: -0.0748,
                            y: 0.0120,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        localPosition: {
                            x: 0,
                            y: 0.0034,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        url: "../assets/tools/physics/buttons/collisions-label.svg",
                        scale: 0.0338,
                        color: UIT.colors.white
                    },
                    sublabel: {
                        localPosition: {
                            x: 0,
                            y: -0.0034,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        url: "../assets/tools/physics/buttons/off-label.svg",
                        scale: 0.0104,
                        color: UIT.colors.white  // SVG has gray color.
                    },
                    onSublabel: {
                        url: "../assets/tools/physics/buttons/on-label.svg",
                        scale: 0.0081
                    },
                    offSublabel: {
                        url: "../assets/tools/physics/buttons/off-label.svg",
                        scale: 0.0104
                    },
                    setting: {
                        key: "VREdit.physicsTool.collideOn",
                        defaultValue: true,
                        callback: "setCollideOn"
                    },
                    command: {
                        method: "setCollideOn"
                    }
                },
                {
                    id: "grabToggle",
                    type: "toggleButton",
                    properties: {
                        dimensions: { x: 0.0668, y: 0.0280, z: UIT.dimensions.buttonDimensions.z },
                        localPosition: {
                            x: -0.0748,
                            y: -0.0240,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        localPosition: {
                            x: 0,
                            y: 0.0034,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        url: "../assets/tools/physics/buttons/grabbable-label.svg",
                        scale: 0.0334,
                        color: UIT.colors.white
                    },
                    sublabel: {
                        localPosition: {
                            x: 0,
                            y: -0.0034,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        url: "../assets/tools/physics/buttons/off-label.svg",
                        scale: 0.0104,
                        color: UIT.colors.white  // SVG has gray color.
                    },
                    onSublabel: {
                        url: "../assets/tools/physics/buttons/on-label.svg",
                        scale: 0.0081
                    },
                    offSublabel: {
                        url: "../assets/tools/physics/buttons/off-label.svg",
                        scale: 0.0104
                    },
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
                    id: "physicsPresetsLabel",
                    type: "image",
                    properties: {
                        color: UIT.colors.white,
                        url: "../assets/tools/physics/presets-label.svg",
                        scale: 0.0270,
                        localPosition: {
                            x: UIT.dimensions.panel.x / 2 - UIT.dimensions.rightMargin - 0.1416 + 0.0270 / 2,
                            y: UIT.dimensions.panel.y / 2 - UIT.dimensions.topMargin - 0.0047 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "physicsRule2",
                    type: "horizontalRule",
                    properties: {
                        dimensions: { x: 0.1416, y: 0.001 },
                        localPosition: {
                            x: UIT.dimensions.panel.x / 2 - UIT.dimensions.rightMargin - 0.1416 / 2,
                            y: UIT.dimensions.panel.y / 2 - 0.0199,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "presetDefault",
                    type: "picklistItem",
                    label: {
                        url: "../assets/tools/physics/presets/default-label.svg",
                        scale: 0.0436,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0436 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    command: {
                        method: "pickPhysicsPreset",
                        value: "default"
                    }
                },
                {
                    id: "presetLead",
                    type: "picklistItem",
                    label: {
                        url: "../assets/tools/physics/presets/lead-label.svg",
                        scale: 0.0243,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0243 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetWood",
                    type: "picklistItem",
                    label: {
                        url: "../assets/tools/physics/presets/wood-label.svg",
                        scale: 0.0316,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0316 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetIce",
                    type: "picklistItem",
                    label: {
                        url: "../assets/tools/physics/presets/ice-label.svg",
                        scale: 0.0144,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0144 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetRubber",
                    type: "picklistItem",
                    label: {
                        url: "../assets/tools/physics/presets/rubber-label.svg",
                        scale: 0.0400,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0400 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetCotton",
                    type: "picklistItem",
                    label: {
                        url: "../assets/tools/physics/presets/cotton-label.svg",
                        scale: 0.0393,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0393 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetTumbleweed",
                    type: "picklistItem",
                    label: {
                        url: "../assets/tools/physics/presets/tumbleweed-label.svg",
                        scale: 0.0687,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0687 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetZeroG",
                    type: "picklistItem",
                    label: {
                        url: "../assets/tools/physics/presets/zero-g-label.svg",
                        scale: 0.0375,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0375 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "presetBalloon",
                    type: "picklistItem",
                    label: {
                        url: "../assets/tools/physics/presets/balloon-label.svg",
                        scale: 0.0459,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0459 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    command: { method: "pickPhysicsPreset" }
                },
                {
                    id: "physicsPresets",
                    type: "picklist",
                    properties: {
                        dimensions: { x: 0.1416, y: 0.0280, z: UIT.dimensions.buttonDimensions.z },
                        localPosition: {
                            x: UIT.dimensions.panel.x / 2 - UIT.dimensions.rightMargin - 0.1416 / 2,
                            y: 0.0480,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        url: "../assets/tools/physics/presets/default-label.svg",
                        scale: 0.0436,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0436 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    sublabel: {
                        url: "../assets/tools/common/down-arrow.svg",
                        scale: 0.0080,
                        localPosition: {
                            x: 0.1416 / 2 - 0.0108 - 0.0080 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        color: UIT.colors.white  // SVG is colored baseGrayHighlight
                    },
                    customLabel: {
                        url: "../assets/tools/physics/presets/custom-label.svg",
                        scale: 0.0522,
                        localPosition: {
                            x: -0.1416 / 2 + 0.017 + 0.0522 / 2,
                            y: 0,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    },
                    setting: {
                        key: "VREdit.physicsTool.physicsPreset",
                        defaultValue: "presetDefault"
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
                    id: "gravitySlider",
                    type: "barSlider",
                    properties: {
                        dimensions: { x: 0.0294, y: 0.1000, z: UIT.dimensions.buttonDimensions.z },
                        localPosition: {
                            x: -0.0187,
                            y: -0.0240,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        localPosition: {
                            x: 0,
                            y: -0.04375,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        url: "../assets/tools/physics/sliders/gravity-label.svg",
                        scale: 0.0240
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
                    id: "bounceSlider",
                    type: "barSlider",
                    properties: {
                        dimensions: { x: 0.0294, y: 0.1000, z: UIT.dimensions.buttonDimensions.z },
                        localPosition: {
                            x: 0.0187,
                            y: -0.0240,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        localPosition: {
                            x: 0,
                            y: -0.04375,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        url: "../assets/tools/physics/sliders/bounce-label.svg",
                        scale: 0.0233
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
                    id: "frictionSlider",
                    type: "barSlider",
                    properties: {
                        dimensions: { x: 0.0294, y: 0.1000, z: UIT.dimensions.buttonDimensions.z },
                        localPosition: {
                            x: 0.0561,
                            y: -0.0240,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        localPosition: {
                            x: 0,
                            y: -0.04375,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        url: "../assets/tools/physics/sliders/friction-label.svg",
                        scale: 0.0258
                    },
                    setting: {
                        key: "VREdit.physicsTool.friction",
                        defaultValue: 0.5,
                        callback: "setFriction"
                    },
                    command: {
                        method: "setFriction"
                    }
                },
                {
                    id: "densitySlider",
                    type: "barSlider",
                    properties: {
                        dimensions: { x: 0.0294, y: 0.1000, z: UIT.dimensions.buttonDimensions.z },
                        localPosition: {
                            x: 0.0935,
                            y: -0.0240,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        localPosition: {
                            x: 0,
                            y: -0.04375,
                            z: UIT.dimensions.buttonDimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        url: "../assets/tools/physics/sliders/density-label.svg",
                        scale: 0.0241
                    },
                    setting: {
                        key: "VREdit.physicsTool.density",
                        defaultValue: 0.5,
                        callback: "setDensity"
                    },
                    command: {
                        method: "setDensity"
                    }
                }
            ],
            deleteOptions: [
                {
                    id: "deleteActionsLabel",
                    type: "image",
                    properties: {
                        color: UIT.colors.white,
                        url: "../assets/tools/common/actions-label.svg",
                        scale: 0.0276,
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0276 / 2,
                            y: UIT.dimensions.panel.y / 2 - UIT.dimensions.topMargin - 0.0047 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "deleteRule1",
                    type: "horizontalRule",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.panel.y / 2 - 0.0199,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "deleteFinishButton",
                    type: "button",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.panel.y / 2 - 0.0280 - UIT.dimensions.buttonDimensions.y / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.buttonDimensions.z / 2
                        }
                    },
                    label: {
                        url: "../assets/tools/common/finish-label.svg",
                        scale: 0.0318
                    },
                    command: {
                        method: "clearTool"
                    }
                },
                {
                    id: "deleteRule2",
                    type: "horizontalRule",
                    properties: {
                        localPosition: {
                            x: 0,
                            y: UIT.dimensions.panel.y / 2 - 0.1197,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        }
                    }
                },
                {
                    id: "deleteInfoIcon",
                    type: "image",
                    properties: {
                        url: "../assets/tools/common/info-icon.svg",
                        dimensions: { x: 0.0321, y: 0.0320 },
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + UIT.dimensions.leftMargin + 0.0321 / 2,
                            y: -UIT.dimensions.panel.y / 2 + 0.0200 + 0.0320 / 2,
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        color: UIT.colors.white  // Icon SVG is already lightGray color.
                    }
                },
                {
                    id: "deleteInfo",
                    type: "image",
                    properties: {
                        url: "../assets/tools/delete/info-text.svg",
                        scale: 0.1416,
                        localPosition: {
                            x: -UIT.dimensions.panel.x / 2 + 0.0679 + 0.1340 / 2,
                            y: -UIT.dimensions.panel.y / 2 + 0.0200 + 0.0240 / 2 + 0.0063 / 2,  // Off-center w.r.t. info icon.
                            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.imageOverlayOffset
                        },
                        color: UIT.colors.white
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
                    properties: {
                        url: "../assets/tools/color-icon.svg",
                        dimensions: { x: 0.0165, y: 0.0187 }
                    },
                    headerOffset: { x: -0.00825, y: 0.0020, z: 0 }
                },
                label: {
                    properties: {
                        url: "../assets/tools/color-label.svg",
                        scale: 0.0241
                    }
                },
                sublabel: {
                    properties: {
                        url: "../assets/tools/tool-label.svg",
                        scale: 0.0152
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
                    properties: {
                        url: "../assets/tools/stretch-icon.svg",
                        dimensions: { x: 0.0167, y: 0.0167 }
                    },
                    headerOffset: { x: -0.00835, y: 0, z: 0 }
                },
                label: {
                    properties: {
                        url: "../assets/tools/stretch-label.svg",
                        scale: 0.0311
                    }
                },
                sublabel: {
                    properties: {
                        url: "../assets/tools/tool-label.svg",
                        scale: 0.0152
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
                    properties: {
                        url: "../assets/tools/clone-icon.svg",
                        dimensions: { x: 0.0154, y: 0.0155 }
                    },
                    headerOffset: { x: -0.0077, y: 0, z: 0 }
                },
                label: {
                    properties: {
                        url: "../assets/tools/clone-label.svg",
                        scale: 0.0231
                    }
                },
                sublabel: {
                    properties: {
                        url: "../assets/tools/tool-label.svg",
                        scale: 0.0152
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
                    properties: {
                        url: "../assets/tools/group-icon.svg",
                        dimensions: { x: 0.0161, y: 0.0114 }
                    },
                    headerOffset: { x: -0.00805, y: 0, z: 0 }
                },
                label: {
                    properties: {
                        url: "../assets/tools/group-label.svg",
                        scale: 0.0250
                    }
                },
                sublabel: {
                    properties: {
                        url: "../assets/tools/tool-label.svg",
                        scale: 0.0152
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
                    properties: {
                        url: "../assets/tools/physics-icon.svg",
                        dimensions: { x: 0.0180, y: 0.0198 }
                    },
                    headerOffset: { x: -0.009, y: 0, z: 0 }
                },
                label: {
                    properties: {
                        url: "../assets/tools/physics-label.svg",
                        scale: 0.0297
                    }
                },
                sublabel: {
                    properties: {
                        url: "../assets/tools/tool-label.svg",
                        scale: 0.0152
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
                    properties: {
                        url: "../assets/tools/delete-icon.svg",
                        dimensions: { x: 0.0161, y: 0.0161 }
                    },
                    headerOffset: { x: -0.00805, y: 0, z: 0 }
                },
                label: {
                    properties: {
                        url: "../assets/tools/delete-label.svg",
                        scale: 0.0254
                    }
                },
                sublabel: {
                    properties: {
                        url: "../assets/tools/tool-label.svg",
                        scale: 0.0152
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
            },
            {
                id: "undoButton",
                type: "menuButton",
                properties: {
                    localPosition: {
                        x: MENU_ITEM_XS[2],
                        y: MENU_ITEM_YS[2],
                        z: UIT.dimensions.panel.z / 2 + UI_ELEMENTS.menuButton.properties.dimensions.z / 2
                    }
                },
                icon: {
                    properties: {
                        url: "../assets/tools/undo-icon.svg",
                        dimensions: { x: 0.0180, y: 0.0186 }
                    }
                },
                label: {
                    properties: {
                        url: "../assets/tools/undo-label.svg",
                        scale: 0.0205
                    }
                },
                callback: {
                    method: "undoAction"
                }
            },
            {
                id: "redoButton",
                type: "menuButton",
                properties: {
                    localPosition: {
                        x: MENU_ITEM_XS[3],
                        y: MENU_ITEM_YS[2],
                        z: UIT.dimensions.panel.z / 2 + UI_ELEMENTS.menuButton.properties.dimensions.z / 2
                    }
                },
                icon: {
                    properties: {
                        url: "../assets/tools/redo-icon.svg",
                        dimensions: { x: 0.0180, y: 0.0186 }
                    }
                },
                label: {
                    properties: {
                        url: "../assets/tools/redo-label.svg",
                        scale: 0.0192
                    }
                },
                callback: {
                    method: "redoAction"
                }
            }
        ],
        COLOR_TOOL = 0,  // Indexes of corresponding MENU_ITEMS item.
        SCALE_TOOL = 1,
        CLONE_TOOL = 2,
        GROUP_TOOL = 3,
        PHYSICS_TOOL = 4,
        DELETE_TOOL = 5,

        NONE = -1,

        optionsItems,
        intersectionOverlays,
        intersectionEnabled,
        hoveredItem,
        hoveredSourceOverlays,
        hoveredSourceItems,
        hoveredElementType = null,
        isHoveringButtonElement,
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
        isVisible = true,
        isOptionsOpen = false,
        isOptionsHeadingRaised = false,
        optionsHeadingURL,
        optionsHeadingScale,

        otherSide,

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
        otherSide = (side + 1) % 2;
    }

    setHand(side);

    function getOverlayIDs() {
        return [menuPanelOverlay, menuHeaderOverlay].concat(menuOverlays).concat(optionsOverlays);
    }

    function getIconInfo(tool) {
        // Provides details of tool icon, label, and sublabel images for the specified tool.
        var sublabelProperties;

        sublabelProperties = Object.clone(UI_ELEMENTS.menuButton.sublabel);
        sublabelProperties = Object.merge(sublabelProperties, MENU_ITEMS[tool].sublabel);
        return {
            icon: MENU_ITEMS[tool].icon,
            label: MENU_ITEMS[tool].label,
            sublabel: sublabelProperties
        };
    }

    function openMenu() {
        var properties,
            itemID,
            buttonID,
            overlayID,
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
            properties.visible = isVisible;
            properties.parentID = menuPanelOverlay;
            itemID = Overlays.addOverlay(UI_ELEMENTS[MENU_ITEMS[i].type].overlay, properties);
            menuOverlays[i] = itemID;
            menuEnabled[i] = true;

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
            properties.visible = isVisible;
            properties.parentID = buttonID;
            overlayID = Overlays.addOverlay(UI_ELEMENTS[UI_ELEMENTS.menuButton.icon.type].overlay, properties);
            menuIconOverlays[i] = overlayID;

            // Label.
            properties = Object.clone(UI_ELEMENTS[UI_ELEMENTS.menuButton.label.type].properties);
            properties = Object.merge(properties, UI_ELEMENTS.menuButton.label.properties);
            properties = Object.merge(properties, MENU_ITEMS[i].label.properties);
            properties.url = Script.resolvePath(properties.url);
            properties.visible = isVisible;
            properties.parentID = itemID;
            overlayID = Overlays.addOverlay(UI_ELEMENTS[UI_ELEMENTS.menuButton.label.type].overlay, properties);
            menuLabelOverlays.push(overlayID);

            // Sublabel.
            if (MENU_ITEMS[i].sublabel) {
                properties = Object.clone(UI_ELEMENTS[UI_ELEMENTS.menuButton.sublabel.type].properties);
                properties = Object.merge(properties, UI_ELEMENTS.menuButton.sublabel.properties);
                properties = Object.merge(properties, MENU_ITEMS[i].sublabel.properties);
                properties.url = Script.resolvePath(properties.url);
                properties.visible = isVisible;
                properties.parentID = itemID;
                overlayID = Overlays.addOverlay(UI_ELEMENTS[UI_ELEMENTS.menuButton.sublabel.type].overlay, properties);
                menuLabelOverlays.push(overlayID);
            }
        }
    }

    function closeMenu() {
        var i,
            length;

        for (i = 0, length = menuOverlays.length; i < length; i += 1) {
            Overlays.deleteOverlay(menuOverlays[i]);
        }

        menuOverlays = [];
        menuHoverOverlays = [];
        menuIconOverlays = [];
        menuLabelOverlays = [];
        pressedItem = null;
    }

    function openOptions(menuItem) {
        var properties,
            childProperties,
            auxiliaryProperties,
            parentID,
            sublabelModifier,
            value,
            imageOffset,
            IMAGE_OFFSET = 0.0005,
            CIRCLE_CURSOR_SPHERE_DIMENSIONS = { x: 0.01, y: 0.01, z: 0.01 },
            CIRCLE_CURSOR_GAP = 0.002,
            id,
            i,
            length;

        // Remove menu items.
        closeMenu();

        // Update header.
        optionsHeadingURL = Script.resolvePath(menuItem.title.url);
        optionsHeadingScale = menuItem.title.scale;
        Overlays.editOverlay(menuHeaderBackOverlay, { visible: isVisible });
        Overlays.editOverlay(menuHeaderTitleOverlay, {
            url: optionsHeadingURL,
            scale: optionsHeadingScale
        });
        Overlays.editOverlay(menuHeaderIconOverlay, {
            url: Script.resolvePath(menuItem.icon.properties.url),
            dimensions: menuItem.icon.properties.dimensions,
            localPosition: Vec3.sum(MENU_HEADER_ICON_OFFSET, menuItem.icon.headerOffset),
            visible: isVisible
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
            sublabelModifier = null;
            if (optionsItems[i].setting) {
                optionsSettings[optionsItems[i].id] = { key: optionsItems[i].setting.key };
                value = Settings.getValue(optionsItems[i].setting.key);
                if (value === "" && optionsItems[i].setting.defaultValue !== undefined) {
                    value = optionsItems[i].setting.defaultValue;
                }
                optionsSettings[optionsItems[i].id].value = value;
                if (value !== "") {
                    properties[optionsItems[i].setting.property] = value;
                    if (optionsItems[i].type === "toggleButton") {
                        optionsToggles[optionsItems[i].id] = value;
                        properties.color = value
                            ? UI_ELEMENTS[optionsItems[i].type].onColor
                            : UI_ELEMENTS[optionsItems[i].type].offColor;
                        if (optionsItems[i].hasOwnProperty("onSublabel")) {
                            sublabelModifier = value ? optionsItems[i].onSublabel : optionsItems[i].offSublabel;
                        }
                    } else if (optionsItems[i].type === "picklist") {
                        if (value === "custom") {
                            optionsItems[i].label = optionsItems[i].customLabel;
                        } else {
                            optionsItems[i].label = optionsItems[optionsOverlaysIDs.indexOf(value)].label;
                        }
                    }
                    if (optionsItems[i].setting.command) {
                        doCommand(optionsItems[i].setting.command, value);
                    }
                    if (optionsItems[i].setting.callback) {
                        uiCommandCallback(optionsItems[i].setting.callback, value);
                    }
                }
            } else if (optionsItems[i].type === "toggleButton") {
                optionsToggles[optionsItems[i].id] = false;  // Default to off.
            }

            optionsOverlays.push(Overlays.addOverlay(UI_ELEMENTS[optionsItems[i].type].overlay, properties));
            optionsOverlaysIDs.push(optionsItems[i].id);
            if (optionsItems[i].label) {
                childProperties = Object.clone(UI_ELEMENTS.image.properties);
                childProperties = Object.merge(childProperties, UI_ELEMENTS[optionsItems[i].type].label);
                childProperties = Object.merge(childProperties, optionsItems[i].label);
                childProperties.url = Script.resolvePath(childProperties.url);
                childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                id = Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                optionsOverlaysLabels[i] = id;
            }
            if (optionsItems[i].sublabel) {
                childProperties = Object.clone(UI_ELEMENTS.image.properties);
                childProperties = Object.merge(childProperties, UI_ELEMENTS[optionsItems[i].type].label);
                childProperties = Object.merge(childProperties, optionsItems[i].sublabel);
                if (sublabelModifier) {
                    childProperties = Object.merge(childProperties, sublabelModifier);
                }
                childProperties.url = Script.resolvePath(childProperties.url);
                childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                id = Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                optionsOverlaysSublabels[i] = id;
            }

            if (optionsItems[i].type === "barSlider") {
                // Value and remainder bars.
                optionsSliderData[i] = {};
                auxiliaryProperties = Object.clone(UI_ELEMENTS.barSlider.sliderValue.properties);
                auxiliaryProperties.localPosition = { x: 0, y: (-0.5 + value / 2) * properties.dimensions.y, z: 0 };
                auxiliaryProperties.dimensions = {
                    x: properties.dimensions.x,
                    y: Math.max(value * properties.dimensions.y, MIN_BAR_SLIDER_DIMENSION),
                    z: properties.dimensions.z
                };
                auxiliaryProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                optionsSliderData[i].value = Overlays.addOverlay(UI_ELEMENTS.barSlider.sliderValue.overlay,
                    auxiliaryProperties);
                optionsExtraOverlays.push(optionsSliderData[i].value);
                auxiliaryProperties = Object.clone(UI_ELEMENTS.barSlider.sliderRemainder.properties);
                auxiliaryProperties.localPosition = { x: 0, y: (0.5 - (1.0 - value) / 2) * properties.dimensions.y, z: 0 };
                auxiliaryProperties.dimensions = {
                    x: properties.dimensions.x,
                    y: Math.max((1.0 - value) * properties.dimensions.y, MIN_BAR_SLIDER_DIMENSION),
                    z: properties.dimensions.z
                };
                auxiliaryProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                optionsSliderData[i].remainder = Overlays.addOverlay(UI_ELEMENTS.barSlider.sliderRemainder.overlay,
                    auxiliaryProperties);
                optionsExtraOverlays.push(optionsSliderData[i].remainder);

                // Zero indicator.
                childProperties = Object.clone(UI_ELEMENTS.barSlider.zeroIndicator.properties);
                childProperties.url = Script.resolvePath(childProperties.url);
                childProperties.dimensions = {
                    x: properties.dimensions.x,
                    y: UI_ELEMENTS.barSlider.zeroIndicator.properties.dimensions.y
                };
                childProperties.localPosition = {
                    x: 0,
                    y: 0,
                    z: properties.dimensions.z / 2 + UIT.dimensions.imageOverlayOffset
                };
                childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                id = Overlays.addOverlay(UI_ELEMENTS.barSlider.zeroIndicator.overlay, childProperties);
                optionsExtraOverlays.push(id);
            }

            if (optionsItems[i].type === "imageSlider") {
                imageOffset = 0.0;

                // Primary image.
                if (optionsItems[i].imageURL) {
                    childProperties = Object.clone(UI_ELEMENTS.image.properties);
                    childProperties.url = Script.resolvePath(optionsItems[i].imageURL);
                    childProperties.dimensions = { x: properties.dimensions.x, y: properties.dimensions.y };
                    imageOffset += 2 * IMAGE_OFFSET;  // Double offset to prevent clipping against background.
                    if (optionsItems[i].useBaseColor) {
                        childProperties.color = properties.color;
                    }
                    childProperties.localPosition = { x: 0, y: 0, z: properties.dimensions.z / 2 + imageOffset };
                    hsvControl.slider.localPosition = childProperties.localPosition;
                    childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    hsvControl.slider.colorOverlay = Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                    hsvControl.slider.length = properties.dimensions.y;
                    optionsExtraOverlays.push(hsvControl.slider.colorOverlay);
                }

                // Overlay image.
                if (optionsItems[i].imageOverlayURL) {
                    childProperties = Object.clone(UI_ELEMENTS.image.properties);
                    childProperties.url = Script.resolvePath(optionsItems[i].imageOverlayURL);
                    childProperties.dimensions = { x: properties.dimensions.x, y: properties.dimensions.y };
                    childProperties.emissive = false;
                    imageOffset += IMAGE_OFFSET;
                    childProperties.localPosition = { x: 0, y: 0, z: properties.dimensions.z / 2 + imageOffset };
                    childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    id = Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                    optionsExtraOverlays.push(id);
                }

                // Value pointers.
                optionsSliderData[i] = {};
                optionsSliderData[i].offset =
                    { x: -properties.dimensions.x / 2, y: 0, z: properties.dimensions.z / 2 + imageOffset };
                auxiliaryProperties = Object.clone(UI_ELEMENTS.sliderPointer.properties);
                auxiliaryProperties.localPosition = optionsSliderData[i].offset;
                hsvControl.slider.localPosition = auxiliaryProperties.localPosition;
                auxiliaryProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                id = optionsSliderData[i].value = Overlays.addOverlay(UI_ELEMENTS.sliderPointer.overlay, auxiliaryProperties);
                optionsExtraOverlays.push(id);
                hsvControl.slider.pointerOverlay = optionsSliderData[i].value;
                auxiliaryProperties.localPosition = { x: 0, y: properties.dimensions.x, z: 0 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 180 });
                auxiliaryProperties.parentID = optionsSliderData[i].value;
                id = Overlays.addOverlay(UI_ELEMENTS.sliderPointer.overlay, auxiliaryProperties);
                optionsExtraOverlays.push(id);
            }

            if (optionsItems[i].type === "colorCircle") {
                imageOffset = 0.0;

                // Primary image.
                if (optionsItems[i].imageURL) {
                    childProperties = Object.clone(UI_ELEMENTS.image.properties);
                    childProperties.url = Script.resolvePath(optionsItems[i].imageURL);
                    childProperties.scale = properties.dimensions.x;
                    imageOffset += 2 * IMAGE_OFFSET;  // Double offset to prevent clipping against background.
                    childProperties.localPosition = { x: 0, y: properties.dimensions.y / 2 + imageOffset, z: 0 };
                    childProperties.localRotation = Quat.fromVec3Degrees({ x: -90, y: 90, z: 0 });
                    childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    id = Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                    optionsExtraOverlays.push(id);
                }

                // Overlay image.
                if (optionsItems[i].imageOverlayURL) {
                    childProperties = Object.clone(UI_ELEMENTS.image.properties);
                    childProperties.url = Script.resolvePath(optionsItems[i].imageOverlayURL);
                    childProperties.scale = properties.dimensions.x;
                    imageOffset += IMAGE_OFFSET;
                    childProperties.emissive = false;
                    childProperties.localPosition = { x: 0, y: properties.dimensions.y / 2 + imageOffset, z: 0 };
                    childProperties.localRotation = Quat.fromVec3Degrees({ x: 90, y: 90, z: 0 });
                    childProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                    childProperties.alpha = 0.0;
                    hsvControl.circle.overlay = Overlays.addOverlay(UI_ELEMENTS.image.overlay, childProperties);
                    optionsExtraOverlays.push(hsvControl.circle.overlay);
                }

                // Value pointers.
                // Invisible sphere at target point with cones as decoration.
                optionsColorData[i] = {};
                optionsColorData[i].offset =
                    { x: 0, y: properties.dimensions.y / 2 + imageOffset, z: 0 };
                auxiliaryProperties = Object.clone(UI_ELEMENTS.sphere.properties);
                auxiliaryProperties.dimensions = CIRCLE_CURSOR_SPHERE_DIMENSIONS;
                auxiliaryProperties.localPosition = optionsColorData[i].offset;
                auxiliaryProperties.parentID = optionsOverlays[optionsOverlays.length - 1];
                auxiliaryProperties.visible = false;
                optionsColorData[i].value = Overlays.addOverlay(UI_ELEMENTS.sphere.overlay, auxiliaryProperties);
                hsvControl.circle.radius = childProperties.scale / 2;
                hsvControl.circle.localPosition = auxiliaryProperties.localPosition;
                hsvControl.circle.cursorOverlay = optionsColorData[i].value;

                auxiliaryProperties = Object.clone(UI_ELEMENTS.circlePointer.properties);
                auxiliaryProperties.parentID = optionsColorData[i].value;
                auxiliaryProperties.localPosition =
                    { x: -(auxiliaryProperties.dimensions.x + CIRCLE_CURSOR_GAP) / 2, y: 0, z: 0 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: 0, y: 90, z: -90 });
                id = Overlays.addOverlay(UI_ELEMENTS.circlePointer.overlay, auxiliaryProperties);
                optionsExtraOverlays.push(id);
                auxiliaryProperties.localPosition =
                    { x: (auxiliaryProperties.dimensions.x + CIRCLE_CURSOR_GAP) / 2, y: 0, z: 0 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 90 });
                id = Overlays.addOverlay(UI_ELEMENTS.circlePointer.overlay, auxiliaryProperties);
                optionsExtraOverlays.push(id);
                auxiliaryProperties.localPosition =
                    { x: 0, y: 0, z: -(auxiliaryProperties.dimensions.x + CIRCLE_CURSOR_GAP) / 2 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: 90, y: 0, z: 0 });
                id = Overlays.addOverlay(UI_ELEMENTS.circlePointer.overlay, auxiliaryProperties);
                optionsExtraOverlays.push(id);
                auxiliaryProperties.localPosition =
                    { x: 0, y: 0, z: (auxiliaryProperties.dimensions.x + CIRCLE_CURSOR_GAP) / 2 };
                auxiliaryProperties.localRotation = Quat.fromVec3Degrees({ x: -90, y: 0, z: 0 });
                id = Overlays.addOverlay(UI_ELEMENTS.circlePointer.overlay, auxiliaryProperties);
                optionsExtraOverlays.push(id);
            }

            if (optionsItems[i].type === "swatch" && swatchHighlightOverlay === null) {
                properties = Object.clone(UI_ELEMENTS.swatchHighlight.properties);
                properties.parentID = optionsOverlays[optionsOverlays.length - 1];
                swatchHighlightOverlay = Overlays.addOverlay(UI_ELEMENTS.swatchHighlight.overlay, properties);
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

        if (swatchHighlightOverlay !== null) {
            Overlays.deleteOverlay(swatchHighlightOverlay);
            swatchHighlightOverlay = null;
        }

        for (i = 0, length = optionsOverlays.length; i < length; i += 1) {
            Overlays.deleteOverlay(optionsOverlays[i]);
        }
        optionsOverlays = [];

        optionsOverlaysIDs = [];
        optionsOverlaysLabels = [];
        optionsOverlaysSublabels = [];
        optionsSliderData = [];
        optionsColorData = [];
        optionsEnabled = [];
        optionsExtraOverlays = [];
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
        var label;
        if (optionsSettings.physicsPresets.value !== "custom") {
            optionsSettings.physicsPresets.value = "custom";
            label = optionsItems[optionsOverlaysIDs.indexOf("physicsPresets")].customLabel;
            Overlays.editOverlay(optionsOverlaysLabels[optionsOverlaysIDs.indexOf("physicsPresets")], {
                url: Script.resolvePath(label.url),
                scale: label.scale,
                localPosition: label.localPosition
            });
            Settings.setValue(optionsSettings.physicsPresets.key, "custom");
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
            optionsSettings.currentColor.value = rgb;
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
            value,
            properties,
            items,
            parentID,
            label,
            values,
            isHighlightingPicklist,
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
            value = optionsSettings[parameter].value;
            if (value !== "") {
                // Set current color to swatch color.
                setCurrentColor(value);
                setColorPicker(value);
                uiCommandCallback("setColor", value);
            } else {
                // Swatch has no color; set swatch color to current color.
                value = optionsSettings.currentColor.value;
                Overlays.editOverlay(optionsOverlays[index], {
                    color: value
                });
                optionsSettings[parameter].value = value;
                Settings.setValue(optionsSettings[parameter].key, value);
            }
            break;

        case "togglePickColor":
            optionsToggles.pickColor = !optionsToggles.pickColor;
            index = optionsOverlaysIDs.indexOf("pickColor");
            Overlays.editOverlay(optionsOverlays[index], {
                color: optionsToggles.pickColor
                    ? UI_ELEMENTS[optionsItems[index].type].onHoverColor
                    : UI_ELEMENTS[optionsItems[index].type].offHoverColor
            });
            uiCommandCallback("pickColorTool", optionsToggles.pickColor);
            break;

        case "setColorFromPick":
            optionsToggles.pickColor = false;
            index = optionsOverlaysIDs.indexOf("pickColor");
            Overlays.editOverlay(optionsOverlays[index], {
                color: UI_ELEMENTS[optionsItems[index].type].offColor
            });
            setCurrentColor(parameter);
            setColorPicker(parameter);
            break;

        case "setGravityOn":
        case "setGrabOn":
        case "setCollideOn":
            value = !optionsSettings[parameter].value;
            optionsSettings[parameter].value = value;
            optionsToggles[parameter] = value;
            Settings.setValue(optionsSettings[parameter].key, value);
            index = optionsOverlaysIDs.indexOf(parameter);
            Overlays.editOverlay(optionsOverlays[index], {
                color: value
                    ? UI_ELEMENTS[optionsItems[index].type].onHoverColor
                    : UI_ELEMENTS[optionsItems[index].type].offHoverColor
            });
            properties = Object.clone(value ? optionsItems[index].onSublabel : optionsItems[index].offSublabel);
            properties.url = Script.resolvePath(properties.url);
            Overlays.editOverlay(optionsOverlaysSublabels[index], properties);
            uiCommandCallback(command, value);
            break;

        case "togglePhysicsPresets":
            if (isPicklistOpen) {
                // Close picklist.
                index = optionsOverlaysIDs.indexOf("physicsPresets");

                // Lower picklist.
                isHighlightingPicklist = hoveredElementType === "picklist";
                Overlays.editOverlay(optionsOverlays[index], {
                    localPosition: isHighlightingPicklist
                        ? Vec3.sum(optionsItems[index].properties.localPosition, OPTION_HOVER_DELTA)
                        : optionsItems[index].properties.localPosition,
                    color: isHighlightingPicklist
                        ? UIT.colors.highlightColor
                        : UI_ELEMENTS.picklist.properties.color
                });
                Overlays.editOverlay(optionsOverlaysSublabels[index], {
                    url: Script.resolvePath("../assets/tools/common/down-arrow.svg")
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
                index = optionsOverlaysIDs.indexOf("physicsPresets");
                parentID = optionsOverlays[index];

                // Raise picklist.
                Overlays.editOverlay(parentID, {
                    localPosition: Vec3.sum(optionsItems[index].properties.localPosition, PICKLIST_HOVER_DELTA)
                });
                Overlays.editOverlay(optionsOverlaysSublabels[index], {
                    url: Script.resolvePath("../assets/tools/common/up-arrow.svg")
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
            doCommand("togglePhysicsPresets");

            // Update picklist label.
            label = optionsItems[optionsOverlaysIDs.indexOf(parameter)].label;
            Overlays.editOverlay(optionsOverlaysLabels[optionsOverlaysIDs.indexOf("physicsPresets")], {
                url: Script.resolvePath(label.url),
                scale: label.scale,
                localPosition: label.localPosition
            });
            optionsSettings.physicsPresets.value = parameter;
            Settings.setValue(optionsSettings.physicsPresets.key, parameter);

            // Update sliders.
            values = PHYSICS_SLIDER_PRESETS[parameter];
            setBarSliderValue(optionsOverlaysIDs.indexOf("gravitySlider"), values.gravity);
            Settings.setValue(optionsSettings.gravitySlider.key, values.gravity);
            uiCommandCallback("setGravity", values.gravity);
            setBarSliderValue(optionsOverlaysIDs.indexOf("bounceSlider"), values.bounce);
            Settings.setValue(optionsSettings.bounceSlider.key, values.bounce);
            uiCommandCallback("setBounce", values.bounce);
            setBarSliderValue(optionsOverlaysIDs.indexOf("frictionSlider"), values.friction);
            Settings.setValue(optionsSettings.frictionSlider.key, values.friction);
            uiCommandCallback("setFriction", values.friction);
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
        case "setFriction":
            setPresetsLabelToCustom();
            Settings.setValue(optionsSettings.frictionSlider.key, parameter);
            uiCommandCallback("setFriction", parameter);
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
        switch (command) {
        case "clearSwatch":
            // Remove highlight ring and change swatch to current color.
            Overlays.editOverlay(swatchHighlightOverlay, { visible: false });
            Overlays.editOverlay(optionsOverlays[optionsOverlaysIDs.indexOf(parameter)], {
                color: optionsSettings.currentColor.value,
                dimensions: UI_ELEMENTS.swatch.properties.dimensions
            });
            optionsSettings[parameter].value = "";  // Emulate Settings.getValue() returning "" for nonexistent setting.
            Settings.setValue(optionsSettings[parameter].key, null);  // Delete settings value.
            break;
        default:
            App.log(side, "ERROR: ToolsMenu: Unexpected command! " + command);
        }
    }

    function adjustSliderFraction(fraction) {
        // Makes slider values achieve and saturate at 0.0 and 1.0.
        return Math.min(1.0, Math.max(0.0, fraction * 1.01 - 0.005));
    }

    function setVisible(visible) {
        var i,
            length;

        for (i = 0, length = staticOverlays.length; i < length; i += 1) {
            Overlays.editOverlay(staticOverlays[i], { visible: visible });
        }

        if (isOptionsOpen) {
            Overlays.editOverlay(menuHeaderBackOverlay, { visible: visible });
            Overlays.editOverlay(menuHeaderIconOverlay, { visible: visible });
            for (i = 0, length = optionsOverlays.length; i < length; i += 1) {
                Overlays.editOverlay(optionsOverlays[i], { visible: visible });
            }
            for (i = 0, length = optionsOverlaysLabels.length; i < length; i += 1) {
                Overlays.editOverlay(optionsOverlaysLabels[i], { visible: visible });
            }
            for (i = 0, length = optionsOverlaysSublabels.length; i < length; i += 1) {
                Overlays.editOverlay(optionsOverlaysSublabels[i], { visible: visible });
            }
            for (i = 0, length = optionsExtraOverlays.length; i < length; i += 1) {
                Overlays.editOverlay(optionsExtraOverlays[i], { visible: visible });
            }
        } else {
            for (i = 0, length = menuOverlays.length; i < length; i += 1) {
                Overlays.editOverlay(menuOverlays[i], { visible: visible });
            }
            for (i = 0, length = menuIconOverlays.length; i < length; i += 1) {
                Overlays.editOverlay(menuIconOverlays[i], { visible: visible });
            }
            for (i = 0, length = menuLabelOverlays.length; i < length; i += 1) {
                Overlays.editOverlay(menuLabelOverlays[i], { visible: visible });
            }
            if (!visible) {
                for (i = 0, length = menuHoverOverlays.length; i < length; i += 1) {
                    Overlays.editOverlay(menuHoverOverlays[i], { visible: false });
                }
            }
        }

        isVisible = visible;
    }

    function update(intersection, groupsCount, entitiesCount) {
        var intersectedItem = NONE,
            intersectionItems,
            color,
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
                Feedback.play(otherSide, Feedback.HOVER_BUTTON);
                Overlays.editOverlay(menuHeaderHeadingOverlay, {
                    localPosition: Vec3.sum(MENU_HEADER_HEADING_PROPERTIES.localPosition, MENU_HEADER_HOVER_OFFSET),
                    color: UIT.colors.greenHighlight,
                    emissive: true  // TODO: This has no effect.
                });
                Overlays.editOverlay(menuHeaderBackOverlay, {
                    color: UIT.colors.white
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
                Overlays.editOverlay(menuHeaderBackOverlay, {
                    color: MENU_HEADER_BACK_PROPERTIES.color
                });
                Overlays.editOverlay(menuHeaderTitleOverlay, {
                    url: optionsHeadingURL,
                    scale: optionsHeadingScale
                });
                Overlays.editOverlay(menuHeaderIconOverlay, {
                    visible: isVisible
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
                intersectionEnabled = menuEnabled;
            } else {
                intersectedItem = optionsOverlays.indexOf(intersection.overlayID);
                if (intersectedItem !== NONE) {
                    intersectionItems = optionsItems;
                    intersectionOverlays = optionsOverlays;
                    intersectionEnabled = optionsEnabled;
                }
            }
        }

        // Hover clickable item.
        if (intersectedItem !== hoveredItem || intersectionOverlays !== hoveredSourceOverlays) {

            if (hoveredItem !== NONE) {
                // Unhover old item.
                switch (hoveredElementType) {
                case "menuButton":
                    Overlays.editOverlay(menuHoverOverlays[hoveredItem], {
                        localPosition: UI_ELEMENTS.menuButton.hoverButton.properties.localPosition,
                        visible: false
                    });
                    Overlays.editOverlay(menuIconOverlays[hoveredItem], {
                        color: UI_ELEMENTS.menuButton.icon.properties.color
                    });
                    break;
                case "button":
                    if (hoveredSourceItems[hoveredItem].enabledColor !== undefined && optionsEnabled[hoveredItem]) {
                        color = hoveredSourceItems[hoveredItem].enabledColor;
                    } else {
                        color = hoveredSourceItems[hoveredItem].properties.color !== undefined
                            ? hoveredSourceItems[hoveredItem].properties.color
                            : UI_ELEMENTS.button.properties.color;
                    }
                    Overlays.editOverlay(hoveredSourceOverlays[hoveredItem], {
                        color: color,
                        localPosition: hoveredSourceItems[hoveredItem].properties.localPosition
                    });
                    break;
                case "toggleButton":
                    color = optionsToggles[hoveredSourceItems[hoveredItem].id]
                        ? UI_ELEMENTS.toggleButton.onColor
                        : UI_ELEMENTS.toggleButton.offColor;
                    Overlays.editOverlay(hoveredSourceOverlays[hoveredItem], {
                        color: color,
                        localPosition: hoveredSourceItems[hoveredItem].properties.localPosition
                    });
                    break;
                case "swatch":
                    Overlays.editOverlay(swatchHighlightOverlay, { visible: false });
                    color = optionsSettings[hoveredSourceItems[hoveredItem].id].value;
                    Overlays.editOverlay(hoveredSourceOverlays[hoveredItem], {
                        dimensions: UI_ELEMENTS.swatch.properties.dimensions,
                        color: color === "" ? EMPTY_SWATCH_COLOR : color,
                        localPosition: hoveredSourceItems[hoveredItem].properties.localPosition
                    });
                    break;
                case "barSlider":
                case "imageSlider":
                case "colorCircle":
                    // Lower old slider or color circle.
                    Overlays.editOverlay(hoveredSourceOverlays[hoveredItem], {
                        localPosition: hoveredSourceItems[hoveredItem].properties.localPosition
                    });
                    break;
                case "picklist":
                    if (hoveredSourceItems[hoveredItem].type !== "picklistItem" && !isPicklistOpen) {
                        Overlays.editOverlay(hoveredSourceOverlays[hoveredItem], {
                            localPosition: hoveredSourceItems[hoveredItem].properties.localPosition,
                            color: UI_ELEMENTS.picklist.properties.color
                        });
                    } else {
                        Overlays.editOverlay(hoveredSourceOverlays[hoveredItem], {
                            color: UIT.colors.darkGray
                        });
                    }
                    break;
                case "picklistItem":
                    Overlays.editOverlay(hoveredSourceOverlays[hoveredItem], {
                        color: UI_ELEMENTS.picklistItem.properties.color
                    });
                    break;
                case null:
                    // Nothing to do.
                    break;
                default:
                    App.log(side, "ERROR: ToolsMenu: Unexpected hover item! " + hoveredElementType);
                }

                // Update status variables.
                hoveredItem = NONE;
                isHoveringButtonElement = false;
                hoveredElementType = null;
            }

            if (intersectedItem !== NONE && intersectionItems[intersectedItem] &&
                    (intersectionItems[intersectedItem].command !== undefined
                    || intersectionItems[intersectedItem].callback !== undefined)) {

                // Update status variables.
                hoveredItem = intersectedItem;
                isHoveringButtonElement = BUTTON_UI_ELEMENTS.indexOf(intersectionItems[hoveredItem].type) !== NONE;
                hoveredElementType = intersectionItems[hoveredItem].type;

                // Hover new item.
                switch (hoveredElementType) {
                case "menuButton":
                    Feedback.play(otherSide, Feedback.HOVER_MENU_ITEM);
                    Overlays.editOverlay(menuHoverOverlays[hoveredItem], {
                        localPosition: Vec3.sum(UI_ELEMENTS.menuButton.hoverButton.properties.localPosition, MENU_HOVER_DELTA),
                        visible: true
                    });
                    Overlays.editOverlay(menuIconOverlays[hoveredItem], {
                        color: UI_ELEMENTS.menuButton.icon.highlightColor
                    });
                    break;
                case "button":
                    if (intersectionEnabled[hoveredItem]) {
                        Feedback.play(otherSide, Feedback.HOVER_BUTTON);
                        localPosition = intersectionItems[hoveredItem].properties.localPosition;
                        Overlays.editOverlay(intersectionOverlays[hoveredItem], {
                            color: intersectionItems[hoveredItem].highlightColor !== undefined
                                ? intersectionItems[hoveredItem].highlightColor
                                : UIT.colors.greenHighlight,
                            localPosition: Vec3.sum(localPosition, OPTION_HOVER_DELTA)
                        });
                    }
                    break;
                case "toggleButton":
                    if (intersectionEnabled[hoveredItem]) {
                        Feedback.play(otherSide, Feedback.HOVER_BUTTON);
                        localPosition = intersectionItems[hoveredItem].properties.localPosition;
                        Overlays.editOverlay(intersectionOverlays[hoveredItem], {
                            color: optionsToggles[intersectionItems[hoveredItem].id]
                                ? UI_ELEMENTS.toggleButton.onHoverColor
                                : UI_ELEMENTS.toggleButton.offHoverColor,
                            localPosition: Vec3.sum(localPosition, OPTION_HOVER_DELTA)
                        });
                    }
                    break;
                case "swatch":
                    Feedback.play(otherSide, Feedback.HOVER_BUTTON);
                    localPosition = intersectionItems[hoveredItem].properties.localPosition;
                    if (optionsSettings[intersectionItems[hoveredItem].id].value === "") {
                        // Swatch is empty; highlight it with current color.
                        Overlays.editOverlay(intersectionOverlays[hoveredItem], {
                            color: optionsSettings.currentColor.value,
                            localPosition: Vec3.sum(localPosition, OPTION_HOVER_DELTA)
                        });
                    } else {
                        // Swatch is full; highlight it with ring.
                        Overlays.editOverlay(intersectionOverlays[hoveredItem], {
                            dimensions: Vec3.subtract(UI_ELEMENTS.swatch.properties.dimensions,
                                { x: SWATCH_HIGHLIGHT_DELTA, y: 0, z: SWATCH_HIGHLIGHT_DELTA }),
                            localPosition: Vec3.sum(localPosition, OPTION_HOVER_DELTA)
                        });
                        Overlays.editOverlay(swatchHighlightOverlay, {
                            parentID: intersectionOverlays[hoveredItem],
                            localPosition: UI_ELEMENTS.swatchHighlight.properties.localPosition,
                            visible: true
                        });
                    }
                    break;
                case "barSlider":
                case "imageSlider":
                case "colorCircle":
                    Feedback.play(otherSide, Feedback.HOVER_BUTTON);
                    localPosition = intersectionItems[hoveredItem].properties.localPosition;
                    Overlays.editOverlay(intersectionOverlays[hoveredItem], {
                        localPosition: Vec3.sum(localPosition, OPTION_HOVER_DELTA)
                    });
                    break;
                case "picklist":
                    Feedback.play(otherSide, Feedback.HOVER_BUTTON);
                    if (!isPicklistOpen) {
                        localPosition = intersectionItems[hoveredItem].properties.localPosition;
                        Overlays.editOverlay(intersectionOverlays[hoveredItem], {
                            localPosition: Vec3.sum(localPosition, OPTION_HOVER_DELTA),
                            color: UIT.colors.greenHighlight
                        });
                    } else {
                        Overlays.editOverlay(intersectionOverlays[hoveredItem], {
                            color: UIT.colors.greenHighlight
                        });
                    }
                    break;
                case "picklistItem":
                    Feedback.play(otherSide, Feedback.HOVER_BUTTON);
                    Overlays.editOverlay(intersectionOverlays[hoveredItem], {
                        color: UIT.colors.greenHighlight
                    });
                    break;
                case null:
                    // Nothing to do.
                    break;
                default:
                    App.log(side, "ERROR: ToolsMenu: Unexpected hover item! " + hoveredElementType);
                }
            }

            hoveredSourceOverlays = intersectionOverlays;
            hoveredSourceItems = intersectionItems;
        }

        // Press/unpress button.
        if ((pressedItem && intersectedItem !== pressedItem.index) || intersectionOverlays !== pressedSource
                || isTriggerClicked !== (pressedItem !== null)) {
            if (pressedItem) {
                // Unpress previous button.
                Overlays.editOverlay(pressedSource[pressedItem.index], {
                    localPosition: isHoveringButtonElement && hoveredItem === pressedItem.index
                        ? Vec3.sum(pressedItem.localPosition, OPTION_HOVER_DELTA)
                        : pressedItem.localPosition
                });
                pressedItem = null;
                pressedSource = null;
            }
            if (isHoveringButtonElement && (intersectionEnabled === null || intersectionEnabled[intersectedItem])
                    && isTriggerClicked && !wasTriggerClicked) {
                // Press new button.
                localPosition = intersectionItems[intersectedItem].properties.localPosition;
                if (hoveredElementType !== "menuButton") {
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
                if (intersectionOverlays === menuOverlays && intersectionItems[intersectedItem].toolOptions) {
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
        if (intersectionItems && ((hoveredElementType === "picklist"
                && controlHand.triggerClicked() !== isPicklistPressed)
                    || (intersectionItems[intersectedItem].type !== "picklist" && isPicklistPressed))) {
            isPicklistPressed = controlHand.triggerClicked();
            if (isPicklistPressed) {
                doCommand(intersectionItems[intersectedItem].command.method, intersectionItems[intersectedItem].id);
            }
        }
        if (intersectionItems && ((hoveredElementType === "picklistItem"
                && controlHand.triggerClicked() !== isPicklistItemPressed)
                    || (intersectionItems[intersectedItem].type !== "picklistItem" && isPicklistItemPressed))) {
            isPicklistItemPressed = controlHand.triggerClicked();
            if (isPicklistItemPressed) {
                doCommand(intersectionItems[intersectedItem].command.method, intersectionItems[intersectedItem].id);
            }
        }
        if (intersectionItems && isPicklistOpen && controlHand.triggerClicked()
                && hoveredElementType !== "picklist" && hoveredElementType !== "picklistItem") {
            doCommand("togglePhysicsPresets");
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
        if (intersectionItems && hoveredElementType === "barSlider" && controlHand.triggerClicked()) {
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
        if (intersectionItems && hoveredElementType === "imageSlider" && controlHand.triggerClicked()) {
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
        if (intersectionItems && hoveredElementType === "colorCircle" && controlHand.triggerClicked()) {
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
                        ? (hoveredItem === groupButtonIndex
                            ? OPTONS_PANELS.groupOptions[groupButtonIndex].highlightColor
                            : OPTONS_PANELS.groupOptions[groupButtonIndex].enabledColor)
                        : OPTONS_PANELS.groupOptions[groupButtonIndex].properties.color
                });
                Overlays.editOverlay(optionsOverlaysLabels[groupButtonIndex], {
                    color: isGroupButtonEnabled
                        ? OPTONS_PANELS.groupOptions[groupButtonIndex].labelEnabledColor
                        : OPTONS_PANELS.groupOptions[groupButtonIndex].label.color
                });
                optionsEnabled[groupButtonIndex] = enableGroupButton;
            }

            enableUngroupButton = groupsCount === 1 && entitiesCount > 1;
            if (enableUngroupButton !== isUngroupButtonEnabled) {
                isUngroupButtonEnabled = enableUngroupButton;
                Overlays.editOverlay(optionsOverlays[ungroupButtonIndex], {
                    color: isUngroupButtonEnabled
                        ? (hoveredItem === ungroupButtonIndex
                            ? OPTONS_PANELS.groupOptions[ungroupButtonIndex].highlightColor
                            : OPTONS_PANELS.groupOptions[ungroupButtonIndex].enabledColor)
                        : OPTONS_PANELS.groupOptions[ungroupButtonIndex].properties.color
                });
                Overlays.editOverlay(optionsOverlaysLabels[ungroupButtonIndex], {
                    color: isUngroupButtonEnabled
                        ? OPTONS_PANELS.groupOptions[ungroupButtonIndex].labelEnabledColor
                        : OPTONS_PANELS.groupOptions[ungroupButtonIndex].label.color
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

        // Always-visible overlays.
        staticOverlays = [menuHeaderOverlay, menuHeaderHeadingOverlay, menuHeaderBarOverlay, menuHeaderTitleOverlay,
            menuPanelOverlay];

        // Menu items.
        openMenu();

        // Initial values.
        optionsItems = null;
        intersectionOverlays = null;
        intersectionEnabled = null;
        hoveredItem = NONE;
        hoveredSourceOverlays = null;
        isHoveringButtonElement = false;
        hoveredElementType = null;
        pressedItem = null;
        pressedSource = null;
        isPicklistOpen = false;
        isPicklistPressed = false;
        isPicklistItemPressed = false;
        isTriggerClicked = false;
        wasTriggerClicked = false;
        isGripClicked = false;

        // Special handling for Group options.
        isGroupButtonEnabled = false;
        isUngroupButtonEnabled = false;
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
        menuIconOverlays = [];
        menuLabelOverlays = [];
        optionsOverlays = [];
        optionsOverlaysLabels = [];
        optionsOverlaysSublabels = [];
        optionsExtraOverlays = [];
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
        overlayIDs: getOverlayIDs,
        clearTool: clearTool,
        doCommand: doCommand,
        setVisible: setVisible,
        update: update,
        display: display,
        clear: clear,
        destroy: destroy
    };
};

ToolsMenu.prototype = {};
