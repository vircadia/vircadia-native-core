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

SelectionDisplay = (function () {
    var that = {};
    
    var mode = "UNKNOWN";
    var overlayNames = new Array();
    var lastAvatarPosition = MyAvatar.position;
    var lastAvatarOrientation = MyAvatar.orientation;
    var lastPlaneIntersection;

    var currentSelection = { id: -1, isKnownID: false };
    var entitySelected = false;
    var selectedEntityProperties;
    var selectedEntityPropertiesOriginalPosition;
    var selectedEntityPropertiesOriginalDimensions;

    var handleHoverColor = { red: 224, green: 67, blue: 36 };
    var handleHoverAlpha = 1.0;

    var rotateHandleColor = { red: 0, green: 0, blue: 0 };
    var rotateHandleAlpha = 0.7;

    
    var grabberSizeCorner = 0.025;
    var grabberSizeEdge = 0.015;
    var grabberSizeFace = 0.025;
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
                    url: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/up-arrow.png",
                    position: { x:0, y: 0, z: 0},
                    color: { red: 0, green: 0, blue: 0 },
                    alpha: 1.0,
                    visible: false,
                    size: 0.1,
                    scale: 0.1,
                    isFacingAvatar: true
                  });
            
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


    var baseOverlayAngles = { x: 0, y: 0, z: 0 };
    var baseOverlayRotation = Quat.fromVec3Degrees(baseOverlayAngles);
    var baseOfEntityProjectionOverlay = Overlays.addOverlay("rectangle3d", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 51, green: 152, blue: 203 },
                    alpha: 0.5,
                    solid: true,
                    visible: false,
                    rotation: baseOverlayRotation
                });

    var yawOverlayAngles = { x: 90, y: 0, z: 0 };
    var yawOverlayRotation = Quat.fromVec3Degrees(yawOverlayAngles);
    var pitchOverlayAngles = { x: 0, y: 90, z: 0 };
    var pitchOverlayRotation = Quat.fromVec3Degrees(pitchOverlayAngles);
    var rollOverlayAngles = { x: 0, y: 180, z: 0 };
    var rollOverlayRotation = Quat.fromVec3Degrees(rollOverlayAngles);


    var rotateOverlayInner = Overlays.addOverlay("circle3d", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 51, green: 152, blue: 203 },
                    alpha: 0.2,
                    solid: true,
                    visible: false,
                    rotation: yawOverlayRotation
                });

    var rotateOverlayOuter = Overlays.addOverlay("circle3d", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 51, green: 152, blue: 203 },
                    alpha: 0.2,
                    solid: true,
                    visible: false,
                    rotation: yawOverlayRotation
                });

    var rotateOverlayCurrent = Overlays.addOverlay("circle3d", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 224, green: 67, blue: 36},
                    alpha: 0.8,
                    solid: true,
                    visible: false,
                    rotation: yawOverlayRotation,
                });

    var yawHandle = Overlays.addOverlay("billboard", {
                                        url: "https://s3.amazonaws.com/uploads.hipchat.com/33953/231323/HRRhkMk8ueLk8ku/rotate-arrow.png",
                                        position: { x:0, y: 0, z: 0},
                                        color: rotateHandleColor,
                                        alpha: rotateHandleAlpha,
                                        visible: false,
                                        size: 0.1,
                                        scale: 0.1,
                                        isFacingAvatar: false
                                      });


    var pitchHandle = Overlays.addOverlay("billboard", {
                                        url: "https://s3.amazonaws.com/uploads.hipchat.com/33953/231323/HRRhkMk8ueLk8ku/rotate-arrow.png",
                                        position: { x:0, y: 0, z: 0},
                                        color: rotateHandleColor,
                                        alpha: rotateHandleAlpha,
                                        visible: false,
                                        size: 0.1,
                                        scale: 0.1,
                                        isFacingAvatar: false
                                      });


    var rollHandle = Overlays.addOverlay("billboard", {
                                        url: "https://s3.amazonaws.com/uploads.hipchat.com/33953/231323/HRRhkMk8ueLk8ku/rotate-arrow.png",
                                        position: { x:0, y: 0, z: 0},
                                        color: rotateHandleColor,
                                        alpha: rotateHandleAlpha,
                                        visible: false,
                                        size: 0.1,
                                        scale: 0.1,
                                        isFacingAvatar: false
                                      });

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


    that.cleanup = function () {
        Overlays.deleteOverlay(highlightBox);
        Overlays.deleteOverlay(selectionBox);
        Overlays.deleteOverlay(grabberMoveUp);
        Overlays.deleteOverlay(baseOfEntityProjectionOverlay);
        Overlays.deleteOverlay(grabberLBN);
        Overlays.deleteOverlay(grabberLBF);
        Overlays.deleteOverlay(grabberRBN);
        Overlays.deleteOverlay(grabberRBF);
        Overlays.deleteOverlay(grabberLTN);
        Overlays.deleteOverlay(grabberLTF);
        Overlays.deleteOverlay(grabberRTN);
        Overlays.deleteOverlay(grabberRTF);

        Overlays.deleteOverlay(grabberTOP);
        Overlays.deleteOverlay(grabberBOTTOM);
        Overlays.deleteOverlay(grabberLEFT);
        Overlays.deleteOverlay(grabberRIGHT);
        Overlays.deleteOverlay(grabberNEAR);
        Overlays.deleteOverlay(grabberFAR);

        Overlays.deleteOverlay(grabberEdgeTR);
        Overlays.deleteOverlay(grabberEdgeTL);
        Overlays.deleteOverlay(grabberEdgeTF);
        Overlays.deleteOverlay(grabberEdgeTN);
        Overlays.deleteOverlay(grabberEdgeBR);
        Overlays.deleteOverlay(grabberEdgeBL);
        Overlays.deleteOverlay(grabberEdgeBF);
        Overlays.deleteOverlay(grabberEdgeBN);
        Overlays.deleteOverlay(grabberEdgeNR);
        Overlays.deleteOverlay(grabberEdgeNL);
        Overlays.deleteOverlay(grabberEdgeFR);
        Overlays.deleteOverlay(grabberEdgeFL);
        
        Overlays.deleteOverlay(yawHandle);
        Overlays.deleteOverlay(pitchHandle);
        Overlays.deleteOverlay(rollHandle);

        Overlays.deleteOverlay(rotateOverlayInner);
        Overlays.deleteOverlay(rotateOverlayOuter);
        Overlays.deleteOverlay(rotateOverlayCurrent);

    };

    that.highlightSelectable = function(entityID) {
        var properties = Entities.getEntityProperties(entityID);
        var center = { x: properties.position.x, y: properties.position.y, z: properties.position.z };
        Overlays.editOverlay(highlightBox, 
                            { 
                                visible: true,
                                position: center,
                                dimensions: properties.dimensions,
                                rotation: properties.rotation,
                            });
    };

    that.unhighlightSelectable = function(entityID) {
        Overlays.editOverlay(highlightBox,{ visible: false});
    };

    that.select = function(entityID, event) {
        var properties = Entities.getEntityProperties(entityID);
        if (currentSelection.isKnownID == true) {
            that.unselect(currentSelection);
        }
        currentSelection = entityID;
        entitySelected = true;
        
        lastAvatarPosition = MyAvatar.position;
        lastAvatarOrientation = MyAvatar.orientation;

        if (event !== false) {
            selectedEntityProperties = properties;
            selectedEntityPropertiesOriginalPosition = properties.position;
            selectedEntityPropertiesOriginalDimensions = properties.dimensions;
            pickRay = Camera.computePickRay(event.x, event.y);
            lastPlaneIntersection = rayPlaneIntersection(pickRay, properties.position, Quat.getFront(lastAvatarOrientation));

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

        var diagonal = (Vec3.length(properties.dimensions) / 2) * 1.1;
        var halfDimensions = Vec3.multiply(properties.dimensions, 0.5);
        var innerRadius = diagonal;
        var outerRadius = diagonal * 1.15;
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
        
        var left = properties.position.x - halfDimensions.x;
        var right = properties.position.x + halfDimensions.x;
        var bottom = properties.position.y - halfDimensions.y;
        var top = properties.position.y + halfDimensions.y;
        var near = properties.position.z - halfDimensions.z;
        var far = properties.position.z + halfDimensions.z;
        var center = { x: properties.position.x, y: properties.position.y, z: properties.position.z };

        var BLN = { x: left, y: bottom, z: near };
        var BRN = { x: right, y: bottom, z: near };
        var BLF = { x: left, y: bottom, z: far };
        var BRF = { x: right, y: bottom, z: far };
        var TLN = { x: left, y: top, z: near };
        var TRN = { x: right, y: top, z: near };
        var TLF = { x: left, y: top, z: far };
        var TRF = { x: right, y: top, z: far };

        var yawCorner;
        var pitchCorner;
        var rollCorner;

        var yawHandleRotation;
        var pitchHandleRotation;
        var rollHandleRotation;

        // determine which bottom corner we are closest to
        /*------------------------------
          example:
          
            BRF +--------+ BLF
                |        |
                |        |
            BRN +--------+ BLN
                   
                   *
                
        ------------------------------*/
        
        if (MyAvatar.position.x > center.x) {
            // must be BRF or BRN
            if (MyAvatar.position.z < center.z) {
                yawHandleRotation = Quat.fromVec3Degrees({ x: 90, y: 0, z: 0 });
                pitchHandleRotation = Quat.fromVec3Degrees({ x: 0, y: 180, z: 180 });
                rollHandleRotation = Quat.fromVec3Degrees({ x: 0, y: 90, z: 180 });

                yawCorner = { x: right + rotateHandleOffset,
                              y: bottom - rotateHandleOffset,
                              z: near - rotateHandleOffset };

                pitchCorner = { x: right + rotateHandleOffset,
                                y: top + rotateHandleOffset,
                                z: far + rotateHandleOffset };

                rollCorner = { x: left - rotateHandleOffset,
                               y: top + rotateHandleOffset,
                               z: near - rotateHandleOffset};

            } else {
                yawHandleRotation = Quat.fromVec3Degrees({ x: 90, y: 270, z: 0 });
                pitchHandleRotation = Quat.fromVec3Degrees({ x: 180, y: 270, z: 0 });
                rollHandleRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 90 });

                yawCorner = { x: right + rotateHandleOffset,
                              y: bottom - rotateHandleOffset,
                              z: far + rotateHandleOffset };

                pitchCorner = { x: left - rotateHandleOffset,
                                y: top + rotateHandleOffset,
                                z: far + rotateHandleOffset };

                rollCorner = { x: right + rotateHandleOffset,
                               y: top + rotateHandleOffset,
                               z: near - rotateHandleOffset};

            }
        } else {
            // must be BLF or BLN
            if (MyAvatar.position.z < center.z) {
                yawHandleRotation = Quat.fromVec3Degrees({ x: 90, y: 90, z: 0 });
                pitchHandleRotation = Quat.fromVec3Degrees({ x: 90, y: 0, z: 90 });
                rollHandleRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 180 });

                yawCorner = { x: left - rotateHandleOffset,
                              y: bottom - rotateHandleOffset,
                              z: near - rotateHandleOffset };

                pitchCorner = { x: right + rotateHandleOffset,
                                y: top + rotateHandleOffset,
                                z: near - rotateHandleOffset };

                rollCorner = { x: left - rotateHandleOffset,
                               y: top + rotateHandleOffset,
                               z: far + rotateHandleOffset};

                
            } else {
                yawHandleRotation = Quat.fromVec3Degrees({ x: 90, y: 180, z: 0 });
                pitchHandleRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 180 });
                rollHandleRotation = Quat.fromVec3Degrees({ x: 180, y: 270, z: 0 });

                yawCorner = { x: left - rotateHandleOffset,
                              y: bottom - rotateHandleOffset,
                              z: far + rotateHandleOffset };

                pitchCorner = { x: left - rotateHandleOffset,
                                y: top + rotateHandleOffset,
                                z: near - rotateHandleOffset };

                rollCorner = { x: right + rotateHandleOffset,
                               y: top + rotateHandleOffset,
                               z: far + rotateHandleOffset};
            }
        }

        Overlays.editOverlay(highlightBox, { visible: false });
        
        Overlays.editOverlay(selectionBox, 
                            { 
                                visible: true,
                                position: center,
                                dimensions: properties.dimensions,
                                rotation: properties.rotation,
                            });
                            
                            
        Overlays.editOverlay(grabberMoveUp, { visible: true, position: { x: center.x, y: top + grabberMoveUpOffset, z: center.z } });

        Overlays.editOverlay(grabberLBN, { visible: true, position: { x: left, y: bottom, z: near } });
        Overlays.editOverlay(grabberRBN, { visible: true, position: { x: right, y: bottom, z: near } });
        Overlays.editOverlay(grabberLBF, { visible: true, position: { x: left, y: bottom, z: far } });
        Overlays.editOverlay(grabberRBF, { visible: true, position: { x: right, y: bottom, z: far } });
        Overlays.editOverlay(grabberLTN, { visible: true, position: { x: left, y: top, z: near } });
        Overlays.editOverlay(grabberRTN, { visible: true, position: { x: right, y: top, z: near } });
        Overlays.editOverlay(grabberLTF, { visible: true, position: { x: left, y: top, z: far } });
        Overlays.editOverlay(grabberRTF, { visible: true, position: { x: right, y: top, z: far } });


        Overlays.editOverlay(grabberTOP, { visible: true, position: { x: center.x, y: top, z: center.z } });
        Overlays.editOverlay(grabberBOTTOM, { visible: true, position: { x: center.x, y: bottom, z: center.z } });
        Overlays.editOverlay(grabberLEFT, { visible: true, position: { x: left, y: center.y, z: center.z } });
        Overlays.editOverlay(grabberRIGHT, { visible: true, position: { x: right, y: center.y, z: center.z } });
        Overlays.editOverlay(grabberNEAR, { visible: true, position: { x: center.x, y: center.y, z: near } });
        Overlays.editOverlay(grabberFAR, { visible: true, position: { x: center.x, y: center.y, z: far } });

        Overlays.editOverlay(grabberEdgeTR, { visible: true, position: { x: right, y: top, z: center.z } });
        Overlays.editOverlay(grabberEdgeTL, { visible: true, position: { x: left, y: top, z: center.z } });
        Overlays.editOverlay(grabberEdgeTF, { visible: true, position: { x: center.x, y: top, z: far } });
        Overlays.editOverlay(grabberEdgeTN, { visible: true, position: { x: center.x, y: top, z: near } });
        Overlays.editOverlay(grabberEdgeBR, { visible: true, position: { x: right, y: bottom, z: center.z } });
        Overlays.editOverlay(grabberEdgeBL, { visible: true, position: { x: left, y: bottom, z: center.z } });
        Overlays.editOverlay(grabberEdgeBF, { visible: true, position: { x: center.x, y: bottom, z: far } });
        Overlays.editOverlay(grabberEdgeBN, { visible: true, position: { x: center.x, y: bottom, z: near } });
        Overlays.editOverlay(grabberEdgeNR, { visible: true, position: { x: right, y: center.y, z: near } });
        Overlays.editOverlay(grabberEdgeNL, { visible: true, position: { x: left, y: center.y, z: near } });
        Overlays.editOverlay(grabberEdgeFR, { visible: true, position: { x: right, y: center.y, z: far } });
        Overlays.editOverlay(grabberEdgeFL, { visible: true, position: { x: left, y: center.y, z: far } });


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


        Overlays.editOverlay(rotateOverlayInner, 
                            { 
                                visible: false,
                                position: { x: properties.position.x,
                                            y: properties.position.y - (properties.dimensions.y / 2),
                                            z: properties.position.z},

                                size: innerRadius,
                                innerRadius: 0.9,
                                alpha: innerAlpha
                            });

        Overlays.editOverlay(rotateOverlayOuter, 
                            { 
                                visible: false,
                                position: { x: properties.position.x,
                                            y: properties.position.y - (properties.dimensions.y / 2),
                                            z: properties.position.z},

                                size: outerRadius,
                                innerRadius: 0.9,
                                startAt: 0,
                                endAt: 360,
                                alpha: outerAlpha
                            });

        Overlays.editOverlay(rotateOverlayCurrent, 
                            { 
                                visible: false,
                                position: { x: properties.position.x,
                                            y: properties.position.y - (properties.dimensions.y / 2),
                                            z: properties.position.z},

                                size: outerRadius,
                                startAt: 0,
                                endAt: 0,
                                innerRadius: 0.9
                            });

        Overlays.editOverlay(yawHandle, { visible: true, position: yawCorner, rotation: yawHandleRotation});
        Overlays.editOverlay(pitchHandle, { visible: true, position: pitchCorner, rotation: pitchHandleRotation});
        Overlays.editOverlay(rollHandle, { visible: true, position: rollCorner, rotation: rollHandleRotation});

        Entities.editEntity(entityID, { localRenderAlpha: 0.1 });
    };

    that.unselectAll = function () {
        if (currentSelection.isKnownID == true) {
            that.unselect(currentSelection);
        }
        currentSelection = { id: -1, isKnownID: false };
        entitySelected = false;
    };

    that.unselect = function (entityID) {
        Overlays.editOverlay(selectionBox, { visible: false });
        Overlays.editOverlay(baseOfEntityProjectionOverlay, { visible: false });
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

        Overlays.editOverlay(yawHandle, { visible: false });
        Overlays.editOverlay(pitchHandle, { visible: false });
        Overlays.editOverlay(rollHandle, { visible: false });

        Overlays.editOverlay(rotateOverlayInner, { visible: false });
        Overlays.editOverlay(rotateOverlayOuter, { visible: false });
        Overlays.editOverlay(rotateOverlayCurrent, { visible: false });

        Entities.editEntity(entityID, { localRenderAlpha: 1.0 });

        currentSelection = { id: -1, isKnownID: false };
        entitySelected = false;
    };

    that.translateXZ = function(event) {
        if (!entitySelected || mode !== "TRANSLATE_XZ") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        // this allows us to use the old editModels "shifted" logic which makes the
        // up/down behavior of the mouse move "in"/"out" of the screen.
        var i = Vec3.dot(vector, Quat.getRight(orientation));
        var j = Vec3.dot(vector, Quat.getUp(orientation));
        vector = Vec3.sum(Vec3.multiply(Quat.getRight(orientation), i),
                          Vec3.multiply(Quat.getFront(orientation), j));
        
        newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, vector);

        var wantDebug = false;
        if (wantDebug) {
            print("translateXZ... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            Vec3.print("       originalPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("         recentPosition:", selectedEntityProperties.position);
            Vec3.print("            newPosition:", newPosition);
        }

        selectedEntityProperties.position = newPosition;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };
    
    that.translateUpDown = function(event) {
        if (!entitySelected || mode !== "TRANSLATE_UP_DOWN") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);
        
        // we only care about the Y axis
        vector.x = 0;
        vector.z = 0;
        
        newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, vector);

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

        selectedEntityProperties.position = newPosition;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };

    that.stretchNEAR = function(event) {
        if (!entitySelected || mode !== "STRETCH_NEAR") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldNEAR = selectedEntityPropertiesOriginalPosition.z - halfDimensions.z;
        var newNEAR = oldNEAR + vector.z;

        // if near is changing, then...
        //   dimensions changes by: (oldNEAR - newNEAR)
        var changeInDimensions = { x: 0, y: 0, z: (oldNEAR - newNEAR) };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: 0, y: 0, z: (oldNEAR - newNEAR) * -0.5 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchNEAR... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("           oldNEAR:" + oldNEAR);
            print("                newNEAR:" + newNEAR);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };

    that.stretchFAR = function(event) {
        if (!entitySelected || mode !== "STRETCH_FAR") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldFAR = selectedEntityPropertiesOriginalPosition.z + halfDimensions.z;
        var newFAR = oldFAR + vector.z;
        var changeInDimensions = { x: 0, y: 0, z: (newFAR - oldFAR) };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: 0, y: 0, z: (newFAR - oldFAR) * 0.5 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchFAR... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldFAR:" + oldFAR);
            print("                 newFAR:" + newFAR);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };

    that.stretchTOP = function(event) {
        if (!entitySelected || mode !== "STRETCH_TOP") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldTOP = selectedEntityPropertiesOriginalPosition.y + halfDimensions.y;
        var newTOP = oldTOP + vector.y;
        var changeInDimensions = { x: 0, y: (newTOP - oldTOP), z: 0 };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: 0, y: (newTOP - oldTOP) * 0.5, z: 0 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchTOP... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldTOP:" + oldTOP);
            print("                 newTOP:" + newTOP);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };

    that.stretchBOTTOM = function(event) {
        if (!entitySelected || mode !== "STRETCH_BOTTOM") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldBOTTOM = selectedEntityPropertiesOriginalPosition.y - halfDimensions.y;
        var newBOTTOM = oldBOTTOM + vector.y;
        var changeInDimensions = { x: 0, y: (oldBOTTOM - newBOTTOM), z: 0 };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: 0, y: (oldBOTTOM - newBOTTOM) * -0.5, z: 0 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchBOTTOM... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldBOTTOM:" + oldBOTTOM);
            print("                 newBOTTOM:" + newBOTTOM);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };
    
    that.stretchRIGHT = function(event) {
        if (!entitySelected || mode !== "STRETCH_RIGHT") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldRIGHT = selectedEntityPropertiesOriginalPosition.x + halfDimensions.x;
        var newRIGHT = oldRIGHT + vector.x;
        var changeInDimensions = { x: (newRIGHT - oldRIGHT), y: 0 , z: 0 };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: (newRIGHT - oldRIGHT) * 0.5, y: 0, z: 0 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchRIGHT... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldRIGHT:" + oldRIGHT);
            print("                 newRIGHT:" + newRIGHT);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };

    that.stretchLEFT = function(event) {
        if (!entitySelected || mode !== "STRETCH_LEFT") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldLEFT = selectedEntityPropertiesOriginalPosition.x - halfDimensions.x;
        var newLEFT = oldLEFT + vector.x;
        var changeInDimensions = { x: (oldLEFT - newLEFT), y: 0, z: 0 };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: (oldLEFT - newLEFT) * -0.5, y: 0, z: 0 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchLEFT... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldLEFT:" + oldLEFT);
            print("                 newLEFT:" + newLEFT);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };
    
    that.stretchRBN = function(event) {
        if (!entitySelected || mode !== "STRETCH_RBN") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldRIGHT = selectedEntityPropertiesOriginalPosition.x + halfDimensions.x;
        var newRIGHT = oldRIGHT + vector.x;

        var oldBOTTOM = selectedEntityPropertiesOriginalPosition.y - halfDimensions.y;
        var newBOTTOM = oldBOTTOM - vector.y;

        var oldNEAR = selectedEntityPropertiesOriginalPosition.z - halfDimensions.z;
        var newNEAR = oldNEAR - vector.z;
        
        
        var changeInDimensions = { x: (newRIGHT - oldRIGHT), y: (newBOTTOM - oldBOTTOM) , z: (newNEAR - oldNEAR) };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: (newRIGHT - oldRIGHT) * 0.5, 
                                 y: (newBOTTOM - oldBOTTOM) * -0.5, 
                                 z: (newNEAR - oldNEAR) * -0.5 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchRBN... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldRIGHT:" + oldRIGHT);
            print("                 newRIGHT:" + newRIGHT);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };

    that.stretchLBN = function(event) {
        if (!entitySelected || mode !== "STRETCH_LBN") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldLEFT = selectedEntityPropertiesOriginalPosition.x - halfDimensions.x;
        var newLEFT = oldLEFT - vector.x;

        var oldBOTTOM = selectedEntityPropertiesOriginalPosition.y - halfDimensions.y;
        var newBOTTOM = oldBOTTOM - vector.y;

        var oldNEAR = selectedEntityPropertiesOriginalPosition.z - halfDimensions.z;
        var newNEAR = oldNEAR - vector.z;
        
        
        var changeInDimensions = { x: (newLEFT - oldLEFT), y: (newBOTTOM - oldBOTTOM) , z: (newNEAR - oldNEAR) };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: (newLEFT - oldLEFT) * -0.5, 
                                 y: (newBOTTOM - oldBOTTOM) * -0.5, 
                                 z: (newNEAR - oldNEAR) * -0.5 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchLBN... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldLEFT:" + oldLEFT);
            print("                 newLEFT:" + newLEFT);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };        

    that.stretchRTN = function(event) {
        if (!entitySelected || mode !== "STRETCH_RTN") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldRIGHT = selectedEntityPropertiesOriginalPosition.x + halfDimensions.x;
        var newRIGHT = oldRIGHT + vector.x;

        var oldTOP = selectedEntityPropertiesOriginalPosition.y + halfDimensions.y;
        var newTOP = oldTOP + vector.y;

        var oldNEAR = selectedEntityPropertiesOriginalPosition.z - halfDimensions.z;
        var newNEAR = oldNEAR - vector.z;
        
        
        var changeInDimensions = { x: (newRIGHT - oldRIGHT), y: (newTOP - oldTOP) , z: (newNEAR - oldNEAR) };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: (newRIGHT - oldRIGHT) * 0.5, 
                                 y: (newTOP - oldTOP) * 0.5, 
                                 z: (newNEAR - oldNEAR) * -0.5 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchRTN... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldRIGHT:" + oldRIGHT);
            print("                 newRIGHT:" + newRIGHT);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };

    that.stretchLTN = function(event) {
        if (!entitySelected || mode !== "STRETCH_LTN") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldLEFT = selectedEntityPropertiesOriginalPosition.x - halfDimensions.x;
        var newLEFT = oldLEFT - vector.x;

        var oldTOP = selectedEntityPropertiesOriginalPosition.y + halfDimensions.y;
        var newTOP = oldTOP + vector.y;

        var oldNEAR = selectedEntityPropertiesOriginalPosition.z - halfDimensions.z;
        var newNEAR = oldNEAR - vector.z;
        
        
        var changeInDimensions = { x: (newLEFT - oldLEFT), y: (newTOP - oldTOP) , z: (newNEAR - oldNEAR) };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: (newLEFT - oldLEFT) * -0.5, 
                                 y: (newTOP - oldTOP) * 0.5, 
                                 z: (newNEAR - oldNEAR) * -0.5 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchLTN... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldLEFT:" + oldLEFT);
            print("                 newLEFT:" + newLEFT);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };   

    that.stretchRBF = function(event) {
        if (!entitySelected || mode !== "STRETCH_RBF") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldRIGHT = selectedEntityPropertiesOriginalPosition.x + halfDimensions.x;
        var newRIGHT = oldRIGHT + vector.x;

        var oldBOTTOM = selectedEntityPropertiesOriginalPosition.y - halfDimensions.y;
        var newBOTTOM = oldBOTTOM - vector.y;

        var oldFAR = selectedEntityPropertiesOriginalPosition.z + halfDimensions.z;
        var newFAR = oldFAR + vector.z;
        
        
        var changeInDimensions = { x: (newRIGHT - oldRIGHT), y: (newBOTTOM - oldBOTTOM) , z: (newFAR - oldFAR) };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: (newRIGHT - oldRIGHT) * 0.5, 
                                 y: (newBOTTOM - oldBOTTOM) * -0.5, 
                                 z: (newFAR - oldFAR) * 0.5 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchRBF... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldRIGHT:" + oldRIGHT);
            print("                 newRIGHT:" + newRIGHT);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };

    that.stretchLBF = function(event) {
        if (!entitySelected || mode !== "STRETCH_LBF") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldLEFT = selectedEntityPropertiesOriginalPosition.x - halfDimensions.x;
        var newLEFT = oldLEFT - vector.x;

        var oldBOTTOM = selectedEntityPropertiesOriginalPosition.y - halfDimensions.y;
        var newBOTTOM = oldBOTTOM - vector.y;

        var oldFAR = selectedEntityPropertiesOriginalPosition.z + halfDimensions.z;
        var newFAR = oldFAR + vector.z;
        
        
        var changeInDimensions = { x: (newLEFT - oldLEFT), y: (newBOTTOM - oldBOTTOM) , z: (newFAR - oldFAR) };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: (newLEFT - oldLEFT) * -0.5, 
                                 y: (newBOTTOM - oldBOTTOM) * -0.5, 
                                 z: (newFAR - oldFAR) * 0.5 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchLBF... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldLEFT:" + oldLEFT);
            print("                 newLEFT:" + newLEFT);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };        

    that.stretchRTF = function(event) {
        if (!entitySelected || mode !== "STRETCH_RTF") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldRIGHT = selectedEntityPropertiesOriginalPosition.x + halfDimensions.x;
        var newRIGHT = oldRIGHT + vector.x;

        var oldTOP = selectedEntityPropertiesOriginalPosition.y + halfDimensions.y;
        var newTOP = oldTOP + vector.y;

        var oldFAR = selectedEntityPropertiesOriginalPosition.z + halfDimensions.z;
        var newFAR = oldFAR + vector.z;
        
        
        var changeInDimensions = { x: (newRIGHT - oldRIGHT), y: (newTOP - oldTOP) , z: (newFAR - oldFAR) };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: (newRIGHT - oldRIGHT) * 0.5, 
                                 y: (newTOP - oldTOP) * 0.5, 
                                 z: (newFAR - oldFAR) * 0.5 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchRTF... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldRIGHT:" + oldRIGHT);
            print("                 newRIGHT:" + newRIGHT);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };

    that.stretchLTF = function(event) {
        if (!entitySelected || mode !== "STRETCH_LTF") {
            return; // not allowed
        }
        pickRay = Camera.computePickRay(event.x, event.y);

        // translate mode left/right based on view toward entity
        var newIntersection = rayPlaneIntersection(pickRay,
                                                   selectedEntityPropertiesOriginalPosition,
                                                   Quat.getFront(lastAvatarOrientation));

        var vector = Vec3.subtract(newIntersection, lastPlaneIntersection);

        var halfDimensions = Vec3.multiply(selectedEntityPropertiesOriginalDimensions, 0.5);
        var oldLEFT = selectedEntityPropertiesOriginalPosition.x - halfDimensions.x;
        var newLEFT = oldLEFT - vector.x;

        var oldTOP = selectedEntityPropertiesOriginalPosition.y + halfDimensions.y;
        var newTOP = oldTOP + vector.y;

        var oldFAR = selectedEntityPropertiesOriginalPosition.z + halfDimensions.z;
        var newFAR = oldFAR + vector.z;
        
        
        var changeInDimensions = { x: (newLEFT - oldLEFT), y: (newTOP - oldTOP) , z: (newFAR - oldFAR) };
        var newDimensions = Vec3.sum(selectedEntityPropertiesOriginalDimensions, changeInDimensions);
        var changeInPosition = { x: (newLEFT - oldLEFT) * -0.5, 
                                 y: (newTOP - oldTOP) * 0.5, 
                                 z: (newFAR - oldFAR) * 0.5 };
        var newPosition = Vec3.sum(selectedEntityPropertiesOriginalPosition, changeInPosition);
        var wantDebug = false;
        if (wantDebug) {
            print("stretchLTF... ");
            Vec3.print("  lastPlaneIntersection:", lastPlaneIntersection);
            Vec3.print("        newIntersection:", newIntersection);
            Vec3.print("                 vector:", vector);
            print("            oldLEFT:" + oldLEFT);
            print("                 newLEFT:" + newLEFT);
            Vec3.print("            oldDimensions:", selectedEntityPropertiesOriginalDimensions);
            Vec3.print("            changeInDimensions:", changeInDimensions);
            Vec3.print("                 newDimensions:", newDimensions);

            Vec3.print("              oldPosition:", selectedEntityPropertiesOriginalPosition);
            Vec3.print("              changeInPosition:", changeInPosition);
            Vec3.print("                   newPosition:", newPosition);
        }
        
        selectedEntityProperties.position = newPosition;
        selectedEntityProperties.dimensions = newDimensions;
        Entities.editEntity(currentSelection, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        that.select(currentSelection, false); // TODO: this should be more than highlighted
    };   

    that.checkMove = function() {
        if (currentSelection.isKnownID && 
            (!Vec3.equal(MyAvatar.position, lastAvatarPosition) || !Quat.equal(MyAvatar.orientation, lastAvatarOrientation))){
            that.select(currentSelection, false);
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

            switch(result.overlayID) {
                case grabberMoveUp:
                    mode = "TRANSLATE_UP_DOWN";
                    somethingClicked = true;
                    break;

                case grabberRBN:
                    mode = "STRETCH_RBN";
                    somethingClicked = true;
                    break;

                case grabberLBN:
                    mode = "STRETCH_LBN";
                    somethingClicked = true;
                    break;

                case grabberRTN:
                    mode = "STRETCH_RTN";
                    somethingClicked = true;
                    break;

                case grabberLTN:
                    mode = "STRETCH_LTN";
                    somethingClicked = true;
                    break;

                case grabberRBF:
                    mode = "STRETCH_RBF";
                    somethingClicked = true;
                    break;

                case grabberLBF:
                    mode = "STRETCH_LBF";
                    somethingClicked = true;
                    break;

                case grabberRTF:
                    mode = "STRETCH_RTF";
                    somethingClicked = true;
                    break;

                case grabberLTF:
                    mode = "STRETCH_LTF";
                    somethingClicked = true;
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
        
        if (!somethingClicked) {
            // After testing our stretch handles, then check out rotate handles
            Overlays.editOverlay(yawHandle, { ignoreRayIntersection: false });
            Overlays.editOverlay(pitchHandle, { ignoreRayIntersection: false });
            Overlays.editOverlay(rollHandle, { ignoreRayIntersection: false });
            var result = Overlays.findRayIntersection(pickRay);
            if (result.intersects) {
                switch(result.overlayID) {
                    default:
                        print("mousePressEvent()...... " + overlayNames[result.overlayID]);
                        mode = "UNKNOWN";
                        break;
                }
            }
        }

        if (!somethingClicked) {
            Overlays.editOverlay(selectionBox, { ignoreRayIntersection: false });
            var result = Overlays.findRayIntersection(pickRay);
            if (result.intersects) {
                switch(result.overlayID) {
                    case selectionBox:
                        mode = "TRANSLATE_XZ";
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
                                                            Quat.getFront(lastAvatarOrientation));
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
    };

    that.mouseMoveEvent = function(event) {
        //print("mouseMoveEvent()... mode:" + mode);
        switch (mode) {
            case "TRANSLATE_UP_DOWN":
                that.translateUpDown(event);
                break;
            case "TRANSLATE_XZ":
                that.translateXZ(event);
                break;
            case "STRETCH_RBN":
                that.stretchRBN(event);
                break;
            case "STRETCH_LBN":
                that.stretchLBN(event);
                break;
            case "STRETCH_RTN":
                that.stretchRTN(event);
                break;
            case "STRETCH_LTN":
                that.stretchLTN(event);
                break;

            case "STRETCH_RBF":
                that.stretchRBF(event);
                break;
            case "STRETCH_LBF":
                that.stretchLBF(event);
                break;
            case "STRETCH_RTF":
                that.stretchRTF(event);
                break;
            case "STRETCH_LTF":
                that.stretchLTF(event);
                break;

            case "STRETCH_NEAR":
                that.stretchNEAR(event);
                break;
            case "STRETCH_FAR":
                that.stretchFAR(event);
                break;
            case "STRETCH_TOP":
                that.stretchTOP(event);
                break;
            case "STRETCH_BOTTOM":
                that.stretchBOTTOM(event);
                break;
            case "STRETCH_RIGHT":
                that.stretchRIGHT(event);
                break;
            case "STRETCH_LEFT":
                that.stretchLEFT(event);
                break;
            default:
                // nothing to do by default
                break;
        }
    };

    that.mouseReleaseEvent = function(event) {
        mode = "UNKNOWN";
        
        // if something is selected, then reset the "original" properties for any potential next click+move operation
        if (entitySelected) {
            selectedEntityProperties = Entities.getEntityProperties(currentSelection);
            selectedEntityPropertiesOriginalPosition = properties.position;
            selectedEntityPropertiesOriginalDimensions = properties.dimensions;
        }
    };

    Controller.mousePressEvent.connect(that.mousePressEvent);
    Controller.mouseMoveEvent.connect(that.mouseMoveEvent);
    Controller.mouseReleaseEvent.connect(that.mouseReleaseEvent);
    
    return that;

}());

