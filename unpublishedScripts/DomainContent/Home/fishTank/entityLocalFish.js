//
//  entityLocalFish.js
//  examples
//
//  Philip Rosedale
//  Copyright 2016 High Fidelity, Inc.   
//  Fish smimming around in a space in front of you
//  These fish are overlays, meaning they can only be seen by you. 
//  Attach this entity script to an object to create fish that stay within the 
//  dimensions of the object.   
//   
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function(){ 
 
    var NUM_FISH = 25;
    var FISH_SCALE = 0.45;
    var MAX_SIGHT_DISTANCE = 0.5;
    var MIN_SEPARATION = 0.08;  
    var AVOIDANCE_FORCE = 0.05;  
    var COHESION_FORCE = 0.025;  
    var ALIGNMENT_FORCE = 0.025;  
    var SWIMMING_FORCE = 0.025;   
    var SWIMMING_SPEED = 0.5;  
    var VELOCITY_FOLLOW_RATE = 0.25;
    var FISH_MODEL_URL = "atp:/fishTank/goodfish5.fbx";

    var fishLoaded = false; 
    var fish = [];

    var lowerCorner = { x: 0, y: 0, z: 0 };
    var upperCorner = { x: 0, y: 0, z: 0 };

    var entityID;
    
    function update(deltaTime) {

        var averageVelocity = { x: 0, y: 0, z: 0 };
        var averagePosition = { x: 0, y: 0, z: 0 };

        for (var i = 0; i < fish.length; i++) {
            
            // Update position by velocity
            fish[i].position = Vec3.sum(fish[i].position, Vec3.multiply(deltaTime, fish[i].velocity));

            averageVelocity = { x: 0, y: 0, z: 0 };
            averagePosition = { x: 0, y: 0, z: 0 };

            var othersCounted = 0;
            for (var j = 0; j < fish.length; j++) {
                if (i != j) {
                    // Get only the properties we need, because that is faster
                    var separation = Vec3.distance(fish[i].position, fish[j].position);
                    if (separation < MAX_SIGHT_DISTANCE) {
                        averageVelocity = Vec3.sum(averageVelocity, fish[j].velocity);
                        averagePosition = Vec3.sum(averagePosition, fish[j].position);
                        othersCounted++;
                    }
                    if (separation < MIN_SEPARATION) {
                        // Separation:  Push away if you are too close to someone else
                        var pushAway = Vec3.multiply(Vec3.normalize(Vec3.subtract(fish[i].position, fish[j].position)), AVOIDANCE_FORCE);
                        fish[i].velocity = Vec3.sum(fish[i].velocity, pushAway);
                    }
                }
            }

            if (othersCounted > 0) {
                averageVelocity = Vec3.multiply(averageVelocity, 1.0 / othersCounted);
                averagePosition = Vec3.multiply(averagePosition, 1.0 / othersCounted);
                //  Alignment: Follow group's direction and speed
                fish[i].velocity = Vec3.mix(fish[i].velocity, Vec3.multiply(Vec3.normalize(averageVelocity), Vec3.length(fish[i].velocity)), ALIGNMENT_FORCE);
                // Cohesion: Steer towards center of flock
                var towardCenter = Vec3.subtract(averagePosition, fish[i].position);
                fish[i].velocity = Vec3.mix(fish[i].velocity, Vec3.multiply(Vec3.normalize(towardCenter), Vec3.length(fish[i].velocity)), COHESION_FORCE);
            }

            //  Try to swim at a constant speed
            fish[i].velocity = Vec3.mix(fish[i].velocity, Vec3.multiply(Vec3.normalize(fish[i].velocity), SWIMMING_SPEED), SWIMMING_FORCE);

            //  Keep fish in their 'tank' 
            if (fish[i].position.x < lowerCorner.x) {
                fish[i].position.x = lowerCorner.x; 
                fish[i].velocity.x *= -1.0;
            } else if (fish[i].position.x > upperCorner.x) {
                fish[i].position.x = upperCorner.x; 
                fish[i].velocity.x *= -1.0;
            }
            if (fish[i].position.y < lowerCorner.y) {
                fish[i].position.y = lowerCorner.y; 
                fish[i].velocity.y *= -1.0;
            } else if (fish[i].position.y > upperCorner.y) {
                fish[i].position.y = upperCorner.y; 
                fish[i].velocity.y *= -1.0;
            } 
            if (fish[i].position.z < lowerCorner.z) {
                fish[i].position.z = lowerCorner.z; 
                fish[i].velocity.z *= -1.0;
            } else if (fish[i].position.z > upperCorner.z) {
                fish[i].position.z = upperCorner.z; 
                fish[i].velocity.z *= -1.0;
            } 

            //  Orient in direction of velocity 
            var rotation = Quat.rotationBetween(Vec3.UNIT_NEG_Z, fish[i].velocity);
            fish[i].rotation = Quat.slerp(fish[i].rotation, rotation, VELOCITY_FOLLOW_RATE);

            //  Update the actual 3D overlay
            Overlays.editOverlay(fish[i].overlayID, {   position: fish[i].position, 
                                                        velocity: fish[i].velocity, 
                                                        rotation: fish[i].rotation });
        }
    }   

    function randomVector(scale) {
        return { x: Math.random() * scale - scale / 2.0, y: Math.random() * scale - scale / 2.0, z: Math.random() * scale - scale / 2.0 };
    }

    function loadFish(entityID, howMany) { 
        var STARTING_FRACTION = 0.25; 
        var properties = Entities.getEntityProperties(entityID);
        lowerCorner = { x: properties.position.x - properties.dimensions.x / 2, 
                        y: properties.position.y - properties.dimensions.y / 2, 
                        z: properties.position.z - properties.dimensions.z / 2 };
        upperCorner = { x: properties.position.x + properties.dimensions.x / 2, 
                        y: properties.position.y + properties.dimensions.y / 2, 
                        z: properties.position.z + properties.dimensions.z / 2 };

        for (var i = 0; i < howMany; i++) {
            var position = { 
                x: lowerCorner.x + (upperCorner.x - lowerCorner.x) / 2.0 + (Math.random() - 0.5) * (upperCorner.x - lowerCorner.x) * STARTING_FRACTION, 
                y: lowerCorner.y + (upperCorner.y - lowerCorner.y) / 2.0 + (Math.random() - 0.5) * (upperCorner.y - lowerCorner.y) * STARTING_FRACTION, 
                z: lowerCorner.z + (upperCorner.z - lowerCorner.x) / 2.0 + (Math.random() - 0.5) * (upperCorner.z - lowerCorner.z) * STARTING_FRACTION
                }; 
            var rotation = { x: 0, y: 0, z: 0, w: 1 };
            fish.push({
                    overlayID:  Overlays.addOverlay("model", {
                                                    url: FISH_MODEL_URL,
                                                    position: position,
                                                    rotation: rotation,
                                                    scale: FISH_SCALE, 
                                                    visible: true }),
                    position: position,
                    rotation: rotation,
                    velocity: randomVector(SWIMMING_SPEED)
                    });
        }
        print("Fish loaded");
    }

    this.preload = function(entityID) { 
        print("Fish Preload v3");
        loadFish(entityID, NUM_FISH);
        fishLoaded = true;
        Script.update.connect(update);

    }; 

    this.unload = function() {
        Script.update.disconnect(update);
        print("Fish Unload");
        for (var i = 0; i < fish.length; i++) {
            Overlays.deleteOverlay(fish[i].overlayID);
        }
    };
})
