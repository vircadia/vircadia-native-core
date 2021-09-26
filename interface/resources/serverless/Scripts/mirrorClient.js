//
//  mirrorClient.js
//
//  Created by Patrick Manalich
//  Edited by Rebecca Stankus on 8/30/17.
//  Edited by David Back on 11/17/17.
//  Edited by Anna Brewer on 7/12/19.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// Attach `mirrorClient.js` to a box entity whose z dimension is very small,
// and whose x and y dimensions are up to you. See comments in `mirrorReflection.js`
// for more information about the mirror on/off zone.

"use strict";

(function () { // BEGIN LOCAL SCOPE

    // VARIABLES
    /* globals utils, Render */
    var _this = this;
    var MAX_MIRROR_RESOLUTION_SIDE_PX = 960;        // The max pixel resolution of the long side of the mirror
    var ZERO_ROT = { w: 1, x: 0, y: 0, z: 0 };   // Constant quaternion for a rotation of 0
    var FAR_CLIP_DISTANCE = 16;     // The far clip distance for the spectator camera when the mirror is on
    var mirrorLocalEntityID = false;            // The entity ID of the local entity that displays the mirror reflection
    var mirrorLocalEntityRunning;       // True if mirror local entity is reflecting, false otherwise
    var mirrorLocalEntityOffset = 0.01; // The distance between the center of the mirror and the mirror local entity
    var spectatorCameraConfig = Render.getConfig("SecondaryCamera");    // Render configuration for the spectator camera
    var lastDimensions = { x: 0, y: 0 };        // The previous dimensions of the mirror
    var previousFarClipDistance;    // Store the specator camera's previous far clip distance that we override for the mirror

    // LOCAL FUNCTIONS    
    function isPositionInsideBox(position, boxProperties) {
        var localPosition = Vec3.multiplyQbyV(Quat.inverse(boxProperties.rotation), 
                                              Vec3.subtract(MyAvatar.position, boxProperties.position));
        var halfDimensions = Vec3.multiply(boxProperties.dimensions, 0.5);
        return -halfDimensions.x <= localPosition.x &&
                halfDimensions.x >= localPosition.x &&
               -halfDimensions.y <= localPosition.y &&
                halfDimensions.y >= localPosition.y &&
               -halfDimensions.z <= localPosition.z &&
                halfDimensions.z >= localPosition.z;
    }
    
    // When x or y dimensions of the mirror change - reset the resolution of the 
    // spectator camera and edit the mirror local entity to adjust for the new dimensions
    function updateMirrorDimensions(forceUpdate) {

        if (!mirrorLocalEntityID) {
            return;
        }

        if (mirrorLocalEntityRunning) {
            var newDimensions = Entities.getEntityProperties(_this.entityID, 'dimensions').dimensions;

            if (forceUpdate === true || (newDimensions.x != lastDimensions.x || newDimensions.y != lastDimensions.y)) {
                var mirrorResolution = _this.calculateMirrorResolution(newDimensions);
                spectatorCameraConfig.resetSizeSpectatorCamera(mirrorResolution.x, mirrorResolution.y);
                Entities.editEntity(mirrorLocalEntityID, {
                    dimensions: {
                        x: (Math.max(newDimensions.x, newDimensions.y)),
                        y: (Math.max(newDimensions.x, newDimensions.y)),
                        z: 0
                    }
                });
            }
            lastDimensions = newDimensions;
        }
    }

    // Takes in an mirror scalar number which is used for the index of "halfDimSigns" that is needed to adjust the mirror 
    // local entity's position. Deletes and re-adds the mirror local entity so the url and position are updated.
    function createMirrorLocalEntity() {
        
        if (mirrorLocalEntityID) {
            Entities.deleteEntity(mirrorLocalEntityID);
            mirrorLocalEntityID = false;
        }

        if (mirrorLocalEntityRunning) {
            mirrorLocalEntityID = Entities.addEntity({
                type: "Image",
                name: "mirrorLocalEntity",
                imageURL: "resource://spectatorCameraFrame",
                emissive: true,
                parentID: _this.entityID,
                alpha: 1,
                localPosition: {
                    x: 0,
                    y: 0,
                    z: mirrorLocalEntityOffset
                },
                localRotation: Quat.fromPitchYawRollDegrees(0, 0, 180),
                isVisibleInSecondaryCamera: false
            }, "local");
            
            updateMirrorDimensions(true);
        }
        
    }

    _this.calculateMirrorResolution = function(entityDimensions) {
        var mirrorResolutionX, mirrorResolutionY;
        if (entityDimensions.x > entityDimensions.y) {
            mirrorResolutionX = MAX_MIRROR_RESOLUTION_SIDE_PX;
            mirrorResolutionY = Math.round(mirrorResolutionX * entityDimensions.y / entityDimensions.x);
        } else {
            mirrorResolutionY = MAX_MIRROR_RESOLUTION_SIDE_PX;
            mirrorResolutionX = Math.round(mirrorResolutionY * entityDimensions.x / entityDimensions.y);
        }

        var resolution = {
            "x": mirrorResolutionX,
            "y": mirrorResolutionY
        };

        return resolution;
    };
    
    // Sets up spectator camera to render the mirror, calls 'createMirrorLocalEntity' once to set up
    // mirror local entity, then connects 'updateMirrorDimensions' to update dimension changes
    _this.mirrorLocalEntityOn = function(onPreload) {
        if (!mirrorLocalEntityRunning) {
            if (!spectatorCameraConfig.attachedEntityId) {
                mirrorLocalEntityRunning = true;
                spectatorCameraConfig.mirrorProjection = true;
                spectatorCameraConfig.attachedEntityId = _this.entityID;
                previousFarClipDistance = spectatorCameraConfig.farClipPlaneDistance;
                spectatorCameraConfig.farClipPlaneDistance = FAR_CLIP_DISTANCE;
                var entityProperties = Entities.getEntityProperties(_this.entityID, ['dimensions']);
                var mirrorEntityDimensions = entityProperties.dimensions;
                var initialResolution = _this.calculateMirrorResolution(mirrorEntityDimensions);
                spectatorCameraConfig.resetSizeSpectatorCamera(initialResolution.x, initialResolution.y);
                spectatorCameraConfig.enableSecondaryCameraRenderConfigs(true);
                createMirrorLocalEntity();
                Script.update.connect(updateMirrorDimensions);
            } else {
                print("Cannot turn on mirror if spectator camera is already in use");
            }
        }
    };

    // Resets spectator camera, deletes the mirror local entity, and disconnects 'updateMirrorDimensions' 
    _this.mirrorLocalEntityOff = function() {
        if (mirrorLocalEntityRunning) {
            spectatorCameraConfig.enableSecondaryCameraRenderConfigs(false);
            spectatorCameraConfig.mirrorProjection = false;
            spectatorCameraConfig.attachedEntityId = null;
            spectatorCameraConfig.farClipPlaneDistance = previousFarClipDistance;
            if (mirrorLocalEntityID) {
                Entities.deleteEntity(mirrorLocalEntityID);
                mirrorLocalEntityID = false;
            }
            Script.update.disconnect(updateMirrorDimensions);
            mirrorLocalEntityRunning = false;
        }
    };
    
    // ENTITY FUNCTIONS
    _this.preload = function(entityID) {
        _this.entityID = entityID;
        mirrorlocalEntityRunning = false;
    
        // If avatar is already inside the mirror zone at the time preload is called then turn on the mirror
        var children = Entities.getChildrenIDs(_this.entityID);
        var childZero = Entities.getEntityProperties(children[0]);
        if (isPositionInsideBox(MyAvatar.position, {
                position: childZero.position, 
                rotation: childZero.rotation, 
                dimensions: childZero.dimensions
            })) {
            _this.mirrorLocalEntityOn(true);
        }
    };
    
    // Turn off mirror on unload
    _this.unload = function(entityID) {
        _this.mirrorLocalEntityOff();
    };
});
