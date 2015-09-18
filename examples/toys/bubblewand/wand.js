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


(function() {

    Script.include("../../utilities.js");
    Script.include("../../libraries/utils.js");

    var BUBBLE_MODEL = "http://hifi-public.s3.amazonaws.com/james/bubblewand/models/bubble/bubble.fbx";
    var BUBBLE_SCRIPT = Script.resolvePath('bubble.js');
//test
    var BUBBLE_USER_DATA_KEY = "BubbleKey";
    var BUBBLE_INITIAL_DIMENSIONS = {
        x: 0.01,
        y: 0.01,
        z: 0.01
    }

    var BUBBLE_LIFETIME_MIN = 3;
    var BUBBLE_LIFETIME_MAX = 8;
    var BUBBLE_SIZE_MIN = 1;
    var BUBBLE_SIZE_MAX = 5;
    var BUBBLE_DIVISOR = 50;
    var BUBBLE_LINEAR_DAMPING = 0.4;
    var GROWTH_FACTOR = 0.005;
    var SHRINK_FACTOR = 0.001;
    var SHRINK_LOWER_LIMIT = 0.02;
    var WAND_TIP_OFFSET = 0.095;
    var VELOCITY_STRENGTH_LOWER_LIMIT = 0.01;
    var VELOCITY_STRENGH_MAX = 10;
    var VELOCITY_THRESHOLD = 0.5;

    var _this;

    var BubbleWand = function() {
        _this = this;
    }

    BubbleWand.prototype = {
        bubbles: [],
        currentBubble: null,
        preload: function(entityID) {
            this.entityID = entityID;
            this.properties = Entities.getEntityProperties(this.entityID);
            Script.update.connect(this.update);
        },
        unload: function(entityID) {
            Script.update.disconnect(this.update);
        },
        update: function(deltaTime) {
            print('BW update')
            var GRAB_USER_DATA_KEY = "grabKey";
            var defaultGrabData = {
                activated: false,
                avatarId: null
            };
            var grabData = getEntityCustomData(GRAB_USER_DATA_KEY, _this.entityID, defaultGrabData);
            print('grabData'+JSON.stringify(grabData))
            if (grabData.activated && grabData.avatarId === MyAvatar.sessionUUID) {

                // remember we're being grabbed so we can detect being released
                _this.beingGrabbed = true;
                print('being grabbed')
                //the first time we want to make a bubble
                if (_this.currentBubble === null) {
                    _this.createBubbleAtTipOfWand();
                }

                var properties = Entities.getEntityProperties(_this.entityID);
                var dt = deltaTime;
                _this.growBubbleWithWandVelocity(properties, dt);

                var wandTipPosition = _this.getWandTipPosition(properties);

                //update the bubble to stay with the wand tip
                Entities.editEntity(_this.currentBubble, {
                    position: wandTipPosition,

                });

            } else if (_this.beingGrabbed) {
                // if we are not being grabbed, and we previously were, then we were just released, remember that
                print('let go')
                _this.beingGrabbed = false;

                //remove the  current bubble when the wand is released
                Entities.deleteEntity(_this.currentBubble);
                _this.currentBubble = null
                return
            }
                print('not grabbed')

        },
        getWandTipPosition: function(properties) {

            //the tip of the wand is going to be in a different place than the center, so we move in space relative to the model to find that position

            var upVector = Quat.getUp(properties.rotation);
            var frontVector = Quat.getFront(properties.rotation);
            var upOffset = Vec3.multiply(upVector, WAND_TIP_OFFSET);
            var wandTipPosition = Vec3.sum(properties.position, upOffset);
            this.wandTipPosition = wandTipPosition;
            return wandTipPosition
        },
        addCollisionsToBubbleAfterCreation: function(bubble) {

            print('adding collisions to bubble' + bubble);
            Entities.editEntity(bubble, {
                collisionsWillMove: true
            })

        },
        randomizeBubbleGravity: function() {

            var randomNumber = randInt(0, 3);
            var gravity = {
                x: 0,
                y: -randomNumber / 10,
                z: 0
            }
            return gravity
        },
        growBubbleWithWandVelocity: function(properties, deltaTime) {
            print('grow bubble')
            var wandPosition = properties.position;
            var wandTipPosition = this.getWandTipPosition(properties)


            var distance = Vec3.subtract(wandPosition, this.lastPosition);
            var velocity = Vec3.multiply(distance, 1 / deltaTime);


            var velocityStrength = Vec3.length(velocity);
            velocityStrength = velocityStrength;
            // if (velocityStrength < VELOCITY_STRENGTH_LOWER_LIMIT) {
            //     velocityStrength = 0
            // }

            // if (velocityStrength > VELOCITY_STRENGTH_MAX) {
            //     velocityStrength = VELOCITY_STRENGTH_MAX
            // }

            //store the last position of the wand for velocity calculations
            this.lastPosition = wandPosition;

            //actually grow the bubble
            var dimensions = Entities.getEntityProperties(this.currentBubble).dimensions;

            if (velocityStrength > VELOCITY_THRESHOLD) {
                print('velocity over threshold')
                //add some variation in bubble sizes
                var bubbleSize = randInt(BUBBLE_SIZE_MIN, BUBBLE_SIZE_MAX);
                bubbleSize = bubbleSize / BUBBLE_DIVISOR;

                //release the bubble if its dimensions are bigger than the bubble size
                if (dimensions.x > bubbleSize) {

                    print('release the bubble')
                    //bubbles pop after existing for a bit -- so set a random lifetime
                    var lifetime = randInt(BUBBLE_LIFETIME_MIN, BUBBLE_LIFETIME_MAX);

                    Entities.editEntity(this.currentBubble, {
                        velocity: velocity,
                        lifetime: lifetime,
                        gravity: this.randomizeBubbleGravity()
                    });

                    //wait to make the bubbles collidable, so that they dont hit each other and the wand
                    Script.setTimeout(this.addCollisionsToBubbleAfterCreation(this.currentBubble), lifetime / 2);

                    this.setBubbleOwner(this.currentBubble);
                    //release the bubble -- when we create a new bubble, it will carry on and this update loop will affect the new bubble
                    this.createBubbleAtTipOfWand();
                    return
                } else {

                    // if the bubble is not yet full size, make the current bubble bigger
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
        setBubbleOwner: function(bubble) {
            print('SET BUBBLE OWNER', bubble)
            setEntityCustomData(BUBBLE_USER_DATA_KEY, bubble, {
                avatarID: MyAvatar.sessionUUID,
            });
        },
        createBubbleAtTipOfWand: function() {

            //create a new bubble at the tip of the wand
            var properties = Entities.getEntityProperties(this.entityID);
            var wandPosition = properties.position;

            wandTipPosition = this.getWandTipPosition(properties);
            this.wandTipPosition = wandTipPosition;

            //store the position of the tip for use in velocity calculations
            this.lastPosition = wandPosition;

            //create a bubble at the wand tip
            this.currentBubble = Entities.addEntity({
                name: 'Bubble',
                type: 'Model',
                modelURL: BUBBLE_MODEL,
                position: wandTipPosition,
                dimensions: BUBBLE_INITIAL_DIMENSIONS,
                collisionsWillMove: false,
                ignoreForCollisions: false,
                linearDamping: BUBBLE_LINEAR_DAMPING,
                shapeType: "sphere",
                script: BUBBLE_SCRIPT,
            });
            //add this bubble to an array of bubbles so we can keep track of them
            this.bubbles.push(this.currentBubble)

        }

    }

    return new BubbleWand();

})