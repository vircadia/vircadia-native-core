//  wand.js
//  part of bubblewand
//
//  Script Type: Entity Script
//  Created by James B. Pollack @imgntn -- 09/03/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Makes bubbles when you wave the object around.
//  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {

    Script.include("../../utilities.js");
    Script.include("../../libraries/utils.js");

    var BUBBLE_MODEL = "http://hifi-public.s3.amazonaws.com/james/bubblewand/models/bubble/bubble.fbx";
    var BUBBLE_SCRIPT = Script.resolvePath('bubble.js');

    var BUBBLE_GRAVITY = {
        x: 0,
        y: -0.1,
        z: 0
    };
    var BUBBLE_DIVISOR = 50;
    var BUBBLE_LIFETIME_MIN = 3;
    var BUBBLE_LIFETIME_MAX = 8;
    var BUBBLE_SIZE_MIN = 1;
    var BUBBLE_SIZE_MAX = 5;
    var GROWTH_FACTOR = 0.005;
    var SHRINK_FACTOR = 0.001;
    var SHRINK_LOWER_LIMIT = 0.02;
    var WAND_TIP_OFFSET = 0.05;
    var VELOCITY_STRENGTH_LOWER_LIMIT = 0.01;
    var VELOCITY_STRENGH_MAX = 10;
    var VELOCITY_STRENGTH_MULTIPLIER = 100;
    var VELOCITY_THRESHOLD = 1;


    var _this;

    var BubbleWand = function() {
        _this = this;
        print('WAND CONSTRUCTOR!')
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
            var GRAB_USER_DATA_KEY = "grabKey";
            var defaultGrabData = {
                activated: false,
                avatarId: null
            };
            var grabData = getEntityCustomData(GRAB_USER_DATA_KEY, _this.entityID, defaultGrabData);
            if (grabData.activated && grabData.avatarId === MyAvatar.sessionUUID) {
                print('being grabbed')

                  _this.beingGrabbed = true;


                if (_this.currentBubble === null) {
                    _this.spawnBubble();
                }
                var properties = Entities.getEntityProperties(_this.entityID);

                // remember we're being grabbed so we can detect being released
              

                // print out that we're being grabbed
                _this.growBubbleWithWandVelocity(properties);

                var wandTipPosition = _this.getWandTipPosition(properties);
                //update the bubble to stay with the wand tip
                Entities.editEntity(_this.currentBubble, {
                    position: wandTipPosition,

                });

            } else if (_this.beingGrabbed) {

                // if we are not being grabbed, and we previously were, then we were just released, remember that
                // and print out a message
                _this.beingGrabbed = false;
                Entities.deleteEntity(_this.currentBubble);
                return
            }


        },
        getWandTipPosition: function(properties) {
            var upVector = Quat.getUp(properties.rotation);
            var frontVector = Quat.getFront(properties.rotation);
            var upOffset = Vec3.multiply(upVector, WAND_TIP_OFFSET);
            var wandTipPosition = Vec3.sum(properties.position, upOffset);
            this.wandTipPosition = wandTipPosition;
            return wandTipPosition
        },
        growBubbleWithWandVelocity: function(properties) {
            print('grow bubble')
            var wandPosition = properties.position;
            var wandTipPosition = this.getWandTipPosition(properties)
          

            var velocity = Vec3.subtract(wandPosition, this.lastPosition)
            var velocityStrength = Vec3.length(velocity) * VELOCITY_STRENGTH_MULTIPLIER;

      

            // if (velocityStrength < VELOCITY_STRENGTH_LOWER_LIMIT) {
            //     velocityStrength = 0
            // }

            // if (velocityStrength > VELOCITY_STRENGTH_MAX) {
            //     velocityStrength = VELOCITY_STRENGTH_MAX
            // }

                  print('VELOCITY STRENGTH:::'+velocityStrength)
             print('V THRESH:'+  VELOCITY_THRESHOLD)
            print('debug 1')
            //store the last position of the wand for velocity calculations
            this.lastPosition = wandPosition;

            //actually grow the bubble
            var dimensions = Entities.getEntityProperties(this.currentBubble).dimensions;
            print('dim x   '+dimensions.x)
            if (velocityStrength > VELOCITY_THRESHOLD) {

                //add some variation in bubble sizes
                var bubbleSize = randInt(BUBBLE_SIZE_MIN, BUBBLE_SIZE_MAX);
                bubbleSize = bubbleSize / BUBBLE_DIVISOR;
            
                print('bubbleSize  '+ bubbleSize)
                //release the bubble if its dimensions are bigger than the bubble size
                if (dimensions.x > bubbleSize) {
                    //bubbles pop after existing for a bit -- so set a random lifetime
                    var lifetime = randInt(BUBBLE_LIFETIME_MIN, BUBBLE_LIFETIME_MAX);

                    Entities.editEntity(this.currentBubble, {
                        velocity: velocity,
                        lifetime: lifetime
                    });

                    //release the bubble -- when we create a new bubble, it will carry on and this update loop will affect the new bubble
                    this.spawnBubble();

                    return
                } else {
                    dimensions.x += GROWTH_FACTOR * velocityStrength;
                    dimensions.y += GROWTH_FACTOR * velocityStrength;
                    dimensions.z += GROWTH_FACTOR * velocityStrength;

                }
            } else {
                if (dimensions.x >= SHRINK_LOWER_LIMIT) {
                    dimensions.x -= SHRINK_FACTOR;
                    dimensions.y -= SHRINK_FACTOR;
                    dimensions.z -= SHRINK_FACTOR;
                }

            }

            //make the bubble bigger
            Entities.editEntity(this.currentBubble, {
                dimensions: dimensions
            });
        },
        spawnBubble: function() {

            //create a new bubble at the tip of the wand
            //the tip of the wand is going to be in a different place than the center, so we move in space relative to the model to find that position
            var properties = Entities.getEntityProperties(this.entityID);
            var wandPosition = properties.position;

            wandTipPosition = this.getWandTipPosition(properties);
            this.wandTipPosition = wandTipPosition;

            //store the position of the tip for use in velocity calculations
            this.lastPosition = wandPosition;

            //create a bubble at the wand tip
            this.currentBubble = Entities.addEntity({
                name:'Bubble',
                type: 'Model',
                modelURL: BUBBLE_MODEL,
                position: wandTipPosition,
                dimensions: {
                    x: 0.01,
                    y: 0.01,
                    z: 0.01
                },
                collisionsWillMove: true, //true
                ignoreForCollisions: false, //false
                gravity: BUBBLE_GRAVITY,
                shapeType: "sphere",
                script: BUBBLE_SCRIPT,
            });
            //add this bubble to an array of bubbles so we can keep track of them
            this.bubbles.push(this.currentBubble)

        }

    }

    return new BubbleWand();

})