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
    
    var lastAvatarPosition = MyAvatar.position;
    var lastAvatarOrientation = MyAvatar.orientation;

    var currentSelection = { id: -1, isKnownID: false };

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

    that.select = function(entityID) {
    
print("select()...... entityID:" + entityID.id);
    
        var properties = Entities.getEntityProperties(entityID);
        if (currentSelection.isKnownID == true) {
            that.unselect(currentSelection);
        }
        currentSelection = entityID;

        lastAvatarPosition = MyAvatar.position;
        lastAvatarOrientation = MyAvatar.orientation;


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
    };

    that.checkMove = function() {
        if (currentSelection.isKnownID && 
            (!Vec3.equal(MyAvatar.position, lastAvatarPosition) || !Quat.equal(MyAvatar.orientation, lastAvatarOrientation))){

print("checkMove calling .... select()");

//print("Vec3.equal(MyAvatar.position, lastAvatarPosition):" + Vec3.equal(MyAvatar.position, lastAvatarPosition);
//Vec3.print("MyAvatar.position:", MyAvatar.position);
//Vec3.print("lastAvatarPosition:", lastAvatarPosition);

//print("Quat.equal(MyAvatar.orientation, lastAvatarOrientation):" + Quat.equal(MyAvatar.orientation, lastAvatarOrientation));
//Quat.print("MyAvatar.orientation:", MyAvatar.orientation);
//Quat.print("lastAvatarOrientation:", lastAvatarOrientation);

            that.select(currentSelection);
        }
    };

    that.mousePressEvent = function(event) {
    };

    that.mouseMoveEvent = function(event) {
    };

    that.mouseReleaseEvent = function(event) {
    };

    Controller.mousePressEvent.connect(that.mousePressEvent);
    Controller.mouseMoveEvent.connect(that.mouseMoveEvent);
    Controller.mouseReleaseEvent.connect(that.mouseReleaseEvent);
    
    return that;

}());

