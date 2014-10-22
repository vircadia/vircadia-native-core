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

Script.include("libraries/globals.js");

SPACE_LOCAL = "local";
SPACE_WORLD = "world";

SelectionManager = (function() {
    var that = {};


    that.savedProperties = {};

    that.eventListener = null;
    that.selections = [];

    that.localRotation = Quat.fromPitchYawRollDegrees(0, 0, 0);
    that.localPosition = { x: 0, y: 0, z: 0 };
    that.localDimensions = { x: 0, y: 0, z: 0 };

    that.worldRotation = Quat.fromPitchYawRollDegrees(0, 0, 0);
    that.worldPosition = { x: 0, y: 0, z: 0 };
    that.worldDimensions = { x: 0, y: 0, z: 0 };
    that.centerPosition = { x: 0, y: 0, z: 0 };

    that.saveProperties = function() {
        that.savedProperties = {};
        for (var i = 0; i < that.selections.length; i++) {
            var entityID = that.selections[i];
            that.savedProperties[entityID.id] = Entities.getEntityProperties(entityID);
        }
    };

    that.setEventListener = function(func) {
        that.eventListener = func;
    };

    that.addEntity = function(entityID) {
        print("Adding: " + entityID.id);
        var idx = that.selections.indexOf(entityID);
        if (idx == -1) {
            that.selections.push(entityID);
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
            SelectionDisplay.setSpaceMode(SPACE_LOCAL);

            var properties = Entities.getEntityProperties(that.selections[0]);
            that.localDimensions = properties.dimensions;
            that.localPosition = properties.position;
            that.localRotation = properties.rotation;

            that.worldDimensions = properties.boundingBox.dimensions;
            that.worldPosition = properties.boundingBox.center;
        } else {
            // For 1+ selections we can only modify selections in world space
            SelectionDisplay.setSpaceMode(SPACE_WORLD);

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
        }

        if (that.eventListener) {
            that.eventListener();
        }
    };

    return that;
})();

SelectionDisplay = (function () {
    var that = {};
    
    var MINIMUM_DIMENSION = 0.001;

    var GRABBER_DISTANCE_TO_SIZE_RATIO = 0.015;
    
    var spaceMode = SPACE_LOCAL;
    var mode = "UNKNOWN";
    var overlayNames = new Array();
    var lastCameraPosition = Camera.getPosition();
    var lastCameraOrientation = Camera.getOrientation();
    var lastPlaneIntersection;

    var currentSelection = { id: -1, isKnownID: false };
    var entitySelected = false;
    var selectedEntityProperties;
    var selectedEntityPropertiesOriginalPosition;
    var selectedEntityPropertiesOriginalDimensions;

    var handleHoverColor = { red: 224, green: 67, blue: 36 };
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
    

    var rotateHandleColor = { red: 0, green: 0, blue: 0 };
    var rotateHandleAlpha = 0.7;

    var highlightedHandleColor = { red: 255, green: 0, blue: 0 };
    var highlightedHandleAlpha = 0.7;
    
    var previousHandle = false;
    var previousHandleColor;
    var previousHandleAlpha;

    var grabberSizeCorner = 0.025;
    var grabberSizeEdge = 0.015;
    var grabberSizeFace = 0.025;
    var grabberAlpha = 1;
    var grabberColorCorner = { red: 120, green: 120, blue: 120 };
    var grabberColorEdge = { red: 0, green: 0, blue: 0 };
    var grabberColorFace = { red: 120, green: 120, blue: 120 };
    var grabberLineWidth = 0.5;
    var grabberSolid = true;

    var grabberPropertiesCorner = {
                position: { x:0, y: 0, z: 0},
                size: grabberSizeCorner,
                color: grabberColorCorner,
                alpha: 1,
                solid: grabberSolid,
                visible: false,
                dashed: false,
                lineWidth: grabberLineWidth,
            };

    var grabberPropertiesEdge = {
                position: { x:0, y: 0, z: 0},
                size: grabberSizeEdge,
                color: grabberColorEdge,
                alpha: 1,
                solid: grabberSolid,
                visible: false,
                dashed: false,
                lineWidth: grabberLineWidth,
            };

    var grabberPropertiesFace = {
                position: { x:0, y: 0, z: 0},
                size: grabberSizeFace,
                color: grabberColorFace,
                alpha: 1,
                solid: grabberSolid,
                visible: false,
                dashed: false,
                lineWidth: grabberLineWidth,
            };
    
    var highlightBox = Overlays.addOverlay("cube", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 180, green: 180, blue: 180},
                    alpha: 1,
                    solid: false,
                    visible: false,
                    dashed: true,
                    lineWidth: 1.0,
                    ignoreRayIntersection: true // this never ray intersects
                });

    var selectionBox = Overlays.addOverlay("cube", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 180, green: 180, blue: 180},
                    alpha: 1,
                    solid: false,
                    visible: false,
                    dashed: true,
                    lineWidth: 1.0,
                });

    var grabberMoveUp = Overlays.addOverlay("billboard", {
                    url: HIFI_PUBLIC_BUCKET + "images/up-arrow.png",
                    position: { x:0, y: 0, z: 0},
                    color: { red: 0, green: 0, blue: 0 },
                    alpha: 1.0,
                    visible: false,
                    size: 0.1,
                    scale: 0.1,
                    isFacingAvatar: true
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
    ];


    var baseOverlayAngles = { x: 0, y: 0, z: 0 };
    var baseOverlayRotation = Quat.fromVec3Degrees(baseOverlayAngles);
    var baseOfEntityProjectionOverlay = Overlays.addOverlay("rectangle3d", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 51, green: 152, blue: 203 },
                    alpha: 0.5,
                    solid: true,
                    visible: false,
                    rotation: baseOverlayRotation,
                    ignoreRayIntersection: true, // always ignore this
                });

    var yawOverlayAngles = { x: 90, y: 0, z: 0 };
    var yawOverlayRotation = Quat.fromVec3Degrees(yawOverlayAngles);
    var pitchOverlayAngles = { x: 0, y: 90, z: 0 };
    var pitchOverlayRotation = Quat.fromVec3Degrees(pitchOverlayAngles);
    var rollOverlayAngles = { x: 0, y: 180, z: 0 };
    var rollOverlayRotation = Quat.fromVec3Degrees(rollOverlayAngles);

    var xRailOverlay = Overlays.addOverlay("line3d", {
                    visible: false,
                    lineWidth: 1.0,
                    start: { x: 0, y: 0, z: 0 },
                    end: { x: 0, y: 0, z: 0 },
                    color: { red: 255, green: 0, blue: 0 },
                    ignoreRayIntersection: true, // always ignore this
                    visible: false,
                });
    var yRailOverlay = Overlays.addOverlay("line3d", {
                    visible: false,
                    lineWidth: 1.0,
                    start: { x: 0, y: 0, z: 0 },
                    end: { x: 0, y: 0, z: 0 },
                    color: { red: 0, green: 255, blue: 0 },
                    ignoreRayIntersection: true, // always ignore this
                    visible: false,
                });
    var zRailOverlay = Overlays.addOverlay("line3d", {
                    visible: false,
                    lineWidth: 1.0,
                    start: { x: 0, y: 0, z: 0 },
                    end: { x: 0, y: 0, z: 0 },
                    color: { red: 0, green: 0, blue: 255 },
                    ignoreRayIntersection: true, // always ignore this
                    visible: false,
                });

    var rotateZeroOverlay = Overlays.addOverlay("line3d", {
                    visible: false,
                    lineWidth: 2.0,
                    start: { x: 0, y: 0, z: 0 },
                    end: { x: 0, y: 0, z: 0 },
                    color: { red: 255, green: 0, blue: 0 },
                    ignoreRayIntersection: true, // always ignore this
                });

    var rotateCurrentOverlay = Overlays.addOverlay("line3d", {
                    visible: false,
                    lineWidth: 2.0,
                    start: { x: 0, y: 0, z: 0 },
                    end: { x: 0, y: 0, z: 0 },
                    color: { red: 0, green: 0, blue: 255 },
                    ignoreRayIntersection: true, // always ignore this
                });


    var rotateOverlayTarget = Overlays.addOverlay("circle3d", {
                    position: { x:0, y: 0, z: 0},
                    size: rotateOverlayTargetSize,
                    color: { red: 0, green: 0, blue: 0 },
                    alpha: 0.0,
                    solid: true,
                    visible: false,
                    rotation: yawOverlayRotation,
                });

    var rotateOverlayInner = Overlays.addOverlay("circle3d", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 51, green: 152, blue: 203 },
                    alpha: 0.2,
                    solid: true,
                    visible: false,
                    rotation: yawOverlayRotation,
                    hasTickMarks: true,
                    majorTickMarksAngle: innerSnapAngle,
                    minorTickMarksAngle: 0,
                    majorTickMarksLength: -0.25,
                    minorTickMarksLength: 0,
                    majorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    minorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    ignoreRayIntersection: true, // always ignore this
                });

    var rotateOverlayOuter = Overlays.addOverlay("circle3d", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 51, green: 152, blue: 203 },
                    alpha: 0.2,
                    solid: true,
                    visible: false,
                    rotation: yawOverlayRotation,

                    hasTickMarks: true,
                    majorTickMarksAngle: 45.0,
                    minorTickMarksAngle: 5,
                    majorTickMarksLength: 0.25,
                    minorTickMarksLength: 0.1,
                    majorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    minorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    ignoreRayIntersection: true, // always ignore this
                });

    var rotateOverlayCurrent = Overlays.addOverlay("circle3d", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 224, green: 67, blue: 36},
                    alpha: 0.8,
                    solid: true,
                    visible: false,
                    rotation: yawOverlayRotation,
                    ignoreRayIntersection: true, // always ignore this
                    hasTickMarks: true,
                    majorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    minorTickMarksColor: { red: 0, green: 0, blue: 0 },
                });

    var yawHandle = Overlays.addOverlay("billboard", {
                                        url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-north.png",
                                        position: { x:0, y: 0, z: 0},
                                        color: rotateHandleColor,
                                        alpha: rotateHandleAlpha,
                                        visible: false,
                                        size: 0.1,
                                        scale: 0.1,
                                        isFacingAvatar: false
                                      });


    var pitchHandle = Overlays.addOverlay("billboard", {
                                        url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-north.png",
                                        position: { x:0, y: 0, z: 0},
                                        color: rotateHandleColor,
                                        alpha: rotateHandleAlpha,
                                        visible: false,
                                        size: 0.1,
                                        scale: 0.1,
                                        isFacingAvatar: false
                                      });


    var rollHandle = Overlays.addOverlay("billboard", {
                                        url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-north.png",
                                        position: { x:0, y: 0, z: 0},
                                        color: rotateHandleColor,
                                        alpha: rotateHandleAlpha,
                                        visible: false,
                                        size: 0.1,
                                        scale: 0.1,
                                        isFacingAvatar: false
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
        xRailOverlay,
        yRailOverlay,
        zRailOverlay,
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
    function addGrabberTool(overlay, tool) {
        grabberTools[overlay] = {
            mode: tool.mode,
            onBegin: tool.onBegin,
            onMove: tool.onMove,
            onEnd: tool.onEnd,
        }
    }

    
    that.cleanup = function () {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.deleteOverlay(allOverlays[i]);
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
        Overlays.editOverlay(highlightBox,{ visible: false});
    };

    that.select = function(entityID, event) {
        var properties = Entities.getEntityProperties(entityID);
        // if (currentSelection.isKnownID == true) {
        //     that.unselect(currentSelection);
        // }
        currentSelection = entityID;
        entitySelected = true;
        
        lastCameraPosition = Camera.getPosition();
        lastCameraOrientation = Camera.getOrientation();

        if (event !== false) {
            selectedEntityProperties = properties;
            selectedEntityPropertiesOriginalPosition = properties.position;
            selectedEntityPropertiesOriginalDimensions = properties.dimensions;
            pickRay = Camera.computePickRay(event.x, event.y);
            lastPlaneIntersection = rayPlaneIntersection(pickRay, properties.position, Quat.getFront(lastCameraOrientation));

            var wantDebug = false;
            if (wantDebug) {
                print("select() with EVENT...... ");
                print("                event.y:" + event.y);
                Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
                Vec3.print("       originalPosition:", selectedEntityPropertiesOriginalPosition);
                Vec3.print("       originalDimensions:", selectedEntityPropertiesOriginalDimensions);
                Vec3.print("       current position:", properties.position);
            }
            
            
        }

        var diagonal = (Vec3.length(selectionManager.worldDimensions) / 2) * 1.1;
        var halfDimensions = Vec3.multiply(selectionManager.worldDimensions, 0.5);
        innerRadius = diagonal;
        outerRadius = diagonal * 1.15;
        var innerActive = false;
        var innerAlpha = 0.2;
        var outerAlpha = 0.2;
        if (innerActive) {
            innerAlpha = 0.5;
        } else {
            outerAlpha = 0.5;
        }

        var rotateHandleOffset = 0.05;
        var grabberMoveUpOffset = 0.1;
        
        var top, far, left, bottom, near, right, boundsCenter, objectCenter, BLN, BRN, BLF, TLN, TRN, TLF, TRF;

        // objectCenter = { x: properties.position.x, y: properties.position.y, z: properties.position.z };

        var dimensions;
        if (spaceMode == SPACE_LOCAL) {
            objectCenter = SelectionManager.localPosition;
            dimensions = SelectionManager.localDimensions;
        } else {
            objectCenter = SelectionManager.worldPosition;
            dimensions = SelectionManager.worldDimensions;
        }
        objectCenter = SelectionManager.worldPosition;
        dimensions = SelectionManager.worldDimensions;

        top = objectCenter.y + (dimensions.y / 2);
        far = objectCenter.z + (dimensions.z / 2);
        left = objectCenter.x + (dimensions.x / 2);

        bottom = objectCenter.y - (dimensions.y / 2);
        near = objectCenter.z - (dimensions.z / 2);
        right = objectCenter.x - (dimensions.x / 2);

        // boundsCenter = { x: properties.boundingBox.center.x, y: properties.boundingBox.center.y, z: properties.boundingBox.center.z };
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

                yawHandleRotation = Quat.fromVec3Degrees({ x: 270, y: 90, z: 0 });
                pitchHandleRotation = Quat.fromVec3Degrees({ x: 0, y: 90, z: 0 });
                rollHandleRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 0 });

                yawNormal   = { x: 0, y: 1, z: 0 };
                pitchNormal  = { x: 1, y: 0, z: 0 };
                rollNormal = { x: 0, y: 0, z: 1 };

                yawCorner = { x: left + rotateHandleOffset,
                              y: bottom - rotateHandleOffset,
                              z: near - rotateHandleOffset };

                pitchCorner = { x: right - rotateHandleOffset,
                               y: top + rotateHandleOffset,
                               z: near - rotateHandleOffset};

                rollCorner = { x: left + rotateHandleOffset,
                                y: top + rotateHandleOffset,
                                z: far + rotateHandleOffset };

                yawCenter = { x: boundsCenter.x, y: bottom, z: boundsCenter.z };
                pitchCenter = { x: right, y: boundsCenter.y, z: boundsCenter.z};
                rollCenter = { x: boundsCenter.x, y: boundsCenter.y, z: far };
                

                Overlays.editOverlay(pitchHandle, { url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-south.png" });
                Overlays.editOverlay(rollHandle, { url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-south.png" });
                

            } else {
            
                yawHandleRotation = Quat.fromVec3Degrees({ x: 270, y: 0, z: 0 });
                pitchHandleRotation = Quat.fromVec3Degrees({ x: 180, y: 270, z: 0 });
                rollHandleRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 90 });

                yawNormal   = { x: 0, y: 1, z: 0 };
                pitchNormal = { x: 1, y: 0, z: 0 };
                rollNormal  = { x: 0, y: 0, z: 1 };


                yawCorner = { x: left + rotateHandleOffset,
                              y: bottom - rotateHandleOffset,
                              z: far + rotateHandleOffset };

                pitchCorner = { x: right - rotateHandleOffset,
                                y: top + rotateHandleOffset,
                                z: far + rotateHandleOffset };

                rollCorner = { x: left + rotateHandleOffset,
                               y: top + rotateHandleOffset,
                               z: near - rotateHandleOffset};


                yawCenter = { x: boundsCenter.x, y: bottom, z: boundsCenter.z };
                pitchCenter = { x: right, y: boundsCenter.y, z: boundsCenter.z };
                rollCenter = { x: boundsCenter.x, y: boundsCenter.y, z: near};

                Overlays.editOverlay(pitchHandle, { url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-north.png" });
                Overlays.editOverlay(rollHandle, { url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-north.png" });
            }
        } else {
        
            // must be BLF or BLN
            if (cameraPosition.z < objectCenter.z) {
            
                yawHandleRotation = Quat.fromVec3Degrees({ x: 270, y: 180, z: 0 });
                pitchHandleRotation = Quat.fromVec3Degrees({ x: 90, y: 0, z: 90 });
                rollHandleRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 180 });

                yawNormal   = { x: 0, y: 1, z: 0 };
                pitchNormal = { x: 1, y: 0, z: 0 };
                rollNormal  = { x: 0, y: 0, z: 1 };

                yawCorner = { x: right - rotateHandleOffset,
                              y: bottom - rotateHandleOffset,
                              z: near - rotateHandleOffset };

                pitchCorner = { x: left + rotateHandleOffset,
                                y: top + rotateHandleOffset,
                                z: near - rotateHandleOffset };

                rollCorner = { x: right - rotateHandleOffset,
                               y: top + rotateHandleOffset,
                               z: far + rotateHandleOffset};

                yawCenter = { x: boundsCenter.x, y: bottom, z: boundsCenter.z };
                pitchCenter = { x: left, y: boundsCenter.y, z: boundsCenter.z };
                rollCenter = { x: boundsCenter.x, y: boundsCenter.y, z: far};

                Overlays.editOverlay(pitchHandle, { url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-north.png" });
                Overlays.editOverlay(rollHandle, { url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-north.png" });

            } else {
            
                yawHandleRotation = Quat.fromVec3Degrees({ x: 270, y: 270, z: 0 });
                rollHandleRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 180 });
                pitchHandleRotation = Quat.fromVec3Degrees({ x: 180, y: 270, z: 0 });

                yawNormal   = { x: 0, y: 1, z: 0 };
                rollNormal = { x: 0, y: 0, z: 1 };
                pitchNormal  = { x: 1, y: 0, z: 0 };

                yawCorner = { x: right - rotateHandleOffset,
                              y: bottom - rotateHandleOffset,
                              z: far + rotateHandleOffset };

                rollCorner = { x: right - rotateHandleOffset,
                                y: top + rotateHandleOffset,
                                z: near - rotateHandleOffset };

                pitchCorner = { x: left + rotateHandleOffset,
                               y: top + rotateHandleOffset,
                               z: far + rotateHandleOffset};

                yawCenter = { x: boundsCenter.x, y: bottom, z: boundsCenter.z };
                rollCenter = { x: boundsCenter.x, y: boundsCenter.y, z: near };
                pitchCenter = { x: left, y: boundsCenter.y, z: boundsCenter.z};

                Overlays.editOverlay(pitchHandle, { url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-north.png" });
                Overlays.editOverlay(rollHandle, { url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/rotate-arrow-west-north.png" });

            }
        }
        
        var rotateHandlesVisible = true;
        var translateHandlesVisible = true;
        var stretchHandlesVisible = true;
        var selectionBoxVisible = true;
        if (mode == "ROTATE_YAW" || mode == "ROTATE_PITCH" || mode == "ROTATE_ROLL" || mode == "TRANSLATE_X in case they Z") {
            rotateHandlesVisible = false;
            translateHandlesVisible = false;
            stretchHandlesVisible = false;
            selectionBoxVisible = false;
        } else if (mode == "TRANSLATE_UP_DOWN") {
            rotateHandlesVisible = false;
            stretchHandlesVisible = false;
        } else if (mode != "UNKNOWN") {
            // every other mode is a stretch mode...
            rotateHandlesVisible = false;
            translateHandlesVisible = false;
        }

        Overlays.editOverlay(highlightBox, { visible: false });
        
        var dimensions, rotation, position;
        if (spaceMode == SPACE_LOCAL) {
            rotation = SelectionManager.localRotation;
            dimensions = SelectionManager.localDimensions;
            position = SelectionManager.localPosition;
        } else {
            rotation = SelectionManager.worldRotation;
            dimensions = SelectionManager.worldDimensions;
            position = SelectionManager.worldPosition;
        }

        Overlays.editOverlay(grabberMoveUp, { visible: translateHandlesVisible, position: { x: boundsCenter.x, y: top + grabberMoveUpOffset, z: boundsCenter.z } });

        that.updateHandles(entityID);


        Overlays.editOverlay(baseOfEntityProjectionOverlay, 
                            { 
                                visible: true,
                                solid:true,
                                lineWidth: 2.0,
                                position: { x: properties.position.x,
                                            y: 0,
                                            z: properties.position.z },

                                dimensions: { x: properties.dimensions.x, y: properties.dimensions.z },
                                rotation: properties.rotation,
                            });

                            
        Overlays.editOverlay(rotateOverlayTarget, { visible: false });

        Overlays.editOverlay(rotateOverlayInner, 
                            { 
                                visible: false,
                                size: innerRadius,
                                innerRadius: 0.9,
                                alpha: innerAlpha
                            });

        Overlays.editOverlay(rotateOverlayOuter, 
                            { 
                                visible: false,
                                size: outerRadius,
                                innerRadius: 0.9,
                                startAt: 0,
                                endAt: 360,
                                alpha: outerAlpha,
                            });

        Overlays.editOverlay(rotateOverlayCurrent, 
                            { 
                                visible: false,
                                size: outerRadius,
                                startAt: 0,
                                endAt: 0,
                                innerRadius: 0.9,
                            });
                            
        Overlays.editOverlay(rotateZeroOverlay, { visible: false });
        Overlays.editOverlay(rotateCurrentOverlay, { visible: false });

        // TODO: we have not implemented the rotating handle/controls yet... so for now, these handles are hidden
        Overlays.editOverlay(yawHandle, { visible: rotateHandlesVisible, position: yawCorner, rotation: yawHandleRotation});
        Overlays.editOverlay(pitchHandle, { visible: rotateHandlesVisible, position: pitchCorner, rotation: pitchHandleRotation});
        Overlays.editOverlay(rollHandle, { visible: rotateHandlesVisible, position: rollCorner, rotation: rollHandleRotation});

        Entities.editEntity(entityID, { localRenderAlpha: 0.1 });
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

    that.unselectAll = function () {
        if (currentSelection.isKnownID == true) {
            that.unselect(currentSelection);
        }
        currentSelection = { id: -1, isKnownID: false };
        entitySelected = false;
    };

    that.updateHandles = function(entityID) {
        if (SelectionManager.selections.length == 0) {
            that.setOverlaysVisible(false);
            return;
        }

        var properties = Entities.getEntityProperties(entityID);

        var rotation, dimensions, position;

        if (spaceMode == SPACE_LOCAL) {
            rotation = properties.rotation;
            dimensions = SelectionManager.localDimensions;
            position = SelectionManager.localPosition;
        } else {
            rotation = Quat.fromPitchYawRollDegrees(0, 0, 0);
            dimensions = SelectionManager.worldDimensions;
            position = SelectionManager.worldPosition;
        }

        var halfDimensions = Vec3.multiply(0.5, dimensions);

        left = -halfDimensions.x;
        right = halfDimensions.x;
        top = halfDimensions.y;
        bottom = -halfDimensions.y;
        var front = far = halfDimensions.z;
        near = -halfDimensions.z;

        var LBN = { x: left, y: bottom, z: near };
        var RBN = { x: right, y: bottom, z: near };
        var LBF = { x: left, y: bottom, z: far };
        var RBF = { x: right, y: bottom, z: far };
        var LTN = { x: left, y: top, z: near };
        var RTN = { x: right, y: top, z: near };
        var LTF = { x: left, y: top, z: far };
        var RTF = { x: right, y: top, z: far };

        var TOP = { x: 0, y: top, z: 0 };
        var BOTTOM = { x: 0, y: bottom, z: 0 };
        var LEFT = { x: left, y: 0, z: 0 };
        var RIGHT = { x: right, y: 0, z: 0 };
        var NEAR = { x: 0, y: 0, z: near };
        var FAR = { x: 0, y: 0, z: far };

        var EdgeTR = { x: right, y: top, z: 0 };
        var EdgeTL = { x: left, y: top, z: 0 };
        var EdgeTF = { x: 0, y: top, z: front };
        var EdgeTN = { x: 0, y: top, z: near };
        var EdgeBR = { x: right, y: bottom, z: 0 };
        var EdgeBL = { x: left, y: bottom, z: 0 };
        var EdgeBF = { x: 0, y: bottom, z: front };
        var EdgeBN = { x: 0, y: bottom, z: near };
        var EdgeNR = { x: right, y: 0, z: near };
        var EdgeNL = { x: left, y: 0, z: near };
        var EdgeFR = { x: right, y: 0, z: front };
        var EdgeFL = { x: left, y: 0, z: front };

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
        Overlays.editOverlay(grabberLBN, { visible: stretchHandlesVisible, rotation: rotation, position: LBN });
        Overlays.editOverlay(grabberRBN, { visible: stretchHandlesVisible, rotation: rotation, position: RBN });
        Overlays.editOverlay(grabberLBF, { visible: stretchHandlesVisible, rotation: rotation, position: LBF });
        Overlays.editOverlay(grabberRBF, { visible: stretchHandlesVisible, rotation: rotation, position: RBF });
        Overlays.editOverlay(grabberLTN, { visible: stretchHandlesVisible, rotation: rotation, position: LTN });
        Overlays.editOverlay(grabberRTN, { visible: stretchHandlesVisible, rotation: rotation, position: RTN });
        Overlays.editOverlay(grabberLTF, { visible: stretchHandlesVisible, rotation: rotation, position: LTF });
        Overlays.editOverlay(grabberRTF, { visible: stretchHandlesVisible, rotation: rotation, position: RTF });

        Overlays.editOverlay(grabberTOP, { visible: stretchHandlesVisible, rotation: rotation, position: TOP });
        Overlays.editOverlay(grabberBOTTOM, { visible: stretchHandlesVisible, rotation: rotation, position: BOTTOM });
        Overlays.editOverlay(grabberLEFT, { visible: stretchHandlesVisible, rotation: rotation, position: LEFT });
        Overlays.editOverlay(grabberRIGHT, { visible: stretchHandlesVisible, rotation: rotation, position: RIGHT });
        Overlays.editOverlay(grabberNEAR, { visible: stretchHandlesVisible, rotation: rotation, position: NEAR });
        Overlays.editOverlay(grabberFAR, { visible: stretchHandlesVisible, rotation: rotation, position: FAR });

        Overlays.editOverlay(selectionBox, {
            position: position,
            dimensions: dimensions,
            rotation: rotation,
            visible: true,
        });

        Overlays.editOverlay(grabberEdgeTR, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeTR });
        Overlays.editOverlay(grabberEdgeTL, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeTL });
        Overlays.editOverlay(grabberEdgeTF, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeTF });
        Overlays.editOverlay(grabberEdgeTN, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeTN });
        Overlays.editOverlay(grabberEdgeBR, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeBR });
        Overlays.editOverlay(grabberEdgeBL, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeBL });
        Overlays.editOverlay(grabberEdgeBF, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeBF });
        Overlays.editOverlay(grabberEdgeBN, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeBN });
        Overlays.editOverlay(grabberEdgeNR, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeNR });
        Overlays.editOverlay(grabberEdgeNL, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeNL });
        Overlays.editOverlay(grabberEdgeFR, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeFR });
        Overlays.editOverlay(grabberEdgeFL, { visible: stretchHandlesVisible, rotation: rotation, position: EdgeFL });

    };

    that.setOverlaysVisible = function(isVisible) {
        var length = allOverlays.length;
        for (var i = 0; i < length; i++) {
            Overlays.editOverlay(allOverlays[i], { visible: isVisible });
        }
    };

    that.unselect = function (entityID) {
        that.setOverlaysVisible(false);

        Entities.editEntity(entityID, { localRenderAlpha: 1.0 });

        currentSelection = { id: -1, isKnownID: false };
        entitySelected = false;
    };

    var lastXZPick = null;
    var translateXZTool = {
        mode: 'TRANSLATE_XZ',
        onBegin: function(event) {
            SelectionManager.saveProperties();
            var position = SelectionManager.worldPosition;
            var dimensions = SelectionManager.worldDimensions;
            var bottom = position.y - (dimensions.y / 2)

            var pickRay = Camera.computePickRay(event.x, event.y);
            lastXZPick = rayPlaneIntersection(pickRay, position, { x: 0, y: 1, z: 0 });
        },
        onEnd: function(event, reason) {
            if (reason == 'cancel') {
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var initialProperties = SelectionManager.savedProperties[entityID.id];
                    Entities.editEntity(entityID, initialProperties);
                }
            }
        },
        onMove: function(event) {
            if (!entitySelected || mode !== "TRANSLATE_XZ") {
                return; // not allowed
            }

            pickRay = Camera.computePickRay(event.x, event.y);

            // translate mode left/right based on view toward entity
            var newIntersection = rayPlaneIntersection(pickRay,
                                                       selectedEntityPropertiesOriginalPosition,
                                                       Quat.getFront(lastCameraOrientation));

            var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

            var pickRay = Camera.computePickRay(event.x, event.y);
            var pick = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, { x: 0, y: 1, z: 0 });
            vector = Vec3.subtract(pick, lastXZPick);
            lastXZPick = pick;

            var wantDebug = false;

            for (var i = 0; i < SelectionManager.selections.length; i++) {
                var properties = Entities.getEntityProperties(SelectionManager.selections[i]);
                var original = properties.position;
                properties.position = Vec3.sum(properties.position, vector);
                Entities.editEntity(SelectionManager.selections[i], properties);

                if (wantDebug) {
                    print("translateXZ... ");
                    Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
                    Vec3.print("        newIntersection:", newIntersection);
                    Vec3.print("                 vector:", vector);
                    Vec3.print("       originalPosition:", original);
                    Vec3.print("            newPosition:", properties.position);
                    Vec3.print("            newPosition:", newPosition);
                }
            }

            tooltip.updateText(selectedEntityProperties);
            that.select(currentSelection, false); // TODO: this should be more than highlighted
            SelectionManager._update();
        }
    };
    
    addGrabberTool(grabberMoveUp, {
        mode: "TRANSLATE_UP_DOWN",
        onBegin: function(event) {
            SelectionManager.saveProperties();
        },
        onEnd: function(event, reason) {
            if (reason == 'cancel') {
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var initialProperties = SelectionManager.savedProperties[entityID.id];
                    Entities.editEntity(entityID, initialProperties);
                }
            }
        },
        onMove: function(event) {
            if (!entitySelected || mode !== "TRANSLATE_UP_DOWN") {
                return; // not allowed
            }
            pickRay = Camera.computePickRay(event.x, event.y);

            // translate mode left/right based on view toward entity
            var newIntersection = rayPlaneIntersection(pickRay,
                                                       selectedEntityPropertiesOriginalPosition,
                                                       Quat.getFront(lastCameraOrientation));

            var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);
            lastPlaneIntersection = newIntersection;
            
            // we only care about the Y axis
            vector.x = 0;
            vector.z = 0;
            
            // newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, vector);

            var wantDebug = false;
            if (wantDebug) {
                print("translateUpDown... ");
                print("                event.y:" + event.y);
                Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
                Vec3.print("        newIntersection:", newIntersection);
                Vec3.print("                 vector:", vector);
                Vec3.print("       originalPosition:", selectedEntityPropertiesOriginalPosition);
                Vec3.print("         recentPosition:", selectedEntityProperties.position);
                Vec3.print("            newPosition:", newPosition);
            }
            for (var i = 0; i < SelectionManager.selections.length; i++) {
                var properties = Entities.getEntityProperties(SelectionManager.selections[i]);
                var original = properties.position;
                properties.position = Vec3.sum(properties.position, vector);
                Entities.editEntity(SelectionManager.selections[i], properties);
            }

            tooltip.updateText(selectedEntityProperties);
            that.select(currentSelection, false); // TODO: this should be more than highlighted
            SelectionManager._update();
        },
    });

    var vec3Mult = function(v1, v2) {
        return { x: v1.x * v2.x, y: v1.y * v2.y, z: v1.z * v2.z };
    }
    var makeStretchTool = function(stretchMode, direction, pivot) {
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
        var rotation = null;

        var onBegin = function(event) {
            print("STARTING: " + stretchMode);
            var properties = Entities.getEntityProperties(currentSelection);
            initialProperties = properties;
            rotation = spaceMode == SPACE_LOCAL ? properties.rotation : Quat.fromPitchYawRollDegrees(0, 0, 0);

            if (spaceMode == SPACE_LOCAL) {
                initialPosition = SelectionManager.localPosition;
                initialDimensions = SelectionManager.localDimensions;
            } else {
                initialPosition = SelectionManager.worldPosition;
                initialDimensions = SelectionManager.worldDimensions;
            }

            if (numDimensions == 1 && mask.x) {
                var start = Vec3.multiplyQbyV(rotation, { x: -10000, y: 0, z: 0 });
                start = Vec3.sum(start, properties.position);
                var end = Vec3.multiplyQbyV(rotation, { x: 10000, y: 0, z: 0 });
                end = Vec3.sum(end, properties.position);
                Overlays.editOverlay(xRailOverlay, {
                    start: start,
                    end: end,
                    visible: true,
                });
            }
            if (numDimensions == 1 && mask.y) {
                var start = Vec3.multiplyQbyV(rotation, { x: 0, y: -10000, z: 0 });
                start = Vec3.sum(start, properties.position);
                var end = Vec3.multiplyQbyV(rotation, { x: 0, y: 10000, z: 0 });
                end = Vec3.sum(end, properties.position);
                Overlays.editOverlay(yRailOverlay, {
                    start: start,
                    end: end,
                    visible: true,
                });
            }
            if (numDimensions == 1 && mask.z) {
                var start = Vec3.multiplyQbyV(rotation, { x: 0, y: 0, z: -10000 });
                start = Vec3.sum(start, properties.position);
                var end = Vec3.multiplyQbyV(rotation, { x: 0, y: 0, z: 10000 });
                end = Vec3.sum(end, properties.position);
                Overlays.editOverlay(zRailOverlay, {
                    start: start,
                    end: end,
                    visible: true,
                });
            }
            if (numDimensions == 1) {
                if (mask.x == 1) {
                    planeNormal = { x: 0, y: 1, z: 0 };
                } else if (mask.y == 1) {
                    planeNormal = { x: 1, y: 0, z: 0 };
                } else {
                    planeNormal = { x: 0, y: 1, z: 0 };
                }
            } else if (numDimensions == 2) {
                if (mask.x == 0) {
                    planeNormal = { x: 1, y: 0, z: 0 };
                } else if (mask.y == 0) {
                    planeNormal = { x: 0, y: 1, z: 0 };
                } else {
                    planeNormal = { x: 0, y: 0, z: z };
                }
            }
            planeNormal = Vec3.multiplyQbyV(rotation, planeNormal);
            var pickRay = Camera.computePickRay(event.x, event.y);
            lastPick = rayPlaneIntersection(pickRay,
                                            initialPosition,
                                            planeNormal);

            // Overlays.editOverlay(normalLine, {
            //     start: initialPosition,
            //     end: Vec3.sum(Vec3.multiply(100000, planeNormal), initialPosition),
            // });

            SelectionManager.saveProperties();
        };

        var onEnd = function(event, reason) {
            print("ENDING: " + stretchMode);
            Overlays.editOverlay(xRailOverlay, { visible: false });
            Overlays.editOverlay(yRailOverlay, { visible: false });
            Overlays.editOverlay(zRailOverlay, { visible: false });

            if (reason == 'cancel') {
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var initialProperties = SelectionManager.savedProperties[entityID.id];
                    Entities.editEntity(entityID, initialProperties);
                }
            }
        };

        var onMove = function(event) {
            if (!entitySelected || mode !== stretchMode) {
                return; // not allowed
            }
            var proportional = spaceMode == SPACE_WORLD || event.isShifted;

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

            var pickRay = Camera.computePickRay(event.x, event.y);
            newPick = rayPlaneIntersection(pickRay,
                                           initialPosition,
                                           planeNormal);
            var vector = Vec3.subtract(newPick, lastPick);

            vector = Vec3.multiplyQbyV(Quat.inverse(rotation), vector);

            vector = vec3Mult(mask, vector);

            var changeInDimensions = Vec3.multiply(-1, vec3Mult(signs, vector));
            var newDimensions;
            if (proportional) {
                var absX = Math.abs(changeInDimensions.x);
                var absY = Math.abs(changeInDimensions.y);
                var absZ = Math.abs(changeInDimensions.z);
                print('abs: ' + absX + ', ' + absY + ', ' + absZ);
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
                print('change: ' + pctChange);
                pctChange += 1.0;
                newDimensions = Vec3.multiply(pctChange, initialDimensions);
            } else {
                newDimensions = Vec3.sum(initialDimensions, changeInDimensions);
            }
            
            newDimensions.x = Math.max(newDimensions.x, MINIMUM_DIMENSION);
            newDimensions.y = Math.max(newDimensions.y, MINIMUM_DIMENSION);
            newDimensions.z = Math.max(newDimensions.z, MINIMUM_DIMENSION);
            
            var p = Vec3.multiply(0.5, pivot);
            var changeInPosition = Vec3.multiplyQbyV(rotation, vec3Mult(p, changeInDimensions));
            var newPosition = Vec3.sum(initialPosition, changeInPosition);
            
            
            selectedEntityProperties.position = newPosition;
            selectedEntityProperties.dimensions = newDimensions;
            for (var i = 0; i < SelectionManager.selections.length; i++) {
                Entities.editEntity(SelectionManager.selections[i], selectedEntityProperties);
            }

            var wantDebug = false;
            if (wantDebug) {
                print(stretchMode);
                Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
                Vec3.print("        newIntersection:", newIntersection);
                Vec3.print("                 vector:", vector);
                Vec3.print("           oldPOS:", oldPOS);
                Vec3.print("                newPOS:", newPOS);
                Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
                Vec3.print("            changeInDimensions:", changeInDimensions);
                Vec3.print("                 newDimensions:", newDimensions);

                Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
                Vec3.print("              changeInPosition:", changeInPosition);
                Vec3.print("                   newPosition:", newPosition);
            }

            tooltip.updateText(selectedEntityProperties);
            SelectionManager._update();
            that.select(currentSelection, false); // TODO: this should be more than highlighted

        };

        return {
            mode: stretchMode,
            onBegin: onBegin,
            onMove: onMove,
            onEnd: onEnd
        };
    };

    that.cancelTool = function() {
        if (activeTool) {
            activeTool.onEnd(null, 'cancel');
            activeTool = null;
            SelectionManager._update();
        }
    };

    function addStretchTool(overlay, mode, pivot, direction) {
        if (!pivot) {
            pivot = Vec3.multiply(-1, direction);
            pivot.y = direction.y;
        }
        var tool = makeStretchTool(mode, direction, pivot);

        addGrabberTool(overlay, tool);
    }

    addStretchTool(grabberNEAR, "STRETCH_NEAR", { x: 0, y: 0, z: -1 }, { x: 0, y: 0, z: 1 });
    addStretchTool(grabberFAR, "STRETCH_FAR", { x: 0, y: 0, z: 1 }, { x: 0, y: 0, z: -1 });
    addStretchTool(grabberTOP, "STRETCH_TOP", { x: 0, y: 1, z: 0 }, { x: 0, y: -1, z: 0 });
    addStretchTool(grabberBOTTOM, "STRETCH_BOTTOM", { x: 0, y: -1, z: 0 }, { x: 0, y: 1, z: 0 });
    addStretchTool(grabberRIGHT, "STRETCH_RIGHT", { x: 1, y: 0, z: 0 }, { x: -1, y: 0, z: 0 });
    addStretchTool(grabberLEFT, "STRETCH_LEFT", { x: -1, y: 0, z: 0 }, { x: 1, y: 0, z: 0 });

    addStretchTool(grabberLBN, "STRETCH_LBN", null, {x: 1, y: 0, z: 1});
    addStretchTool(grabberRBN, "STRETCH_RBN", null, {x: -1, y: 0, z: 1});
    addStretchTool(grabberLBF, "STRETCH_LBF", null, {x: 1, y: 0, z: -1});
    addStretchTool(grabberRBF, "STRETCH_RBF", null, {x: -1, y: 0, z: -1});
    addStretchTool(grabberLTN, "STRETCH_LTN", null, {x: 1, y: 0, z: 1});
    addStretchTool(grabberRTN, "STRETCH_RTN", null, {x: -1, y: 0, z: 1});
    addStretchTool(grabberLTF, "STRETCH_LTF", null, {x: 1, y: 0, z: -1});
    addStretchTool(grabberRTF, "STRETCH_RTF", null, {x: -1, y: 0, z: -1});

    addStretchTool(grabberEdgeTR, "STRETCH_EdgeTR", null, {x: 1, y: 1, z: 0});
    addStretchTool(grabberEdgeTL, "STRETCH_EdgeTL", null, {x: -1, y: 1, z: 0});
    addStretchTool(grabberEdgeTF, "STRETCH_EdgeTF", null, {x: 0, y: 1, z: -1});
    addStretchTool(grabberEdgeTN, "STRETCH_EdgeTN", null, {x: 0, y: 1, z: 1});
    addStretchTool(grabberEdgeBR, "STRETCH_EdgeBR", null, {x: 1, y: -1, z: 0});
    addStretchTool(grabberEdgeBL, "STRETCH_EdgeBL", null, {x: -1, y: -1, z: 0});
    addStretchTool(grabberEdgeBF, "STRETCH_EdgeBF", null, {x: 0, y: -1, z: 1});
    addStretchTool(grabberEdgeBN, "STRETCH_EdgeBN", null, {x: 0, y: -1, z: -1});
    addStretchTool(grabberEdgeNR, "STRETCH_EdgeNR", null, {x: -1, y: 0, z: 1});
    addStretchTool(grabberEdgeNL, "STRETCH_EdgeNL", null, {x: 1, y: 0, z: 1});
    addStretchTool(grabberEdgeFR, "STRETCH_EdgeFR", null, {x: -1, y: 0, z: -1});
    addStretchTool(grabberEdgeFL, "STRETCH_EdgeFL", null, {x: 1, y: 0, z: -1});

    var initialPosition = SelectionManager.worldPosition;
    addGrabberTool(yawHandle, {
        mode: "ROTATE_YAW",
        onBegin: function(event) {
            SelectionManager.saveProperties();
            initialPosition = SelectionManager.worldPosition;
        },
        onEnd: function(event, reason) {
            if (reason == 'cancel') {
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var initialProperties = SelectionManager.savedProperties[entityID.id];
                    Entities.editEntity(entityID, initialProperties);
                }
            }
        },
        onMove: function(event) {
            if (!entitySelected || mode !== "ROTATE_YAW") {
                return; // not allowed
            }
            
            var debug = Menu.isOptionChecked("Debug Ryans Rotation Problems");

            if (debug) {
                print("rotateYaw()...");
                print("    event.x,y:" + event.x + "," + event.y);
            }

            var pickRay = Camera.computePickRay(event.x, event.y);
            Overlays.editOverlay(selectionBox, { ignoreRayIntersection: true, visible: false});
            Overlays.editOverlay(baseOfEntityProjectionOverlay, { ignoreRayIntersection: true, visible: false });
            Overlays.editOverlay(rotateOverlayTarget, { ignoreRayIntersection: false });
            Overlays.editOverlay(rotateOverlayInner, { ignoreRayIntersection: true });
            Overlays.editOverlay(rotateOverlayOuter, { ignoreRayIntersection: true });
            Overlays.editOverlay(rotateOverlayCurrent, { ignoreRayIntersection: true });
            
            var result = Overlays.findRayIntersection(pickRay);

            if (debug) {
                print("    findRayIntersection() .... result.intersects:" + result.intersects);
            }

            if (result.intersects) {
                var properties = Entities.getEntityProperties(currentSelection);
                var center = yawCenter;
                var zero = yawZero;
                var centerToZero = Vec3.subtract(center, zero);
                var centerToIntersect = Vec3.subtract(center, result.intersection);
                var angleFromZero = Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal);
                
                var distanceFromCenter = Vec3.distance(center, result.intersection);
                var snapToInner = false;
                if (distanceFromCenter < innerRadius) {
                    angleFromZero = Math.floor(angleFromZero/innerSnapAngle) * innerSnapAngle;
                    snapToInner = true;
                }
                
                // for debugging
                if (debug) {
                    Vec3.print("    result.intersection:",result.intersection);
                    Overlays.editOverlay(rotateCurrentOverlay, { visible: true, start: center,  end: result.intersection });
                    print("    angleFromZero:" + angleFromZero);
                }

                var yawChange = Quat.fromVec3Degrees({ x: 0, y: angleFromZero, z: 0 });
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var properties = Entities.getEntityProperties(entityID);
                    var initialProperties = SelectionManager.savedProperties[entityID.id];
                    var dPos = Vec3.subtract(initialProperties.position, initialPosition);
                    dPos = Vec3.multiplyQbyV(yawChange, dPos);

                    Entities.editEntity(entityID, {
                        position: Vec3.sum(initialPosition, dPos),
                        rotation: Quat.multiply(yawChange, initialProperties.rotation),
                    });
                }
                
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
                    Overlays.editOverlay(rotateOverlayOuter, { startAt: 0, endAt: 360 });
                    Overlays.editOverlay(rotateOverlayInner, { startAt: startAtRemainder, endAt: endAtRemainder });
                    Overlays.editOverlay(rotateOverlayCurrent, { startAt: startAtCurrent, endAt: endAtCurrent, size: innerRadius,
                                                                    majorTickMarksAngle: innerSnapAngle, minorTickMarksAngle: 0,
                                                                    majorTickMarksLength: -0.25, minorTickMarksLength: 0, });
                } else {
                    Overlays.editOverlay(rotateOverlayInner, { startAt: 0, endAt: 360 });
                    Overlays.editOverlay(rotateOverlayOuter, { startAt: startAtRemainder, endAt: endAtRemainder });
                    Overlays.editOverlay(rotateOverlayCurrent, { startAt: startAtCurrent, endAt: endAtCurrent, size: outerRadius,
                                                                    majorTickMarksAngle: 45.0, minorTickMarksAngle: 5,
                                                                    majorTickMarksLength: 0.25, minorTickMarksLength: 0.1, });
                }
                
            }
        }
    });

    addGrabberTool(pitchHandle, {
        mode: "ROTATE_PITCH",
        onBegin: function(event) {
            SelectionManager.saveProperties();
            initialPosition = SelectionManager.worldPosition;
        },
        onEnd: function(event, reason) {
            if (reason == 'cancel') {
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var initialProperties = SelectionManager.savedProperties[entityID.id];
                    Entities.editEntity(entityID, initialProperties);
                }
            }
        },
        onMove: function(event) {
            if (!entitySelected || mode !== "ROTATE_PITCH") {
                return; // not allowed
            }
            var debug = Menu.isOptionChecked("Debug Ryans Rotation Problems");

            if (debug) {
                print("rotatePitch()...");
                print("    event.x,y:" + event.x + "," + event.y);
            }

            var pickRay = Camera.computePickRay(event.x, event.y);
            Overlays.editOverlay(selectionBox, { ignoreRayIntersection: true, visible: false});
            Overlays.editOverlay(baseOfEntityProjectionOverlay, { ignoreRayIntersection: true, visible: false });
            Overlays.editOverlay(rotateOverlayTarget, { ignoreRayIntersection: false });
            Overlays.editOverlay(rotateOverlayInner, { ignoreRayIntersection: true });
            Overlays.editOverlay(rotateOverlayOuter, { ignoreRayIntersection: true });
            Overlays.editOverlay(rotateOverlayCurrent, { ignoreRayIntersection: true });
            var result = Overlays.findRayIntersection(pickRay);

            if (debug) {
                print("    findRayIntersection() .... result.intersects:" + result.intersects);
            }

            if (result.intersects) {
                var properties = Entities.getEntityProperties(currentSelection);
                var center = pitchCenter;
                var zero = pitchZero;
                var centerToZero = Vec3.subtract(center, zero);
                var centerToIntersect = Vec3.subtract(center, result.intersection);
                var angleFromZero = Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal);

                var distanceFromCenter = Vec3.distance(center, result.intersection);
                var snapToInner = false;
                if (distanceFromCenter < innerRadius) {
                    angleFromZero = Math.floor(angleFromZero/innerSnapAngle) * innerSnapAngle;
                    snapToInner = true;
                }
                
                // for debugging
                if (debug) {
                    Vec3.print("    result.intersection:",result.intersection);
                    Overlays.editOverlay(rotateCurrentOverlay, { visible: true, start: center,  end: result.intersection });
                    print("    angleFromZero:" + angleFromZero);
                }

                var pitchChange = Quat.fromVec3Degrees({ x: angleFromZero, y: 0, z: 0 });

                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var properties = Entities.getEntityProperties(entityID);
                    var initialProperties = SelectionManager.savedProperties[entityID.id];
                    var dPos = Vec3.subtract(initialProperties.position, initialPosition);
                    dPos = Vec3.multiplyQbyV(pitchChange, dPos);

                    Entities.editEntity(entityID, {
                        position: Vec3.sum(initialPosition, dPos),
                        rotation: Quat.multiply(pitchChange, initialProperties.rotation),
                    });
                }

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
                    Overlays.editOverlay(rotateOverlayOuter, { startAt: 0, endAt: 360 });
                    Overlays.editOverlay(rotateOverlayInner, { startAt: startAtRemainder, endAt: endAtRemainder });
                    Overlays.editOverlay(rotateOverlayCurrent, { startAt: startAtCurrent, endAt: endAtCurrent, size: innerRadius,
                                                                    majorTickMarksAngle: innerSnapAngle, minorTickMarksAngle: 0,
                                                                    majorTickMarksLength: -0.25, minorTickMarksLength: 0, });
                } else {
                    Overlays.editOverlay(rotateOverlayInner, { startAt: 0, endAt: 360 });
                    Overlays.editOverlay(rotateOverlayOuter, { startAt: startAtRemainder, endAt: endAtRemainder });
                    Overlays.editOverlay(rotateOverlayCurrent, { startAt: startAtCurrent, endAt: endAtCurrent, size: outerRadius,
                                                                    majorTickMarksAngle: 45.0, minorTickMarksAngle: 5,
                                                                    majorTickMarksLength: 0.25, minorTickMarksLength: 0.1, });
                }
            }
        }
    });

    addGrabberTool(rollHandle, {
        mode: "ROTATE_ROLL",
        onBegin: function(event) {
            SelectionManager.saveProperties();
            initialPosition = SelectionManager.worldPosition;
        },
        onEnd: function(event, reason) {
            if (reason == 'cancel') {
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var initialProperties = SelectionManager.savedProperties[entityID.id];
                    Entities.editEntity(entityID, initialProperties);
                }
            }
        },
        onMove: function(event) {
            if (!entitySelected || mode !== "ROTATE_ROLL") {
                return; // not allowed
            }
            var debug = Menu.isOptionChecked("Debug Ryans Rotation Problems");

            if (debug) {
                print("rotateRoll()...");
                print("    event.x,y:" + event.x + "," + event.y);
            }
            
            var pickRay = Camera.computePickRay(event.x, event.y);
            Overlays.editOverlay(selectionBox, { ignoreRayIntersection: true, visible: false});
            Overlays.editOverlay(baseOfEntityProjectionOverlay, { ignoreRayIntersection: true, visible: false });
            Overlays.editOverlay(rotateOverlayTarget, { ignoreRayIntersection: false });
            Overlays.editOverlay(rotateOverlayInner, { ignoreRayIntersection: true });
            Overlays.editOverlay(rotateOverlayOuter, { ignoreRayIntersection: true });
            Overlays.editOverlay(rotateOverlayCurrent, { ignoreRayIntersection: true });
            var result = Overlays.findRayIntersection(pickRay);

            if (debug) {
                print("    findRayIntersection() .... result.intersects:" + result.intersects);
            }

            if (result.intersects) {
                var properties = Entities.getEntityProperties(currentSelection);
                var center = rollCenter;
                var zero = rollZero;
                var centerToZero = Vec3.subtract(center, zero);
                var centerToIntersect = Vec3.subtract(center, result.intersection);
                var angleFromZero = Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal);

                var distanceFromCenter = Vec3.distance(center, result.intersection);
                var snapToInner = false;
                if (distanceFromCenter < innerRadius) {
                    angleFromZero = Math.floor(angleFromZero/innerSnapAngle) * innerSnapAngle;
                    snapToInner = true;
                }

                // for debugging
                if (debug) {
                    Vec3.print("    result.intersection:",result.intersection);
                    Overlays.editOverlay(rotateCurrentOverlay, { visible: true, start: center,  end: result.intersection });
                    print("    angleFromZero:" + angleFromZero);
                }

                var rollChange = Quat.fromVec3Degrees({ x: 0, y: 0, z: angleFromZero });
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var entityID = SelectionManager.selections[i];
                    var properties = Entities.getEntityProperties(entityID);
                    var initialProperties = SelectionManager.savedProperties[entityID.id];
                    var dPos = Vec3.subtract(initialProperties.position, initialPosition);
                    dPos = Vec3.multiplyQbyV(rollChange, dPos);

                    Entities.editEntity(entityID, {
                        position: Vec3.sum(initialPosition, dPos),
                        rotation: Quat.multiply(rollChange, initialProperties.rotation),
                    });
                }

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
                    Overlays.editOverlay(rotateOverlayOuter, { startAt: 0, endAt: 360 });
                    Overlays.editOverlay(rotateOverlayInner, { startAt: startAtRemainder, endAt: endAtRemainder });
                    Overlays.editOverlay(rotateOverlayCurrent, { startAt: startAtCurrent, endAt: endAtCurrent, size: innerRadius,
                                                                    majorTickMarksAngle: innerSnapAngle, minorTickMarksAngle: 0,
                                                                    majorTickMarksLength: -0.25, minorTickMarksLength: 0, });
                } else {
                    Overlays.editOverlay(rotateOverlayInner, { startAt: 0, endAt: 360 });
                    Overlays.editOverlay(rotateOverlayOuter, { startAt: startAtRemainder, endAt: endAtRemainder });
                    Overlays.editOverlay(rotateOverlayCurrent, { startAt: startAtCurrent, endAt: endAtCurrent, size: outerRadius,
                                                                    majorTickMarksAngle: 45.0, minorTickMarksAngle: 5,
                                                                    majorTickMarksLength: 0.25, minorTickMarksLength: 0.1, });
                }
            }
        }
    });

    that.checkMove = function() {
        if (currentSelection.isKnownID && 
            (!Vec3.equal(Camera.getPosition(), lastCameraPosition) || !Quat.equal(Camera.getOrientation(), lastCameraOrientation))){
            that.select(currentSelection, false, false);
        }
    };

    that.mousePressEvent = function(event) {

        var somethingClicked = false;
        var pickRay = Camera.computePickRay(event.x, event.y);
        
        // before we do a ray test for grabbers, disable the ray intersection for our selection box
        Overlays.editOverlay(selectionBox, { ignoreRayIntersection: true });
        Overlays.editOverlay(yawHandle, { ignoreRayIntersection: true });
        Overlays.editOverlay(pitchHandle, { ignoreRayIntersection: true });
        Overlays.editOverlay(rollHandle, { ignoreRayIntersection: true });
        var result = Overlays.findRayIntersection(pickRay);

        if (result.intersects) {

            var wantDebug = false;
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
                print("FOUND TOOL! " + tool.mode);
                activeTool = tool;
                mode = tool.mode;
                somethingClicked = true;
                if (activeTool && activeTool.onBegin) {
                    activeTool.onBegin(event);
                }
            } else {
                switch(result.overlayID) {
                    case grabberMoveUp:
                        mode = "TRANSLATE_UP_DOWN";
                        somethingClicked = true;

                        // in translate mode, we hide our stretch handles...
                        for (var i = 0; i < stretchHandles.length; i++) {
                            Overlays.editOverlay(stretchHandles[i], { visible: false });
                        }
                        break;


                    case grabberNEAR:
                    case grabberEdgeTN: // TODO: maybe this should be TOP+NEAR stretching?
                    case grabberEdgeBN: // TODO: maybe this should be BOTTOM+FAR stretching?
                        mode = "STRETCH_NEAR";
                        somethingClicked = true;
                        break;

                    case grabberFAR:
                    case grabberEdgeTF:  // TODO: maybe this should be TOP+FAR stretching?
                    case grabberEdgeBF:  // TODO: maybe this should be BOTTOM+FAR stretching?
                        mode = "STRETCH_FAR";
                        somethingClicked = true;
                        break;
                    case grabberTOP:
                        mode = "STRETCH_TOP";
                        somethingClicked = true;
                        break;
                    case grabberBOTTOM:
                        mode = "STRETCH_BOTTOM";
                        somethingClicked = true;
                        break;
                    case grabberRIGHT:
                    case grabberEdgeTR:  // TODO: maybe this should be TOP+RIGHT stretching?
                    case grabberEdgeBR:  // TODO: maybe this should be BOTTOM+RIGHT stretching?
                        mode = "STRETCH_RIGHT";
                        somethingClicked = true;
                        break;
                    case grabberLEFT:
                    case grabberEdgeTL:  // TODO: maybe this should be TOP+LEFT stretching?
                    case grabberEdgeBL:  // TODO: maybe this should be BOTTOM+LEFT stretching?
                        mode = "STRETCH_LEFT";
                        somethingClicked = true;
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
            Overlays.editOverlay(yawHandle, { visible: false });
            Overlays.editOverlay(pitchHandle, { visible: false });
            Overlays.editOverlay(rollHandle, { visible: false });
            
            if (mode != "TRANSLATE_UP_DOWN") {
                Overlays.editOverlay(grabberMoveUp, { visible: false });
            }
        }
        
        if (!somethingClicked) {
        
            print("rotate handle case...");
            
            // After testing our stretch handles, then check out rotate handles
            Overlays.editOverlay(yawHandle, { ignoreRayIntersection: false });
            Overlays.editOverlay(pitchHandle, { ignoreRayIntersection: false });
            Overlays.editOverlay(rollHandle, { ignoreRayIntersection: false });
            var result = Overlays.findRayIntersection(pickRay);
            
            var overlayOrientation;
            var overlayCenter;

            var properties = Entities.getEntityProperties(currentSelection);            
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
                    print("FOUND TOOL! " + tool.mode);
                    activeTool = tool;
                    mode = tool.mode;
                    somethingClicked = true;
                    if (activeTool && activeTool.onBegin) {
                        activeTool.onBegin(event);
                    }
                }
                switch(result.overlayID) {
                    case yawHandle:
                        mode = "ROTATE_YAW";
                        somethingClicked = true;
                        overlayOrientation = yawHandleRotation;
                        overlayCenter = yawCenter;
                        yawZero = result.intersection;
                        rotationNormal = yawNormal;
                        break;

                    case pitchHandle:
                        mode = "ROTATE_PITCH";
                        initialPosition = SelectionManager.worldPosition;
                        somethingClicked = true;
                        overlayOrientation = pitchHandleRotation;
                        overlayCenter = pitchCenter;
                        pitchZero = result.intersection;
                        rotationNormal = pitchNormal;
                        break;

                    case rollHandle:
                        mode = "ROTATE_ROLL";
                        somethingClicked = true;
                        overlayOrientation = rollHandleRotation;
                        overlayCenter = rollCenter;
                        rollZero = result.intersection;
                        rotationNormal = rollNormal;
                        break;

                    default:
                        print("mousePressEvent()...... " + overlayNames[result.overlayID]);
                        mode = "UNKNOWN";
                        break;
                }
            }

            print("    somethingClicked:" + somethingClicked);
            print("                mode:" + mode);
            

            if (somethingClicked) {
            
                Overlays.editOverlay(rotateOverlayTarget, { visible: true, rotation: overlayOrientation, position: overlayCenter });
                Overlays.editOverlay(rotateOverlayInner, { visible: true, rotation: overlayOrientation, position: overlayCenter });
                Overlays.editOverlay(rotateOverlayOuter, { visible: true, rotation: overlayOrientation, position: overlayCenter, startAt: 0, endAt: 360 });
                Overlays.editOverlay(rotateOverlayCurrent, { visible: true, rotation: overlayOrientation, position: overlayCenter, startAt: 0, endAt: 0 });
                  
                // for debugging                  
                var debug = Menu.isOptionChecked("Debug Ryans Rotation Problems");
                if (debug) {
                    Overlays.editOverlay(rotateZeroOverlay, { visible: true, start: overlayCenter, end: result.intersection });
                    Overlays.editOverlay(rotateCurrentOverlay, { visible: true, start: overlayCenter, end: result.intersection });
                }

                Overlays.editOverlay(yawHandle, { visible: false });
                Overlays.editOverlay(pitchHandle, { visible: false });
                Overlays.editOverlay(rollHandle, { visible: false });


                Overlays.editOverlay(yawHandle, { visible: false });
                Overlays.editOverlay(pitchHandle, { visible: false });
                Overlays.editOverlay(rollHandle, { visible: false });
                Overlays.editOverlay(grabberMoveUp, { visible: false });
                Overlays.editOverlay(grabberLBN, { visible: false });
                Overlays.editOverlay(grabberLBF, { visible: false });
                Overlays.editOverlay(grabberRBN, { visible: false });
                Overlays.editOverlay(grabberRBF, { visible: false });
                Overlays.editOverlay(grabberLTN, { visible: false });
                Overlays.editOverlay(grabberLTF, { visible: false });
                Overlays.editOverlay(grabberRTN, { visible: false });
                Overlays.editOverlay(grabberRTF, { visible: false });

                Overlays.editOverlay(grabberTOP, { visible: false });
                Overlays.editOverlay(grabberBOTTOM, { visible: false });
                Overlays.editOverlay(grabberLEFT, { visible: false });
                Overlays.editOverlay(grabberRIGHT, { visible: false });
                Overlays.editOverlay(grabberNEAR, { visible: false });
                Overlays.editOverlay(grabberFAR, { visible: false });

                Overlays.editOverlay(grabberEdgeTR, { visible: false });
                Overlays.editOverlay(grabberEdgeTL, { visible: false });
                Overlays.editOverlay(grabberEdgeTF, { visible: false });
                Overlays.editOverlay(grabberEdgeTN, { visible: false });
                Overlays.editOverlay(grabberEdgeBR, { visible: false });
                Overlays.editOverlay(grabberEdgeBL, { visible: false });
                Overlays.editOverlay(grabberEdgeBF, { visible: false });
                Overlays.editOverlay(grabberEdgeBN, { visible: false });
                Overlays.editOverlay(grabberEdgeNR, { visible: false });
                Overlays.editOverlay(grabberEdgeNL, { visible: false });
                Overlays.editOverlay(grabberEdgeFR, { visible: false });
                Overlays.editOverlay(grabberEdgeFL, { visible: false });
            }
        }

        if (!somethingClicked) {
            Overlays.editOverlay(selectionBox, { ignoreRayIntersection: false });
            var result = Overlays.findRayIntersection(pickRay);
            if (result.intersects) {
                switch(result.overlayID) {
                    case selectionBox:
                        activeTool = translateXZTool;
                        mode = translateXZTool.mode;
                        activeTool.onBegin(event);
                        somethingClicked = true;
                        break;
                    default:
                        print("mousePressEvent()...... " + overlayNames[result.overlayID]);
                        mode = "UNKNOWN";
                        break;
                }
            }
        }

        if (somethingClicked) {
            pickRay = Camera.computePickRay(event.x, event.y);
            lastPlaneIntersection = rayPlaneIntersection(pickRay, selectedEntityPropertiesOriginalPosition, 
                                                            Quat.getFront(lastCameraOrientation));
            if (wantDebug) {
                     print("mousePressEvent()...... " + overlayNames[result.overlayID]);
                Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
                Vec3.print("       originalPosition:", selectedEntityPropertiesOriginalPosition);
            }
        }

        // reset everything as intersectable...
        // TODO: we could optimize this since some of these were already flipped back
        Overlays.editOverlay(selectionBox, { ignoreRayIntersection: false });
        Overlays.editOverlay(yawHandle, { ignoreRayIntersection: false });
        Overlays.editOverlay(pitchHandle, { ignoreRayIntersection: false });
        Overlays.editOverlay(rollHandle, { ignoreRayIntersection: false });
        
        return somethingClicked;
    };

    that.mouseMoveEvent = function(event) {
        if (activeTool) {
            activeTool.onMove(event);
            SelectionManager._update();
            return true;
        }

        // if no tool is active, then just look for handles to highlight...
        var pickRay = Camera.computePickRay(event.x, event.y);
        var result = Overlays.findRayIntersection(pickRay);
        var pickedColor;
        var pickedAlpha;
        var highlightNeeded = false;

        if (result.intersects) {
            switch(result.overlayID) {
                case yawHandle:
                case pitchHandle:
                case rollHandle:
                    pickedColor = rotateHandleColor;
                    pickedAlpha = rotateHandleAlpha;
                    highlightNeeded = true;
                    break;
                    
                case grabberMoveUp:
                    pickedColor = rotateHandleColor;
                    pickedAlpha = rotateHandleAlpha;
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
                    pickedColor = grabberColorEdge;
                    pickedAlpha = grabberAlpha;
                    highlightNeeded = true;
                    break;

                default:
                    if (previousHandle) {
                        Overlays.editOverlay(previousHandle, { color: previousHandleColor, alpha: previousHandleAlpha });
                        previousHandle = false;
                    }
                    break;
            }
            
            if (highlightNeeded) {
                if (previousHandle) {
                    Overlays.editOverlay(previousHandle, { color: previousHandleColor, alpha: previousHandleAlpha });
                    previousHandle = false;
                }
                Overlays.editOverlay(result.overlayID, { color: highlightedHandleColor, alpha: highlightedHandleAlpha });
                previousHandle = result.overlayID;
                previousHandleColor = pickedColor;
                previousHandleAlpha = pickedAlpha;
            }
            
        } else {
            if (previousHandle) {
                Overlays.editOverlay(previousHandle, { color: previousHandleColor, alpha: previousHandleAlpha });
                previousHandle = false;
            }
        }
        
        return false;
    };

    that.updateHandleSizes = function() {
        if (selectedEntityProperties) {
            var diff = Vec3.subtract(selectedEntityProperties.position, Camera.getPosition());
            var grabberSize = Vec3.length(diff) * GRABBER_DISTANCE_TO_SIZE_RATIO;
            for (var i = 0; i < stretchHandles.length; i++) {
                Overlays.editOverlay(stretchHandles[i], {
                    size: grabberSize,
                });
            }
            var handleSize = Vec3.length(diff) * GRABBER_DISTANCE_TO_SIZE_RATIO * 5;
            Overlays.editOverlay(yawHandle, {
                scale: handleSize,
            });
            Overlays.editOverlay(pitchHandle, {
                scale: handleSize,
            });
            Overlays.editOverlay(rollHandle, {
                scale: handleSize,
            });
            Overlays.editOverlay(grabberMoveUp, {
                scale: handleSize,
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
            Overlays.editOverlay(rotateOverlayTarget, { visible: false });
            Overlays.editOverlay(rotateOverlayInner, { visible: false });
            Overlays.editOverlay(rotateOverlayOuter, { visible: false });
            Overlays.editOverlay(rotateOverlayCurrent, { visible: false });
            showHandles = true;
        }

        if (mode != "UNKNOWN") {
            showHandles = true;
        }
        
        mode = "UNKNOWN";
        
        // if something is selected, then reset the "original" properties for any potential next click+move operation
        if (entitySelected) {

            if (showHandles) {
                that.select(currentSelection, event);
            }

            selectedEntityProperties = Entities.getEntityProperties(currentSelection);
            selectedEntityPropertiesOriginalPosition = properties.position;
            selectedEntityPropertiesOriginalDimensions = properties.dimensions;
        }
        
    };

    // NOTE: mousePressEvent and mouseMoveEvent from the main script should call us., so we don't hook these:
    //       Controller.mousePressEvent.connect(that.mousePressEvent);
    //       Controller.mouseMoveEvent.connect(that.mouseMoveEvent);
    Controller.mouseReleaseEvent.connect(that.mouseReleaseEvent);
    
    return that;

}());

