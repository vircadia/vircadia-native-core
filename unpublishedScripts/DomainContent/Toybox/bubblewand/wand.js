//  wand.js
//  part of bubblewand
//
//  Script Type: Entity Script
//  Created by James B. Pollack @imgntn -- 09/03/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Makes bubbles when you wave the object around.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

(function() {

    Script.include("../libraries/utils.js");

    var BUBBLE_MODEL = "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/bubblewand/bubble.fbx";

    var BUBBLE_INITIAL_DIMENSIONS = {
        x: 0.01,
        y: 0.01,
        z: 0.01
    };

    var BUBBLE_LIFETIME_MIN = 3;
    var BUBBLE_LIFETIME_MAX = 8;
    var BUBBLE_SIZE_MIN = 0.02;
    var BUBBLE_SIZE_MAX = 0.1;
    var BUBBLE_LINEAR_DAMPING = 0.2;
    var BUBBLE_GRAVITY_MIN = 0.1;
    var BUBBLE_GRAVITY_MAX = 0.3;
    var GROWTH_FACTOR = 0.005;
    var SHRINK_FACTOR = 0.001;
    var SHRINK_LOWER_LIMIT = 0.02;
    var WAND_TIP_OFFSET = 0.095;
    var VELOCITY_THRESHOLD = 0.5;

    //this helps us get the time passed since the last function call, for use in velocity calculations
    function interval() {
        var lastTime = new Date().getTime() / 1000;

        return function getInterval() {
            var newTime = new Date().getTime() / 1000;
            var delta = newTime - lastTime;
            lastTime = newTime;
            return delta;
        };
    }

    var checkInterval = interval();

    function BubbleWand() {
        return;
    }

    BubbleWand.prototype = {
        timePassed: null,
        currentBubble: null,
        preload: function(entityID) {
            this.entityID = entityID;
        },
        getWandTipPosition: function(properties) {
            //the tip of the wand is going to be in a different place than the center, so we move in space relative to the model to find that position
            var upVector = Quat.getUp(properties.rotation);
            var upOffset = Vec3.multiply(upVector, WAND_TIP_OFFSET);
            var wandTipPosition = Vec3.sum(properties.position, upOffset);
            return wandTipPosition;
        },
        addCollisionsToBubbleAfterCreation: function(bubble) {
            //if the bubble collide immediately, we get weird effects.  so we add collisions after release
            Entities.editEntity(bubble, {
                dynamic: true
            });
        },
        randomizeBubbleGravity: function() {
            //change up the gravity a little bit for variation in floating effects
            var randomNumber = randFloat(BUBBLE_GRAVITY_MIN, BUBBLE_GRAVITY_MAX);
            var gravity = {
                x: 0,
                y: -randomNumber,
                z: 0
            };
            return gravity;
        },
        growBubbleWithWandVelocity: function(properties, deltaTime) {
            //get the wand and tip position for calculations
            var wandPosition = properties.position;
            this.getWandTipPosition(properties);
            // velocity = change in position / time
            var velocity = Vec3.multiply(Vec3.subtract(wandPosition, this.lastPosition), 1 / deltaTime);


            var velocityStrength = Vec3.length(velocity);

            //store the last position of the wand for velocity calculations
            this.lastPosition = wandPosition;

            //actually grow the bubble
            var dimensions = Entities.getEntityProperties(this.currentBubble, "dimensions").dimensions;

            if (velocityStrength > VELOCITY_THRESHOLD) {
                //add some variation in bubble sizes
                var bubbleSize = randFloat(BUBBLE_SIZE_MIN, BUBBLE_SIZE_MAX);
                //release the bubble if its dimensions are bigger than the bubble size
                if (dimensions.x > bubbleSize) {

                    //bubbles pop after existing for a bit -- so set a random lifetime
                    var lifetime = randInt(BUBBLE_LIFETIME_MIN, BUBBLE_LIFETIME_MAX);

                    //edit the bubble properties at release
                    Entities.editEntity(this.currentBubble, {
                        velocity: velocity,
                        lifetime: lifetime,
                        gravity: this.randomizeBubbleGravity()
                    });

                    //wait to make the bubbles collidable, so that they dont hit each other and the wand
                    Script.setTimeout(this.addCollisionsToBubbleAfterCreation(this.currentBubble), lifetime / 2);

                    //release the bubble -- when we create a new bubble, it will carry on and this update loop will
                    // affect the new bubble
                    this.createBubbleAtTipOfWand();
                    return;
                } else {
                    //grow small bubbles
                    dimensions.x += GROWTH_FACTOR * velocityStrength;
                    dimensions.y += GROWTH_FACTOR * velocityStrength;
                    dimensions.z += GROWTH_FACTOR * velocityStrength;

                }

            } else {
                // if the wand is not moving, make the current bubble smaller
                if (dimensions.x >= SHRINK_LOWER_LIMIT) {
                    dimensions.x -= SHRINK_FACTOR;
                    dimensions.y -= SHRINK_FACTOR;
                    dimensions.z -= SHRINK_FACTOR;
                }

            }

            //adjust the bubble dimensions
            Entities.editEntity(this.currentBubble, {
                dimensions: dimensions
            });
        },
        createBubbleAtTipOfWand: function() {

            //create a new bubble at the tip of the wand
            var properties = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
            var wandPosition = properties.position;

            //store the position of the tip for use in velocity calculations
            this.lastPosition = wandPosition;

            //create a bubble at the wand tip
            this.currentBubble = Entities.addEntity({
                name: 'Bubble',
                type: 'Model',
                modelURL: BUBBLE_MODEL,
                position: this.getWandTipPosition(properties),
                dimensions: BUBBLE_INITIAL_DIMENSIONS,
                dynamic: false,
                collisionless: true,
                damping: BUBBLE_LINEAR_DAMPING,
                shapeType: "sphere"
            });

        },
        startNearGrab: function() {
            //create a bubble to grow at the start of the grab
            if (this.currentBubble === null) {
                this.createBubbleAtTipOfWand();
            }
        },
        startEquip: function(id, params) {
            this.startNearGrab(id, params);
        },
        continueNearGrab: function() {
            var deltaTime = checkInterval();
            //only get the properties that we need
            var properties = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);

            var wandTipPosition = this.getWandTipPosition(properties);

            //update the bubble to stay with the wand tip
            Entities.editEntity(this.currentBubble, {
                position: wandTipPosition,
            });
            this.growBubbleWithWandVelocity(properties, deltaTime);

        },
        continueEquip: function() {
            this.continueNearGrab();
        },
        releaseGrab: function() {
            //delete the  current buble and reset state when the wand is released
            Entities.deleteEntity(this.currentBubble);
            this.currentBubble = null;
        },
        releaseEquip: function() {
            this.releaseGrab();
        },

    };

    return new BubbleWand();

});
