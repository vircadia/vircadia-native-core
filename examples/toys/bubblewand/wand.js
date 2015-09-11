//  wand.js
//  part of bubblewand
//
//  Created by James B. Pollack @imgntn -- 09/03/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Makes bubbles when you wave the object around, or hold it near your mouth and make noise into the microphone.
//  
//  For the example, it's attached to a wand -- but you can attach it to whatever entity you want.  I dream of BubbleBees :) bzzzz...pop!
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

function convertRange(value, r1, r2) {
    return (value - r1[0]) * (r2[1] - r2[0]) / (r1[1] - r1[0]) + r2[0];
}

(function() {

    Script.include("https://raw.githubusercontent.com/highfidelity/hifi/master/examples/utilities.js");
    Script.include("https://raw.githubusercontent.com/highfidelity/hifi/master/examples/libraries/utils.js");


    // Script.include("../../utilities.js");
    // Script.include("../../libraries/utils.js");


    var bubbleModel = "http://hifi-public.s3.amazonaws.com/james/bubblewand/models/bubble/bubble.fbx";
    var popSound = SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop.wav");
    var bubbleScript = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/scripts/bubble.js?' + randInt(1, 10000);

    // var bubbleScript = 'http://localhost:8080/bubble.js?' + randInt(1, 10000); //for local testing

    var POP_SOUNDS = [
        SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop0.wav"),
        SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop1.wav"),
        SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop2.wav"),
        SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop3.wav")
    ]

    var overlays = false;

    //debug overlays for hand position to detect when wand is near avatar head
    var TARGET_SIZE = 0.5;
    var TARGET_COLOR = {
        red: 128,
        green: 128,
        blue: 128
    };
    var TARGET_COLOR_HIT = {
        red: 0,
        green: 255,
        blue: 0
    };

    var HAND_SIZE = 0.25;

    if (overlays) {


        var leftCubePosition = MyAvatar.getLeftPalmPosition();
        var rightCubePosition = MyAvatar.getRightPalmPosition();

        var leftHand = Overlays.addOverlay("cube", {
            position: leftCubePosition,
            size: HAND_SIZE,
            color: {
                red: 0,
                green: 0,
                blue: 255
            },
            alpha: 1,
            solid: false
        });

        var rightHand = Overlays.addOverlay("cube", {
            position: rightCubePosition,
            size: HAND_SIZE,
            color: {
                red: 255,
                green: 0,
                blue: 0
            },
            alpha: 1,
            solid: false
        });

        var gustZoneOverlay = Overlays.addOverlay("cube", {
            position: getGustDetectorPosition(),
            size: TARGET_SIZE,
            color: TARGET_COLOR,
            alpha: 1,
            solid: false
        });
    }



    function getGustDetectorPosition() {
        //put the zone in front of your avatar's face
        var DISTANCE_IN_FRONT = 0.2;
        var DISTANCE_UP = 0.5;
        var DISTANCE_TO_SIDE = 0.0;

        var up = Quat.getUp(MyAvatar.orientation);
        var front = Quat.getFront(MyAvatar.orientation);
        var right = Quat.getRight(MyAvatar.orientation);

        var upOffset = Vec3.multiply(up, DISTANCE_UP);
        var rightOffset = Vec3.multiply(right, DISTANCE_TO_SIDE);
        var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

        var offset = Vec3.sum(Vec3.sum(rightOffset, frontOffset), upOffset);
        var position = Vec3.sum(MyAvatar.position, offset);
        return position;
    }


    var BUBBLE_GRAVITY = {
        x: 0,
        y: -0.05,
        z: 0
    }

    var BUBBLE_PARTICLE_COLOR = {
        red: 0,
        blue: 255,
        green: 40
    }

    var wandEntity = this;

    this.preload = function(entityID) {
        // print('PRELOAD')
        this.entityID = entityID;
        this.properties = Entities.getEntityProperties(this.entityID);
        BubbleWand.originalProperties = this.properties;
        print('rotation???' + JSON.stringify(BubbleWand.originalProperties.rotation));

    }

    this.unload = function(entityID) {
        if (overlays) {
            Overlays.deleteOverlay(leftHand);
            Overlays.deleteOverlay(rightHand);
            Overlays.deleteOverlay(gustZoneOverlay);
        }

        Entities.editEntity(entityID, {
            name: ""
        });
        Script.update.disconnect(BubbleWand.update);
        Entities.deleteEntity(BubbleWand.currentBubble);
        while (BubbleWand.bubbles.length > 0) {
            Entities.deleteEntity(BubbleWand.bubbles.pop());
        }

    };


    var BubbleWand = {
        bubbles: [],
        timeSinceMoved: 0,
        resetAtTime: 5,
        currentBubble: null,
        update: function(dt) {
            BubbleWand.internalUpdate(dt);
        },
        internalUpdate: function(dt) {

            var _t = this;
            //get the current position of the wand
            var properties = Entities.getEntityProperties(wandEntity.entityID);
            var wandPosition = properties.position;
            //if the wand is in the gust detector, activate mouth mode and change the overlay color
            var hitTargetWithWand = findSphereSphereHit(wandPosition, HAND_SIZE / 2, getGustDetectorPosition(), TARGET_SIZE / 2)

            var velocity = Vec3.subtract(wandPosition, _t.lastPosition)
            var velocityStrength = Vec3.length(velocity) * 100;


            var upVector = Quat.getUp(properties.rotation);
            var frontVector = Quat.getFront(properties.rotation);
            var upOffset = Vec3.multiply(upVector, 0.2);
            var wandTipPosition = Vec3.sum(wandPosition, upOffset);
            _t.wandTipPosition = wandTipPosition;

            var mouthMode;

            if (hitTargetWithWand) {
                mouthMode = true;
            } else {
                mouthMode = false;
            }
            //print('velocityStrength'+velocityStrength)

            //we want to reset the object to its original position if its been a while since it has moved
            if (velocityStrength === 0) {
                _t.timeSinceMoved = _t.timeSinceMoved + dt;
                if (_t.timeSinceMoved > _t.resetAtTime) {
                    _t.timeSinceMoved = 0;
                    _t.returnToOriginalLocation();
                }
            } else {
                _t.timeSinceMoved = 0;
            }

            //debug overlays for mouth mode
            if (overlays) {
                var leftHandPos = MyAvatar.getLeftPalmPosition();
                var rightHandPos = MyAvatar.getRightPalmPosition();

                Overlays.editOverlay(leftHand, {
                    position: leftHandPos
                });
                Overlays.editOverlay(rightHand, {
                    position: rightHandPos
                });
            }

            if (mouthMode === true && overlays === true) {
                Overlays.editOverlay(gustZoneOverlay, {
                    position: getGustDetectorPosition(),
                    color: TARGET_COLOR_HIT
                })
            } else if (overlays) {
                Overlays.editOverlay(gustZoneOverlay, {
                    position: getGustDetectorPosition(),
                    color: TARGET_COLOR
                })
            }

            var volumeLevel = MyAvatar.audioAverageLoudness;
            //volume numbers are pretty large, so lets scale them down. 
            var convertedVolume = convertRange(volumeLevel, [0, 5000], [0, 10]);

            // default is 'wave mode', where waving the object around grows the bubbles

            //store the last position of the wand for velocity calculations
            _t.lastPosition = wandPosition;

            // velocity numbers are pretty small, so lets make them a bit bigger
            var velocityStrength = Vec3.length(velocity) * 100;

            if (velocityStrength > 10) {
                velocityStrength = 10
            }

            //actually grow the bubble
            var dimensions = Entities.getEntityProperties(_t.currentBubble).dimensions;
            var avatarFront = Quat.getFront(MyAvatar.orientation);
            var forwardOffset = Vec3.multiply(avatarFront, 0.1);

            if (velocityStrength > 1 || convertedVolume > 1) {

                //add some variation in bubble sizes
                var bubbleSize = randInt(1, 5);
                bubbleSize = bubbleSize / 50;

                //release the bubble if its dimensions are bigger than the bubble size
                if (dimensions.x > bubbleSize) {
                    //bubbles pop after existing for a bit -- so set a random lifetime
                    var lifetime = randInt(3, 8);

                    // var angularVelocity = Controller.getSpatialControlRawAngularVelocity(hands.leftHand.tip);

                    Entities.editEntity(_t.currentBubble, {
                        // collisionsWillMove:true,
                        // ignoreForCollisions:false,
                        velocity: mouthMode ? avatarFront : velocity,
                        //  angularVelocity: Controller.getSpatialControlRawAngularVelocity(hands.leftHand.tip),
                        lifetime: lifetime
                    });

                    _t.lastBubble = _t.currentBubble;
                    //release the bubble -- when we create a new bubble, it will carry on and this update loop will affect the new bubble
                    BubbleWand.spawnBubble();

                    return
                } else {
                    if (mouthMode) {
                        dimensions.x += 0.005 * convertedVolume;
                        dimensions.y += 0.005 * convertedVolume;
                        dimensions.z += 0.005 * convertedVolume;

                    } else {
                        dimensions.x += 0.005 * velocityStrength;
                        dimensions.y += 0.005 * velocityStrength;
                        dimensions.z += 0.005 * velocityStrength;
                    }

                }
            } else {
                if (dimensions.x >= 0.02) {
                    dimensions.x -= 0.001;
                    dimensions.y -= 0.001;
                    dimensions.z -= 0.001;
                }

            }

            //update the bubble to stay with the wand tip
            Entities.editEntity(_t.currentBubble, {
                position: _t.wandTipPosition,
                dimensions: dimensions
            });

        },
        spawnBubble: function() {
            var _t = this;
            //create a new bubble at the tip of the wand
            //the tip of the wand is going to be in a different place than the center, so we move in space relative to the model to find that position

            var properties = Entities.getEntityProperties(wandEntity.entityID);
            var wandPosition = properties.position;
            var upVector = Quat.getUp(properties.rotation);
            var frontVector = Quat.getFront(properties.rotation);
            var upOffset = Vec3.multiply(upVector, 0.2);
            var wandTipPosition = Vec3.sum(wandPosition, upOffset);
            _t.wandTipPosition = wandTipPosition;

            //store the position of the tip on spawn for use in velocity calculations
            _t.lastPosition = wandTipPosition;

            //create a bubble at the wand tip
            _t.currentBubble = Entities.addEntity({
                type: 'Model',
                modelURL: bubbleModel,
                position: wandTipPosition,
                dimensions: {
                    x: 0.01,
                    y: 0.01,
                    z: 0.01
                },
                collisionsWillMove: true, //true
                ignoreForCollisions: false, //false
                gravity: BUBBLE_GRAVITY,
                collisionSoundURL: POP_SOUNDS[randInt(0, 4)],
                shapeType: "sphere",
                script: bubbleScript,
            });

            //add this bubble to an array of bubbles so we can keep track of them
            _t.bubbles.push(_t.currentBubble)

        },
        returnToOriginalLocation: function() {
            var _t = this;
            Script.update.disconnect(BubbleWand.update)
            _t.currentBubble = null;
            Entities.deleteEntity(_t.currentBubble);
            Entities.editEntity(wandEntity.entityID, _t.originalProperties)
            _t.spawnBubble();
            Script.update.connect(BubbleWand.update);

        },
        init: function() {
            var _t = this;
            this.spawnBubble();
            Script.update.connect(BubbleWand.update);

        }
    }

    BubbleWand.init();

})