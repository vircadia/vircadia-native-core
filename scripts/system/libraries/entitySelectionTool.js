//
//  entitySelectionToolClass.js
//  examples
//
//  Created by Brad hefta-Gaub on 10/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script implements a class useful for building tools for editing entities.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

SPACE_LOCAL = "local";
SPACE_WORLD = "world";

Script.include("./controllers.js");

function objectTranslationPlanePoint(position, dimensions) {
    var newPosition = { x: position.x, y: position.y, z: position.z };
    newPosition.y -= dimensions.y / 2.0;
    return newPosition;
}

SelectionManager = (function() {
    var that = {};

    function subscribeToUpdateMessages() {
        Messages.subscribe('entityToolUpdates');
        Messages.messageReceived.connect(handleEntitySelectionToolUpdates);
    }

    function handleEntitySelectionToolUpdates(channel, message, sender) {
        if (channel !== 'entityToolUpdates') {
            return;
        }
        if (sender !== MyAvatar.sessionUUID) {
            return;
        }

        if (message === 'callUpdate') {
            that._update();
        }
    }

    subscribeToUpdateMessages();

    that.savedProperties = {};
    that.selections = [];
    var listeners = [];

    that.localRotation = Quat.fromPitchYawRollDegrees(0, 0, 0);
    that.localPosition = {
        x: 0,
        y: 0,
        z: 0
    };
    that.localDimensions = {
        x: 0,
        y: 0,
        z: 0
    };
    that.localRegistrationPoint = {
        x: 0.5,
        y: 0.5,
        z: 0.5
    };

    that.worldRotation = Quat.fromPitchYawRollDegrees(0, 0, 0);
    that.worldPosition = {
        x: 0,
        y: 0,
        z: 0
    };
    that.worldDimensions = {
        x: 0,
        y: 0,
        z: 0
    };
    that.worldRegistrationPoint = {
        x: 0.5,
        y: 0.5,
        z: 0.5
    };
    that.centerPosition = {
        x: 0,
        y: 0,
        z: 0
    };

    that.saveProperties = function() {
        that.savedProperties = {};
        for (var i = 0; i < that.selections.length; i++) {
            var entityID = that.selections[i];
            that.savedProperties[entityID] = Entities.getEntityProperties(entityID);
        }
    };

    that.addEventListener = function(func) {
        listeners.push(func);
    };

    that.hasSelection = function() {
        return that.selections.length > 0;
    };

    that.setSelections = function(entityIDs) {
        that.selections = [];
        for (var i = 0; i < entityIDs.length; i++) {
            var entityID = entityIDs[i];
            that.selections.push(entityID);
        }

        that._update();
    };

    that.addEntity = function(entityID, toggleSelection) {
        if (entityID) {
            var idx = -1;
            for (var i = 0; i < that.selections.length; i++) {
                if (entityID == that.selections[i]) {
                    idx = i;
                    break;
                }
            }
            if (idx == -1) {
                that.selections.push(entityID);
            } else if (toggleSelection) {
                that.selections.splice(idx, 1);
            }
        }

        that._update();
    };

    that.removeEntity = function(entityID) {
        var idx = that.selections.indexOf(entityID);
        if (idx >= 0) {
            that.selections.splice(idx, 1);
        }
        that._update();
    };

    that.clearSelections = function() {
        that.selections = [];
        that._update();
    };

    that._update = function() {
        if (that.selections.length == 0) {
            that.localDimensions = null;
            that.localPosition = null;
            that.worldDimensions = null;
            that.worldPosition = null;
        } else if (that.selections.length == 1) {
            var properties = Entities.getEntityProperties(that.selections[0]);
            that.localDimensions = properties.dimensions;
            that.localPosition = properties.position;
            that.localRotation = properties.rotation;
            that.localRegistrationPoint = properties.registrationPoint;

            that.worldDimensions = properties.boundingBox.dimensions;
            that.worldPosition = properties.boundingBox.center;

            SelectionDisplay.setSpaceMode(SPACE_LOCAL);
        } else {
            that.localRotation = null;
            that.localDimensions = null;
            that.localPosition = null;

            var properties = Entities.getEntityProperties(that.selections[0]);

            var brn = properties.boundingBox.brn;
            var tfl = properties.boundingBox.tfl;

            for (var i = 1; i < that.selections.length; i++) {
                properties = Entities.getEntityProperties(that.selections[i]);
                var bb = properties.boundingBox;
                brn.x = Math.min(bb.brn.x, brn.x);
                brn.y = Math.min(bb.brn.y, brn.y);
                brn.z = Math.min(bb.brn.z, brn.z);
                tfl.x = Math.max(bb.tfl.x, tfl.x);
                tfl.y = Math.max(bb.tfl.y, tfl.y);
                tfl.z = Math.max(bb.tfl.z, tfl.z);
            }

            that.localDimensions = null;
            that.localPosition = null;
            that.worldDimensions = {
                x: tfl.x - brn.x,
                y: tfl.y - brn.y,
                z: tfl.z - brn.z
            };
            that.worldPosition = {
                x: brn.x + (that.worldDimensions.x / 2),
                y: brn.y + (that.worldDimensions.y / 2),
                z: brn.z + (that.worldDimensions.z / 2),
            };

            // For 1+ selections we can only modify selections in world space
            SelectionDisplay.setSpaceMode(SPACE_WORLD);
        }

        for (var i = 0; i < listeners.length; i++) {
            try {
                listeners[i]();
            } catch (e) {
                print("EntitySelectionTool got exception: " + JSON.stringify(e));
            }
        }
    };

    return that;
})();

// Normalize degrees to be in the range (-180, 180]
function normalizeDegrees(degrees) {
    while (degrees > 180) degrees -= 360;
    while (degrees <= -180) degrees += 360;
    return degrees;
}

// Return the enter position of an entity relative to it's registrationPoint
// A registration point of (0.5, 0.5, 0.5) will have an offset of (0, 0, 0)
// A registration point of (1.0, 1.0, 1.0) will have an offset of (-dimensions.x / 2, -dimensions.y / 2, -dimensions.z / 2)
function getRelativeCenterPosition(dimensions, registrationPoint) {
    return {
        x: -dimensions.x * (registrationPoint.x - 0.5),
        y: -dimensions.y * (registrationPoint.y - 0.5),
        z: -dimensions.z * (registrationPoint.z - 0.5),
    }
}

SelectionDisplay = (function() {
    var that = {};

    var MINIMUM_DIMENSION = 0.001;

    var GRABBER_DISTANCE_TO_SIZE_RATIO = 0.0075;

    // These are multipliers for sizing the rotation degrees display while rotating an entity
    var ROTATION_DISPLAY_DISTANCE_MULTIPLIER = 1.2;
    var ROTATION_DISPLAY_SIZE_X_MULTIPLIER = 0.6;
    var ROTATION_DISPLAY_SIZE_Y_MULTIPLIER = 0.18;
    var ROTATION_DISPLAY_LINE_HEIGHT_MULTIPLIER = 0.14;

    var ROTATE_ARROW_WEST_NORTH_URL = HIFI_PUBLIC_BUCKET + "images/rotate-arrow-west-north.svg";
    var ROTATE_ARROW_WEST_SOUTH_URL = HIFI_PUBLIC_BUCKET + "images/rotate-arrow-west-south.svg";

    var showExtendedStretchHandles = false;

    var spaceMode = SPACE_LOCAL;
    var mode = "UNKNOWN";
    var overlayNames = new Array();
    var lastCameraPosition = Camera.getPosition();
    var lastCameraOrientation = Camera.getOrientation();

    var handleHoverColor = {
        red: 224,
        green: 67,
        blue: 36
    };
    var handleHoverAlpha = 1.0;

    var rotateOverlayTargetSize = 10000; // really big target
    var innerSnapAngle = 22.5; // the angle which we snap to on the inner rotation tool
    var innerRadius;
    var outerRadius;
    var yawHandleRotation;
    var pitchHandleRotation;
    var rollHandleRotation;
    var yawCenter;
    var pitchCenter;
    var rollCenter;
    var yawZero;
    var pitchZero;
    var rollZero;
    var yawNormal;
    var pitchNormal;
    var rollNormal;
    var rotationNormal;

    var originalRotation;
    var originalPitch;
    var originalYaw;
    var originalRoll;


    var handleColor = {
        red: 255,
        green: 255,
        blue: 255
    };
    var handleAlpha = 0.7;

    var highlightedHandleColor = {
        red: 183,
        green: 64,
        blue: 44
    };
    var highlightedHandleAlpha = 0.9;

    var previousHandle = false;
    var previousHandleColor;
    var previousHandleAlpha;

    var grabberSizeCorner = 0.025; // These get resized by updateHandleSizes().
    var grabberSizeEdge = 0.015;
    var grabberSizeFace = 0.025;
    var grabberAlpha = 1;
    var grabberColorCorner = {
        red: 120,
        green: 120,
        blue: 120
    };
    var grabberColorEdge = {
        red: 0,
        green: 0,
        blue: 0
    };
    var grabberColorFace = {
        red: 120,
        green: 120,
        blue: 120
    };
    var grabberLineWidth = 0.5;
    var grabberSolid = true;
    var grabberMoveUpPosition = {
        x: 0,
        y: 0,
        z: 0
    };

    var lightOverlayColor = {
        red: 255,
        green: 153,
        blue: 0
    };

    var grabberPropertiesCorner = {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        size: grabberSizeCorner,
        color: grabberColorCorner,
        alpha: 1,
        solid: grabberSolid,
        visible: false,
        dashed: false,
        lineWidth: grabberLineWidth,
        drawInFront: true,
        borderSize: 1.4,
    };

    var grabberPropertiesEdge = {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        size: grabberSizeEdge,
        color: grabberColorEdge,
        alpha: 1,
        solid: grabberSolid,
        visible: false,
        dashed: false,
        lineWidth: grabberLineWidth,
        drawInFront: true,
        borderSize: 1.4,
    };

    var grabberPropertiesFace = {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        size: grabberSizeFace,
        color: grabberColorFace,
        alpha: 1,
        solid: grabberSolid,
        visible: false,
        dashed: false,
        lineWidth: grabberLineWidth,
        drawInFront: true,
        borderSize: 1.4,
    };

    var spotLightLineProperties = {
        color: lightOverlayColor,
        lineWidth: 1.5,
    };

    var highlightBox = Overlays.addOverlay("cube", {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        size: 1,
        color: {
            red: 90,
            green: 90,
            blue: 90
        },
        alpha: 1,
        solid: false,
        visible: false,
        dashed: true,
        lineWidth: 2.0,
        ignoreRayIntersection: true, // this never ray intersects
        drawInFront: true
    });

    var selectionBox = Overlays.addOverlay("cube", {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        size: 1,
        color: {
            red: 255,
            green: 0,
            blue: 0
        },
        alpha: 1,
        solid: false,
        visible: false,
        dashed: false,
        lineWidth: 1.0,
    });

    var selectionBoxes = [];

    var rotationDegreesDisplay = Overlays.addOverlay("text3d", {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        text: "",
        color: {
            red: 0,
            green: 0,
            blue: 0
        },
        backgroundColor: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 0.7,
        backgroundAlpha: 0.7,
        visible: false,
        isFacingAvatar: true,
        drawInFront: true,
        ignoreRayIntersection: true,
        dimensions: {
            x: 0,
            y: 0
        },
        lineHeight: 0.0,
        topMargin: 0,
        rightMargin: 0,
        bottomMargin: 0,
        leftMargin: 0,
    });

    var grabberMoveUp = Overlays.addOverlay("image3d", {
        url: HIFI_PUBLIC_BUCKET + "images/up-arrow.svg",
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        color: handleColor,
        alpha: handleAlpha,
        visible: false,
        size: 0.1,
        scale: 0.1,
        isFacingAvatar: true,
        drawInFront: true,
    });

    // var normalLine = Overlays.addOverlay("line3d", {
    //                 visible: true,
    //                 lineWidth: 2.0,
    //                 start: { x: 0, y: 0, z: 0 },
    //                 end: { x: 0, y: 0, z: 0 },
    //                 color: { red: 255, green: 255, blue: 0 },
    //                 ignoreRayIntersection: true,
    // });

    var grabberLBN = Overlays.addOverlay("cube", grabberPropertiesCorner);
    var grabberRBN = Overlays.addOverlay("cube", grabberPropertiesCorner);
    var grabberLBF = Overlays.addOverlay("cube", grabberPropertiesCorner);
    var grabberRBF = Overlays.addOverlay("cube", grabberPropertiesCorner);
    var grabberLTN = Overlays.addOverlay("cube", grabberPropertiesCorner);
    var grabberRTN = Overlays.addOverlay("cube", grabberPropertiesCorner);
    var grabberLTF = Overlays.addOverlay("cube", grabberPropertiesCorner);
    var grabberRTF = Overlays.addOverlay("cube", grabberPropertiesCorner);

    var grabberTOP = Overlays.addOverlay("cube", grabberPropertiesFace);
    var grabberBOTTOM = Overlays.addOverlay("cube", grabberPropertiesFace);
    var grabberLEFT = Overlays.addOverlay("cube", grabberPropertiesFace);
    var grabberRIGHT = Overlays.addOverlay("cube", grabberPropertiesFace);
    var grabberNEAR = Overlays.addOverlay("cube", grabberPropertiesFace);
    var grabberFAR = Overlays.addOverlay("cube", grabberPropertiesFace);

    var grabberEdgeTR = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeTL = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeTF = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeTN = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeBR = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeBL = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeBF = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeBN = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeNR = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeNL = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeFR = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberEdgeFL = Overlays.addOverlay("cube", grabberPropertiesEdge);

    var grabberSpotLightCircle = Overlays.addOverlay("circle3d", {
        color: lightOverlayColor,
        isSolid: false,
        visible: false
    });
    var grabberSpotLightLineT = Overlays.addOverlay("line3d", spotLightLineProperties);
    var grabberSpotLightLineB = Overlays.addOverlay("line3d", spotLightLineProperties);
    var grabberSpotLightLineL = Overlays.addOverlay("line3d", spotLightLineProperties);
    var grabberSpotLightLineR = Overlays.addOverlay("line3d", spotLightLineProperties);

    var grabberSpotLightCenter = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberSpotLightRadius = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberSpotLightL = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberSpotLightR = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberSpotLightT = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberSpotLightB = Overlays.addOverlay("cube", grabberPropertiesEdge);

    var grabberPointLightCircleX = Overlays.addOverlay("circle3d", {
        rotation: Quat.fromPitchYawRollDegrees(0, 90, 0),
        color: lightOverlayColor,
        isSolid: false,
        visible: false
    });
    var grabberPointLightCircleY = Overlays.addOverlay("circle3d", {
        rotation: Quat.fromPitchYawRollDegrees(90, 0, 0),
        color: lightOverlayColor,
        isSolid: false,
        visible: false
    });
    var grabberPointLightCircleZ = Overlays.addOverlay("circle3d", {
        rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
        color: lightOverlayColor,
        isSolid: false,
        visible: false
    });
    var grabberPointLightT = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberPointLightB = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberPointLightL = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberPointLightR = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberPointLightF = Overlays.addOverlay("cube", grabberPropertiesEdge);
    var grabberPointLightN = Overlays.addOverlay("cube", grabberPropertiesEdge);

    var stretchHandles = [
        grabberLBN,
        grabberRBN,
        grabberLBF,
        grabberRBF,
        grabberLTN,
        grabberRTN,
        grabberLTF,
        grabberRTF,
        grabberTOP,
        grabberBOTTOM,
        grabberLEFT,
        grabberRIGHT,
        grabberNEAR,
        grabberFAR,
        grabberEdgeTR,
        grabberEdgeTL,
        grabberEdgeTF,
        grabberEdgeTN,
        grabberEdgeBR,
        grabberEdgeBL,
        grabberEdgeBF,
        grabberEdgeBN,
        grabberEdgeNR,
        grabberEdgeNL,
        grabberEdgeFR,
        grabberEdgeFL,

        grabberSpotLightLineT,
        grabberSpotLightLineB,
        grabberSpotLightLineL,
        grabberSpotLightLineR,

        grabberSpotLightCenter,
        grabberSpotLightRadius,
        grabberSpotLightL,
        grabberSpotLightR,
        grabberSpotLightT,
        grabberSpotLightB,

        grabberPointLightT,
        grabberPointLightB,
        grabberPointLightL,
        grabberPointLightR,
        grabberPointLightF,
        grabberPointLightN,
    ];


    var baseOverlayAngles = {
        x: 0,
        y: 0,
        z: 0
    };
    var baseOverlayRotation = Quat.fromVec3Degrees(baseOverlayAngles);
    var baseOfEntityProjectionOverlay = Overlays.addOverlay("rectangle3d", {
        position: {
            x: 1,
            y: 0,
            z: 0
        },
        color: {
            red: 51,
            green: 152,
            blue: 203
        },
        alpha: 0.5,
        solid: true,
        visible: false,
        width: 300,
        height: 200,
        rotation: baseOverlayRotation,
        ignoreRayIntersection: true, // always ignore this
    });

    var yawOverlayAngles = {
        x: 90,
        y: 0,
        z: 0
    };
    var yawOverlayRotation = Quat.fromVec3Degrees(yawOverlayAngles);
    var pitchOverlayAngles = {
        x: 0,
        y: 90,
        z: 0
    };
    var pitchOverlayRotation = Quat.fromVec3Degrees(pitchOverlayAngles);
    var rollOverlayAngles = {
        x: 0,
        y: 180,
        z: 0
    };
    var rollOverlayRotation = Quat.fromVec3Degrees(rollOverlayAngles);

    var xRailOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        lineWidth: 1.0,
        start: {
            x: 0,
            y: 0,
            z: 0
        },
        end: {
            x: 0,
            y: 0,
            z: 0
        },
        color: {
            red: 255,
            green: 0,
            blue: 0
        },
        ignoreRayIntersection: true, // always ignore this
        visible: false,
    });
    var yRailOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        lineWidth: 1.0,
        start: {
            x: 0,
            y: 0,
            z: 0
        },
        end: {
            x: 0,
            y: 0,
            z: 0
        },
        color: {
            red: 0,
            green: 255,
            blue: 0
        },
        ignoreRayIntersection: true, // always ignore this
        visible: false,
    });
    var zRailOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        lineWidth: 1.0,
        start: {
            x: 0,
            y: 0,
            z: 0
        },
        end: {
            x: 0,
            y: 0,
            z: 0
        },
        color: {
            red: 0,
            green: 0,
            blue: 255
        },
        ignoreRayIntersection: true, // always ignore this
        visible: false,
    });

    var rotateZeroOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        lineWidth: 2.0,
        start: {
            x: 0,
            y: 0,
            z: 0
        },
        end: {
            x: 0,
            y: 0,
            z: 0
        },
        color: {
            red: 255,
            green: 0,
            blue: 0
        },
        ignoreRayIntersection: true, // always ignore this
    });

    var rotateCurrentOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        lineWidth: 2.0,
        start: {
            x: 0,
            y: 0,
            z: 0
        },
        end: {
            x: 0,
            y: 0,
            z: 0
        },
        color: {
            red: 0,
            green: 0,
            blue: 255
        },
        ignoreRayIntersection: true, // always ignore this
    });


    var rotateOverlayTarget = Overlays.addOverlay("circle3d", {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        size: rotateOverlayTargetSize,
        color: {
            red: 0,
            green: 0,
            blue: 0
        },
        alpha: 0.0,
        solid: true,
        visible: false,
        rotation: yawOverlayRotation,
    });

    var rotateOverlayInner = Overlays.addOverlay("circle3d", {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        size: 1,
        color: {
            red: 51,
            green: 152,
            blue: 203
        },
        alpha: 0.2,
        solid: true,
        visible: false,
        rotation: yawOverlayRotation,
        hasTickMarks: true,
        majorTickMarksAngle: innerSnapAngle,
        minorTickMarksAngle: 0,
        majorTickMarksLength: -0.25,
        minorTickMarksLength: 0,
        majorTickMarksColor: {
            red: 0,
            green: 0,
            blue: 0
        },
        minorTickMarksColor: {
            red: 0,
            green: 0,
            blue: 0
        },
        ignoreRayIntersection: true, // always ignore this
    });

    var rotateOverlayOuter = Overlays.addOverlay("circle3d", {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        size: 1,
        color: {
            red: 51,
            green: 152,
            blue: 203
        },
        alpha: 0.2,
        solid: true,
        visible: false,
        rotation: yawOverlayRotation,

        hasTickMarks: true,
        majorTickMarksAngle: 45.0,
        minorTickMarksAngle: 5,
        majorTickMarksLength: 0.25,
        minorTickMarksLength: 0.1,
        majorTickMarksColor: {
            red: 0,
            green: 0,
            blue: 0
        },
        minorTickMarksColor: {
            red: 0,
            green: 0,
            blue: 0
        },
        ignoreRayIntersection: true, // always ignore this
    });

    var rotateOverlayCurrent = Overlays.addOverlay("circle3d", {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        size: 1,
        color: {
            red: 224,
            green: 67,
            blue: 36
        },
        alpha: 0.8,
        solid: true,
        visible: false,
        rotation: yawOverlayRotation,
        ignoreRayIntersection: true, // always ignore this
        hasTickMarks: true,
        majorTickMarksColor: {
            red: 0,
            green: 0,
            blue: 0
        },
        minorTickMarksColor: {
            red: 0,
            green: 0,
            blue: 0
        },
    });

    var yawHandle = Overlays.addOverlay("image3d", {
        url: ROTATE_ARROW_WEST_NORTH_URL,
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        color: handleColor,
        alpha: handleAlpha,
        visible: false,
        size: 0.1,
        scale: 0.1,
        isFacingAvatar: false,
        drawInFront: true,
    });


    var pitchHandle = Overlays.addOverlay("image3d", {
        url: ROTATE_ARROW_WEST_NORTH_URL,
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        color: handleColor,
        alpha: handleAlpha,
        visible: false,
        size: 0.1,
        scale: 0.1,
        isFacingAvatar: false,
        drawInFront: true,
    });


    var rollHandle = Overlays.addOverlay("image3d", {
        url: ROTATE_ARROW_WEST_NORTH_URL,
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        color: handleColor,
        alpha: handleAlpha,
        visible: false,
        size: 0.1,
        scale: 0.1,
        isFacingAvatar: false,
        drawInFront: true,
    });

    var allOverlays = [
        highlightBox,
        selectionBox,
        grabberMoveUp,
        yawHandle,
        pitchHandle,
        rollHandle,
        rotateOverlayTarget,
        rotateOverlayInner,
        rotateOverlayOuter,
        rotateOverlayCurrent,
        rotateZeroOverlay,
        rotateCurrentOverlay,
        rotationDegreesDisplay,
        xRailOverlay,
        yRailOverlay,
        zRailOverlay,
        baseOfEntityProjectionOverlay,
        grabberSpotLightCircle,
        grabberPointLightCircleX,
        grabberPointLightCircleY,
        grabberPointLightCircleZ,
    ].concat(stretchHandles);

    overlayNames[highlightBox] = "highlightBox";
    overlayNames[selectionBox] = "selectionBox";
    overlayNames[baseOfEntityProjectionOverlay] = "baseOfEntityProjectionOverlay";
    overlayNames[grabberMoveUp] = "grabberMoveUp";
    overlayNames[grabberLBN] = "grabberLBN";
    overlayNames[grabberLBF] = "grabberLBF";
    overlayNames[grabberRBN] = "grabberRBN";
    overlayNames[grabberRBF] = "grabberRBF";
    overlayNames[grabberLTN] = "grabberLTN";
    overlayNames[grabberLTF] = "grabberLTF";
    overlayNames[grabberRTN] = "grabberRTN";
    overlayNames[grabberRTF] = "grabberRTF";

    overlayNames[grabberTOP] = "grabberTOP";
    overlayNames[grabberBOTTOM] = "grabberBOTTOM";
    overlayNames[grabberLEFT] = "grabberLEFT";
    overlayNames[grabberRIGHT] = "grabberRIGHT";
    overlayNames[grabberNEAR] = "grabberNEAR";
    overlayNames[grabberFAR] = "grabberFAR";

    overlayNames[grabberEdgeTR] = "grabberEdgeTR";
    overlayNames[grabberEdgeTL] = "grabberEdgeTL";
    overlayNames[grabberEdgeTF] = "grabberEdgeTF";
    overlayNames[grabberEdgeTN] = "grabberEdgeTN";
    overlayNames[grabberEdgeBR] = "grabberEdgeBR";
    overlayNames[grabberEdgeBL] = "grabberEdgeBL";
    overlayNames[grabberEdgeBF] = "grabberEdgeBF";
    overlayNames[grabberEdgeBN] = "grabberEdgeBN";
    overlayNames[grabberEdgeNR] = "grabberEdgeNR";
    overlayNames[grabberEdgeNL] = "grabberEdgeNL";
    overlayNames[grabberEdgeFR] = "grabberEdgeFR";
    overlayNames[grabberEdgeFL] = "grabberEdgeFL";

    overlayNames[yawHandle] = "yawHandle";
    overlayNames[pitchHandle] = "pitchHandle";
    overlayNames[rollHandle] = "rollHandle";

    overlayNames[rotateOverlayTarget] = "rotateOverlayTarget";
    overlayNames[rotateOverlayInner] = "rotateOverlayInner";
    overlayNames[rotateOverlayOuter] = "rotateOverlayOuter";
    overlayNames[rotateOverlayCurrent] = "rotateOverlayCurrent";

    overlayNames[rotateZeroOverlay] = "rotateZeroOverlay";
    overlayNames[rotateCurrentOverlay] = "rotateCurrentOverlay";

    var activeTool = null;
    var grabberTools = {};

    // We get mouseMoveEvents from the handControllers, via handControllerPointer.
    // But we dont' get mousePressEvents.
    that.triggerMapping = Controller.newMapping(Script.resolvePath('') + '-click');
    Script.scriptEnding.connect(that.triggerMapping.disable);
    that.TRIGGER_GRAB_VALUE = 0.85;  //  From handControllerGrab/Pointer.js. Should refactor.
    that.TRIGGER_ON_VALUE = 0.4;
    that.TRIGGER_OFF_VALUE = 0.15;
    that.triggered = false;
    var activeHand = Controller.Standard.RightHand;
    function makeTriggerHandler(hand) {
        return function (value) {
            if (!that.triggered && (value > that.TRIGGER_GRAB_VALUE)) { // should we smooth?
                that.triggered = true;
                if (activeHand !== hand) {
                    // No switching while the other is already triggered, so no need to release.
                    activeHand = (activeHand === Controller.Standard.RightHand) ? Controller.Standard.LeftHand : Controller.Standard.RightHand;
                }
                if (Reticle.pointingAtSystemOverlay || Overlays.getOverlayAtPoint(Reticle.position)) {
                    return;
                }
                var eventResult = that.mousePressEvent({});
                if (!eventResult || (eventResult === 'selectionBox')) {
                    var pickRay = controllerComputePickRay();
                    if (pickRay) {
                        var entityIntersection = Entities.findRayIntersection(pickRay, true);


                        var overlayIntersection = Overlays.findRayIntersection(pickRay);
                        if (entityIntersection.intersects &&
                            (!overlayIntersection.intersects || (entityIntersection.distance < overlayIntersection.distance))) {

                            if (HMD.tabletID == entityIntersection.entityID) {
                                return;
                            }

                            selectionManager.setSelections([entityIntersection.entityID]);
                        }
                    }
                }
            } else if (that.triggered && (value < that.TRIGGER_OFF_VALUE)) {
                that.triggered = false;
                that.mouseReleaseEvent({});
            }
        };
    }
    that.triggerMapping.from(Controller.Standard.RT).peek().to(makeTriggerHandler(Controller.Standard.RightHand));
    that.triggerMapping.from(Controller.Standard.LT).peek().to(makeTriggerHandler(Controller.Standard.LeftHand));
    function controllerComputePickRay() {
        var controllerPose = getControllerWorldLocation(activeHand, true);
        if (controllerPose.valid && that.triggered) {
            var controllerPosition = controllerPose.translation;
            // This gets point direction right, but if you want general quaternion it would be more complicated:
            var controllerDirection = Quat.getUp(controllerPose.rotation);
            return {origin: controllerPosition, direction: controllerDirection};
        }
    }
    function generalComputePickRay(x, y) {
        return controllerComputePickRay() || Camera.computePickRay(x, y);
    }
    function addGrabberTool(overlay, tool) {
        grabberTools[overlay] = {
            mode: tool.mode,
            onBegin: tool.onBegin,
            onMove: tool.onMove,
            onEnd: tool.onEnd,
        }
    }


    that.cleanup = function() {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.deleteOverlay(allOverlays[i]);
        }
        for (var i = 0; i < selectionBoxes.length; i++) {
            Overlays.deleteOverlay(selectionBoxes[i]);
        }
    };

    that.highlightSelectable = function(entityID) {
        var properties = Entities.getEntityProperties(entityID);
        Overlays.editOverlay(highlightBox, {
            visible: true,
            position: properties.boundingBox.center,
            dimensions: properties.dimensions,
            rotation: properties.rotation
        });
    };

    that.unhighlightSelectable = function(entityID) {
        Overlays.editOverlay(highlightBox, {
            visible: false
        });
    };

    that.select = function(entityID, event) {
        var properties = Entities.getEntityProperties(SelectionManager.selections[0]);

        lastCameraPosition = Camera.getPosition();
        lastCameraOrientation = Camera.getOrientation();

        if (event !== false) {
            pickRay = generalComputePickRay(event.x, event.y);

            var wantDebug = false;
            if (wantDebug) {
                print("select() with EVENT...... ");
                print("                event.y:" + event.y);
                Vec3.print("       current position:", properties.position);
            }


        }

        Overlays.editOverlay(highlightBox, {
            visible: false
        });

        that.updateHandles();
    }

    that.updateRotationHandles = function() {
        var diagonal = (Vec3.length(selectionManager.worldDimensions) / 2) * 1.1;
        var halfDimensions = Vec3.multiply(selectionManager.worldDimensions, 0.5);
        var innerActive = false;
        var innerAlpha = 0.2;
        var outerAlpha = 0.2;
        if (innerActive) {
            innerAlpha = 0.5;
        } else {
            outerAlpha = 0.5;
        }

        var rotateHandleOffset = 0.05;

        var top, far, left, bottom, near, right, boundsCenter, objectCenter, BLN, BRN, BLF, TLN, TRN, TLF, TRF;

        var dimensions, rotation;
        if (spaceMode == SPACE_LOCAL) {
            rotation = SelectionManager.localRotation;
        } else {
            rotation = SelectionManager.worldRotation;
        }
        objectCenter = SelectionManager.worldPosition;
        dimensions = SelectionManager.worldDimensions;
        var position = objectCenter;

        top = objectCenter.y + (dimensions.y / 2);
        far = objectCenter.z + (dimensions.z / 2);
        left = objectCenter.x + (dimensions.x / 2);

        bottom = objectCenter.y - (dimensions.y / 2);
        near = objectCenter.z - (dimensions.z / 2);
        right = objectCenter.x - (dimensions.x / 2);

        boundsCenter = objectCenter;

        var yawCorner;
        var pitchCorner;
        var rollCorner;

        // determine which bottom corner we are closest to
        /*------------------------------
          example:
          
            BRF +--------+ BLF
                |        |
                |        |
            BRN +--------+ BLN
                   
                   *
                
        ------------------------------*/

        var cameraPosition = Camera.getPosition();
        if (cameraPosition.x > objectCenter.x) {
            // must be BRF or BRN
            if (cameraPosition.z < objectCenter.z) {

                yawHandleRotation = Quat.fromVec3Degrees({
                    x: 270,
                    y: 90,
                    z: 0
                });
                pitchHandleRotation = Quat.fromVec3Degrees({
                    x: 0,
                    y: 90,
                    z: 0
                });
                rollHandleRotation = Quat.fromVec3Degrees({
                    x: 0,
                    y: 0,
                    z: 0
                });

                yawNormal = {
                    x: 0,
                    y: 1,
                    z: 0
                };
                pitchNormal = {
                    x: 1,
                    y: 0,
                    z: 0
                };
                rollNormal = {
                    x: 0,
                    y: 0,
                    z: 1
                };

                yawCorner = {
                    x: left + rotateHandleOffset,
                    y: bottom - rotateHandleOffset,
                    z: near - rotateHandleOffset
                };

                pitchCorner = {
                    x: right - rotateHandleOffset,
                    y: top + rotateHandleOffset,
                    z: near - rotateHandleOffset
                };

                rollCorner = {
                    x: left + rotateHandleOffset,
                    y: top + rotateHandleOffset,
                    z: far + rotateHandleOffset
                };

                yawCenter = {
                    x: boundsCenter.x,
                    y: bottom,
                    z: boundsCenter.z
                };
                pitchCenter = {
                    x: right,
                    y: boundsCenter.y,
                    z: boundsCenter.z
                };
                rollCenter = {
                    x: boundsCenter.x,
                    y: boundsCenter.y,
                    z: far
                };


                Overlays.editOverlay(pitchHandle, {
                    url: ROTATE_ARROW_WEST_SOUTH_URL
                });
                Overlays.editOverlay(rollHandle, {
                    url: ROTATE_ARROW_WEST_SOUTH_URL
                });


            } else {

                yawHandleRotation = Quat.fromVec3Degrees({
                    x: 270,
                    y: 0,
                    z: 0
                });
                pitchHandleRotation = Quat.fromVec3Degrees({
                    x: 180,
                    y: 270,
                    z: 0
                });
                rollHandleRotation = Quat.fromVec3Degrees({
                    x: 0,
                    y: 0,
                    z: 90
                });

                yawNormal = {
                    x: 0,
                    y: 1,
                    z: 0
                };
                pitchNormal = {
                    x: 1,
                    y: 0,
                    z: 0
                };
                rollNormal = {
                    x: 0,
                    y: 0,
                    z: 1
                };


                yawCorner = {
                    x: left + rotateHandleOffset,
                    y: bottom - rotateHandleOffset,
                    z: far + rotateHandleOffset
                };

                pitchCorner = {
                    x: right - rotateHandleOffset,
                    y: top + rotateHandleOffset,
                    z: far + rotateHandleOffset
                };

                rollCorner = {
                    x: left + rotateHandleOffset,
                    y: top + rotateHandleOffset,
                    z: near - rotateHandleOffset
                };


                yawCenter = {
                    x: boundsCenter.x,
                    y: bottom,
                    z: boundsCenter.z
                };
                pitchCenter = {
                    x: right,
                    y: boundsCenter.y,
                    z: boundsCenter.z
                };
                rollCenter = {
                    x: boundsCenter.x,
                    y: boundsCenter.y,
                    z: near
                };

                Overlays.editOverlay(pitchHandle, {
                    url: ROTATE_ARROW_WEST_NORTH_URL
                });
                Overlays.editOverlay(rollHandle, {
                    url: ROTATE_ARROW_WEST_NORTH_URL
                });
            }
        } else {

            // must be BLF or BLN
            if (cameraPosition.z < objectCenter.z) {

                yawHandleRotation = Quat.fromVec3Degrees({
                    x: 270,
                    y: 180,
                    z: 0
                });
                pitchHandleRotation = Quat.fromVec3Degrees({
                    x: 90,
                    y: 0,
                    z: 90
                });
                rollHandleRotation = Quat.fromVec3Degrees({
                    x: 0,
                    y: 0,
                    z: 180
                });

                yawNormal = {
                    x: 0,
                    y: 1,
                    z: 0
                };
                pitchNormal = {
                    x: 1,
                    y: 0,
                    z: 0
                };
                rollNormal = {
                    x: 0,
                    y: 0,
                    z: 1
                };

                yawCorner = {
                    x: right - rotateHandleOffset,
                    y: bottom - rotateHandleOffset,
                    z: near - rotateHandleOffset
                };

                pitchCorner = {
                    x: left + rotateHandleOffset,
                    y: top + rotateHandleOffset,
                    z: near - rotateHandleOffset
                };

                rollCorner = {
                    x: right - rotateHandleOffset,
                    y: top + rotateHandleOffset,
                    z: far + rotateHandleOffset
                };

                yawCenter = {
                    x: boundsCenter.x,
                    y: bottom,
                    z: boundsCenter.z
                };
                pitchCenter = {
                    x: left,
                    y: boundsCenter.y,
                    z: boundsCenter.z
                };
                rollCenter = {
                    x: boundsCenter.x,
                    y: boundsCenter.y,
                    z: far
                };

                Overlays.editOverlay(pitchHandle, {
                    url: ROTATE_ARROW_WEST_NORTH_URL
                });
                Overlays.editOverlay(rollHandle, {
                    url: ROTATE_ARROW_WEST_NORTH_URL
                });

            } else {

                yawHandleRotation = Quat.fromVec3Degrees({
                    x: 270,
                    y: 270,
                    z: 0
                });
                pitchHandleRotation = Quat.fromVec3Degrees({
                    x: 180,
                    y: 270,
                    z: 0
                });
                rollHandleRotation = Quat.fromVec3Degrees({
                    x: 0,
                    y: 0,
                    z: 180
                });

                yawNormal = {
                    x: 0,
                    y: 1,
                    z: 0
                };
                rollNormal = {
                    x: 0,
                    y: 0,
                    z: 1
                };
                pitchNormal = {
                    x: 1,
                    y: 0,
                    z: 0
                };

                yawCorner = {
                    x: right - rotateHandleOffset,
                    y: bottom - rotateHandleOffset,
                    z: far + rotateHandleOffset
                };

                rollCorner = {
                    x: right - rotateHandleOffset,
                    y: top + rotateHandleOffset,
                    z: near - rotateHandleOffset
                };

                pitchCorner = {
                    x: left + rotateHandleOffset,
                    y: top + rotateHandleOffset,
                    z: far + rotateHandleOffset
                };

                yawCenter = {
                    x: boundsCenter.x,
                    y: bottom,
                    z: boundsCenter.z
                };
                rollCenter = {
                    x: boundsCenter.x,
                    y: boundsCenter.y,
                    z: near
                };
                pitchCenter = {
                    x: left,
                    y: boundsCenter.y,
                    z: boundsCenter.z
                };

                Overlays.editOverlay(pitchHandle, {
                    url: ROTATE_ARROW_WEST_NORTH_URL
                });
                Overlays.editOverlay(rollHandle, {
                    url: ROTATE_ARROW_WEST_NORTH_URL
                });

            }
        }

        var rotateHandlesVisible = true;
        var rotationOverlaysVisible = false;
        var translateHandlesVisible = true;
        var stretchHandlesVisible = true;
        var selectionBoxVisible = true;
        var isPointLight = false;

        if (selectionManager.selections.length == 1) {
            var properties = Entities.getEntityProperties(selectionManager.selections[0]);
            isPointLight = properties.type == "Light" && !properties.isSpotlight;
        }

        if (mode == "ROTATE_YAW" || mode == "ROTATE_PITCH" || mode == "ROTATE_ROLL" || mode == "TRANSLATE_X in case they Z") {
            rotationOverlaysVisible = true;
            rotateHandlesVisible = false;
            translateHandlesVisible = false;
            stretchHandlesVisible = false;
            selectionBoxVisible = false;
        } else if (mode == "TRANSLATE_UP_DOWN" || isPointLight) {
            rotateHandlesVisible = false;
            stretchHandlesVisible = false;
        } else if (mode != "UNKNOWN") {
            // every other mode is a stretch mode...
            rotateHandlesVisible = false;
            translateHandlesVisible = false;
        }

        var rotation = selectionManager.worldRotation;
        var dimensions = selectionManager.worldDimensions;
        var position = selectionManager.worldPosition;


        Overlays.editOverlay(rotateOverlayTarget, {
            visible: rotationOverlaysVisible
        });
        Overlays.editOverlay(rotateZeroOverlay, {
            visible: rotationOverlaysVisible
        });
        Overlays.editOverlay(rotateCurrentOverlay, {
            visible: rotationOverlaysVisible
        });

        // TODO: we have not implemented the rotating handle/controls yet... so for now, these handles are hidden
        Overlays.editOverlay(yawHandle, {
            visible: rotateHandlesVisible,
            position: yawCorner,
            rotation: yawHandleRotation
        });
        Overlays.editOverlay(pitchHandle, {
            visible: rotateHandlesVisible,
            position: pitchCorner,
            rotation: pitchHandleRotation
        });
        Overlays.editOverlay(rollHandle, {
            visible: rotateHandlesVisible,
            position: rollCorner,
            rotation: rollHandleRotation
        });
    };

    that.setSpaceMode = function(newSpaceMode) {
        if (spaceMode != newSpaceMode) {
            spaceMode = newSpaceMode;
            that.updateHandles();
        }
    };

    that.toggleSpaceMode = function() {
        if (spaceMode == SPACE_WORLD && SelectionManager.selections.length > 1) {
            print("Local space editing is not available with multiple selections");
            return;
        }
        spaceMode = spaceMode == SPACE_LOCAL ? SPACE_WORLD : SPACE_LOCAL;
        that.updateHandles();
    };

    that.unselectAll = function() {};

    that.updateHandles = function() {
        if (SelectionManager.selections.length == 0) {
            that.setOverlaysVisible(false);
            return;
        }

        that.updateRotationHandles();

        var rotation, dimensions, position, registrationPoint;

        if (spaceMode == SPACE_LOCAL) {
            rotation = SelectionManager.localRotation;
            dimensions = SelectionManager.localDimensions;
            position = SelectionManager.localPosition;
            registrationPoint = SelectionManager.localRegistrationPoint;
        } else {
            rotation = Quat.fromPitchYawRollDegrees(0, 0, 0);
            dimensions = SelectionManager.worldDimensions;
            position = SelectionManager.worldPosition;
            registrationPoint = SelectionManager.worldRegistrationPoint;
        }

        var registrationPointDimensions = {
            x: dimensions.x * registrationPoint.x,
            y: dimensions.y * registrationPoint.y,
            z: dimensions.z * registrationPoint.z,
        };

        // Center of entity, relative to registration point
        center = getRelativeCenterPosition(dimensions, registrationPoint);

        // Distances in world coordinates relative to the registration point
        var left = -registrationPointDimensions.x;
        var right = dimensions.x - registrationPointDimensions.x;
        var bottom = -registrationPointDimensions.y;
        var top = dimensions.y - registrationPointDimensions.y;
        var near = -registrationPointDimensions.z;
        var front = far = dimensions.z - registrationPointDimensions.z;

        var worldTop = SelectionManager.worldDimensions.y / 2;

        var LBN = {
            x: left,
            y: bottom,
            z: near
        };
        var RBN = {
            x: right,
            y: bottom,
            z: near
        };
        var LBF = {
            x: left,
            y: bottom,
            z: far
        };
        var RBF = {
            x: right,
            y: bottom,
            z: far
        };
        var LTN = {
            x: left,
            y: top,
            z: near
        };
        var RTN = {
            x: right,
            y: top,
            z: near
        };
        var LTF = {
            x: left,
            y: top,
            z: far
        };
        var RTF = {
            x: right,
            y: top,
            z: far
        };

        var TOP = {
            x: center.x,
            y: top,
            z: center.z
        };
        var BOTTOM = {
            x: center.x,
            y: bottom,
            z: center.z
        };
        var LEFT = {
            x: left,
            y: center.y,
            z: center.z
        };
        var RIGHT = {
            x: right,
            y: center.y,
            z: center.z
        };
        var NEAR = {
            x: center.x,
            y: center.y,
            z: near
        };
        var FAR = {
            x: center.x,
            y: center.y,
            z: far
        };

        var EdgeTR = {
            x: right,
            y: top,
            z: center.z
        };
        var EdgeTL = {
            x: left,
            y: top,
            z: center.z
        };
        var EdgeTF = {
            x: center.x,
            y: top,
            z: front
        };
        var EdgeTN = {
            x: center.x,
            y: top,
            z: near
        };
        var EdgeBR = {
            x: right,
            y: bottom,
            z: center.z
        };
        var EdgeBL = {
            x: left,
            y: bottom,
            z: center.z
        };
        var EdgeBF = {
            x: center.x,
            y: bottom,
            z: front
        };
        var EdgeBN = {
            x: center.x,
            y: bottom,
            z: near
        };
        var EdgeNR = {
            x: right,
            y: center.y,
            z: near
        };
        var EdgeNL = {
            x: left,
            y: center.y,
            z: near
        };
        var EdgeFR = {
            x: right,
            y: center.y,
            z: front
        };
        var EdgeFL = {
            x: left,
            y: center.y,
            z: front
        };

        LBN = Vec3.multiplyQbyV(rotation, LBN);
        RBN = Vec3.multiplyQbyV(rotation, RBN);
        LBF = Vec3.multiplyQbyV(rotation, LBF);
        RBF = Vec3.multiplyQbyV(rotation, RBF);
        LTN = Vec3.multiplyQbyV(rotation, LTN);
        RTN = Vec3.multiplyQbyV(rotation, RTN);
        LTF = Vec3.multiplyQbyV(rotation, LTF);
        RTF = Vec3.multiplyQbyV(rotation, RTF);

        TOP = Vec3.multiplyQbyV(rotation, TOP);
        BOTTOM = Vec3.multiplyQbyV(rotation, BOTTOM);
        LEFT = Vec3.multiplyQbyV(rotation, LEFT);
        RIGHT = Vec3.multiplyQbyV(rotation, RIGHT);
        NEAR = Vec3.multiplyQbyV(rotation, NEAR);
        FAR = Vec3.multiplyQbyV(rotation, FAR);

        EdgeTR = Vec3.multiplyQbyV(rotation, EdgeTR);
        EdgeTL = Vec3.multiplyQbyV(rotation, EdgeTL);
        EdgeTF = Vec3.multiplyQbyV(rotation, EdgeTF);
        EdgeTN = Vec3.multiplyQbyV(rotation, EdgeTN);
        EdgeBR = Vec3.multiplyQbyV(rotation, EdgeBR);
        EdgeBL = Vec3.multiplyQbyV(rotation, EdgeBL);
        EdgeBF = Vec3.multiplyQbyV(rotation, EdgeBF);
        EdgeBN = Vec3.multiplyQbyV(rotation, EdgeBN);
        EdgeNR = Vec3.multiplyQbyV(rotation, EdgeNR);
        EdgeNL = Vec3.multiplyQbyV(rotation, EdgeNL);
        EdgeFR = Vec3.multiplyQbyV(rotation, EdgeFR);
        EdgeFL = Vec3.multiplyQbyV(rotation, EdgeFL);

        LBN = Vec3.sum(position, LBN);
        RBN = Vec3.sum(position, RBN);
        LBF = Vec3.sum(position, LBF);
        RBF = Vec3.sum(position, RBF);
        LTN = Vec3.sum(position, LTN);
        RTN = Vec3.sum(position, RTN);
        LTF = Vec3.sum(position, LTF);
        RTF = Vec3.sum(position, RTF);

        TOP = Vec3.sum(position, TOP);
        BOTTOM = Vec3.sum(position, BOTTOM);
        LEFT = Vec3.sum(position, LEFT);
        RIGHT = Vec3.sum(position, RIGHT);
        NEAR = Vec3.sum(position, NEAR);
        FAR = Vec3.sum(position, FAR);

        EdgeTR = Vec3.sum(position, EdgeTR);
        EdgeTL = Vec3.sum(position, EdgeTL);
        EdgeTF = Vec3.sum(position, EdgeTF);
        EdgeTN = Vec3.sum(position, EdgeTN);
        EdgeBR = Vec3.sum(position, EdgeBR);
        EdgeBL = Vec3.sum(position, EdgeBL);
        EdgeBF = Vec3.sum(position, EdgeBF);
        EdgeBN = Vec3.sum(position, EdgeBN);
        EdgeNR = Vec3.sum(position, EdgeNR);
        EdgeNL = Vec3.sum(position, EdgeNL);
        EdgeFR = Vec3.sum(position, EdgeFR);
        EdgeFL = Vec3.sum(position, EdgeFL);

        var stretchHandlesVisible = spaceMode == SPACE_LOCAL;
        var extendedStretchHandlesVisible = stretchHandlesVisible && showExtendedStretchHandles;

        if (selectionManager.selections.length == 1) {
            var properties = Entities.getEntityProperties(selectionManager.selections[0]);
            if (properties.type == "Light" && properties.isSpotlight == true) {
                var stretchHandlesVisible = false;
                var extendedStretchHandlesVisible = false;

                Overlays.editOverlay(grabberSpotLightCenter, {
                    position: position,
                    visible: false,
                });
                Overlays.editOverlay(grabberSpotLightRadius, {
                    position: NEAR,
                    rotation: rotation,
                    visible: true,
                });
                var distance = (properties.dimensions.z / 2) * Math.sin(properties.cutoff * (Math.PI / 180));

                Overlays.editOverlay(grabberSpotLightL, {
                    position: EdgeNL,
                    rotation: rotation,
                    visible: true,
                });
                Overlays.editOverlay(grabberSpotLightR, {
                    position: EdgeNR,
                    rotation: rotation,
                    visible: true,
                });
                Overlays.editOverlay(grabberSpotLightT, {
                    position: EdgeTN,
                    rotation: rotation,
                    visible: true,
                });
                Overlays.editOverlay(grabberSpotLightB, {
                    position: EdgeBN,
                    rotation: rotation,
                    visible: true,
                });
                Overlays.editOverlay(grabberSpotLightCircle, {
                    position: NEAR,
                    dimensions: {
                        x: distance,
                        y: distance,
                        z: 1
                    },
                    lineWidth: 1.5,
                    rotation: rotation,
                    visible: true,
                });

                Overlays.editOverlay(grabberSpotLightLineT, {
                    start: position,
                    end: EdgeTN,
                    visible: true,
                });
                Overlays.editOverlay(grabberSpotLightLineB, {
                    start: position,
                    end: EdgeBN,
                    visible: true,
                });
                Overlays.editOverlay(grabberSpotLightLineR, {
                    start: position,
                    end: EdgeNR,
                    visible: true,
                });
                Overlays.editOverlay(grabberSpotLightLineL, {
                    start: position,
                    end: EdgeNL,
                    visible: true,
                });

                Overlays.editOverlay(grabberPointLightCircleX, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightCircleY, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightCircleZ, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightT, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightB, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightL, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightR, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightF, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightN, {
                    visible: false
                });
            } else if (properties.type == "Light" && properties.isSpotlight == false) {
                var stretchHandlesVisible = false;
                var extendedStretchHandlesVisible = false;
                Overlays.editOverlay(grabberPointLightT, {
                    position: TOP,
                    rotation: rotation,
                    visible: true,
                });
                Overlays.editOverlay(grabberPointLightB, {
                    position: BOTTOM,
                    rotation: rotation,
                    visible: true,
                });
                Overlays.editOverlay(grabberPointLightL, {
                    position: LEFT,
                    rotation: rotation,
                    visible: true,
                });
                Overlays.editOverlay(grabberPointLightR, {
                    position: RIGHT,
                    rotation: rotation,
                    visible: true,
                });
                Overlays.editOverlay(grabberPointLightF, {
                    position: FAR,
                    rotation: rotation,
                    visible: true,
                });
                Overlays.editOverlay(grabberPointLightN, {
                    position: NEAR,
                    rotation: rotation,
                    visible: true,
                });
                Overlays.editOverlay(grabberPointLightCircleX, {
                    position: position,
                    rotation: Quat.multiply(rotation, Quat.fromPitchYawRollDegrees(0, 90, 0)),
                    dimensions: {
                        x: properties.dimensions.z / 2.0,
                        y: properties.dimensions.z / 2.0,
                        z: 1
                    },
                    visible: true,
                });
                Overlays.editOverlay(grabberPointLightCircleY, {
                    position: position,
                    rotation: Quat.multiply(rotation, Quat.fromPitchYawRollDegrees(90, 0, 0)),
                    dimensions: {
                        x: properties.dimensions.z / 2.0,
                        y: properties.dimensions.z / 2.0,
                        z: 1
                    },
                    visible: true,
                });
                Overlays.editOverlay(grabberPointLightCircleZ, {
                    position: position,
                    rotation: rotation,
                    dimensions: {
                        x: properties.dimensions.z / 2.0,
                        y: properties.dimensions.z / 2.0,
                        z: 1
                    },
                    visible: true,
                });

                Overlays.editOverlay(grabberSpotLightRadius, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightL, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightR, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightT, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightB, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightCircle, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightLineL, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightLineR, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightLineT, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightLineB, {
                    visible: false
                });
            } else {
                Overlays.editOverlay(grabberSpotLightCenter, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightRadius, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightL, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightR, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightT, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightB, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightCircle, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightLineL, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightLineR, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightLineT, {
                    visible: false
                });
                Overlays.editOverlay(grabberSpotLightLineB, {
                    visible: false
                });

                Overlays.editOverlay(grabberPointLightCircleX, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightCircleY, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightCircleZ, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightT, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightB, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightL, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightR, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightF, {
                    visible: false
                });
                Overlays.editOverlay(grabberPointLightN, {
                    visible: false
                });
            }
        }



        Overlays.editOverlay(grabberLBN, {
            visible: stretchHandlesVisible,
            rotation: rotation,
            position: LBN
        });
        Overlays.editOverlay(grabberRBN, {
            visible: stretchHandlesVisible,
            rotation: rotation,
            position: RBN
        });
        Overlays.editOverlay(grabberLBF, {
            visible: stretchHandlesVisible,
            rotation: rotation,
            position: LBF
        });
        Overlays.editOverlay(grabberRBF, {
            visible: stretchHandlesVisible,
            rotation: rotation,
            position: RBF
        });

        Overlays.editOverlay(grabberLTN, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: LTN
        });
        Overlays.editOverlay(grabberRTN, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: RTN
        });
        Overlays.editOverlay(grabberLTF, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: LTF
        });
        Overlays.editOverlay(grabberRTF, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: RTF
        });

        Overlays.editOverlay(grabberTOP, {
            visible: stretchHandlesVisible,
            rotation: rotation,
            position: TOP
        });
        Overlays.editOverlay(grabberBOTTOM, {
            visible: stretchHandlesVisible,
            rotation: rotation,
            position: BOTTOM
        });
        Overlays.editOverlay(grabberLEFT, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: LEFT
        });
        Overlays.editOverlay(grabberRIGHT, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: RIGHT
        });
        Overlays.editOverlay(grabberNEAR, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: NEAR
        });
        Overlays.editOverlay(grabberFAR, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: FAR
        });

        var boxPosition = Vec3.multiplyQbyV(rotation, center);
        boxPosition = Vec3.sum(position, boxPosition);
        Overlays.editOverlay(selectionBox, {
            position: boxPosition,
            dimensions: dimensions,
            rotation: rotation,
            visible: !(mode == "ROTATE_YAW" || mode == "ROTATE_PITCH" || mode == "ROTATE_ROLL"),
        });

        // Create more selection box overlays if we don't have enough
        var overlaysNeeded = selectionManager.selections.length - selectionBoxes.length;
        for (var i = 0; i < overlaysNeeded; i++) {
            selectionBoxes.push(
                Overlays.addOverlay("cube", {
                    position: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    size: 1,
                    color: {
                        red: 255,
                        green: 153,
                        blue: 0
                    },
                    alpha: 1,
                    solid: false,
                    visible: false,
                    dashed: false,
                    lineWidth: 1.0,
                    ignoreRayIntersection: true,
                }));
        }

        var i = 0;
        // Only show individual selections boxes if there is more than 1 selection
        if (selectionManager.selections.length > 1) {
            for (; i < selectionManager.selections.length; i++) {
                var properties = Entities.getEntityProperties(selectionManager.selections[i]);

                // Adjust overlay position to take registrationPoint into account
                // centeredRP = registrationPoint with range [-0.5, 0.5]
                var centeredRP = Vec3.subtract(properties.registrationPoint, {
                    x: 0.5,
                    y: 0.5,
                    z: 0.5
                });
                var offset = vec3Mult(properties.dimensions, centeredRP);
                offset = Vec3.multiply(-1, offset);
                offset = Vec3.multiplyQbyV(properties.rotation, offset);
                var boxPosition = Vec3.sum(properties.position, offset);

                Overlays.editOverlay(selectionBoxes[i], {
                    position: boxPosition,
                    rotation: properties.rotation,
                    dimensions: properties.dimensions,
                    visible: true,
                });
            }
        }
        // Hide any remaining selection boxes
        for (; i < selectionBoxes.length; i++) {
            Overlays.editOverlay(selectionBoxes[i], {
                visible: false
            });
        }

        Overlays.editOverlay(grabberEdgeTR, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: EdgeTR
        });
        Overlays.editOverlay(grabberEdgeTL, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: EdgeTL
        });
        Overlays.editOverlay(grabberEdgeTF, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: EdgeTF
        });
        Overlays.editOverlay(grabberEdgeTN, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: EdgeTN
        });
        Overlays.editOverlay(grabberEdgeBR, {
            visible: stretchHandlesVisible,
            rotation: rotation,
            position: EdgeBR
        });
        Overlays.editOverlay(grabberEdgeBL, {
            visible: stretchHandlesVisible,
            rotation: rotation,
            position: EdgeBL
        });
        Overlays.editOverlay(grabberEdgeBF, {
            visible: stretchHandlesVisible,
            rotation: rotation,
            position: EdgeBF
        });
        Overlays.editOverlay(grabberEdgeBN, {
            visible: stretchHandlesVisible,
            rotation: rotation,
            position: EdgeBN
        });
        Overlays.editOverlay(grabberEdgeNR, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: EdgeNR
        });
        Overlays.editOverlay(grabberEdgeNL, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: EdgeNL
        });
        Overlays.editOverlay(grabberEdgeFR, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: EdgeFR
        });
        Overlays.editOverlay(grabberEdgeFL, {
            visible: extendedStretchHandlesVisible,
            rotation: rotation,
            position: EdgeFL
        });

        var grabberMoveUpOffset = 0.1;
        grabberMoveUpPosition = {
            x: position.x,
            y: position.y + worldTop + grabberMoveUpOffset,
            z: position.z
        }
        Overlays.editOverlay(grabberMoveUp, {
            visible: activeTool == null || mode == "TRANSLATE_UP_DOWN"
        });

        Overlays.editOverlay(baseOfEntityProjectionOverlay, {
            visible: mode != "ROTATE_YAW" && mode != "ROTATE_PITCH" && mode != "ROTATE_ROLL",
            solid: true,
            position: {
                x: selectionManager.worldPosition.x,
                y: grid.getOrigin().y,
                z: selectionManager.worldPosition.z
            },
            dimensions: {
                x: selectionManager.worldDimensions.x,
                y: selectionManager.worldDimensions.z
            },
            rotation: Quat.fromPitchYawRollDegrees(90, 0, 0),
        });


    };

    that.setOverlaysVisible = function(isVisible) {
        var length = allOverlays.length;
        for (var i = 0; i < length; i++) {
            Overlays.editOverlay(allOverlays[i], {
                visible: isVisible
            });
        }
        length = selectionBoxes.length;
        for (var i = 0; i < length; i++) {
            Overlays.editOverlay(selectionBoxes[i], {
                visible: isVisible
            });
        }
    };

    that.unselect = function(entityID) {};

    var initialXZPick = null;
    var isConstrained = false;
    var constrainMajorOnly = false;
    var startPosition = null;
    var duplicatedEntityIDs = null;

    var translateXZTool = {
        mode: 'TRANSLATE_XZ',
        pickPlanePosition: { x: 0, y: 0, z: 0 },
        greatestDimension: 0.0,
        startingDistance: 0.0,
        startingElevation: 0.0,
        onBegin: function(event) {
            SelectionManager.saveProperties();
            startPosition = SelectionManager.worldPosition;
            var dimensions = SelectionManager.worldDimensions;

            var pickRay = generalComputePickRay(event.x, event.y);
            initialXZPick = rayPlaneIntersection(pickRay, translateXZTool.pickPlanePosition, {
                x: 0,
                y: 1,
                z: 0
            });

            // Duplicate entities if alt is pressed.  This will make a
            // copy of the selected entities and move the _original_ entities, not
            // the new ones.
            if (event.isAlt) {
                duplicatedEntityIDs = [];
                for (var otherEntityID in SelectionManager.savedProperties) {
                    var properties = SelectionManager.savedProperties[otherEntityID];
                    if (!properties.locked) {
                        var entityID = Entities.addEntity(properties);
                        duplicatedEntityIDs.push({
                            entityID: entityID,
                            properties: properties,
                        });
                    }
                }
            } else {
                duplicatedEntityIDs = null;
            }

            isConstrained = false;
        },
        onEnd: function(event, reason) {
            pushCommandForSelections(duplicatedEntityIDs);

            Overlays.editOverlay(xRailOverlay, {
                visible: false
            });
            Overlays.editOverlay(zRailOverlay, {
                visible: false
            });
        },
        elevation: function(origin, intersection) {
            return (origin.y - intersection.y) / Vec3.distance(origin, intersection);
        },
        onMove: function(event) {
            var wantDebug = false;
            pickRay = generalComputePickRay(event.x, event.y);

            var pick = rayPlaneIntersection2(pickRay, translateXZTool.pickPlanePosition, {
                x: 0,
                y: 1,
                z: 0
            });

            // If the pick ray doesn't hit the pick plane in this direction, do nothing.
            // this will happen when someone drags across the horizon from the side they started on.
            if (!pick) {
                if (wantDebug) {
                    print("Pick ray does not intersect XZ plane.");
                }
                return;
            }

            var vector = Vec3.subtract(pick, initialXZPick);

            // If the mouse is too close to the horizon of the pick plane, stop moving
            var MIN_ELEVATION = 0.02;   //  largest dimension of object divided by distance to it
            var elevation = translateXZTool.elevation(pickRay.origin, pick);
            if (wantDebug) {
                    print("Start Elevation: " + translateXZTool.startingElevation + ", elevation: " + elevation);
            }
            if ((translateXZTool.startingElevation > 0.0 && elevation < MIN_ELEVATION) || 
                (translateXZTool.startingElevation < 0.0 && elevation > -MIN_ELEVATION)) {
                if (wantDebug) {
                    print("too close to horizon!");
                }
                return;
            }

            //  If the angular size of the object is too small, stop moving
            var MIN_ANGULAR_SIZE = 0.01;   //  Radians
            if (translateXZTool.greatestDimension > 0) {
                var angularSize = Math.atan(translateXZTool.greatestDimension / Vec3.distance(pickRay.origin, pick));
                if (wantDebug) {
                    print("Angular size = " + angularSize);
                }
                if (angularSize < MIN_ANGULAR_SIZE) {
                    return;
                }
            }

            // If shifted, constrain to one axis
            if (event.isShifted) {
                if (Math.abs(vector.x) > Math.abs(vector.z)) {
                    vector.z = 0;
                } else {
                    vector.x = 0;
                }
                if (!isConstrained) {
                    Overlays.editOverlay(xRailOverlay, {
                        visible: true
                    });
                    var xStart = Vec3.sum(startPosition, {
                        x: -10000,
                        y: 0,
                        z: 0
                    });
                    var xEnd = Vec3.sum(startPosition, {
                        x: 10000,
                        y: 0,
                        z: 0
                    });
                    var zStart = Vec3.sum(startPosition, {
                        x: 0,
                        y: 0,
                        z: -10000
                    });
                    var zEnd = Vec3.sum(startPosition, {
                        x: 0,
                        y: 0,
                        z: 10000
                    });
                    Overlays.editOverlay(xRailOverlay, {
                        start: xStart,
                        end: xEnd,
                        visible: true
                    });
                    Overlays.editOverlay(zRailOverlay, {
                        start: zStart,
                        end: zEnd,
                        visible: true
                    });
                    isConstrained = true;
                }
            } else {
                if (isConstrained) {
                    Overlays.editOverlay(xRailOverlay, {
                        visible: false
                    });
                    Overlays.editOverlay(zRailOverlay, {
                        visible: false
                    });
                    isConstrained = false;
                }
            }

            constrainMajorOnly = event.isControl;
            var cornerPosition = Vec3.sum(startPosition, Vec3.multiply(-0.5, selectionManager.worldDimensions));
            vector = Vec3.subtract(
                grid.snapToGrid(Vec3.sum(cornerPosition, vector), constrainMajorOnly),
                cornerPosition);



            for (var i = 0; i < SelectionManager.selections.length; i++) {
                var properties = SelectionManager.savedProperties[SelectionManager.selections[i]];
                if (!properties) {
                    continue;
                }
                var newPosition = Vec3.sum(properties.position, {
                    x: vector.x,
                    y: 0,
                    z: vector.z
                });
                Entities.editEntity(SelectionManager.selections[i], {
                    position: newPosition,
                });

                if (wantDebug) {
                    print("translateXZ... ");
                    Vec3.print("                 vector:", vector);
                    Vec3.print("            newPosition:", properties.position);
                    Vec3.print("            newPosition:", newPosition);
                }
            }

            SelectionManager._update();
        }
    };

    var lastXYPick = null
    var upDownPickNormal = null;
    addGrabberTool(grabberMoveUp, {
        mode: "TRANSLATE_UP_DOWN",
        onBegin: function(event) {
            pickRay = generalComputePickRay(event.x, event.y);

            upDownPickNormal = Quat.getFront(lastCameraOrientation);
            // Remove y component so the y-axis lies along the plane we picking on - this will
            // give movements that follow the mouse.
            upDownPickNormal.y = 0;
            lastXYPick = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, upDownPickNormal);

            SelectionManager.saveProperties();

            // Duplicate entities if alt is pressed.  This will make a
            // copy of the selected entities and move the _original_ entities, not
            // the new ones.
            if (event.isAlt) {
                duplicatedEntityIDs = [];
                for (var otherEntityID in SelectionManager.savedProperties) {
                    var properties = SelectionManager.savedProperties[otherEntityID];
                    if (!properties.locked) {
                        var entityID = Entities.addEntity(properties);
                        duplicatedEntityIDs.push({
                            entityID: entityID,
                            properties: properties,
                        });
                    }
                }
            } else {
                duplicatedEntityIDs = null;
            }
        },
        onEnd: function(event, reason) {
            pushCommandForSelections(duplicatedEntityIDs);
        },
        onMove: function(event) {
            pickRay = generalComputePickRay(event.x, event.y);

            // translate mode left/right based on view toward entity
            var newIntersection = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, upDownPickNormal);

            var vector = Vec3.subtract(newIntersection, lastXYPick);
            vector = grid.snapToGrid(vector);

            // we only care about the Y axis
            vector.x = 0;
            vector.z = 0;

            var wantDebug = false;
            if (wantDebug) {
                print("translateUpDown... ");
                print("                event.y:" + event.y);
                Vec3.print("        newIntersection:", newIntersection);
                Vec3.print("                 vector:", vector);
                Vec3.print("            newPosition:", newPosition);
            }
            for (var i = 0; i < SelectionManager.selections.length; i++) {
                var id = SelectionManager.selections[i];
                var properties = selectionManager.savedProperties[id];

                var original = properties.position;
                var newPosition = Vec3.sum(properties.position, vector);

                Entities.editEntity(id, {
                    position: newPosition,
                });
            }

            SelectionManager._update();
        },
    });

    var vec3Mult = function(v1, v2) {
            return {
                x: v1.x * v2.x,
                y: v1.y * v2.y,
                z: v1.z * v2.z
            };
        }
        // stretchMode - name of mode
        // direction - direction to stretch in
        // pivot - point to use as a pivot
        // offset - the position of the overlay tool relative to the selections center position
    var makeStretchTool = function(stretchMode, direction, pivot, offset, customOnMove) {
        var signs = {
            x: direction.x < 0 ? -1 : (direction.x > 0 ? 1 : 0),
            y: direction.y < 0 ? -1 : (direction.y > 0 ? 1 : 0),
            z: direction.z < 0 ? -1 : (direction.z > 0 ? 1 : 0),
        };

        var mask = {
            x: Math.abs(direction.x) > 0 ? 1 : 0,
            y: Math.abs(direction.y) > 0 ? 1 : 0,
            z: Math.abs(direction.z) > 0 ? 1 : 0,
        };

        var numDimensions = mask.x + mask.y + mask.z;

        var planeNormal = null;
        var lastPick = null;
        var initialPosition = null;
        var initialDimensions = null;
        var initialIntersection = null;
        var initialProperties = null;
        var registrationPoint = null;
        var deltaPivot = null;
        var pickRayPosition = null;
        var rotation = null;

        var onBegin = function(event) {
            var properties = Entities.getEntityProperties(SelectionManager.selections[0]);
            initialProperties = properties;
            rotation = spaceMode == SPACE_LOCAL ? properties.rotation : Quat.fromPitchYawRollDegrees(0, 0, 0);

            if (spaceMode == SPACE_LOCAL) {
                rotation = SelectionManager.localRotation;
                initialPosition = SelectionManager.localPosition;
                initialDimensions = SelectionManager.localDimensions;
                registrationPoint = SelectionManager.localRegistrationPoint;
            } else {
                rotation = SelectionManager.worldRotation;
                initialPosition = SelectionManager.worldPosition;
                initialDimensions = SelectionManager.worldDimensions;
                registrationPoint = SelectionManager.worldRegistrationPoint;
            }

            // Modify range of registrationPoint to be [-0.5, 0.5]
            var centeredRP = Vec3.subtract(registrationPoint, {
                x: 0.5,
                y: 0.5,
                z: 0.5
            });

            // Scale pivot to be in the same range as registrationPoint
            var scaledPivot = Vec3.multiply(0.5, pivot)
            deltaPivot = Vec3.subtract(centeredRP, scaledPivot);

            var scaledOffset = Vec3.multiply(0.5, offset);

            // Offset from the registration point
            offsetRP = Vec3.subtract(scaledOffset, centeredRP);

            // Scaled offset in world coordinates
            var scaledOffsetWorld = vec3Mult(initialDimensions, offsetRP);
            pickRayPosition = Vec3.sum(initialPosition, Vec3.multiplyQbyV(rotation, scaledOffsetWorld));

            if (numDimensions == 1 && mask.x) {
                var start = Vec3.multiplyQbyV(rotation, {
                    x: -10000,
                    y: 0,
                    z: 0
                });
                start = Vec3.sum(start, properties.position);
                var end = Vec3.multiplyQbyV(rotation, {
                    x: 10000,
                    y: 0,
                    z: 0
                });
                end = Vec3.sum(end, properties.position);
                Overlays.editOverlay(xRailOverlay, {
                    start: start,
                    end: end,
                    visible: true,
                });
            }
            if (numDimensions == 1 && mask.y) {
                var start = Vec3.multiplyQbyV(rotation, {
                    x: 0,
                    y: -10000,
                    z: 0
                });
                start = Vec3.sum(start, properties.position);
                var end = Vec3.multiplyQbyV(rotation, {
                    x: 0,
                    y: 10000,
                    z: 0
                });
                end = Vec3.sum(end, properties.position);
                Overlays.editOverlay(yRailOverlay, {
                    start: start,
                    end: end,
                    visible: true,
                });
            }
            if (numDimensions == 1 && mask.z) {
                var start = Vec3.multiplyQbyV(rotation, {
                    x: 0,
                    y: 0,
                    z: -10000
                });
                start = Vec3.sum(start, properties.position);
                var end = Vec3.multiplyQbyV(rotation, {
                    x: 0,
                    y: 0,
                    z: 10000
                });
                end = Vec3.sum(end, properties.position);
                Overlays.editOverlay(zRailOverlay, {
                    start: start,
                    end: end,
                    visible: true,
                });
            }
            if (numDimensions == 1) {
                if (mask.x == 1) {
                    planeNormal = {
                        x: 0,
                        y: 1,
                        z: 0
                    };
                } else if (mask.y == 1) {
                    planeNormal = {
                        x: 1,
                        y: 0,
                        z: 0
                    };
                } else {
                    planeNormal = {
                        x: 0,
                        y: 1,
                        z: 0
                    };
                }
            } else if (numDimensions == 2) {
                if (mask.x == 0) {
                    planeNormal = {
                        x: 1,
                        y: 0,
                        z: 0
                    };
                } else if (mask.y == 0) {
                    planeNormal = {
                        x: 0,
                        y: 1,
                        z: 0
                    };
                } else {
                    planeNormal = {
                        x: 0,
                        y: 0,
                        z: z
                    };
                }
            }
            planeNormal = Vec3.multiplyQbyV(rotation, planeNormal);
            var pickRay = generalComputePickRay(event.x, event.y);
            lastPick = rayPlaneIntersection(pickRay,
                pickRayPosition,
                planeNormal);

            SelectionManager.saveProperties();
        };

        var onEnd = function(event, reason) {
            Overlays.editOverlay(xRailOverlay, {
                visible: false
            });
            Overlays.editOverlay(yRailOverlay, {
                visible: false
            });
            Overlays.editOverlay(zRailOverlay, {
                visible: false
            });

            pushCommandForSelections();
        };

        var onMove = function(event) {
            var proportional = spaceMode == SPACE_WORLD || event.isShifted || activeTool.mode == "STRETCH_RADIUS";

            var position, dimensions, rotation;
            if (spaceMode == SPACE_LOCAL) {
                position = SelectionManager.localPosition;
                dimensions = SelectionManager.localDimensions;
                rotation = SelectionManager.localRotation;
            } else {
                position = SelectionManager.worldPosition;
                dimensions = SelectionManager.worldDimensions;
                rotation = SelectionManager.worldRotation;
            }

            var pickRay = generalComputePickRay(event.x, event.y);
            newPick = rayPlaneIntersection(pickRay,
                pickRayPosition,
                planeNormal);
            var vector = Vec3.subtract(newPick, lastPick);

            vector = Vec3.multiplyQbyV(Quat.inverse(rotation), vector);

            vector = vec3Mult(mask, vector);

            if (customOnMove) {
                var change = Vec3.multiply(-1, vec3Mult(signs, vector));
                customOnMove(vector, change);
            } else {
                vector = grid.snapToSpacing(vector);

                var changeInDimensions = Vec3.multiply(-1, vec3Mult(signs, vector));
                var newDimensions;
                if (proportional) {
                    var absX = Math.abs(changeInDimensions.x);
                    var absY = Math.abs(changeInDimensions.y);
                    var absZ = Math.abs(changeInDimensions.z);
                    var pctChange = 0;
                    if (absX > absY && absX > absZ) {
                        pctChange = changeInDimensions.x / initialProperties.dimensions.x;
                        pctChange = changeInDimensions.x / initialDimensions.x;
                    } else if (absY > absZ) {
                        pctChange = changeInDimensions.y / initialProperties.dimensions.y;
                        pctChange = changeInDimensions.y / initialDimensions.y;
                    } else {
                        pctChange = changeInDimensions.z / initialProperties.dimensions.z;
                        pctChange = changeInDimensions.z / initialDimensions.z;
                    }
                    pctChange += 1.0;
                    newDimensions = Vec3.multiply(pctChange, initialDimensions);
                } else {
                    newDimensions = Vec3.sum(initialDimensions, changeInDimensions);
                }

                newDimensions.x = Math.max(newDimensions.x, MINIMUM_DIMENSION);
                newDimensions.y = Math.max(newDimensions.y, MINIMUM_DIMENSION);
                newDimensions.z = Math.max(newDimensions.z, MINIMUM_DIMENSION);

                var changeInPosition = Vec3.multiplyQbyV(rotation, vec3Mult(deltaPivot, changeInDimensions));
                var newPosition = Vec3.sum(initialPosition, changeInPosition);

                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    Entities.editEntity(SelectionManager.selections[i], {
                        position: newPosition,
                        dimensions: newDimensions,
                    });
                }

                var wantDebug = false;
                if (wantDebug) {
                    print(stretchMode);
                    //Vec3.print("        newIntersection:", newIntersection);
                    Vec3.print("                 vector:", vector);
                    //Vec3.print("           oldPOS:", oldPOS);
                    //Vec3.print("                newPOS:", newPOS);
                    Vec3.print("            changeInDimensions:", changeInDimensions);
                    Vec3.print("                 newDimensions:", newDimensions);

                    Vec3.print("              changeInPosition:", changeInPosition);
                    Vec3.print("                   newPosition:", newPosition);
                }

                SelectionManager._update();
            }

        };

        return {
            mode: stretchMode,
            onBegin: onBegin,
            onMove: onMove,
            onEnd: onEnd
        };
    };

    function addStretchTool(overlay, mode, pivot, direction, offset, handleMove) {
        if (!pivot) {
            pivot = direction;
        }
        var tool = makeStretchTool(mode, direction, pivot, offset, handleMove);

        addGrabberTool(overlay, tool);
    }

    function cutoffStretchFunc(vector, change) {
        vector = change;
        Vec3.print("Radius stretch: ", vector);
        var length = vector.x + vector.y + vector.z;
        var props = selectionManager.savedProperties[selectionManager.selections[0]];

        var radius = props.dimensions.z / 2;
        var originalCutoff = props.cutoff;

        var originalSize = radius * Math.tan(originalCutoff * (Math.PI / 180));
        var newSize = originalSize + length;
        var cutoff = Math.atan2(newSize, radius) * 180 / Math.PI;

        Entities.editEntity(selectionManager.selections[0], {
            cutoff: cutoff,
        });

        SelectionManager._update();
    };

    function radiusStretchFunc(vector, change) {
        var props = selectionManager.savedProperties[selectionManager.selections[0]];

        // Find the axis being adjusted
        var size;
        if (Math.abs(change.x) > 0) {
            size = props.dimensions.x + change.x;
        } else if (Math.abs(change.y) > 0) {
            size = props.dimensions.y + change.y;
        } else if (Math.abs(change.z) > 0) {
            size = props.dimensions.z + change.z;
        }

        var newDimensions = {
            x: size,
            y: size,
            z: size
        };

        Entities.editEntity(selectionManager.selections[0], {
            dimensions: newDimensions,
        });

        SelectionManager._update();
    }

    addStretchTool(grabberNEAR, "STRETCH_NEAR", {
        x: 0,
        y: 0,
        z: 1
    }, {
        x: 0,
        y: 0,
        z: 1
    }, {
        x: 0,
        y: 0,
        z: -1
    });
    addStretchTool(grabberFAR, "STRETCH_FAR", {
        x: 0,
        y: 0,
        z: -1
    }, {
        x: 0,
        y: 0,
        z: -1
    }, {
        x: 0,
        y: 0,
        z: 1
    });
    addStretchTool(grabberTOP, "STRETCH_TOP", {
        x: 0,
        y: -1,
        z: 0
    }, {
        x: 0,
        y: -1,
        z: 0
    }, {
        x: 0,
        y: 1,
        z: 0
    });
    addStretchTool(grabberBOTTOM, "STRETCH_BOTTOM", {
        x: 0,
        y: 1,
        z: 0
    }, {
        x: 0,
        y: 1,
        z: 0
    }, {
        x: 0,
        y: -1,
        z: 0
    });
    addStretchTool(grabberRIGHT, "STRETCH_RIGHT", {
        x: -1,
        y: 0,
        z: 0
    }, {
        x: -1,
        y: 0,
        z: 0
    }, {
        x: 1,
        y: 0,
        z: 0
    });
    addStretchTool(grabberLEFT, "STRETCH_LEFT", {
        x: 1,
        y: 0,
        z: 0
    }, {
        x: 1,
        y: 0,
        z: 0
    }, {
        x: -1,
        y: 0,
        z: 0
    });

    addStretchTool(grabberSpotLightRadius, "STRETCH_RADIUS", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: 0,
        y: 0,
        z: 1
    }, {
        x: 0,
        y: 0,
        z: -1
    });
    addStretchTool(grabberSpotLightT, "STRETCH_CUTOFF_T", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: 0,
        y: -1,
        z: 0
    }, {
        x: 0,
        y: 1,
        z: 0
    }, cutoffStretchFunc);
    addStretchTool(grabberSpotLightB, "STRETCH_CUTOFF_B", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: 0,
        y: 1,
        z: 0
    }, {
        x: 0,
        y: -1,
        z: 0
    }, cutoffStretchFunc);
    addStretchTool(grabberSpotLightL, "STRETCH_CUTOFF_L", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: 1,
        y: 0,
        z: 0
    }, {
        x: -1,
        y: 0,
        z: 0
    }, cutoffStretchFunc);
    addStretchTool(grabberSpotLightR, "STRETCH_CUTOFF_R", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: -1,
        y: 0,
        z: 0
    }, {
        x: 1,
        y: 0,
        z: 0
    }, cutoffStretchFunc);

    addStretchTool(grabberPointLightT, "STRETCH_RADIUS_T", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: 0,
        y: -1,
        z: 0
    }, {
        x: 0,
        y: 0,
        z: 1
    }, radiusStretchFunc);
    addStretchTool(grabberPointLightB, "STRETCH_RADIUS_B", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: 0,
        y: 1,
        z: 0
    }, {
        x: 0,
        y: 0,
        z: 1
    }, radiusStretchFunc);
    addStretchTool(grabberPointLightL, "STRETCH_RADIUS_L", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: 1,
        y: 0,
        z: 0
    }, {
        x: 0,
        y: 0,
        z: 1
    }, radiusStretchFunc);
    addStretchTool(grabberPointLightR, "STRETCH_RADIUS_R", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: -1,
        y: 0,
        z: 0
    }, {
        x: 0,
        y: 0,
        z: 1
    }, radiusStretchFunc);
    addStretchTool(grabberPointLightF, "STRETCH_RADIUS_F", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: 0,
        y: 0,
        z: -1
    }, {
        x: 0,
        y: 0,
        z: 1
    }, radiusStretchFunc);
    addStretchTool(grabberPointLightN, "STRETCH_RADIUS_N", {
        x: 0,
        y: 0,
        z: 0
    }, {
        x: 0,
        y: 0,
        z: 1
    }, {
        x: 0,
        y: 0,
        z: -1
    }, radiusStretchFunc);

    addStretchTool(grabberLBN, "STRETCH_LBN", null, {
        x: 1,
        y: 0,
        z: 1
    }, {
        x: -1,
        y: -1,
        z: -1
    });
    addStretchTool(grabberRBN, "STRETCH_RBN", null, {
        x: -1,
        y: 0,
        z: 1
    }, {
        x: 1,
        y: -1,
        z: -1
    });
    addStretchTool(grabberLBF, "STRETCH_LBF", null, {
        x: 1,
        y: 0,
        z: -1
    }, {
        x: -1,
        y: -1,
        z: 1
    });
    addStretchTool(grabberRBF, "STRETCH_RBF", null, {
        x: -1,
        y: 0,
        z: -1
    }, {
        x: 1,
        y: -1,
        z: 1
    });
    addStretchTool(grabberLTN, "STRETCH_LTN", null, {
        x: 1,
        y: 0,
        z: 1
    }, {
        x: -1,
        y: 1,
        z: -1
    });
    addStretchTool(grabberRTN, "STRETCH_RTN", null, {
        x: -1,
        y: 0,
        z: 1
    }, {
        x: 1,
        y: 1,
        z: -1
    });
    addStretchTool(grabberLTF, "STRETCH_LTF", null, {
        x: 1,
        y: 0,
        z: -1
    }, {
        x: -1,
        y: 1,
        z: 1
    });
    addStretchTool(grabberRTF, "STRETCH_RTF", null, {
        x: -1,
        y: 0,
        z: -1
    }, {
        x: 1,
        y: 1,
        z: 1
    });

    addStretchTool(grabberEdgeTR, "STRETCH_EdgeTR", null, {
        x: 1,
        y: 1,
        z: 0
    }, {
        x: 1,
        y: 1,
        z: 0
    });
    addStretchTool(grabberEdgeTL, "STRETCH_EdgeTL", null, {
        x: -1,
        y: 1,
        z: 0
    }, {
        x: -1,
        y: 1,
        z: 0
    });
    addStretchTool(grabberEdgeTF, "STRETCH_EdgeTF", null, {
        x: 0,
        y: 1,
        z: -1
    }, {
        x: 0,
        y: 1,
        z: -1
    });
    addStretchTool(grabberEdgeTN, "STRETCH_EdgeTN", null, {
        x: 0,
        y: 1,
        z: 1
    }, {
        x: 0,
        y: 1,
        z: 1
    });
    addStretchTool(grabberEdgeBR, "STRETCH_EdgeBR", null, {
        x: -1,
        y: 0,
        z: 0
    }, {
        x: 1,
        y: -1,
        z: 0
    });
    addStretchTool(grabberEdgeBL, "STRETCH_EdgeBL", null, {
        x: 1,
        y: 0,
        z: 0
    }, {
        x: -1,
        y: -1,
        z: 0
    });
    addStretchTool(grabberEdgeBF, "STRETCH_EdgeBF", null, {
        x: 0,
        y: 0,
        z: -1
    }, {
        x: 0,
        y: -1,
        z: -1
    });
    addStretchTool(grabberEdgeBN, "STRETCH_EdgeBN", null, {
        x: 0,
        y: 0,
        z: 1
    }, {
        x: 0,
        y: -1,
        z: 1
    });
    addStretchTool(grabberEdgeNR, "STRETCH_EdgeNR", null, {
        x: -1,
        y: 0,
        z: 1
    }, {
        x: 1,
        y: 0,
        z: -1
    });
    addStretchTool(grabberEdgeNL, "STRETCH_EdgeNL", null, {
        x: 1,
        y: 0,
        z: 1
    }, {
        x: -1,
        y: 0,
        z: -1
    });
    addStretchTool(grabberEdgeFR, "STRETCH_EdgeFR", null, {
        x: -1,
        y: 0,
        z: -1
    }, {
        x: 1,
        y: 0,
        z: 1
    });
    addStretchTool(grabberEdgeFL, "STRETCH_EdgeFL", null, {
        x: 1,
        y: 0,
        z: -1
    }, {
        x: -1,
        y: 0,
        z: 1
    });

    function updateRotationDegreesOverlay(angleFromZero, handleRotation, centerPosition) {
        var angle = angleFromZero * (Math.PI / 180);
        var position = {
            x: Math.cos(angle) * outerRadius * ROTATION_DISPLAY_DISTANCE_MULTIPLIER,
            y: Math.sin(angle) * outerRadius * ROTATION_DISPLAY_DISTANCE_MULTIPLIER,
            z: 0,
        };
        position = Vec3.multiplyQbyV(handleRotation, position);
        position = Vec3.sum(centerPosition, position);
        Overlays.editOverlay(rotationDegreesDisplay, {
            position: position,
            dimensions: {
                x: innerRadius * ROTATION_DISPLAY_SIZE_X_MULTIPLIER,
                y: innerRadius * ROTATION_DISPLAY_SIZE_Y_MULTIPLIER
            },
            lineHeight: innerRadius * ROTATION_DISPLAY_LINE_HEIGHT_MULTIPLIER,
            text: normalizeDegrees(angleFromZero) + "°",
        });
    }

    var initialPosition = SelectionManager.worldPosition;
    addGrabberTool(yawHandle, {
        mode: "ROTATE_YAW",
        onBegin: function(event) {
            SelectionManager.saveProperties();
            initialPosition = SelectionManager.worldPosition;

            // Size the overlays to the current selection size
            var diagonal = (Vec3.length(selectionManager.worldDimensions) / 2) * 1.1;
            var halfDimensions = Vec3.multiply(selectionManager.worldDimensions, 0.5);
            innerRadius = diagonal;
            outerRadius = diagonal * 1.15;
            var innerAlpha = 0.2;
            var outerAlpha = 0.2;
            Overlays.editOverlay(rotateOverlayInner, {
                visible: true,
                size: innerRadius,
                innerRadius: 0.9,
                startAt: 0,
                endAt: 360,
                alpha: innerAlpha
            });

            Overlays.editOverlay(rotateOverlayOuter, {
                visible: true,
                size: outerRadius,
                innerRadius: 0.9,
                startAt: 0,
                endAt: 360,
                alpha: outerAlpha,
            });

            Overlays.editOverlay(rotateOverlayCurrent, {
                visible: true,
                size: outerRadius,
                startAt: 0,
                endAt: 0,
                innerRadius: 0.9,
            });

            Overlays.editOverlay(rotationDegreesDisplay, {
                visible: true,
            });

            updateRotationDegreesOverlay(0, yawHandleRotation, yawCenter);
        },
        onEnd: function(event, reason) {
            Overlays.editOverlay(rotateOverlayInner, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayOuter, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayCurrent, {
                visible: false
            });
            Overlays.editOverlay(rotationDegreesDisplay, {
                visible: false
            });

            pushCommandForSelections();
        },
        onMove: function(event) {
            var pickRay = generalComputePickRay(event.x, event.y);
            Overlays.editOverlay(selectionBox, {
                ignoreRayIntersection: true,
                visible: false
            });
            Overlays.editOverlay(baseOfEntityProjectionOverlay, {
                ignoreRayIntersection: true,
                visible: false
            });
            Overlays.editOverlay(rotateOverlayTarget, {
                ignoreRayIntersection: false
            });

            var result = Overlays.findRayIntersection(pickRay);

            if (result.intersects) {
                var center = yawCenter;
                var zero = yawZero;
                var centerToZero = Vec3.subtract(center, zero);
                var centerToIntersect = Vec3.subtract(center, result.intersection);
                var angleFromZero = Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal);
                var distanceFromCenter = Vec3.distance(center, result.intersection);
                var snapToInner = distanceFromCenter < innerRadius;
                var snapAngle = snapToInner ? innerSnapAngle : 1.0;
                angleFromZero = Math.floor(angleFromZero / snapAngle) * snapAngle;
                var yawChange = Quat.fromVec3Degrees({
                    x: 0,
                    y: angleFromZero,
                    z: 0
                });

                // Entities should only reposition if we are rotating multiple selections around
                // the selections center point.  Otherwise, the rotation will be around the entities
                // registration point which does not need repositioning.
                var reposition = SelectionManager.selections.length > 1;
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var properties = Entities.getEntityProperties(entityID);
                    var initialProperties = SelectionManager.savedProperties[entityID];

                    var newProperties = {
                        rotation: Quat.multiply(yawChange, initialProperties.rotation),
                    };

                    if (reposition) {
                        var dPos = Vec3.subtract(initialProperties.position, initialPosition);
                        dPos = Vec3.multiplyQbyV(yawChange, dPos);
                        newProperties.position = Vec3.sum(initialPosition, dPos);
                    }

                    Entities.editEntity(entityID, newProperties);
                }

                updateRotationDegreesOverlay(angleFromZero, yawHandleRotation, yawCenter);

                // update the rotation display accordingly...
                var startAtCurrent = 0;
                var endAtCurrent = angleFromZero;
                var startAtRemainder = angleFromZero;
                var endAtRemainder = 360;
                if (angleFromZero < 0) {
                    startAtCurrent = 360 + angleFromZero;
                    endAtCurrent = 360;
                    startAtRemainder = 0;
                    endAtRemainder = startAtCurrent;
                }
                if (snapToInner) {
                    Overlays.editOverlay(rotateOverlayOuter, {
                        startAt: 0,
                        endAt: 360
                    });
                    Overlays.editOverlay(rotateOverlayInner, {
                        startAt: startAtRemainder,
                        endAt: endAtRemainder
                    });
                    Overlays.editOverlay(rotateOverlayCurrent, {
                        startAt: startAtCurrent,
                        endAt: endAtCurrent,
                        size: innerRadius,
                        majorTickMarksAngle: innerSnapAngle,
                        minorTickMarksAngle: 0,
                        majorTickMarksLength: -0.25,
                        minorTickMarksLength: 0,
                    });
                } else {
                    Overlays.editOverlay(rotateOverlayInner, {
                        startAt: 0,
                        endAt: 360
                    });
                    Overlays.editOverlay(rotateOverlayOuter, {
                        startAt: startAtRemainder,
                        endAt: endAtRemainder
                    });
                    Overlays.editOverlay(rotateOverlayCurrent, {
                        startAt: startAtCurrent,
                        endAt: endAtCurrent,
                        size: outerRadius,
                        majorTickMarksAngle: 45.0,
                        minorTickMarksAngle: 5,
                        majorTickMarksLength: 0.25,
                        minorTickMarksLength: 0.1,
                    });
                }

            }
        }
    });

    addGrabberTool(pitchHandle, {
        mode: "ROTATE_PITCH",
        onBegin: function(event) {
            SelectionManager.saveProperties();
            initialPosition = SelectionManager.worldPosition;

            // Size the overlays to the current selection size
            var diagonal = (Vec3.length(selectionManager.worldDimensions) / 2) * 1.1;
            var halfDimensions = Vec3.multiply(selectionManager.worldDimensions, 0.5);
            innerRadius = diagonal;
            outerRadius = diagonal * 1.15;
            var innerAlpha = 0.2;
            var outerAlpha = 0.2;
            Overlays.editOverlay(rotateOverlayInner, {
                visible: true,
                size: innerRadius,
                innerRadius: 0.9,
                startAt: 0,
                endAt: 360,
                alpha: innerAlpha
            });

            Overlays.editOverlay(rotateOverlayOuter, {
                visible: true,
                size: outerRadius,
                innerRadius: 0.9,
                startAt: 0,
                endAt: 360,
                alpha: outerAlpha,
            });

            Overlays.editOverlay(rotateOverlayCurrent, {
                visible: true,
                size: outerRadius,
                startAt: 0,
                endAt: 0,
                innerRadius: 0.9,
            });

            Overlays.editOverlay(rotationDegreesDisplay, {
                visible: true,
            });

            updateRotationDegreesOverlay(0, pitchHandleRotation, pitchCenter);
        },
        onEnd: function(event, reason) {
            Overlays.editOverlay(rotateOverlayInner, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayOuter, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayCurrent, {
                visible: false
            });
            Overlays.editOverlay(rotationDegreesDisplay, {
                visible: false
            });

            pushCommandForSelections();
        },
        onMove: function(event) {
            var pickRay = generalComputePickRay(event.x, event.y);
            Overlays.editOverlay(selectionBox, {
                ignoreRayIntersection: true,
                visible: false
            });
            Overlays.editOverlay(baseOfEntityProjectionOverlay, {
                ignoreRayIntersection: true,
                visible: false
            });
            Overlays.editOverlay(rotateOverlayTarget, {
                ignoreRayIntersection: false
            });
            var result = Overlays.findRayIntersection(pickRay);

            if (result.intersects) {
                var properties = Entities.getEntityProperties(selectionManager.selections[0]);
                var center = pitchCenter;
                var zero = pitchZero;
                var centerToZero = Vec3.subtract(center, zero);
                var centerToIntersect = Vec3.subtract(center, result.intersection);
                var angleFromZero = Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal);

                var distanceFromCenter = Vec3.distance(center, result.intersection);
                var snapToInner = distanceFromCenter < innerRadius;
                var snapAngle = snapToInner ? innerSnapAngle : 1.0;
                angleFromZero = Math.floor(angleFromZero / snapAngle) * snapAngle;

                var pitchChange = Quat.fromVec3Degrees({
                    x: angleFromZero,
                    y: 0,
                    z: 0
                });

                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var properties = Entities.getEntityProperties(entityID);
                    var initialProperties = SelectionManager.savedProperties[entityID];
                    var dPos = Vec3.subtract(initialProperties.position, initialPosition);
                    dPos = Vec3.multiplyQbyV(pitchChange, dPos);

                    Entities.editEntity(entityID, {
                        position: Vec3.sum(initialPosition, dPos),
                        rotation: Quat.multiply(pitchChange, initialProperties.rotation),
                    });
                }

                updateRotationDegreesOverlay(angleFromZero, pitchHandleRotation, pitchCenter);

                // update the rotation display accordingly...
                var startAtCurrent = 0;
                var endAtCurrent = angleFromZero;
                var startAtRemainder = angleFromZero;
                var endAtRemainder = 360;
                if (angleFromZero < 0) {
                    startAtCurrent = 360 + angleFromZero;
                    endAtCurrent = 360;
                    startAtRemainder = 0;
                    endAtRemainder = startAtCurrent;
                }
                if (snapToInner) {
                    Overlays.editOverlay(rotateOverlayOuter, {
                        startAt: 0,
                        endAt: 360
                    });
                    Overlays.editOverlay(rotateOverlayInner, {
                        startAt: startAtRemainder,
                        endAt: endAtRemainder
                    });
                    Overlays.editOverlay(rotateOverlayCurrent, {
                        startAt: startAtCurrent,
                        endAt: endAtCurrent,
                        size: innerRadius,
                        majorTickMarksAngle: innerSnapAngle,
                        minorTickMarksAngle: 0,
                        majorTickMarksLength: -0.25,
                        minorTickMarksLength: 0,
                    });
                } else {
                    Overlays.editOverlay(rotateOverlayInner, {
                        startAt: 0,
                        endAt: 360
                    });
                    Overlays.editOverlay(rotateOverlayOuter, {
                        startAt: startAtRemainder,
                        endAt: endAtRemainder
                    });
                    Overlays.editOverlay(rotateOverlayCurrent, {
                        startAt: startAtCurrent,
                        endAt: endAtCurrent,
                        size: outerRadius,
                        majorTickMarksAngle: 45.0,
                        minorTickMarksAngle: 5,
                        majorTickMarksLength: 0.25,
                        minorTickMarksLength: 0.1,
                    });
                }
            }
        }
    });

    addGrabberTool(rollHandle, {
        mode: "ROTATE_ROLL",
        onBegin: function(event) {
            SelectionManager.saveProperties();
            initialPosition = SelectionManager.worldPosition;

            // Size the overlays to the current selection size
            var diagonal = (Vec3.length(selectionManager.worldDimensions) / 2) * 1.1;
            var halfDimensions = Vec3.multiply(selectionManager.worldDimensions, 0.5);
            innerRadius = diagonal;
            outerRadius = diagonal * 1.15;
            var innerAlpha = 0.2;
            var outerAlpha = 0.2;
            Overlays.editOverlay(rotateOverlayInner, {
                visible: true,
                size: innerRadius,
                innerRadius: 0.9,
                startAt: 0,
                endAt: 360,
                alpha: innerAlpha
            });

            Overlays.editOverlay(rotateOverlayOuter, {
                visible: true,
                size: outerRadius,
                innerRadius: 0.9,
                startAt: 0,
                endAt: 360,
                alpha: outerAlpha,
            });

            Overlays.editOverlay(rotateOverlayCurrent, {
                visible: true,
                size: outerRadius,
                startAt: 0,
                endAt: 0,
                innerRadius: 0.9,
            });

            Overlays.editOverlay(rotationDegreesDisplay, {
                visible: true,
            });

            updateRotationDegreesOverlay(0, rollHandleRotation, rollCenter);
        },
        onEnd: function(event, reason) {
            Overlays.editOverlay(rotateOverlayInner, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayOuter, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayCurrent, {
                visible: false
            });
            Overlays.editOverlay(rotationDegreesDisplay, {
                visible: false
            });

            pushCommandForSelections();
        },
        onMove: function(event) {
            var pickRay = generalComputePickRay(event.x, event.y);
            Overlays.editOverlay(selectionBox, {
                ignoreRayIntersection: true,
                visible: false
            });
            Overlays.editOverlay(baseOfEntityProjectionOverlay, {
                ignoreRayIntersection: true,
                visible: false
            });
            Overlays.editOverlay(rotateOverlayTarget, {
                ignoreRayIntersection: false
            });
            var result = Overlays.findRayIntersection(pickRay);

            if (result.intersects) {
                var properties = Entities.getEntityProperties(selectionManager.selections[0]);
                var center = rollCenter;
                var zero = rollZero;
                var centerToZero = Vec3.subtract(center, zero);
                var centerToIntersect = Vec3.subtract(center, result.intersection);
                var angleFromZero = Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal);

                var distanceFromCenter = Vec3.distance(center, result.intersection);
                var snapToInner = distanceFromCenter < innerRadius;
                var snapAngle = snapToInner ? innerSnapAngle : 1.0;
                angleFromZero = Math.floor(angleFromZero / snapAngle) * snapAngle;

                var rollChange = Quat.fromVec3Degrees({
                    x: 0,
                    y: 0,
                    z: angleFromZero
                });
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var properties = Entities.getEntityProperties(entityID);
                    var initialProperties = SelectionManager.savedProperties[entityID];
                    var dPos = Vec3.subtract(initialProperties.position, initialPosition);
                    dPos = Vec3.multiplyQbyV(rollChange, dPos);

                    Entities.editEntity(entityID, {
                        position: Vec3.sum(initialPosition, dPos),
                        rotation: Quat.multiply(rollChange, initialProperties.rotation),
                    });
                }

                updateRotationDegreesOverlay(angleFromZero, rollHandleRotation, rollCenter);

                // update the rotation display accordingly...
                var startAtCurrent = 0;
                var endAtCurrent = angleFromZero;
                var startAtRemainder = angleFromZero;
                var endAtRemainder = 360;
                if (angleFromZero < 0) {
                    startAtCurrent = 360 + angleFromZero;
                    endAtCurrent = 360;
                    startAtRemainder = 0;
                    endAtRemainder = startAtCurrent;
                }
                if (snapToInner) {
                    Overlays.editOverlay(rotateOverlayOuter, {
                        startAt: 0,
                        endAt: 360
                    });
                    Overlays.editOverlay(rotateOverlayInner, {
                        startAt: startAtRemainder,
                        endAt: endAtRemainder
                    });
                    Overlays.editOverlay(rotateOverlayCurrent, {
                        startAt: startAtCurrent,
                        endAt: endAtCurrent,
                        size: innerRadius,
                        majorTickMarksAngle: innerSnapAngle,
                        minorTickMarksAngle: 0,
                        majorTickMarksLength: -0.25,
                        minorTickMarksLength: 0,
                    });
                } else {
                    Overlays.editOverlay(rotateOverlayInner, {
                        startAt: 0,
                        endAt: 360
                    });
                    Overlays.editOverlay(rotateOverlayOuter, {
                        startAt: startAtRemainder,
                        endAt: endAtRemainder
                    });
                    Overlays.editOverlay(rotateOverlayCurrent, {
                        startAt: startAtCurrent,
                        endAt: endAtCurrent,
                        size: outerRadius,
                        majorTickMarksAngle: 45.0,
                        minorTickMarksAngle: 5,
                        majorTickMarksLength: 0.25,
                        minorTickMarksLength: 0.1,
                    });
                }
            }
        }
    });

    that.checkMove = function() {
        if (SelectionManager.hasSelection()) {

            // FIXME - this cause problems with editing in the entity properties window
            //SelectionManager._update();

            if (!Vec3.equal(Camera.getPosition(), lastCameraPosition) ||
                !Quat.equal(Camera.getOrientation(), lastCameraOrientation)) {

                that.updateRotationHandles();
            }
        }
    };

    that.mousePressEvent = function(event) {
        var wantDebug = false; 
        if (!event.isLeftButton && !that.triggered) {
            // if another mouse button than left is pressed ignore it
            return false;
        }

        var somethingClicked = false;
        var pickRay = generalComputePickRay(event.x, event.y);

        // before we do a ray test for grabbers, disable the ray intersection for our selection box
        Overlays.editOverlay(selectionBox, {
            ignoreRayIntersection: true
        });
        Overlays.editOverlay(yawHandle, {
            ignoreRayIntersection: true
        });
        Overlays.editOverlay(pitchHandle, {
            ignoreRayIntersection: true
        });
        Overlays.editOverlay(rollHandle, {
            ignoreRayIntersection: true
        });
        var result = Overlays.findRayIntersection(pickRay);

        if (result.intersects) {

            
            if (wantDebug) {
                print("something intersects... ");
                print("   result.overlayID:" + result.overlayID + "[" + overlayNames[result.overlayID] + "]");
                print("   result.intersects:" + result.intersects);
                print("   result.overlayID:" + result.overlayID);
                print("   result.distance:" + result.distance);
                print("   result.face:" + result.face);
                Vec3.print("   result.intersection:", result.intersection);
            }

            var tool = grabberTools[result.overlayID];
            if (tool) {
                activeTool = tool;
                mode = tool.mode;
                somethingClicked = 'tool';
                if (activeTool && activeTool.onBegin) {
                    activeTool.onBegin(event);
                }
            } else {
                switch (result.overlayID) {
                    case grabberMoveUp:
                        mode = "TRANSLATE_UP_DOWN";
                        somethingClicked = mode;

                        // in translate mode, we hide our stretch handles...
                        for (var i = 0; i < stretchHandles.length; i++) {
                            Overlays.editOverlay(stretchHandles[i], {
                                visible: false
                            });
                        }
                        break;


                    case grabberNEAR:
                    case grabberEdgeTN: // TODO: maybe this should be TOP+NEAR stretching?
                    case grabberEdgeBN: // TODO: maybe this should be BOTTOM+FAR stretching?
                        mode = "STRETCH_NEAR";
                        somethingClicked = mode;
                        break;

                    case grabberFAR:
                    case grabberEdgeTF: // TODO: maybe this should be TOP+FAR stretching?
                    case grabberEdgeBF: // TODO: maybe this should be BOTTOM+FAR stretching?
                        mode = "STRETCH_FAR";
                        somethingClicked = mode;
                        break;
                    case grabberTOP:
                        mode = "STRETCH_TOP";
                        somethingClicked = mode;
                        break;
                    case grabberBOTTOM:
                        mode = "STRETCH_BOTTOM";
                        somethingClicked = mode;
                        break;
                    case grabberRIGHT:
                    case grabberEdgeTR: // TODO: maybe this should be TOP+RIGHT stretching?
                    case grabberEdgeBR: // TODO: maybe this should be BOTTOM+RIGHT stretching?
                        mode = "STRETCH_RIGHT";
                        somethingClicked = mode;
                        break;
                    case grabberLEFT:
                    case grabberEdgeTL: // TODO: maybe this should be TOP+LEFT stretching?
                    case grabberEdgeBL: // TODO: maybe this should be BOTTOM+LEFT stretching?
                        mode = "STRETCH_LEFT";
                        somethingClicked = mode;
                        break;

                    default:
                        mode = "UNKNOWN";
                        break;
                }
            }
        }

        // if one of the items above was clicked, then we know we are in translate or stretch mode, and we
        // should hide our rotate handles...
        if (somethingClicked) {
            Overlays.editOverlay(yawHandle, {
                visible: false
            });
            Overlays.editOverlay(pitchHandle, {
                visible: false
            });
            Overlays.editOverlay(rollHandle, {
                visible: false
            });

            if (mode != "TRANSLATE_UP_DOWN") {
                Overlays.editOverlay(grabberMoveUp, {
                    visible: false
                });
            }
        }

        if (!somethingClicked) {

            if (wantDebug) {
                print("rotate handle case...");
            }
            

            // After testing our stretch handles, then check out rotate handles
            Overlays.editOverlay(yawHandle, {
                ignoreRayIntersection: false
            });
            Overlays.editOverlay(pitchHandle, {
                ignoreRayIntersection: false
            });
            Overlays.editOverlay(rollHandle, {
                ignoreRayIntersection: false
            });
            var result = Overlays.findRayIntersection(pickRay);

            var overlayOrientation;
            var overlayCenter;

            var properties = Entities.getEntityProperties(selectionManager.selections[0]);
            var angles = Quat.safeEulerAngles(properties.rotation);
            var pitch = angles.x;
            var yaw = angles.y;
            var roll = angles.z;

            originalRotation = properties.rotation;
            originalPitch = pitch;
            originalYaw = yaw;
            originalRoll = roll;

            if (result.intersects) {
                var tool = grabberTools[result.overlayID];
                if (tool) {
                    activeTool = tool;
                    mode = tool.mode;
                    somethingClicked = 'tool';
                    if (activeTool && activeTool.onBegin) {
                        activeTool.onBegin(event);
                    }
                }
                switch (result.overlayID) {
                    case yawHandle:
                        mode = "ROTATE_YAW";
                        somethingClicked = mode;
                        overlayOrientation = yawHandleRotation;
                        overlayCenter = yawCenter;
                        yawZero = result.intersection;
                        rotationNormal = yawNormal;
                        break;

                    case pitchHandle:
                        mode = "ROTATE_PITCH";
                        initialPosition = SelectionManager.worldPosition;
                        somethingClicked = mode;
                        overlayOrientation = pitchHandleRotation;
                        overlayCenter = pitchCenter;
                        pitchZero = result.intersection;
                        rotationNormal = pitchNormal;
                        break;

                    case rollHandle:
                        mode = "ROTATE_ROLL";
                        somethingClicked = mode;
                        overlayOrientation = rollHandleRotation;
                        overlayCenter = rollCenter;
                        rollZero = result.intersection;
                        rotationNormal = rollNormal;
                        break;

                    default:
                        if (wantDebug) {
                            print("mousePressEvent()...... " + overlayNames[result.overlayID]);
                        }
                        mode = "UNKNOWN";
                        break;
                }
            }
            if (wantDebug) {
                print("    somethingClicked:" + somethingClicked);
                print("                mode:" + mode);
            }

            if (somethingClicked) {

                Overlays.editOverlay(rotateOverlayTarget, {
                    visible: true,
                    rotation: overlayOrientation,
                    position: overlayCenter
                });
                Overlays.editOverlay(rotateOverlayInner, {
                    visible: true,
                    rotation: overlayOrientation,
                    position: overlayCenter
                });
                Overlays.editOverlay(rotateOverlayOuter, {
                    visible: true,
                    rotation: overlayOrientation,
                    position: overlayCenter,
                    startAt: 0,
                    endAt: 360
                });
                Overlays.editOverlay(rotateOverlayCurrent, {
                    visible: true,
                    rotation: overlayOrientation,
                    position: overlayCenter,
                    startAt: 0,
                    endAt: 0
                });
                Overlays.editOverlay(yawHandle, {
                    visible: false
                });
                Overlays.editOverlay(pitchHandle, {
                    visible: false
                });
                Overlays.editOverlay(rollHandle, {
                    visible: false
                });


                Overlays.editOverlay(yawHandle, {
                    visible: false
                });
                Overlays.editOverlay(pitchHandle, {
                    visible: false
                });
                Overlays.editOverlay(rollHandle, {
                    visible: false
                });
                Overlays.editOverlay(grabberMoveUp, {
                    visible: false
                });
                Overlays.editOverlay(grabberLBN, {
                    visible: false
                });
                Overlays.editOverlay(grabberLBF, {
                    visible: false
                });
                Overlays.editOverlay(grabberRBN, {
                    visible: false
                });
                Overlays.editOverlay(grabberRBF, {
                    visible: false
                });
                Overlays.editOverlay(grabberLTN, {
                    visible: false
                });
                Overlays.editOverlay(grabberLTF, {
                    visible: false
                });
                Overlays.editOverlay(grabberRTN, {
                    visible: false
                });
                Overlays.editOverlay(grabberRTF, {
                    visible: false
                });

                Overlays.editOverlay(grabberTOP, {
                    visible: false
                });
                Overlays.editOverlay(grabberBOTTOM, {
                    visible: false
                });
                Overlays.editOverlay(grabberLEFT, {
                    visible: false
                });
                Overlays.editOverlay(grabberRIGHT, {
                    visible: false
                });
                Overlays.editOverlay(grabberNEAR, {
                    visible: false
                });
                Overlays.editOverlay(grabberFAR, {
                    visible: false
                });

                Overlays.editOverlay(grabberEdgeTR, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeTL, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeTF, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeTN, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeBR, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeBL, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeBF, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeBN, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeNR, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeNL, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeFR, {
                    visible: false
                });
                Overlays.editOverlay(grabberEdgeFL, {
                    visible: false
                });
            }
        }

        if (!somethingClicked) {
            Overlays.editOverlay(selectionBox, {
                ignoreRayIntersection: false
            });
            var result = Overlays.findRayIntersection(pickRay);
            if (result.intersects) {
                switch (result.overlayID) {
                    case selectionBox:
                        activeTool = translateXZTool;
                        translateXZTool.pickPlanePosition = result.intersection;
                        translateXZTool.greatestDimension = Math.max(Math.max(SelectionManager.worldDimensions.x, SelectionManager.worldDimensions.y), 
                            SelectionManager.worldDimensions.z);
                        if (wantDebug) {
                            print("longest dimension: " + translateXZTool.greatestDimension);
                            translateXZTool.startingDistance = Vec3.distance(pickRay.origin, SelectionManager.position);
                            print("starting distance: " + translateXZTool.startingDistance);
                            translateXZTool.startingElevation = translateXZTool.elevation(pickRay.origin, translateXZTool.pickPlanePosition);
                            print(" starting elevation: " + translateXZTool.startingElevation);
                        }
                        
                        mode = translateXZTool.mode;
                        activeTool.onBegin(event);
                        somethingClicked = 'selectionBox';
                        break;
                    default:
                        if (wantDebug) {
                            print("mousePressEvent()...... " + overlayNames[result.overlayID]);
                        }
                        mode = "UNKNOWN";
                        break;
                }
            }
        }

        if (somethingClicked) {
            pickRay = generalComputePickRay(event.x, event.y);
            if (wantDebug) {
                print("mousePressEvent()...... " + overlayNames[result.overlayID]);
            }
        }

        // reset everything as intersectable...
        // TODO: we could optimize this since some of these were already flipped back
        Overlays.editOverlay(selectionBox, {
            ignoreRayIntersection: false
        });
        Overlays.editOverlay(yawHandle, {
            ignoreRayIntersection: false
        });
        Overlays.editOverlay(pitchHandle, {
            ignoreRayIntersection: false
        });
        Overlays.editOverlay(rollHandle, {
            ignoreRayIntersection: false
        });

        return somethingClicked;
    };

    that.mouseMoveEvent = function(event) {
        if (activeTool) {
            activeTool.onMove(event);
            SelectionManager._update();
            return true;
        }

        // if no tool is active, then just look for handles to highlight...
        var pickRay = generalComputePickRay(event.x, event.y);
        var result = Overlays.findRayIntersection(pickRay);
        var pickedColor;
        var pickedAlpha;
        var highlightNeeded = false;

        if (result.intersects) {
            switch (result.overlayID) {
                case yawHandle:
                case pitchHandle:
                case rollHandle:
                    pickedColor = handleColor;
                    pickedAlpha = handleAlpha;
                    highlightNeeded = true;
                    break;

                case grabberMoveUp:
                    pickedColor = handleColor;
                    pickedAlpha = handleAlpha;
                    highlightNeeded = true;
                    break;

                case grabberLBN:
                case grabberLBF:
                case grabberRBN:
                case grabberRBF:
                case grabberLTN:
                case grabberLTF:
                case grabberRTN:
                case grabberRTF:
                    pickedColor = grabberColorCorner;
                    pickedAlpha = grabberAlpha;
                    highlightNeeded = true;
                    break;

                case grabberTOP:
                case grabberBOTTOM:
                case grabberLEFT:
                case grabberRIGHT:
                case grabberNEAR:
                case grabberFAR:
                    pickedColor = grabberColorFace;
                    pickedAlpha = grabberAlpha;
                    highlightNeeded = true;
                    break;

                case grabberEdgeTR:
                case grabberEdgeTL:
                case grabberEdgeTF:
                case grabberEdgeTN:
                case grabberEdgeBR:
                case grabberEdgeBL:
                case grabberEdgeBF:
                case grabberEdgeBN:
                case grabberEdgeNR:
                case grabberEdgeNL:
                case grabberEdgeFR:
                case grabberEdgeFL:
                case grabberSpotLightRadius:
                case grabberSpotLightT:
                case grabberSpotLightB:
                case grabberSpotLightL:
                case grabberSpotLightR:
                case grabberPointLightT:
                case grabberPointLightB:
                case grabberPointLightR:
                case grabberPointLightL:
                case grabberPointLightN:
                case grabberPointLightF:
                    pickedColor = grabberColorEdge;
                    pickedAlpha = grabberAlpha;
                    highlightNeeded = true;
                    break;

                default:
                    if (previousHandle) {
                        Overlays.editOverlay(previousHandle, {
                            color: previousHandleColor,
                            alpha: previousHandleAlpha
                        });
                        previousHandle = false;
                    }
                    break;
            }

            if (highlightNeeded) {
                if (previousHandle) {
                    Overlays.editOverlay(previousHandle, {
                        color: previousHandleColor,
                        alpha: previousHandleAlpha
                    });
                    previousHandle = false;
                }
                Overlays.editOverlay(result.overlayID, {
                    color: highlightedHandleColor,
                    alpha: highlightedHandleAlpha
                });
                previousHandle = result.overlayID;
                previousHandleColor = pickedColor;
                previousHandleAlpha = pickedAlpha;
            }

        } else {
            if (previousHandle) {
                Overlays.editOverlay(previousHandle, {
                    color: previousHandleColor,
                    alpha: previousHandleAlpha
                });
                previousHandle = false;
            }
        }

        return false;
    };


    that.updateHandleSizes = function() {
        if (selectionManager.hasSelection()) {
            var diff = Vec3.subtract(selectionManager.worldPosition, Camera.getPosition());
            var grabberSize = Vec3.length(diff) * GRABBER_DISTANCE_TO_SIZE_RATIO * 5;
            var dimensions = SelectionManager.worldDimensions;
            var avgDimension = (dimensions.x + dimensions.y + dimensions.z) / 3;
            grabberSize = Math.min(grabberSize, avgDimension / 10);

            for (var i = 0; i < stretchHandles.length; i++) {
                Overlays.editOverlay(stretchHandles[i], {
                    size: grabberSize,
                });
            }
            var handleSize = Vec3.length(diff) * GRABBER_DISTANCE_TO_SIZE_RATIO * 7;
            handleSize = Math.min(handleSize, avgDimension / 3);

            Overlays.editOverlay(yawHandle, {
                scale: handleSize,
            });
            Overlays.editOverlay(pitchHandle, {
                scale: handleSize,
            });
            Overlays.editOverlay(rollHandle, {
                scale: handleSize,
            });
            var pos = Vec3.sum(grabberMoveUpPosition, {
                x: 0,
                y: Vec3.length(diff) * GRABBER_DISTANCE_TO_SIZE_RATIO * 3,
                z: 0
            });
            Overlays.editOverlay(grabberMoveUp, {
                position: pos,
                scale: handleSize / 1.25,
            });
        }
    }
    Script.update.connect(that.updateHandleSizes);

    that.mouseReleaseEvent = function(event) {
        var showHandles = false;
        if (activeTool && activeTool.onEnd) {
            activeTool.onEnd(event);
        }
        activeTool = null;
        // hide our rotation overlays..., and show our handles
        if (mode == "ROTATE_YAW" || mode == "ROTATE_PITCH" || mode == "ROTATE_ROLL") {
            Overlays.editOverlay(rotateOverlayTarget, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayInner, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayOuter, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayCurrent, {
                visible: false
            });
            showHandles = true;
        }

        if (mode != "UNKNOWN") {
            showHandles = true;
        }

        mode = "UNKNOWN";

        // if something is selected, then reset the "original" properties for any potential next click+move operation
        if (SelectionManager.hasSelection()) {
            if (showHandles) {
                that.select(SelectionManager.selections[0], event);
            }
        }

    };

    // NOTE: mousePressEvent and mouseMoveEvent from the main script should call us., so we don't hook these:
    //       Controller.mousePressEvent.connect(that.mousePressEvent);
    //       Controller.mouseMoveEvent.connect(that.mouseMoveEvent);
    Controller.mouseReleaseEvent.connect(that.mouseReleaseEvent);



    return that;

}());
