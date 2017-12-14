//
//  entitySelectionToolClass.js
//  examples
//
//  Created by Brad hefta-Gaub on 10/1/14.
//    Modified by Daniela Fontes * @DanielaFifo and Tiago Andrade @TagoWill on 4/7/2017
//  Copyright 2014 High Fidelity, Inc.
//
//  This script implements a class useful for building tools for editing entities.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global HIFI_PUBLIC_BUCKET, SPACE_LOCAL, Script, SelectionManager */

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

    // FUNCTION: SUBSCRIBE TO UPDATE MESSAGES
    function subscribeToUpdateMessages() {
        Messages.subscribe("entityToolUpdates");
        Messages.messageReceived.connect(handleEntitySelectionToolUpdates);
    }

    // FUNCTION: HANDLE ENTITY SELECTION TOOL UDPATES
    function handleEntitySelectionToolUpdates(channel, message, sender) {
        if (channel !== 'entityToolUpdates') {
            return;
        }
        if (sender !== MyAvatar.sessionUUID) {
            return;
        }

        var wantDebug = false;
        var messageParsed;
        try {
            messageParsed = JSON.parse(message);
        } catch (err) {
            print("ERROR: entitySelectionTool.handleEntitySelectionToolUpdates - got malformed message: " + message);
        }

        // if (message === 'callUpdate') {
        //     that._update();
        // }

        if (messageParsed.method === "selectEntity") {
            if (wantDebug) {
                print("setting selection to " + messageParsed.entityID);
            }
            that.setSelections([messageParsed.entityID]);
        }
    }

    subscribeToUpdateMessages();

    that.savedProperties = {};
    that.selections = [];
    var listeners = [];

    that.localRotation = Quat.IDENTITY;
    that.localPosition = Vec3.ZERO;
    that.localDimensions = Vec3.ZERO;
    that.localRegistrationPoint = Vec3.HALF;

    that.worldRotation = Quat.IDENTITY;
    that.worldPosition = Vec3.ZERO;
    that.worldDimensions = Vec3.ZERO;
    that.worldRegistrationPoint = Vec3.HALF;
    that.centerPosition = Vec3.ZERO;

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

        that._update(true);
    };

    that.addEntity = function(entityID, toggleSelection) {
        if (entityID) {
            var idx = -1;
            for (var i = 0; i < that.selections.length; i++) {
                if (entityID === that.selections[i]) {
                    idx = i;
                    break;
                }
            }
            if (idx === -1) {
                that.selections.push(entityID);
            } else if (toggleSelection) {
                that.selections.splice(idx, 1);
            }
        }

        that._update(true);
    };

    that.removeEntity = function(entityID) {
        var idx = that.selections.indexOf(entityID);
        if (idx >= 0) {
            that.selections.splice(idx, 1);
        }
        that._update(true);
    };

    that.clearSelections = function() {
        that.selections = [];
        that._update(true);
    };

    that._update = function(selectionUpdated) {
        var properties = null;
        if (that.selections.length === 0) {
            that.localDimensions = null;
            that.localPosition = null;
            that.worldDimensions = null;
            that.worldPosition = null;
        } else if (that.selections.length === 1) {
            properties = Entities.getEntityProperties(that.selections[0]);
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

            properties = Entities.getEntityProperties(that.selections[0]);

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
                z: brn.z + (that.worldDimensions.z / 2)
            };

            // For 1+ selections we can only modify selections in world space
            SelectionDisplay.setSpaceMode(SPACE_WORLD);
        }

        for (var j = 0; j < listeners.length; j++) {
            try {
                listeners[j](selectionUpdated === true);
            } catch (e) {
                print("ERROR: entitySelectionTool.update got exception: " + JSON.stringify(e));
            }
        }

    };

    return that;
})();

// Normalize degrees to be in the range (-180, 180]
function normalizeDegrees(degrees) {
    degrees = ((degrees + 180) % 360) - 180;
    if (degrees <= -180) {
        degrees += 360;
    }

    return degrees;
}

// FUNCTION: getRelativeCenterPosition
// Return the enter position of an entity relative to it's registrationPoint
// A registration point of (0.5, 0.5, 0.5) will have an offset of (0, 0, 0)
// A registration point of (1.0, 1.0, 1.0) will have an offset of (-dimensions.x / 2, -dimensions.y / 2, -dimensions.z / 2)
function getRelativeCenterPosition(dimensions, registrationPoint) {
    return {
        x: -dimensions.x * (registrationPoint.x - 0.5),
        y: -dimensions.y * (registrationPoint.y - 0.5),
        z: -dimensions.z * (registrationPoint.z - 0.5)
    };
}

// SELECTION DISPLAY DEFINITION
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
    var overlayNames = [];
    var lastCameraPosition = Camera.getPosition();
    var lastCameraOrientation = Camera.getOrientation();
    var lastControllerPoses = [
        getControllerWorldLocation(Controller.Standard.LeftHand, true),
        getControllerWorldLocation(Controller.Standard.RightHand, true)
    ];

    var handleHoverColor = {
        red: 224,
        green: 67,
        blue: 36
    };
    var handleHoverAlpha = 1.0;

    var innerSnapAngle = 22.5; // the angle which we snap to on the inner rotation tool
    var innerRadius;
    var outerRadius;
    var yawHandleRotation;
    var pitchHandleRotation;
    var rollHandleRotation;
    var yawCenter;
    var pitchCenter;
    var rollCenter;
    var rotZero;
    var rotationNormal;


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
    var grabberColorCloner = {
        red: 0,
        green: 155,
        blue: 0
    };
    var grabberLineWidth = 0.5;
    var grabberSolid = true;
    var grabberMoveUpPosition = Vec3.ZERO;

    var lightOverlayColor = {
        red: 255,
        green: 153,
        blue: 0
    };

    var grabberPropertiesCorner = {
        position: Vec3.ZERO,
        size: grabberSizeCorner,
        color: grabberColorCorner,
        alpha: 1,
        solid: grabberSolid,
        visible: false,
        dashed: false,
        drawInFront: true,
        borderSize: 1.4
    };

    var grabberPropertiesEdge = {
        position: Vec3.ZERO,
        size: grabberSizeEdge,
        color: grabberColorEdge,
        alpha: 1,
        solid: grabberSolid,
        visible: false,
        dashed: false,
        drawInFront: true,
        borderSize: 1.4
    };

    var grabberPropertiesFace = {
        position: Vec3.ZERO,
        size: grabberSizeFace,
        color: grabberColorFace,
        alpha: 1,
        solid: grabberSolid,
        visible: false,
        dashed: false,
        drawInFront: true,
        borderSize: 1.4
    };

    var grabberPropertiesCloner = {
        position: Vec3.ZERO,
        size: grabberSizeCorner,
        color: grabberColorCloner,
        alpha: 1,
        solid: grabberSolid,
        visible: false,
        dashed: false,
        drawInFront: true,
        borderSize: 1.4
    };

    var spotLightLineProperties = {
        color: lightOverlayColor
    };

    var highlightBox = Overlays.addOverlay("cube", {
        position: Vec3.ZERO,
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
        ignoreRayIntersection: true, // this never ray intersects
        drawInFront: true
    });

    var selectionBox = Overlays.addOverlay("cube", {
        position: Vec3.ZERO,
        size: 1,
        color: {
            red: 255,
            green: 0,
            blue: 0
        },
        alpha: 1,
        solid: false,
        visible: false,
        dashed: false
    });

    var selectionBoxes = [];

    var rotationDegreesDisplay = Overlays.addOverlay("text3d", {
        position: Vec3.ZERO,
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
        leftMargin: 0
    });

    var grabberMoveUp = Overlays.addOverlay("image3d", {
        url: HIFI_PUBLIC_BUCKET + "images/up-arrow.svg",
        position: Vec3.ZERO,
        color: handleColor,
        alpha: handleAlpha,
        visible: false,
        size: 0.1,
        scale: 0.1,
        isFacingAvatar: true,
        drawInFront: true
    });

    // var normalLine = Overlays.addOverlay("line3d", {
    //                 visible: true,
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

    var spotLightGrabberHandles = [
        grabberSpotLightCircle, grabberSpotLightCenter, grabberSpotLightRadius,
        grabberSpotLightLineT, grabberSpotLightLineB, grabberSpotLightLineL, grabberSpotLightLineR,
        grabberSpotLightT, grabberSpotLightB, grabberSpotLightL, grabberSpotLightR
    ];

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

    var pointLightGrabberHandles = [
        grabberPointLightCircleX, grabberPointLightCircleY, grabberPointLightCircleZ,
        grabberPointLightT, grabberPointLightB, grabberPointLightL,
        grabberPointLightR, grabberPointLightF, grabberPointLightN
    ];

    var grabberCloner = Overlays.addOverlay("cube", grabberPropertiesCloner);

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

        grabberCloner
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
        ignoreRayIntersection: true // always ignore this
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
        start: Vec3.ZERO,
        end: Vec3.ZERO,
        color: {
            red: 255,
            green: 0,
            blue: 0
        },
        ignoreRayIntersection: true // always ignore this
    });
    var yRailOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        start: Vec3.ZERO,
        end: Vec3.ZERO,
        color: {
            red: 0,
            green: 255,
            blue: 0
        },
        ignoreRayIntersection: true // always ignore this
    });
    var zRailOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        start: Vec3.ZERO,
        end: Vec3.ZERO,
        color: {
            red: 0,
            green: 0,
            blue: 255
        },
        ignoreRayIntersection: true // always ignore this
    });

    var rotateZeroOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        start: Vec3.ZERO,
        end: Vec3.ZERO,
        color: {
            red: 255,
            green: 0,
            blue: 0
        },
        ignoreRayIntersection: true // always ignore this
    });

    var rotateCurrentOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        start: Vec3.ZERO,
        end: Vec3.ZERO,
        color: {
            red: 0,
            green: 0,
            blue: 255
        },
        ignoreRayIntersection: true // always ignore this
    });


    var rotateOverlayInner = Overlays.addOverlay("circle3d", {
        position: Vec3.ZERO,
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
        ignoreRayIntersection: true // always ignore this
    });

    var rotateOverlayOuter = Overlays.addOverlay("circle3d", {
        position: Vec3.ZERO,
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
        ignoreRayIntersection: true // always ignore this
    });

    var rotateOverlayCurrent = Overlays.addOverlay("circle3d", {
        position: Vec3.ZERO,
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
        }
    });

    var yawHandle = Overlays.addOverlay("image3d", {
        url: ROTATE_ARROW_WEST_NORTH_URL,
        position: Vec3.ZERO,
        color: handleColor,
        alpha: handleAlpha,
        visible: false,
        size: 0.1,
        scale: 0.1,
        isFacingAvatar: false,
        drawInFront: true
    });


    var pitchHandle = Overlays.addOverlay("image3d", {
        url: ROTATE_ARROW_WEST_NORTH_URL,
        position: Vec3.ZERO,
        color: handleColor,
        alpha: handleAlpha,
        visible: false,
        size: 0.1,
        scale: 0.1,
        isFacingAvatar: false,
        drawInFront: true
    });


    var rollHandle = Overlays.addOverlay("image3d", {
        url: ROTATE_ARROW_WEST_NORTH_URL,
        position: Vec3.ZERO,
        color: handleColor,
        alpha: handleAlpha,
        visible: false,
        size: 0.1,
        scale: 0.1,
        isFacingAvatar: false,
        drawInFront: true
    });

    var allOverlays = [
        highlightBox,
        selectionBox,
        grabberMoveUp,
        yawHandle,
        pitchHandle,
        rollHandle,
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
        grabberPointLightCircleZ

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

    overlayNames[rotateOverlayInner] = "rotateOverlayInner";
    overlayNames[rotateOverlayOuter] = "rotateOverlayOuter";
    overlayNames[rotateOverlayCurrent] = "rotateOverlayCurrent";

    overlayNames[rotateZeroOverlay] = "rotateZeroOverlay";
    overlayNames[rotateCurrentOverlay] = "rotateCurrentOverlay";
    overlayNames[grabberCloner] = "grabberCloner";
    var activeTool = null;
    var grabberTools = {};

    // We get mouseMoveEvents from the handControllers, via handControllerPointer.
    // But we dont' get mousePressEvents.
    that.triggerMapping = Controller.newMapping(Script.resolvePath('') + '-click');
    Script.scriptEnding.connect(that.triggerMapping.disable);
    that.TRIGGER_GRAB_VALUE = 0.85; //  From handControllerGrab/Pointer.js. Should refactor.
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
                    activeHand = (activeHand === Controller.Standard.RightHand) ?
                        Controller.Standard.LeftHand : Controller.Standard.RightHand;
                }
                if (Reticle.pointingAtSystemOverlay || Overlays.getOverlayAtPoint(Reticle.position)) {
                    return;
                }
                that.mousePressEvent({});
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
        grabberTools[overlay] = tool;
        return tool;
    }

    // @param: toolHandle:  The overlayID associated with the tool
    //         that correlates to the tool you wish to query.
    // @note: If toolHandle is null or undefined then activeTool
    //        will be checked against those values as opposed to
    //        the tool registered under toolHandle.  Null & Undefined 
    //        are treated as separate values.
    // @return: bool - Indicates if the activeTool is that queried.
    function isActiveTool(toolHandle) {
        if (!toolHandle) {
            // Allow isActiveTool(null) and similar to return true if there's
            // no active tool
            return (activeTool === toolHandle);
        }

        if (!grabberTools.hasOwnProperty(toolHandle)) {
            print("WARNING: entitySelectionTool.isActiveTool - Encountered unknown grabberToolHandle: " + toolHandle + ". Tools should be egistered via addGrabberTool.");
            // EARLY EXIT
            return false;
        }

        return (activeTool === grabberTools[ toolHandle ]);
    }

    // @return string - The mode of the currently active tool;
    //                  otherwise, "UNKNOWN" if there's no active tool.
    function getMode() {
        return (activeTool ? activeTool.mode : "UNKNOWN");
    }


    that.cleanup = function() {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.deleteOverlay(allOverlays[i]);
        }
        for (var j = 0; j < selectionBoxes.length; j++) {
            Overlays.deleteOverlay(selectionBoxes[j]);
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
    };

    // Function: Calculate New Bound Extremes
    // uses dot product to discover new top and bottom on the new referential (max and min)
    that.calculateNewBoundExtremes = function(boundPointList, referenceVector) {
        
        if (boundPointList.length < 2) {
            return [null, null];
        }
        
        var refMax = boundPointList[0];
        var refMin = boundPointList[1];
        
        var dotMax = Vec3.dot(boundPointList[0], referenceVector);
        var dotMin = Vec3.dot(boundPointList[1], referenceVector);
        
        if (dotMin > dotMax) {
            dotMax = dotMin;
            dotMin = Vec3.dot(boundPointList[0], referenceVector);
            refMax = boundPointList[1];
            refMin = boundPointList[0];
        }
        
        for (var i = 2; i < boundPointList.length ; i++) {
            var dotAux = Vec3.dot(boundPointList[i], referenceVector);
            if (dotAux > dotMax) {
                dotMax = dotAux;
                refMax = boundPointList[i];
            } else if (dotAux < dotMin) {
                dotMin = dotAux;
                refMin = boundPointList[i];
            }
        }
        return [refMin, refMax];
    }

    // Function: Project Bounding Box Points
    // Projects all 6 bounding box points: Top, Bottom, Left, Right, Near, Far (assumes center 0,0,0) onto 
    // one of the basis of the new avatar referencial
    // dimensions - dimensions of the AABB (axis aligned bounding box) on the standard basis 
    // [1, 0, 0], [0, 1, 0], [0, 0, 1]
    // v - projection vector
    // rotateHandleOffset - offset for the rotation handle gizmo position
    that.projectBoundingBoxPoints = function(dimensions, v, rotateHandleOffset) {
        var projT_v = Vec3.dot(Vec3.multiply((dimensions.y / 2) + rotateHandleOffset, Vec3.UNIT_Y), v);
        projT_v = Vec3.multiply(projT_v, v);
        
        var projB_v = Vec3.dot(Vec3.multiply(-(dimensions.y / 2) - rotateHandleOffset, Vec3.UNIT_Y), v);
        projB_v = Vec3.multiply(projB_v, v);
        
        var projL_v = Vec3.dot(Vec3.multiply((dimensions.x / 2) + rotateHandleOffset, Vec3.UNIT_X), v);
        projL_v = Vec3.multiply(projL_v, v);
        
        var projR_v = Vec3.dot(Vec3.multiply(-1.0 * (dimensions.x / 2) - 1.0 * rotateHandleOffset, Vec3.UNIT_X), v);
        projR_v = Vec3.multiply(projR_v, v);
        
        var projN_v = Vec3.dot(Vec3.multiply((dimensions.z / 2) + rotateHandleOffset, Vec3.FRONT), v);
        projN_v = Vec3.multiply(projN_v, v);
        
        var projF_v = Vec3.dot(Vec3.multiply(-1.0 * (dimensions.z / 2) - 1.0 * rotateHandleOffset, Vec3.FRONT), v);
        projF_v = Vec3.multiply(projF_v, v);
        
        var projList = [projT_v, projB_v, projL_v, projR_v, projN_v, projF_v];

        return that.calculateNewBoundExtremes(projList, v);
    };
    
    // FUNCTION: UPDATE ROTATION HANDLES
    that.updateRotationHandles = function() {
        var diagonal = (Vec3.length(SelectionManager.worldDimensions) / 2) * 1.1;
        var halfDimensions = Vec3.multiply(SelectionManager.worldDimensions, 0.5);
        var innerActive = false;
        var innerAlpha = 0.2;
        var outerAlpha = 0.2;
        if (innerActive) {
            innerAlpha = 0.5;
        } else {
            outerAlpha = 0.5;
        }
            // prev 0.05
        var rotateHandleOffset = 0.05;

        var boundsCenter, objectCenter;

        var dimensions, rotation;
        if (spaceMode === SPACE_LOCAL) {
            rotation = SelectionManager.localRotation;
        } else {
            rotation = SelectionManager.worldRotation;
        }
        objectCenter = SelectionManager.worldPosition;
        dimensions = SelectionManager.worldDimensions;
        var position = objectCenter;

        boundsCenter = objectCenter;

        var yawCorner;
        var pitchCorner;
        var rollCorner;

        var cameraPosition = Camera.getPosition();
        var look = Vec3.normalize(Vec3.subtract(cameraPosition, objectCenter));

        // place yaw, pitch and roll rotations on the avatar referential
        
        var avatarReferential = Quat.multiply(MyAvatar.orientation, Quat.fromVec3Degrees({
            x: 0,
            y: 180,
            z: 0
        }));
        var upVector = Quat.getUp(avatarReferential);
        var rightVector = Vec3.multiply(-1, Quat.getRight(avatarReferential));
        var frontVector = Quat.getFront(avatarReferential);
        
        // project all 6 bounding box points: Top, Bottom, Left, Right, Near, Far (assumes center 0,0,0) 
        // onto the new avatar referential

        // UP
        var projUP = that.projectBoundingBoxPoints(dimensions, upVector, rotateHandleOffset);
        // RIGHT
        var projRIGHT = that.projectBoundingBoxPoints(dimensions, rightVector, rotateHandleOffset);
        // FRONT
        var projFRONT = that.projectBoundingBoxPoints(dimensions, frontVector, rotateHandleOffset);
        
        // YAW
        yawCenter = Vec3.sum(boundsCenter, projUP[0]); 
        yawCorner = Vec3.sum(boundsCenter, Vec3.sum(Vec3.sum(projUP[0], projRIGHT[1]), projFRONT[1]));
        
        yawHandleRotation = Quat.lookAt(
            yawCorner, 
            Vec3.sum(yawCorner, upVector), 
            Vec3.subtract(yawCenter,yawCorner));
        yawHandleRotation = Quat.multiply(Quat.angleAxis(45, upVector), yawHandleRotation);
        
        // PTCH
        pitchCorner = Vec3.sum(boundsCenter, Vec3.sum(Vec3.sum(projUP[1], projRIGHT[0]), projFRONT[1]));
        pitchCenter = Vec3.sum(boundsCenter, projRIGHT[0]); 
        
        pitchHandleRotation = Quat.lookAt(
            pitchCorner, 
            Vec3.sum(pitchCorner, rightVector), 
            Vec3.subtract(pitchCenter,pitchCorner));
        pitchHandleRotation = Quat.multiply(Quat.angleAxis(45, rightVector), pitchHandleRotation);
        
        // ROLL
        rollCorner = Vec3.sum(boundsCenter, Vec3.sum(Vec3.sum(projUP[1], projRIGHT[1]), projFRONT[0]));
        rollCenter = Vec3.sum(boundsCenter, projFRONT[0]); 
        
        rollHandleRotation = Quat.lookAt(
            rollCorner, 
            Vec3.sum(rollCorner, frontVector), 
            Vec3.subtract(rollCenter,rollCorner));
        rollHandleRotation = Quat.multiply(Quat.angleAxis(45, frontVector), rollHandleRotation);
        

        var rotateHandlesVisible = true;
        var rotationOverlaysVisible = false;
        // note:  Commented out as these are currently unused here; however,
        //         leaving them around as they document intent of state as it
        //         relates to modes that may be useful later.
        // var translateHandlesVisible = true;
        // var selectionBoxVisible = true;
        var isPointLight = false;
        if (SelectionManager.selections.length === 1) {
            var properties = Entities.getEntityProperties(SelectionManager.selections[0]);
            isPointLight = (properties.type === "Light") && !properties.isSpotlight;
        }

        if (isActiveTool(yawHandle) || isActiveTool(pitchHandle) || 
                isActiveTool(rollHandle) || isActiveTool(selectionBox) || isActiveTool(grabberCloner)) {
            rotationOverlaysVisible = true;
            rotateHandlesVisible = false;
            // translateHandlesVisible = false;
            // selectionBoxVisible = false;
        } else if (isActiveTool(grabberMoveUp) || isPointLight) {
            rotateHandlesVisible = false;
        } else if (activeTool) {
            // every other mode is a stretch mode...
            rotateHandlesVisible = false;
            // translateHandlesVisible = false;
        }

        Overlays.editOverlay(rotateZeroOverlay, {
            visible: rotationOverlaysVisible
        });
        Overlays.editOverlay(rotateCurrentOverlay, {
            visible: rotationOverlaysVisible
        });

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

    // FUNCTION: UPDATE HANDLE SIZES
    that.updateHandleSizes = function() {
        if (SelectionManager.hasSelection()) {
            var diff = Vec3.subtract(SelectionManager.worldPosition, Camera.getPosition());
            var grabberSize = Vec3.length(diff) * GRABBER_DISTANCE_TO_SIZE_RATIO * 5;
            var dimensions = SelectionManager.worldDimensions;
            var avgDimension = (dimensions.x + dimensions.y + dimensions.z) / 3;
            grabberSize = Math.min(grabberSize, avgDimension / 10);

            for (var i = 0; i < stretchHandles.length; i++) {
                Overlays.editOverlay(stretchHandles[i], {
                    size: grabberSize
                });
            }
            var handleSize = Vec3.length(diff) * GRABBER_DISTANCE_TO_SIZE_RATIO * 7;
            handleSize = Math.min(handleSize, avgDimension / 3);

            Overlays.editOverlay(yawHandle, {
                scale: handleSize
            });
            Overlays.editOverlay(pitchHandle, {
                scale: handleSize
            });
            Overlays.editOverlay(rollHandle, {
                scale: handleSize
            });
            var upDiff = Vec3.multiply((
                Vec3.length(diff) * GRABBER_DISTANCE_TO_SIZE_RATIO * 3), 
                Quat.getUp(MyAvatar.orientation)
            );
            var pos = Vec3.sum(grabberMoveUpPosition, upDiff);
            Overlays.editOverlay(grabberMoveUp, {
                position: pos,
                scale: handleSize / 1.25
            });
        }
    };
    Script.update.connect(that.updateHandleSizes);

    // FUNCTION: SET SPACE MODE
    that.setSpaceMode = function(newSpaceMode) {
        var wantDebug = false;
        if (wantDebug) {
            print("======> SetSpaceMode called. ========");
        }

        if (spaceMode !== newSpaceMode) {
            if (wantDebug) {
                print("    Updating SpaceMode From: " + spaceMode + " To: " + newSpaceMode);
            }
            spaceMode = newSpaceMode;
            that.updateHandles();
        } else if (wantDebug) {
            print("WARNING: entitySelectionTool.setSpaceMode - Can't update SpaceMode. CurrentMode: " + spaceMode + " DesiredMode: " + newSpaceMode);
        }
        if (wantDebug) {
            print("====== SetSpaceMode called. <========");
        }
    };

    // FUNCTION: TOGGLE SPACE MODE
    that.toggleSpaceMode = function() {
        var wantDebug = false;
        if (wantDebug) {
            print("========> ToggleSpaceMode called. =========");
        }
        if ((spaceMode === SPACE_WORLD) && (SelectionManager.selections.length > 1)) {
            if (wantDebug) {
                print("Local space editing is not available with multiple selections");
            }
            return;
        }
        if (wantDebug) {
            print("PreToggle: " + spaceMode);
        }
        spaceMode = (spaceMode === SPACE_LOCAL) ? SPACE_WORLD : SPACE_LOCAL;
        that.updateHandles();
        if (wantDebug) {
            print("PostToggle: " + spaceMode);        
            print("======== ToggleSpaceMode called. <=========");
        }
    };

    // FUNCTION: UNSELECT ALL
    // TODO?: Needs implementation
    that.unselectAll = function() {};

    // FUNCTION: UPDATE HANDLES
    that.updateHandles = function() {
        var wantDebug = false;
        if (wantDebug) {
            print("======> Update Handles =======");
            print("    Selections Count: " + SelectionManager.selections.length);
            print("    SpaceMode: " + spaceMode);
            print("    DisplayMode: " + getMode());
        }
        if (SelectionManager.selections.length === 0) {
            that.setOverlaysVisible(false);
            return;
        }

        // print("    Triggering updateRotationHandles");
        that.updateRotationHandles();

        var rotation, dimensions, position, registrationPoint;

        if (spaceMode === SPACE_LOCAL) {
            rotation = SelectionManager.localRotation;
            dimensions = SelectionManager.localDimensions;
            position = SelectionManager.localPosition;
            registrationPoint = SelectionManager.localRegistrationPoint;
        } else {
            rotation = Quat.IDENTITY;
            dimensions = SelectionManager.worldDimensions;
            position = SelectionManager.worldPosition;
            registrationPoint = SelectionManager.worldRegistrationPoint;
        }

        var registrationPointDimensions = {
            x: dimensions.x * registrationPoint.x,
            y: dimensions.y * registrationPoint.y,
            z: dimensions.z * registrationPoint.z
        };

        // Center of entity, relative to registration point
        var center = getRelativeCenterPosition(dimensions, registrationPoint);

        // Distances in world coordinates relative to the registration point
        var left = -registrationPointDimensions.x;
        var right = dimensions.x - registrationPointDimensions.x;
        var bottom = -registrationPointDimensions.y;
        var top = dimensions.y - registrationPointDimensions.y;
        var near = -registrationPointDimensions.z;
        var far = dimensions.z - registrationPointDimensions.z;
        var front = far;

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

        var inModeRotate = (isActiveTool(yawHandle) || isActiveTool(pitchHandle) || isActiveTool(rollHandle));
        var inModeTranslate = (isActiveTool(selectionBox) || isActiveTool(grabberCloner) || isActiveTool(grabberMoveUp));
        var stretchHandlesVisible = !(inModeRotate || inModeTranslate) && (spaceMode === SPACE_LOCAL);
        var extendedStretchHandlesVisible = (stretchHandlesVisible && showExtendedStretchHandles);
        var cloneHandleVisible = !(inModeRotate || inModeTranslate);
        if (wantDebug) {
            print("    Set Non-Light Grabbers Visible - Norm: " + stretchHandlesVisible + " Ext: " + extendedStretchHandlesVisible);
        }
        var isSingleSelection = (SelectionManager.selections.length === 1);

        if (isSingleSelection) {
            var properties = Entities.getEntityProperties(SelectionManager.selections[0]);
            var isLightSelection = (properties.type === "Light");
            if (isLightSelection) {
                if (wantDebug) {
                    print("    Light Selection revoking Non-Light Grabbers Visibility!");
                }
                stretchHandlesVisible = false;
                extendedStretchHandlesVisible = false;
                cloneHandleVisible = false;
                if (properties.isSpotlight) {
                    that.setPointLightHandlesVisible(false);

                    var distance = (properties.dimensions.z / 2) * Math.sin(properties.cutoff * (Math.PI / 180));
                    var showEdgeSpotGrabbers = !(inModeTranslate || inModeRotate);
                    Overlays.editOverlay(grabberSpotLightCenter, {
                        position: position,
                        visible: false
                    });
                    Overlays.editOverlay(grabberSpotLightRadius, {
                        position: NEAR,
                        rotation: rotation,
                        visible: showEdgeSpotGrabbers
                    });

                    Overlays.editOverlay(grabberSpotLightL, {
                        position: EdgeNL,
                        rotation: rotation,
                        visible: showEdgeSpotGrabbers
                    });
                    Overlays.editOverlay(grabberSpotLightR, {
                        position: EdgeNR,
                        rotation: rotation,
                        visible: showEdgeSpotGrabbers
                    });
                    Overlays.editOverlay(grabberSpotLightT, {
                        position: EdgeTN,
                        rotation: rotation,
                        visible: showEdgeSpotGrabbers
                    });
                    Overlays.editOverlay(grabberSpotLightB, {
                        position: EdgeBN,
                        rotation: rotation,
                        visible: showEdgeSpotGrabbers
                    });
                    Overlays.editOverlay(grabberSpotLightCircle, {
                        position: NEAR,
                        dimensions: {
                            x: distance,
                            y: distance,
                            z: 1
                        },
                        rotation: rotation,
                        visible: true
                    });

                    Overlays.editOverlay(grabberSpotLightLineT, {
                        start: position,
                        end: EdgeTN,
                        visible: true
                    });
                    Overlays.editOverlay(grabberSpotLightLineB, {
                        start: position,
                        end: EdgeBN,
                        visible: true
                    });
                    Overlays.editOverlay(grabberSpotLightLineR, {
                        start: position,
                        end: EdgeNR,
                        visible: true
                    });
                    Overlays.editOverlay(grabberSpotLightLineL, {
                        start: position,
                        end: EdgeNL,
                        visible: true
                    });

                } else { // ..it's a PointLight
                    that.setSpotLightHandlesVisible(false);

                    var showEdgePointGrabbers = !inModeTranslate;
                    Overlays.editOverlay(grabberPointLightT, {
                        position: TOP,
                        rotation: rotation,
                        visible: showEdgePointGrabbers
                    });
                    Overlays.editOverlay(grabberPointLightB, {
                        position: BOTTOM,
                        rotation: rotation,
                        visible: showEdgePointGrabbers
                    });
                    Overlays.editOverlay(grabberPointLightL, {
                        position: LEFT,
                        rotation: rotation,
                        visible: showEdgePointGrabbers
                    });
                    Overlays.editOverlay(grabberPointLightR, {
                        position: RIGHT,
                        rotation: rotation,
                        visible: showEdgePointGrabbers
                    });
                    Overlays.editOverlay(grabberPointLightF, {
                        position: FAR,
                        rotation: rotation,
                        visible: showEdgePointGrabbers
                    });
                    Overlays.editOverlay(grabberPointLightN, {
                        position: NEAR,
                        rotation: rotation,
                        visible: showEdgePointGrabbers
                    });
                    Overlays.editOverlay(grabberPointLightCircleX, {
                        position: position,
                        rotation: Quat.multiply(rotation, Quat.fromPitchYawRollDegrees(0, 90, 0)),
                        dimensions: {
                            x: properties.dimensions.z / 2.0,
                            y: properties.dimensions.z / 2.0,
                            z: 1
                        },
                        visible: true
                    });
                    Overlays.editOverlay(grabberPointLightCircleY, {
                        position: position,
                        rotation: Quat.multiply(rotation, Quat.fromPitchYawRollDegrees(90, 0, 0)),
                        dimensions: {
                            x: properties.dimensions.z / 2.0,
                            y: properties.dimensions.z / 2.0,
                            z: 1
                        },
                        visible: true
                    });
                    Overlays.editOverlay(grabberPointLightCircleZ, {
                        position: position,
                        rotation: rotation,
                        dimensions: {
                            x: properties.dimensions.z / 2.0,
                            y: properties.dimensions.z / 2.0,
                            z: 1
                        },
                        visible: true
                    });
                }
            } else { // ..it's not a light at all
                that.setSpotLightHandlesVisible(false);
                that.setPointLightHandlesVisible(false);
            }
        }// end of isSingleSelection


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

        Overlays.editOverlay(grabberCloner, {
            visible: cloneHandleVisible,
            rotation: rotation,
            position: EdgeTR
        });

        var selectionBoxPosition = Vec3.multiplyQbyV(rotation, center);
        selectionBoxPosition = Vec3.sum(position, selectionBoxPosition);
        Overlays.editOverlay(selectionBox, {
            position: selectionBoxPosition,
            dimensions: dimensions,
            rotation: rotation,
            visible: !inModeRotate
        });

        // Create more selection box overlays if we don't have enough
        var overlaysNeeded = SelectionManager.selections.length - selectionBoxes.length;
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
                    ignoreRayIntersection: true
                }));
        }

        i = 0;
        // Only show individual selections boxes if there is more than 1 selection
        if (SelectionManager.selections.length > 1) {
            for (; i < SelectionManager.selections.length; i++) {
                var props = Entities.getEntityProperties(SelectionManager.selections[i]);

                // Adjust overlay position to take registrationPoint into account
                // centeredRP = registrationPoint with range [-0.5, 0.5]
                var centeredRP = Vec3.subtract(props.registrationPoint, {
                    x: 0.5,
                    y: 0.5,
                    z: 0.5
                });
                var offset = vec3Mult(props.dimensions, centeredRP);
                offset = Vec3.multiply(-1, offset);
                offset = Vec3.multiplyQbyV(props.rotation, offset);
                var curBoxPosition = Vec3.sum(props.position, offset);

                var color = {red: 255, green: 128, blue: 0};
                if (i >= SelectionManager.selections.length - 1) {
                    color = {red: 255, green: 255, blue: 64};
                }

                Overlays.editOverlay(selectionBoxes[i], {
                    position: curBoxPosition,
                    color: color,
                    rotation: props.rotation,
                    dimensions: props.dimensions,
                    visible: true
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
        var upVec = Quat.getUp(MyAvatar.orientation);
        grabberMoveUpPosition = {
            x: position.x + (grabberMoveUpOffset + worldTop) * upVec.x ,
            y: position.y+ (grabberMoveUpOffset + worldTop) * upVec.y,
            z: position.z + (grabberMoveUpOffset + worldTop) * upVec.z
        };
        Overlays.editOverlay(grabberMoveUp, {
            visible: (!activeTool) || isActiveTool(grabberMoveUp)
        });

        Overlays.editOverlay(baseOfEntityProjectionOverlay, {
            visible: !inModeRotate,
            solid: true,
            position: {
                x: SelectionManager.worldPosition.x,
                y: grid.getOrigin().y,
                z: SelectionManager.worldPosition.z
            },
            dimensions: {
                x: SelectionManager.worldDimensions.x,
                y: SelectionManager.worldDimensions.z
            },
            rotation: Quat.fromPitchYawRollDegrees(90, 0, 0)
        });

        if (wantDebug) {
            print("====== Update Handles <=======");
        }

    };

    function helperSetOverlaysVisibility(handleArray, isVisible) {
        var numHandles = handleArray.length;
        var visibilityUpdate = { visible: isVisible };
        for (var handleIndex = 0; handleIndex < numHandles; ++handleIndex) {
            Overlays.editOverlay(handleArray[ handleIndex ], visibilityUpdate);
        }
    }

    // FUNCTION: SET OVERLAYS VISIBLE
    that.setOverlaysVisible = function(isVisible) {
        helperSetOverlaysVisibility(allOverlays, isVisible);
        helperSetOverlaysVisibility(selectionBoxes, isVisible);
    };

    // FUNCTION: SET ROTATION HANDLES VISIBLE
    that.setRotationHandlesVisible = function(isVisible) {
        var visibilityUpdate = { visible: isVisible };
        Overlays.editOverlay(yawHandle, visibilityUpdate);
        Overlays.editOverlay(pitchHandle, visibilityUpdate);
        Overlays.editOverlay(rollHandle, visibilityUpdate);
    };

    // FUNCTION: SET STRETCH HANDLES VISIBLE
    that.setStretchHandlesVisible = function(isVisible) {
        helperSetOverlaysVisibility(stretchHandles, isVisible);
    };

    // FUNCTION: SET GRABBER MOVE UP VISIBLE
    that.setGrabberMoveUpVisible = function(isVisible) {
        Overlays.editOverlay(grabberMoveUp, { visible: isVisible });
    };

    // FUNCTION: SET GRABBER TOOLS UP VISIBLE
    that.setGrabberToolsVisible = function(isVisible) {
        var visibilityUpdate = { visible: isVisible };
        for (var toolKey in grabberTools) {
            if (!grabberTools.hasOwnProperty(toolKey)) {
                // EARLY ITERATION EXIT--(On to the next one)
                continue;
            }

            Overlays.editOverlay(toolKey, visibilityUpdate);
        }
    };

    // FUNCTION: SET POINT LIGHT HANDLES VISIBLE
    that.setPointLightHandlesVisible = function(isVisible) {
        helperSetOverlaysVisibility(pointLightGrabberHandles, isVisible);
    };

    // FUNCTION: SET SPOT LIGHT HANDLES VISIBLE
    that.setSpotLightHandlesVisible = function(isVisible) {
        helperSetOverlaysVisibility(spotLightGrabberHandles, isVisible);
    };

    // FUNCTION: UNSELECT
    // TODO?: Needs implementation
    that.unselect = function(entityID) {};

    var initialXZPick = null;
    var isConstrained = false;
    var constrainMajorOnly = false;
    var startPosition = null;
    var duplicatedEntityIDs = null;

    // TOOL DEFINITION: TRANSLATE XZ TOOL
    var translateXZTool = addGrabberTool(selectionBox,{
        mode: 'TRANSLATE_XZ',
        pickPlanePosition: { x: 0, y: 0, z: 0 },
        greatestDimension: 0.0,
        startingDistance: 0.0,
        startingElevation: 0.0,
        onBegin: function(event, pickRay, pickResult, doClone) {
            var wantDebug = false;
            if (wantDebug) {
                print("================== TRANSLATE_XZ(Beg) -> =======================");
                Vec3.print("    pickRay", pickRay);
                Vec3.print("    pickRay.origin", pickRay.origin);
                Vec3.print("    pickResult.intersection", pickResult.intersection);
            }

            SelectionManager.saveProperties();
            that.setRotationHandlesVisible(false);
            that.setStretchHandlesVisible(false);
            that.setGrabberMoveUpVisible(false);

            startPosition = SelectionManager.worldPosition;

            translateXZTool.pickPlanePosition = pickResult.intersection;
            translateXZTool.greatestDimension = Math.max(Math.max(SelectionManager.worldDimensions.x, SelectionManager.worldDimensions.y), SelectionManager.worldDimensions.z);
            translateXZTool.startingDistance = Vec3.distance(pickRay.origin, SelectionManager.position);
            translateXZTool.startingElevation = translateXZTool.elevation(pickRay.origin, translateXZTool.pickPlanePosition);
            if (wantDebug) {
                print("    longest dimension: " + translateXZTool.greatestDimension);
                print("    starting distance: " + translateXZTool.startingDistance);
                print("    starting elevation: " + translateXZTool.startingElevation);
            }

            initialXZPick = rayPlaneIntersection(pickRay, translateXZTool.pickPlanePosition, {
                x: 0,
                y: 1,
                z: 0
            });

            // Duplicate entities if alt is pressed.  This will make a
            // copy of the selected entities and move the _original_ entities, not
            // the new ones.
            if (event.isAlt || doClone) {
                duplicatedEntityIDs = [];
                for (var otherEntityID in SelectionManager.savedProperties) {
                    var properties = SelectionManager.savedProperties[otherEntityID];
                    if (!properties.locked) {
                        var entityID = Entities.addEntity(properties);
                        duplicatedEntityIDs.push({
                            entityID: entityID,
                            properties: properties
                        });
                    }
                }
            } else {
                duplicatedEntityIDs = null;
            }

            isConstrained = false;
            if (wantDebug) {
                print("================== TRANSLATE_XZ(End) <- =======================");
            }
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
                    print("    "+ translateXZTool.mode + "Pick ray does not intersect XZ plane.");
                }
                
                // EARLY EXIT--(Invalid ray detected.)
                return;
            }

            var vector = Vec3.subtract(pick, initialXZPick);

            // If the mouse is too close to the horizon of the pick plane, stop moving
            var MIN_ELEVATION = 0.02; //  largest dimension of object divided by distance to it
            var elevation = translateXZTool.elevation(pickRay.origin, pick);
            if (wantDebug) {
                print("Start Elevation: " + translateXZTool.startingElevation + ", elevation: " + elevation);
            }
            if ((translateXZTool.startingElevation > 0.0 && elevation < MIN_ELEVATION) ||
                (translateXZTool.startingElevation < 0.0 && elevation > -MIN_ELEVATION)) {
                if (wantDebug) {
                    print("    "+ translateXZTool.mode + " - too close to horizon!");
                }

                // EARLY EXIT--(Don't proceed past the reached limit.)
                return;
            }

            //  If the angular size of the object is too small, stop moving
            var MIN_ANGULAR_SIZE = 0.01; //  Radians
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
            var cornerPosition = Vec3.sum(startPosition, Vec3.multiply(-0.5, SelectionManager.worldDimensions));
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
                    position: newPosition
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
    });
    
    // GRABBER TOOL: GRABBER MOVE UP
    var lastXYPick = null;
    var upDownPickNormal = null;
    addGrabberTool(grabberMoveUp, {
        mode: "TRANSLATE_UP_DOWN",
        onBegin: function(event, pickRay, pickResult) {
            upDownPickNormal = Quat.getForward(lastCameraOrientation);
            lastXYPick = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, upDownPickNormal);

            SelectionManager.saveProperties();
            that.setGrabberMoveUpVisible(true);
            that.setStretchHandlesVisible(false);
            that.setRotationHandlesVisible(false);

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
                            properties: properties
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

            // project vector onto avatar up vector
            // we want the avatar referential not the camera.
            var avatarUpVector = Quat.getUp(MyAvatar.orientation);
            var dotVectorUp = Vec3.dot(vector, avatarUpVector);
            vector = Vec3.multiply(dotVectorUp, avatarUpVector);


            vector = grid.snapToGrid(vector);

            

            var wantDebug = false;
            if (wantDebug) {
                print("translateUpDown... ");
                print("                event.y:" + event.y);
                Vec3.print("        newIntersection:", newIntersection);
                Vec3.print("                 vector:", vector);
                // Vec3.print("            newPosition:", newPosition);
            }
            for (var i = 0; i < SelectionManager.selections.length; i++) {
                var id = SelectionManager.selections[i];
                var properties = SelectionManager.savedProperties[id];

                var original = properties.position;
                var newPosition = Vec3.sum(properties.position, vector);

                Entities.editEntity(id, {
                    position: newPosition
                });
            }

            SelectionManager._update();
        }
    });

    // GRABBER TOOL: GRABBER CLONER
    addGrabberTool(grabberCloner, {
        mode: "CLONE",
        onBegin: function(event, pickRay, pickResult) {
            var doClone = true;
            translateXZTool.onBegin(event,pickRay,pickResult,doClone);
        },
        elevation: function (event) {
            translateXZTool.elevation(event);
        },

        onEnd: function (event) {
            translateXZTool.onEnd(event);
        },

        onMove: function (event) {
            translateXZTool.onMove(event);
        }
    });


    // FUNCTION: VEC 3 MULT
    var vec3Mult = function(v1, v2) {
        return {
            x: v1.x * v2.x,
            y: v1.y * v2.y,
            z: v1.z * v2.z
        };
    };

    // FUNCTION: MAKE STRETCH TOOL
    // stretchMode - name of mode
    // direction - direction to stretch in
    // pivot - point to use as a pivot
    // offset - the position of the overlay tool relative to the selections center position
    // @return: tool obj
    var makeStretchTool = function(stretchMode, direction, pivot, offset, customOnMove) {
        //  directionFor3DStretch - direction and pivot for 3D stretch
        //  distanceFor3DStretch - distance from the intersection point and the handController 
        //     used to increase the scale taking into account the distance to the object
        //    DISTANCE_INFLUENCE_THRESHOLD - constant that holds the minimum distance where the 
        //     distance to the object will influence the stretch/resize/scale
        var directionFor3DStretch = getDirectionsFor3DStretch(stretchMode);
        var distanceFor3DStretch = 0;
        var DISTANCE_INFLUENCE_THRESHOLD = 1.2;
        
        
        var signs = {
            x: direction.x < 0 ? -1 : (direction.x > 0 ? 1 : 0),
            y: direction.y < 0 ? -1 : (direction.y > 0 ? 1 : 0),
            z: direction.z < 0 ? -1 : (direction.z > 0 ? 1 : 0)
        };

        var mask = {
            x: Math.abs(direction.x) > 0 ? 1 : 0,
            y: Math.abs(direction.y) > 0 ? 1 : 0,
            z: Math.abs(direction.z) > 0 ? 1 : 0
        };


        var numDimensions = mask.x + mask.y + mask.z;

        var planeNormal = null;
        var lastPick = null;
        var lastPick3D = null;
        var initialPosition = null;
        var initialDimensions = null;
        var initialIntersection = null;
        var initialProperties = null;
        var registrationPoint = null;
        var deltaPivot = null;
        var deltaPivot3D = null;
        var pickRayPosition = null;
        var pickRayPosition3D = null;
        var rotation = null;

        var onBegin = function(event, pickRay, pickResult) {
            var properties = Entities.getEntityProperties(SelectionManager.selections[0]);
            initialProperties = properties;
            rotation = (spaceMode === SPACE_LOCAL) ? properties.rotation : Quat.IDENTITY;

            if (spaceMode === SPACE_LOCAL) {
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
            var scaledPivot = Vec3.multiply(0.5, pivot);
            deltaPivot = Vec3.subtract(centeredRP, scaledPivot);

            var scaledOffset = Vec3.multiply(0.5, offset);

            // Offset from the registration point
            offsetRP = Vec3.subtract(scaledOffset, centeredRP);

            // Scaled offset in world coordinates
            var scaledOffsetWorld = vec3Mult(initialDimensions, offsetRP);
            
            pickRayPosition = Vec3.sum(initialPosition, Vec3.multiplyQbyV(rotation, scaledOffsetWorld));
            
            if (directionFor3DStretch) {
                // pivot, offset and pickPlanePosition for 3D manipulation
                var scaledPivot3D = Vec3.multiply(0.5, Vec3.multiply(1.0, directionFor3DStretch));
                deltaPivot3D = Vec3.subtract(centeredRP, scaledPivot3D);
                
                var scaledOffsetWorld3D = vec3Mult(initialDimensions, 
                    Vec3.subtract(Vec3.multiply(0.5, Vec3.multiply(-1.0, directionFor3DStretch)), centeredRP));
                
                pickRayPosition3D = Vec3.sum(initialPosition, Vec3.multiplyQbyV(rotation, scaledOffsetWorld));
            }
            var start = null;
            var end = null;
            if ((numDimensions === 1) && mask.x) {
                start = Vec3.multiplyQbyV(rotation, {
                    x: -10000,
                    y: 0,
                    z: 0
                });
                start = Vec3.sum(start, properties.position);
                end = Vec3.multiplyQbyV(rotation, {
                    x: 10000,
                    y: 0,
                    z: 0
                });
                end = Vec3.sum(end, properties.position);
                Overlays.editOverlay(xRailOverlay, {
                    start: start,
                    end: end,
                    visible: true
                });
            }
            if ((numDimensions === 1) && mask.y) {
                start = Vec3.multiplyQbyV(rotation, {
                    x: 0,
                    y: -10000,
                    z: 0
                });
                start = Vec3.sum(start, properties.position);
                end = Vec3.multiplyQbyV(rotation, {
                    x: 0,
                    y: 10000,
                    z: 0
                });
                end = Vec3.sum(end, properties.position);
                Overlays.editOverlay(yRailOverlay, {
                    start: start,
                    end: end,
                    visible: true
                });
            }
            if ((numDimensions === 1) && mask.z) {
                start = Vec3.multiplyQbyV(rotation, {
                    x: 0,
                    y: 0,
                    z: -10000
                });
                start = Vec3.sum(start, properties.position);
                end = Vec3.multiplyQbyV(rotation, {
                    x: 0,
                    y: 0,
                    z: 10000
                });
                end = Vec3.sum(end, properties.position);
                Overlays.editOverlay(zRailOverlay, {
                    start: start,
                    end: end,
                    visible: true
                });
            }
            if (numDimensions === 1) {
                if (mask.x === 1) {
                    planeNormal = {
                        x: 0,
                        y: 1,
                        z: 0
                    };
                } else if (mask.y === 1) {
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
            } else if (numDimensions === 2) {
                if (mask.x === 0) {
                    planeNormal = {
                        x: 1,
                        y: 0,
                        z: 0
                    };
                } else if (mask.y === 0) {
                    planeNormal = {
                        x: 0,
                        y: 1,
                        z: 0
                    };
                } else {
                    planeNormal = {
                        x: 0,
                        y: 0,
                        z: 1
                    };
                }
            }
            
            planeNormal = Vec3.multiplyQbyV(rotation, planeNormal);
            lastPick = rayPlaneIntersection(pickRay,
                pickRayPosition,
                planeNormal);
                
            var planeNormal3D = {
                x: 0,
                y: 0,
                z: 0
            };
            if (directionFor3DStretch) {
                lastPick3D = rayPlaneIntersection(pickRay,
                    pickRayPosition3D,
                    planeNormal3D);
                distanceFor3DStretch = Vec3.length(Vec3.subtract(pickRayPosition3D, pickRay.origin));
            }
        
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
            var proportional = (spaceMode === SPACE_WORLD) || event.isShifted || isActiveTool(grabberSpotLightRadius);

            var position, dimensions, rotation;
            if (spaceMode === SPACE_LOCAL) {
                position = SelectionManager.localPosition;
                dimensions = SelectionManager.localDimensions;
                rotation = SelectionManager.localRotation;
            } else {
                position = SelectionManager.worldPosition;
                dimensions = SelectionManager.worldDimensions;
                rotation = SelectionManager.worldRotation;
            }
            
            var localDeltaPivot = deltaPivot;
            var localSigns = signs;

            var pickRay = generalComputePickRay(event.x, event.y);
            
            // Are we using handControllers or Mouse - only relevant for 3D tools
            var controllerPose = getControllerWorldLocation(activeHand, true);
            var vector = null;
            if (HMD.isHMDAvailable() && HMD.isHandControllerAvailable() && 
                    controllerPose.valid && that.triggered && directionFor3DStretch) {
                localDeltaPivot = deltaPivot3D;

                newPick = pickRay.origin;
            
                vector = Vec3.subtract(newPick, lastPick3D);
                
                vector = Vec3.multiplyQbyV(Quat.inverse(rotation), vector);
            
                if (distanceFor3DStretch > DISTANCE_INFLUENCE_THRESHOLD) {
                    // Range of Motion
                    vector = Vec3.multiply(distanceFor3DStretch , vector);
                }
                
                localSigns = directionFor3DStretch;
                
            } else {
                newPick = rayPlaneIntersection(pickRay,
                    pickRayPosition,
                    planeNormal);
                vector = Vec3.subtract(newPick, lastPick);

                vector = Vec3.multiplyQbyV(Quat.inverse(rotation), vector);

                vector = vec3Mult(mask, vector);
                
            }
            
            if (customOnMove) {
                var change = Vec3.multiply(-1, vec3Mult(localSigns, vector));
                customOnMove(vector, change);
            } else {
                vector = grid.snapToSpacing(vector);

                var changeInDimensions = Vec3.multiply(-1, vec3Mult(localSigns, vector));
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

                var changeInPosition = Vec3.multiplyQbyV(rotation, vec3Mult(localDeltaPivot, changeInDimensions));
                var newPosition = Vec3.sum(initialPosition, changeInPosition);

                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    Entities.editEntity(SelectionManager.selections[i], {
                        position: newPosition,
                        dimensions: newDimensions
                    });
                }
                

                var wantDebug = false;
                if (wantDebug) {
                    print(stretchMode);
                    // Vec3.print("        newIntersection:", newIntersection);
                    Vec3.print("                 vector:", vector);
                    // Vec3.print("           oldPOS:", oldPOS);
                    // Vec3.print("                newPOS:", newPOS);
                    Vec3.print("            changeInDimensions:", changeInDimensions);
                    Vec3.print("                 newDimensions:", newDimensions);

                    Vec3.print("              changeInPosition:", changeInPosition);
                    Vec3.print("                   newPosition:", newPosition);
                }
            }

            SelectionManager._update();
        };// End of onMove def

        return {
            mode: stretchMode,
            onBegin: onBegin,
            onMove: onMove,
            onEnd: onEnd
        };
    };
    
    // Direction for the stretch tool when using hand controller
    var directionsFor3DGrab = {
        LBN: {
            x: 1,
            y: 1,
            z: 1
        },
        RBN: {
            x: -1,
            y: 1,
            z: 1
        },
        LBF: {
            x: 1,
            y: 1,
            z: -1
        },
        RBF: {
            x: -1,
            y: 1,
            z: -1
        },
        LTN: {
            x: 1,
            y: -1,
            z: 1
        },
        RTN: {
            x: -1,
            y: -1,
            z: 1
        },
        LTF: {
            x: 1,
            y: -1,
            z: -1
        },
        RTF: {
            x: -1,
            y: -1,
            z: -1
        }
    };

    // FUNCTION: GET DIRECTION FOR 3D STRETCH    
    // Returns a vector with directions for the stretch tool in 3D using hand controllers
    function getDirectionsFor3DStretch(mode) {
        if (mode === "STRETCH_LBN") {
            return directionsFor3DGrab.LBN;
        } else if (mode === "STRETCH_RBN") {
            return directionsFor3DGrab.RBN;
        } else if (mode === "STRETCH_LBF") {
            return directionsFor3DGrab.LBF;
        } else if (mode === "STRETCH_RBF") {
            return directionsFor3DGrab.RBF;
        } else if (mode === "STRETCH_LTN") {
            return directionsFor3DGrab.LTN;
        } else if (mode === "STRETCH_RTN") {
            return directionsFor3DGrab.RTN;
        } else if (mode === "STRETCH_LTF") {
            return directionsFor3DGrab.LTF;
        } else if (mode === "STRETCH_RTF") {
            return directionsFor3DGrab.RTF;
        } else {
            return null;
        }
    }
    
    
    // FUNCTION: ADD STRETCH TOOL
    function addStretchTool(overlay, mode, pivot, direction, offset, handleMove) {
        if (!pivot) {
            pivot = direction;
        }
        var tool = makeStretchTool(mode, direction, pivot, offset, handleMove);

        return addGrabberTool(overlay, tool);
    }

    // FUNCTION: CUTOFF STRETCH FUNC
    function cutoffStretchFunc(vector, change) {
        vector = change;
        var wantDebug = false;
        if (wantDebug) {
            Vec3.print("Radius stretch: ", vector);
        }
        var length = vector.x + vector.y + vector.z;
        var props = SelectionManager.savedProperties[SelectionManager.selections[0]];

        var radius = props.dimensions.z / 2;
        var originalCutoff = props.cutoff;

        var originalSize = radius * Math.tan(originalCutoff * (Math.PI / 180));
        var newSize = originalSize + length;
        var cutoff = Math.atan2(newSize, radius) * 180 / Math.PI;

        Entities.editEntity(SelectionManager.selections[0], {
            cutoff: cutoff
        });

        SelectionManager._update();
    }

    // FUNCTION: RADIUS STRETCH FUNC
    function radiusStretchFunc(vector, change) {
        var props = SelectionManager.savedProperties[SelectionManager.selections[0]];

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

        Entities.editEntity(SelectionManager.selections[0], {
            dimensions: newDimensions
        });

        SelectionManager._update();
    }

    // STRETCH TOOL DEF SECTION
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

    // FUNCTION: UPDATE ROTATION DEGREES OVERLAY
    function updateRotationDegreesOverlay(angleFromZero, handleRotation, centerPosition) {
        var wantDebug = false;
        if (wantDebug) {
            print("---> updateRotationDegreesOverlay ---");
            print("    AngleFromZero: " + angleFromZero);
            print("    HandleRotation - X: " + handleRotation.x + " Y: " + handleRotation.y + " Z: " + handleRotation.z);
            print("    CenterPos - " + centerPosition.x + " Y: " + centerPosition.y + " Z: " + centerPosition.z);
        }

        var angle = angleFromZero * (Math.PI / 180);
        var position = {
            x: Math.cos(angle) * outerRadius * ROTATION_DISPLAY_DISTANCE_MULTIPLIER,
            y: Math.sin(angle) * outerRadius * ROTATION_DISPLAY_DISTANCE_MULTIPLIER,
            z: 0
        };
        if (wantDebug) {
            print("    Angle: " + angle);
            print("    InitialPos: " + position.x + ", " + position.y + ", " + position.z);
        }
        
        position = Vec3.multiplyQbyV(handleRotation, position);
        position = Vec3.sum(centerPosition, position);
        var overlayProps = {
            position: position,
            dimensions: {
                x: innerRadius * ROTATION_DISPLAY_SIZE_X_MULTIPLIER,
                y: innerRadius * ROTATION_DISPLAY_SIZE_Y_MULTIPLIER
            },
            lineHeight: innerRadius * ROTATION_DISPLAY_LINE_HEIGHT_MULTIPLIER,
            text: normalizeDegrees(-angleFromZero) + "°"
        };
        if (wantDebug) {
            print("    TranslatedPos: " + position.x + ", " + position.y + ", " + position.z);
            print("    OverlayDim - X: " + overlayProps.dimensions.x + " Y: " + overlayProps.dimensions.y + " Z: " + overlayProps.dimensions.z);
            print("    OverlayLineHeight: " + overlayProps.lineHeight);
            print("    OverlayText: " + overlayProps.text);
        }

        Overlays.editOverlay(rotationDegreesDisplay, overlayProps);
        if (wantDebug) {
            print("<--- updateRotationDegreesOverlay ---");
        }
    }

    // FUNCTION DEF: updateSelectionsRotation
    //    Helper func used by rotation grabber tools 
    function updateSelectionsRotation(rotationChange) {
        if (!rotationChange) {
            print("ERROR: entitySelectionTool.updateSelectionsRotation - Invalid arg specified!!");

            // EARLY EXIT
            return;
        }
        
        // Entities should only reposition if we are rotating multiple selections around
        // the selections center point.  Otherwise, the rotation will be around the entities
        // registration point which does not need repositioning.
        var reposition = (SelectionManager.selections.length > 1);
        for (var i = 0; i < SelectionManager.selections.length; i++) {
            var entityID = SelectionManager.selections[i];
            var initialProperties = SelectionManager.savedProperties[entityID];

            var newProperties = {
                rotation: Quat.multiply(rotationChange, initialProperties.rotation)
            };

            if (reposition) {
                var dPos = Vec3.subtract(initialProperties.position, initialPosition);
                dPos = Vec3.multiplyQbyV(rotationChange, dPos);
                newProperties.position = Vec3.sum(initialPosition, dPos);
            }

            Entities.editEntity(entityID, newProperties);
        }
    }

    function helperRotationHandleOnBegin(event, pickRay, rotAroundAxis, rotCenter, handleRotation) {
        var wantDebug = false;
        if (wantDebug) {
            print("================== " + getMode() + "(rotation helper onBegin) -> =======================");
        }

        SelectionManager.saveProperties();
        that.setRotationHandlesVisible(false);
        that.setStretchHandlesVisible(false);
        that.setGrabberMoveUpVisible(false);

        initialPosition = SelectionManager.worldPosition;
        rotationNormal = { x: 0, y: 0, z: 0 };
        rotationNormal[rotAroundAxis] = 1;
        //get the correct axis according to the avatar referencial
        var avatarReferential = Quat.multiply(MyAvatar.orientation, Quat.fromVec3Degrees({
            x: 0,
            y: 0,
            z: 0
        }));
        rotationNormal = Vec3.multiplyQbyV(avatarReferential, rotationNormal);

        // Size the overlays to the current selection size
        var diagonal = (Vec3.length(SelectionManager.worldDimensions) / 2) * 1.1;
        var halfDimensions = Vec3.multiply(SelectionManager.worldDimensions, 0.5);
        innerRadius = diagonal;
        outerRadius = diagonal * 1.15;
        var innerAlpha = 0.2;
        var outerAlpha = 0.2;
        Overlays.editOverlay(rotateOverlayInner, {
            visible: true,
            rotation: handleRotation,
            position: rotCenter,
            size: innerRadius,
            innerRadius: 0.9,
            startAt: 0,
            endAt: 360,
            alpha: innerAlpha
        });

        Overlays.editOverlay(rotateOverlayOuter, {
            visible: true,
            rotation: handleRotation,
            position: rotCenter,
            size: outerRadius,
            innerRadius: 0.9,
            startAt: 0,
            endAt: 360,
            alpha: outerAlpha
        });

        Overlays.editOverlay(rotateOverlayCurrent, {
            visible: true,
            rotation: handleRotation,
            position: rotCenter,
            size: outerRadius,
            startAt: 0,
            endAt: 0,
            innerRadius: 0.9
        });

        Overlays.editOverlay(rotationDegreesDisplay, {
            visible: true
        });

        updateRotationDegreesOverlay(0, handleRotation, rotCenter);
        
        // editOverlays may not have committed rotation changes.
        // Compute zero position based on where the overlay will be eventually.
        var result = rayPlaneIntersection(pickRay, rotCenter, rotationNormal);
        // In case of a parallel ray, this will be null, which will cause early-out
        // in the onMove helper.
        rotZero = result;
        
        if (wantDebug) {
            print("================== " + getMode() + "(rotation helper onBegin) <- =======================");
        }
    }// End_Function(helperRotationHandleOnBegin)

    function helperRotationHandleOnMove(event, rotAroundAxis, rotCenter, handleRotation) {

        if (!rotZero) {
            print("ERROR: entitySelectionTool.handleRotationHandleOnMove - Invalid RotationZero Specified (missed rotation target plane?)");

            // EARLY EXIT
            return;
        }

        var wantDebug = false;
        if (wantDebug) {
            print("================== "+ getMode() + "(rotation helper onMove) -> =======================");
            Vec3.print("    rotZero: ", rotZero);
        }
        var pickRay = generalComputePickRay(event.x, event.y);
        Overlays.editOverlay(selectionBox, {
            visible: false
        });
        Overlays.editOverlay(baseOfEntityProjectionOverlay, {
            visible: false
        });

        var result = rayPlaneIntersection(pickRay, rotCenter, rotationNormal);
        if (result) {
            var centerToZero = Vec3.subtract(rotZero, rotCenter);
            var centerToIntersect = Vec3.subtract(result, rotCenter);
            if (wantDebug) {
                Vec3.print("    RotationNormal:    ", rotationNormal);
                Vec3.print("    rotZero:           ", rotZero);
                Vec3.print("    rotCenter:         ", rotCenter);
                Vec3.print("    intersect:         ", result);
                Vec3.print("    centerToZero:      ", centerToZero);
                Vec3.print("    centerToIntersect: ", centerToIntersect);
            }
            // Note: orientedAngle which wants normalized centerToZero and centerToIntersect
            //             handles that internally, so it's to pass unnormalized vectors here.
            var angleFromZero = Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal);

            var distanceFromCenter = Vec3.length(centerToIntersect);
            var snapToInner = distanceFromCenter < innerRadius;
            var snapAngle = snapToInner ? innerSnapAngle : 1.0;
            angleFromZero = Math.floor(angleFromZero / snapAngle) * snapAngle;

            
            var rotChange = Quat.angleAxis(angleFromZero, rotationNormal);
            updateSelectionsRotation(rotChange);
            //present angle in avatar referencial
            angleFromZero = -angleFromZero;
            updateRotationDegreesOverlay(angleFromZero, handleRotation, rotCenter);

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
                    minorTickMarksLength: 0
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
                    minorTickMarksLength: 0.1
                });
            }
        }// End_If(results.intersects)

        if (wantDebug) {
            print("================== "+ getMode() + "(rotation helper onMove) <- =======================");
        }
    }// End_Function(helperRotationHandleOnMove)

    function helperRotationHandleOnEnd() {
        var wantDebug = false;
        if (wantDebug) {
            print("================== " + getMode() + "(onEnd) -> =======================");
        }
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

        if (wantDebug) {
            print("================== " + getMode() + "(onEnd) <- =======================");
        }
    }// End_Function(helperRotationHandleOnEnd)


    // YAW GRABBER TOOL DEFINITION
    var initialPosition = SelectionManager.worldPosition;
    addGrabberTool(yawHandle, {
        mode: "ROTATE_YAW",
        onBegin: function(event, pickRay, pickResult) {
            helperRotationHandleOnBegin(event, pickRay, "y", yawCenter, yawHandleRotation);
        },
        onEnd: function(event, reason) {
            helperRotationHandleOnEnd();
        },
        onMove: function(event) {
            helperRotationHandleOnMove(event, "y", yawCenter, yawHandleRotation);
        }
    });


    // PITCH GRABBER TOOL DEFINITION
    addGrabberTool(pitchHandle, {
        mode: "ROTATE_PITCH",
        onBegin: function(event, pickRay, pickResult) {
            helperRotationHandleOnBegin(event, pickRay, "x", pitchCenter, pitchHandleRotation);
        },
        onEnd: function(event, reason) {
            helperRotationHandleOnEnd();
        },
        onMove: function (event) {
            helperRotationHandleOnMove(event, "x", pitchCenter, pitchHandleRotation);
        }
    });


    // ROLL GRABBER TOOL DEFINITION
    addGrabberTool(rollHandle, {
        mode: "ROTATE_ROLL",
        onBegin: function(event, pickRay, pickResult) {
            helperRotationHandleOnBegin(event, pickRay, "z", rollCenter, rollHandleRotation);
        },
        onEnd: function (event, reason) {
            helperRotationHandleOnEnd();
        },
        onMove: function(event) {
            helperRotationHandleOnMove(event, "z", rollCenter, rollHandleRotation);
        }
    });

    // FUNCTION: CHECK MOVE
    that.checkMove = function() {
        if (SelectionManager.hasSelection()) {

            // FIXME - this cause problems with editing in the entity properties window
            // SelectionManager._update();

            if (!Vec3.equal(Camera.getPosition(), lastCameraPosition) ||
                !Quat.equal(Camera.getOrientation(), lastCameraOrientation)) {

                that.updateRotationHandles();
            }
        }
    };

    that.checkControllerMove = function() {
        if (SelectionManager.hasSelection()) {
            var controllerPose = getControllerWorldLocation(activeHand, true);
            var hand = (activeHand === Controller.Standard.LeftHand) ? 0 : 1;
            if (controllerPose.valid && lastControllerPoses[hand].valid) {
                if (!Vec3.equal(controllerPose.position, lastControllerPoses[hand].position) ||
                    !Vec3.equal(controllerPose.rotation, lastControllerPoses[hand].rotation)) {
                    that.mouseMoveEvent({});
                }
            }
            lastControllerPoses[hand] = controllerPose;
        }
    };


    // FUNCTION DEF(s): Intersection Check Helpers
    function testRayIntersect(queryRay, overlayIncludes, overlayExcludes) {
        var wantDebug = false;
        if ((queryRay === undefined) || (queryRay === null)) {
            if (wantDebug) {
                print("testRayIntersect - EARLY EXIT -> queryRay is undefined OR null!");
            }
            return null;
        }

        var intersectObj = Overlays.findRayIntersection(queryRay, true, overlayIncludes, overlayExcludes);

        if (wantDebug) {
            if (!overlayIncludes) {
                print("testRayIntersect - no overlayIncludes provided.");
            }
            if (!overlayExcludes) {
                print("testRayIntersect - no overlayExcludes provided.");
            }
            print("testRayIntersect - Hit: " + intersectObj.intersects);
            print("    intersectObj.overlayID:" + intersectObj.overlayID + "[" + overlayNames[intersectObj.overlayID] + "]");
            print("        OverlayName: " + overlayNames[intersectObj.overlayID]);
            print("    intersectObj.distance:" + intersectObj.distance);
            print("    intersectObj.face:" + intersectObj.face);
            Vec3.print("    intersectObj.intersection:", intersectObj.intersection);
        }

        return intersectObj;
    }

    // FUNCTION: MOUSE PRESS EVENT
    that.mousePressEvent = function (event) {
        var wantDebug = false;
        if (wantDebug) {
            print("=============== eST::MousePressEvent BEG =======================");
        }
        if (!event.isLeftButton && !that.triggered) {
            // EARLY EXIT-(if another mouse button than left is pressed ignore it)
            return false;
        }

        var pickRay = generalComputePickRay(event.x, event.y);
        // TODO_Case6491:  Move this out to setup just to make it once
        var interactiveOverlays = [HMD.tabletID, HMD.tabletScreenID, HMD.homeButtonID, selectionBox];
        for (var key in grabberTools) {
            if (grabberTools.hasOwnProperty(key)) {
                interactiveOverlays.push(key);
            }
        }

        // Start with unknown mode, in case no tool can handle this.
        activeTool = null;

        var results = testRayIntersect(pickRay, interactiveOverlays);
        if (results.intersects) {
            var hitOverlayID = results.overlayID;
            if ((hitOverlayID === HMD.tabletID) || (hitOverlayID === HMD.tabletScreenID) || (hitOverlayID === HMD.homeButtonID)) {
                // EARLY EXIT-(mouse clicks on the tablet should override the edit affordances)
                return false;
            }

            entityIconOverlayManager.setIconsSelectable(SelectionManager.selections, true);

            var hitTool = grabberTools[ hitOverlayID ];
            if (hitTool) {
                activeTool = hitTool;
                if (activeTool.onBegin) {
                    activeTool.onBegin(event, pickRay, results);
                } else {
                    print("ERROR: entitySelectionTool.mousePressEvent - ActiveTool(" + activeTool.mode + ") missing onBegin");
                }
            } else {
                print("ERROR: entitySelectionTool.mousePressEvent - Hit unexpected object, check interactiveOverlays");
            }// End_if (hitTool)
        }// End_If(results.intersects)

        if (wantDebug) {
            print("    DisplayMode: " + getMode());
            print("=============== eST::MousePressEvent END =======================");
        }

        // If mode is known then we successfully handled this;
        // otherwise, we're missing a tool.
        return activeTool;
    };

    // FUNCTION: MOUSE MOVE EVENT
    that.mouseMoveEvent = function(event) {
        var wantDebug = false;
        if (wantDebug) {
            print("=============== eST::MouseMoveEvent BEG =======================");
        }
        if (activeTool) {
            if (wantDebug) {
                print("    Trigger ActiveTool(" + activeTool.mode + ")'s onMove");
            }
            activeTool.onMove(event);

            if (wantDebug) {
                print("    Trigger SelectionManager::update");
            }
            SelectionManager._update();

            if (wantDebug) {
                print("=============== eST::MouseMoveEvent END =======================");
            }
            // EARLY EXIT--(Move handled via active tool)
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

                case grabberCloner:
                    pickedColor = grabberColorCloner;
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

        if (wantDebug) {
            print("=============== eST::MouseMoveEvent END =======================");
        }
        return false;
    };

    // FUNCTION: MOUSE RELEASE EVENT
    that.mouseReleaseEvent = function(event) {
        var wantDebug = false;
        if (wantDebug) {
            print("=============== eST::MouseReleaseEvent BEG =======================");
        }
        var showHandles = false;
        if (activeTool) {
            if (activeTool.onEnd) {
                if (wantDebug) {
                    print("    Triggering ActiveTool(" + activeTool.mode + ")'s onEnd");
                }
                activeTool.onEnd(event);
            } else if (wantDebug) {
                print("    ActiveTool(" + activeTool.mode + ")'s missing onEnd");
            }
        }
        
        // hide our rotation overlays..., and show our handles
        if (isActiveTool(yawHandle) || isActiveTool(pitchHandle) || isActiveTool(rollHandle)) {
            if (wantDebug) {
                print("    Triggering hide of RotateOverlays");
            }
            Overlays.editOverlay(rotateOverlayInner, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayOuter, {
                visible: false
            });
            Overlays.editOverlay(rotateOverlayCurrent, {
                visible: false
            });
            
        }

        showHandles = activeTool; // base on prior tool value
        activeTool = null;

        // if something is selected, then reset the "original" properties for any potential next click+move operation
        if (SelectionManager.hasSelection()) {
            if (showHandles) {
                if (wantDebug) {
                    print("    Triggering that.select");
                }
                that.select(SelectionManager.selections[0], event);
            }
        }

        if (wantDebug) {
            print("=============== eST::MouseReleaseEvent END =======================");
        }
    };

    // NOTE: mousePressEvent and mouseMoveEvent from the main script should call us., so we don't hook these:
    //       Controller.mousePressEvent.connect(that.mousePressEvent);
    //       Controller.mouseMoveEvent.connect(that.mouseMoveEvent);
    Controller.mouseReleaseEvent.connect(that.mouseReleaseEvent);


    return that;

}());
