//
//  magneticBlock.js
//
//  Created by Matti Lahtinen 4/3/2017
//  Copyright 2017 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Makes the entity the script is bound to connect to nearby, similarly sized entities, like a magnet.

(function() {
    var SNAPSOUND_SOURCE = SoundCache.getSound(Script.resolvePath("../../system/assets/sounds/entitySnap.wav?xrs"));
    var RANGE_MULTIPLER = 1.5;
    var MAX_SCALE = 2;
    var MIN_SCALE = 0.5;

    // Helper for detecting nearby objects near entityProperties, with the scale calculated by the dimensions of the object.
    function findEntitiesInRange(entityProperties) {
        var dimensions = entityProperties.dimensions;
        // Average of the dimensions instead of full value.
        return Entities.findEntities(entityProperties.position,
            ((dimensions.x + dimensions.y + dimensions.z) / 3) * RANGE_MULTIPLER);
    }

    function getNearestValidEntityProperties(releasedProperties) {
        var entities = findEntitiesInRange(releasedProperties);
        var nearestEntity = null;
        var nearest = Number.MAX_VALUE - 1;
        var releaseSize = Vec3.length(releasedProperties.dimensions);
        entities.forEach(function(entityId) {
            if (entityId !== releasedProperties.id) {
                var entity = Entities.getEntityProperties(entityId, ['position', 'rotation', 'dimensions']);
                var distance = Vec3.distance(releasedProperties.position, entity.position);
                var scale = releaseSize / Vec3.length(entity.dimensions);

                if (distance < nearest && (scale >= MIN_SCALE && scale <= MAX_SCALE)) {
                    nearestEntity = entity;
                    nearest = distance;
                }
            }
        });
        return nearestEntity;
    }
    // Create the 'class'
    function MagneticBlock() {}
    // Bind pre-emptive events
    MagneticBlock.prototype = {
        /*
          When script is bound to an entity, preload is the first callback called with the entityID.
          It will behave as the constructor
        */
        preload: function(id) {
            /*
              We will now override any existing userdata with the grabbable property.
              Only retrieving userData
            */
            var entityProperties = Entities.getEntityProperties(id, ['userData']);
            var userData = {
                grabbableKey: {}
            };
            // Check if existing userData field exists.
            if (entityProperties.userData && entityProperties.userData.length > 0) {
                try {
                    userData = JSON.parse(entityProperties.userData);
                    if (!userData.grabbableKey) {
                        userData.grabbableKey = {}; // If by random change there is no grabbableKey in the userData.
                    }
                } catch (e) {
                    // if user data is not valid json, we will simply overwrite it.
                }
            }
            // Object must be triggerable inorder to bind releaseGrabEvent
            userData.grabbableKey.grabbable = true;

            // Apply the new properties to entity of id
            Entities.editEntity(id, {
                userData: JSON.stringify(userData)
            });
            Script.scriptEnding.connect(function() {
                Script.removeEventHandler(id, "releaseGrab", this.releaseGrab);
            });
        },
        releaseGrab: function(entityId) {
            // Release grab is called with entityId,
            var released = Entities.getEntityProperties(entityId, ["position", "rotation", "dimensions"]);
            var target = getNearestValidEntityProperties(released);
            if (target !== null) {
                // We found nearest, now lets do the snap calculations
                // Plays the snap sound between the two objects.
                Audio.playSound(SNAPSOUND_SOURCE, {
                    volume: 1,
                    position: Vec3.mix(target.position, released.position, 0.5)
                });
                // Check Nearest Axis
                var difference = Vec3.subtract(released.position, target.position);
                var relativeDifference = Vec3.multiplyQbyV(Quat.inverse(target.rotation), difference);

                var abs = {
                    x: Math.abs(relativeDifference.x),
                    y: Math.abs(relativeDifference.y),
                    z: Math.abs(relativeDifference.z)
                };
                // Check what value is greater. and lock down to that axis.
                var newRelative = {
                    x: 0,
                    y: 0,
                    z: 0
                };
                if (abs.x >= abs.y && abs.x >= abs.z) {
                    newRelative.x = target.dimensions.x / 2 + released.dimensions.x / 2;
                    if (relativeDifference.x < 0) {
                        newRelative.x = -newRelative.x;
                    }
                } else if (abs.y >= abs.x && abs.y >= abs.z) {
                    newRelative.y = target.dimensions.y / 2 + released.dimensions.y / 2;
                    if (relativeDifference.y < 0) {
                        newRelative.y = -newRelative.y;
                    }
                } else if (abs.z >= abs.x && abs.z >= abs.y) {
                    newRelative.z = target.dimensions.z / 2 + released.dimensions.z / 2;
                    if (relativeDifference.z < 0) {
                        newRelative.z = -newRelative.z;
                    }
                }
                // Can be expanded upon to work in nearest 90 degree rotation as well, but was not in spec.
                var newPosition = Vec3.multiplyQbyV(target.rotation, newRelative);
                Entities.editEntity(entityId, {
                    // Script relies on the registrationPoint being at the very center of the object. Thus override.
                    registrationPoint: {
                        x: 0.5,
                        y: 0.5,
                        z: 0.5
                    },
                    rotation: target.rotation,
                    position: Vec3.sum(target.position, newPosition)
                });
                // Script relies on the registrationPoint being at the very center of the object. Thus override.
                Entities.editEntity(target.id, {
                    registrationPoint: {
                        x: 0.5,
                        y: 0.5,
                        z: 0.5
                    }
                });
            }
        }
    };
    return new MagneticBlock();
});
