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

function randInt(min, max) {
    return Math.floor(Math.random() * (max - min)) + min;
}


function convertRange(value, r1, r2) {
    return (value - r1[0]) * (r2[1] - r2[0]) / (r1[1] - r1[0]) + r2[0];
}

// helpers -- @zappoman
// Computes the penetration between a point and a sphere (centered at the origin)
// if point is inside sphere: returns true and stores the result in 'penetration'
// (the vector that would move the point outside the sphere)
// otherwise returns false
function findSphereHit(point, sphereRadius) {
    var EPSILON = 0.000001; //smallish positive number - used as margin of error for some computations
    var vectorLength = Vec3.length(point);
    if (vectorLength < EPSILON) {
        return true;
    }
    var distance = vectorLength - sphereRadius;
    if (distance < 0.0) {
        return true;
    }
    return false;
}

function findSpherePointHit(sphereCenter, sphereRadius, point) {
    return findSphereHit(Vec3.subtract(point, sphereCenter), sphereRadius);
}

function findSphereSphereHit(firstCenter, firstRadius, secondCenter, secondRadius) {
    return findSpherePointHit(firstCenter, firstRadius + secondRadius, secondCenter);
}

(function() {
    var console = {};
    console.log = function(p) {
        if (arguments.length > 1) {

            for (var i = 1; i < arguments.length; i++) {
                print(arguments[i])
            }

        } else {
            print(p)
        }

    }

    var bubbleModel = "http://hifi-public.s3.amazonaws.com/james/bubblewand/models/bubble/bubble.fbx";
    var bubbleScript = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/scripts/bubble.js?' + randInt(2, 5096);
    var popSound = SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop.wav");
    var wandModel = "http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/wand.fbx";



    var targetSize = 0.4;
    var targetColor = {
        red: 128,
        green: 128,
        blue: 128
    };
    var targetColorHit = {
        red: 0,
        green: 255,
        blue: 0
    };
    var moveCycleColor = {
        red: 255,
        green: 255,
        blue: 0
    };

    var handSize = 0.25;
    var leftCubePosition = MyAvatar.getLeftPalmPosition();
    var rightCubePosition = MyAvatar.getRightPalmPosition();

    var RIGHT_FRONT = 512;

    var leftHand = Overlays.addOverlay("cube", {
        position: leftCubePosition,
        size: handSize,
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
        size: handSize,
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
        size: targetSize,
        color: targetColor,
        alpha: 1,
        solid: false
    });


    function getGustDetectorPosition() {
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


    var thisEntity = this;

    this.preload = function(entityID) {
       // print('PRELOAD')
        this.entityID = entityID;
        this.properties = Entities.getEntityProperties(this.entityID);
        BubbleWand.lastPosition = this.properties.position;
    }

    this.unload = function(entityID) {
        Overlays.deleteOverlay(leftHand);
        Overlays.deleteOverlay(rightHand);
        Overlays.deleteOverlay(rightFront)
        Entities.editEntity(entityID, {
            name: ""
        });
        Script.update.disconnect(BubbleWand.update);
        collectGarbage();
    };


    var BubbleWand = {
        bubbles: [],
        currentBubble: null,
        update: function() {
            BubbleWand.updateControllerState();
        },
        updateControllerState: function() {
            var _t = this;
            var properties = Entities.getEntityProperties(thisEntity.entityID);
            var wandPosition = properties.position;

            var leftHandPos = MyAvatar.getLeftPalmPosition();
            var rightHandPos = MyAvatar.getRightPalmPosition();

            Overlays.editOverlay(leftHand, {
                position: leftHandPos
            });
            Overlays.editOverlay(rightHand, {
                position: rightHandPos
            });

            //not really a sphere...
            var hitTargetWithWand = findSphereSphereHit(wandPosition, handSize / 2, getGustDetectorPosition(), targetSize / 2)
           
            var mouthMode;
            if (hitTargetWithWand) {
                Overlays.editOverlay(gustZoneOverlay, {
                    position: getGustDetectorPosition(),
                    color: targetColorHit
                })
                mouthMode = true;

            } else {
                Overlays.editOverlay(gustZoneOverlay, {
                    position: getGustDetectorPosition(),
                    color: targetColor
                })
                mouthMode=false;
            }

            var velocity = Vec3.subtract(wandPosition,BubbleWand.lastPosition)

          

            _t.lastPosition = wandPosition; 

             //print('VELOCITY:::'+JSON.stringify(velocity))
             var velocityStrength = Vec3.length(velocity) *100;
           //   print('velocityStrength::' + velocityStrength);
            //todo: angular velocity without the controller 
            // var angularVelocity = Controller.getSpatialControlRawAngularVelocity(hands.leftHand.tip);
            var dimensions = Entities.getEntityProperties(_t.currentBubble).dimensions;

            var volumeLevel = MyAvatar.audioAverageLoudness;
            var convertedVolume = convertRange(volumeLevel, [0, 5000], [0, 10]);
            // print('CONVERTED VOLUME:' + convertedVolume);
           
            var growthFactor = convertedVolume + velocityStrength;
          //  print('growthFactor::' + growthFactor);
            if (velocityStrength > 1 || convertedVolume > 1) {
                var bubbleSize = randInt(1, 5)
                bubbleSize = bubbleSize / 10;
                if (dimensions.x > bubbleSize) {
                   // console.log('RELEASE BUBBLE')
                    var lifetime = randInt(3, 8);
                    //sound is somewhat unstable at the moment
                    // Script.setTimeout(function() {
                    //     _t.burstBubbleSound(_t.currentBubble)
                    // }, lifetime * 1000)
                    //need to add some kind of forward velocity for bubble that you blow
                    Entities.editEntity(_t.currentBubble, {
                        velocity: Vec3.normalize(velocity),
                        //  angularVelocity: Controller.getSpatialControlRawAngularVelocity(hands.leftHand.tip),
                        lifetime: lifetime
                    });

                    _t.spawnBubble();
                    return
                } else {
                    if (mouthMode) {
                        dimensions.x += 0.015 * convertedVolume;
                        dimensions.y += 0.015 * convertedVolume;
                        dimensions.z += 0.015 * convertedVolume;

                    } else {
                        dimensions.x += 0.015 * velocityStrength;
                        dimensions.y += 0.015 * velocityStrength;
                        dimensions.z += 0.015 * velocityStrength;
                    }

                }
            } else {
                if (dimensions.x >= 0.02) {
                    dimensions.x -= 0.001;
                    dimensions.y -= 0.001;
                    dimensions.z -= 0.001;
                }

            }

            Entities.editEntity(_t.currentBubble, {
                position: wandPosition,
                dimensions: dimensions
            });

        },
        burstBubbleSound: function(bubble) {
            var position = Entities.getEntityProperties(bubble).position;
            var orientation = Entities.getEntityProperties(bubble).orientation;
            //console.log('bubble position at pop: ' + JSON.stringify(position));
            var audioOptions = {
                volume: 0.5,
                position: position,
                orientation: orientation
            }

            //Audio.playSound(popSound, audioOptions);

            //remove this bubble from the array
            var i = BubbleWand.bubbles.indexOf(bubble);

            if (i != -1) {
                BubbleWand.bubbles.splice(i, 1);
            }

        },
        spawnBubble: function() {
         //   console.log('spawning bubble')
            var _t = this;
            var properties = Entities.getEntityProperties(thisEntity.entityID);
            var wandPosition = properties.position;


            _t.currentBubble = Entities.addEntity({
                type: 'Model',
                modelURL: bubbleModel,
                position: wandPosition,
                dimensions: {
                    x: 0.01,
                    y: 0.01,
                    z: 0.01
                },
                ignoreForCollisions: true,
                gravity: BUBBLE_GRAVITY,
                // collisionSoundURL:popSound,
                shapeType: "sphere",
                script: bubbleScript,
            });
            _t.bubbles.push(_t.currentBubble)
        },
        init: function() {
            var _t = this;
            _t.spawnBubble();
            Script.update.connect(BubbleWand.update);
        }
    }

    function collectGarbage() {
      //  console.log('COLLECTING GARBAGE!!!')
        Entities.deleteEntity(BubbleWand.currentBubble);
        while (BubbleWand.bubbles.length > 0) {
            Entities.deleteEntity(BubbleWand.bubbles.pop());
        }
    }

    BubbleWand.init();



})