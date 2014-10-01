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
    var selectionBox = Overlays.addOverlay("cube", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 180, green: 180, blue: 180},
                    alpha: 1,
                    solid: false,
                    visible: false
                });

    var baseOverlayAngles = { x: 0, y: 0, z: 0 };
    var baseOverlayRotation = Quat.fromVec3Degrees(baseOverlayAngles);

    var baseOfEntityOverlay = Overlays.addOverlay("rectangle3d", {
                    position: { x:0, y: 0, z: 0},
                    color: { red: 0, green: 0, blue: 0},
                    alpha: 1,
                    solid: false,
                    visible: false,
                    lineWidth: 2.0,
                    isDashedLine: true
                });

    var baseOfEntityProjectionOverlay = Overlays.addOverlay("rectangle3d", {
                    position: { x:0, y: 0, z: 0},
                    size: 1,
                    color: { red: 51, green: 152, blue: 203 },
                    alpha: 0.5,
                    solid: true,
                    visible: false,
                    rotation: baseOverlayRotation
                });

    var heightOfEntityOverlay = Overlays.addOverlay("line3d", {
                    position: { x:0, y: 0, z: 0},
                    end: { x:0, y: 0, z: 0},
                    color: { red: 0, green: 0, blue: 0},
                    alpha: 1,
                    solid: false,
                    visible: false,
                    lineWidth: 2.0,
                    isDashedLine: true
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

    var yawHandleAngles = { x: 90, y: 90, z: 0 };
    var yawHandleRotation = Quat.fromVec3Degrees(yawHandleAngles);
    var yawHandle = Overlays.addOverlay("billboard", {
                                        url: "https://s3.amazonaws.com/uploads.hipchat.com/33953/231323/oocBjCwXpWlHpF9/rotate_arrow_black.png",
                                        position: { x:0, y: 0, z: 0},
                                        color: { red: 0, green: 0, blue: 255 },
                                        alpha: 0.3,
                                        visible: false,
                                        size: 0.1,
                                        scale: 0.1,
                                        rotation: yawHandleRotation,
                                        isFacingAvatar: false
                                      });


    var pitchHandleAngles = { x: 90, y: 0, z: 90 };
    var pitchHandleRotation = Quat.fromVec3Degrees(pitchHandleAngles);
    var pitchHandle = Overlays.addOverlay("billboard", {
                                        url: "https://s3.amazonaws.com/uploads.hipchat.com/33953/231323/oocBjCwXpWlHpF9/rotate_arrow_black.png",
                                        position: { x:0, y: 0, z: 0},
                                        color: { red: 0, green: 0, blue: 255 },
                                        alpha: 0.3,
                                        visible: false,
                                        size: 0.1,
                                        scale: 0.1,
                                        rotation: pitchHandleRotation,
                                        isFacingAvatar: false
                                      });


    var rollHandleAngles = { x: 0, y: 0, z: 180 };
    var rollHandleRotation = Quat.fromVec3Degrees(rollHandleAngles);
    var rollHandle = Overlays.addOverlay("billboard", {
                                        url: "https://s3.amazonaws.com/uploads.hipchat.com/33953/231323/oocBjCwXpWlHpF9/rotate_arrow_black.png",
                                        position: { x:0, y: 0, z: 0},
                                        color: { red: 0, green: 0, blue: 255 },
                                        alpha: 0.3,
                                        visible: false,
                                        size: 0.1,
                                        scale: 0.1,
                                        rotation: rollHandleRotation,
                                        isFacingAvatar: false
                                      });

    that.cleanup = function () {
        Overlays.deleteOverlay(selectionBox);
        Overlays.deleteOverlay(baseOfEntityOverlay);
        Overlays.deleteOverlay(baseOfEntityProjectionOverlay);
        Overlays.deleteOverlay(heightOfEntityOverlay);

        Overlays.deleteOverlay(yawHandle);
        Overlays.deleteOverlay(pitchHandle);
        Overlays.deleteOverlay(rollHandle);

        Overlays.deleteOverlay(rotateOverlayInner);
        Overlays.deleteOverlay(rotateOverlayOuter);
        Overlays.deleteOverlay(rotateOverlayCurrent);

    };

    that.showSelection = function (entityID, properties) {
    
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
        
        Overlays.editOverlay(selectionBox, 
                            { 
                                visible: false,
                                solid:false,
                                lineWidth: 2.0,
                                position: { x: properties.position.x,
                                            y: properties.position.y,
                                            z: properties.position.z },

                                dimensions: properties.dimensions,
                                rotation: properties.rotation,

                                pulseMin: 0.1,
                                pulseMax: 1.0,
                                pulsePeriod: 4.0,
                                glowLevelPulse: 1.0,
                                alphaPulse: 0.5,
                                colorPulse: -0.5
                            });

        Overlays.editOverlay(baseOfEntityOverlay, 
                            { 
                                visible: true,
                                position: { x: properties.position.x,
                                            y: properties.position.y - halfDimensions.y,
                                            z: properties.position.z },

                                dimensions: { x: properties.dimensions.x, y: properties.dimensions.z },
                                rotation: properties.rotation,
                            });

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

        Overlays.editOverlay(heightOfEntityOverlay, 
                            { 
                                visible: true,
                                position: { x: properties.position.x - halfDimensions.x,
                                            y: properties.position.y - halfDimensions.y,
                                            z: properties.position.z - halfDimensions.z},

                                end:      { x: properties.position.x - halfDimensions.x,
                                            y: properties.position.y + halfDimensions.y,
                                            z: properties.position.z - halfDimensions.z},

                                rotation: properties.rotation,
                            });
                            

        Overlays.editOverlay(rotateOverlayInner, 
                            { 
                                visible: true,
                                position: { x: properties.position.x,
                                            y: properties.position.y - (properties.dimensions.y / 2),
                                            z: properties.position.z},

                                size: innerRadius,
                                innerRadius: 0.9,
                                alpha: innerAlpha
                            });

        Overlays.editOverlay(rotateOverlayOuter, 
                            { 
                                visible: true,
                                position: { x: properties.position.x,
                                            y: properties.position.y - (properties.dimensions.y / 2),
                                            z: properties.position.z},

                                size: outerRadius,
                                innerRadius: 0.9,
                                startAt: 90,
                                endAt: 405,
                                alpha: outerAlpha
                            });

        Overlays.editOverlay(rotateOverlayCurrent, 
                            { 
                                visible: true,
                                position: { x: properties.position.x,
                                            y: properties.position.y - (properties.dimensions.y / 2),
                                            z: properties.position.z},

                                size: outerRadius,
                                startAt: 45,
                                endAt: 90,
                                innerRadius: 0.9
                            });

        Overlays.editOverlay(yawHandle, 
                            { 
                                visible: true,
                                position: { x: properties.position.x - (properties.dimensions.x / 2),
                                            y: properties.position.y - (properties.dimensions.y / 2),
                                            z: properties.position.z - (properties.dimensions.z / 2)},

                                //dimensions: properties.dimensions,
                                //rotation: properties.rotation
                            });

        Overlays.editOverlay(pitchHandle, 
                            { 
                                visible: true,
                                position: { x: properties.position.x + (properties.dimensions.x / 2),
                                            y: properties.position.y + (properties.dimensions.y / 2),
                                            z: properties.position.z - (properties.dimensions.z / 2)},
                            });

        Overlays.editOverlay(rollHandle, 
                            { 
                                visible: true,
                                position: { x: properties.position.x - (properties.dimensions.x / 2),
                                            y: properties.position.y + (properties.dimensions.y / 2),
                                            z: properties.position.z + (properties.dimensions.z / 2)},
                            });


        Entities.editEntity(entityID, { localRenderAlpha: 0.1 });
    };

    that.hideSelection = function (entityID) {
        Overlays.editOverlay(selectionBox, { visible: false });
        Overlays.editOverlay(baseOfEntityOverlay, { visible: false });
        Overlays.editOverlay(baseOfEntityProjectionOverlay, { visible: false });
        Overlays.editOverlay(heightOfEntityOverlay, { visible: false });

        Overlays.editOverlay(yawHandle, { visible: false });
        Overlays.editOverlay(pitchHandle, { visible: false });
        Overlays.editOverlay(rollHandle, { visible: false });

        Overlays.editOverlay(rotateOverlayInner, { visible: false });
        Overlays.editOverlay(rotateOverlayOuter, { visible: false });
        Overlays.editOverlay(rotateOverlayCurrent, { visible: false });

        Entities.editEntity(entityID, { localRenderAlpha: 1.0 });
    };
    
    return that;

}());

