(function() {

    //  touch.js 
    // 
    //  Sample file using tractor action, haptic vibration, and color change to demonstrate two spheres 
    //  That can give a sense of touch to the holders.  
    //  Create two standard spheres, make them grabbable, and attach this entity script.  Grab them and touch them together.
    //  
    //  Created by Philip Rosedale on March 18, 2017
    //  Copyright 2017 High Fidelity, Inc.
    //  
    //  Distributed under the Apache License, Version 2.0.
    //  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
    //

    var TIMESCALE = 0.03;
    var ACTION_TTL = 10;
    var _this;

    var RIGHT_HAND = 1;
    var LEFT_HAND = 0;
    var HAPTIC_PULSE_FIRST_STRENGTH = 0.5;
    var HAPTIC_PULSE_MIN_STRENGTH = 0.20;
    var HAPTIC_PULSE_MAX_STRENGTH = 0.5;
    var HAPTIC_PULSE_FIRST_DURATION = 1.0;
    var HAPTIC_PULSE_DURATION = 16.0;
    var HAPTIC_PULSE_DISTANCE = 0.0;
    var MAX_PENETRATION = 0.02;
    var HAPTIC_MIN_VELOCITY = 0.002;
    var HAPTIC_MAX_VELOCITY = 0.5;
    var PENETRATION_PULLBACK_FACTOR = 0.65;
    var FRAME_TIME = 0.016;

    var isColliding = false;

    var hand = LEFT_HAND;
    var lastHapticPulseLocation = { x:0, y:0, z:0 };


    var GRAY = { red: 128, green: 128, blue: 128 };
    var RED = { red: 255, green: 0, blue: 0 };

    var targetColor = { x: GRAY.red, y: GRAY.green, z: GRAY.blue };

    var lastPosition = { x: 0, y: 0, z: 0 };
    var velocity = { x: 0, y: 0, z: 0 };

    var lastOtherPosition = { x: 0, y: 0, z: 0 };
    var otherVelocity = { x: 0, y: 0, z: 0 };


    function TouchExample() {
        _this = this;
    }

    function updateTractorAction(timescale) {
        var targetProps = Entities.getEntityProperties(_this.entityID);
        //
        //  Look for nearby entities to touch 
        //
        var copyProps = Entities.getEntityProperties(_this.copy);
        var nearbyEntities = Entities.findEntities(copyProps.position, copyProps.dimensions.x * 2); 
        var wasColliding = isColliding; 
        var targetAdjust = { x: 0, y: 0, z: 0 };

        isColliding = false;
        for (var i = 0; i < nearbyEntities.length; i++) {
            if (_this.copy != nearbyEntities[i] && _this.entityID != nearbyEntities[i]) {
                var otherProps = Entities.getEntityProperties(nearbyEntities[i]);
                var penetration = Vec3.distance(copyProps.position, otherProps.position) - (copyProps.dimensions.x / 2 + otherProps.dimensions.x / 2);
                if (otherProps.type === 'Sphere' && penetration < 0 && penetration > -copyProps.dimensions.x * 3) {
                    isColliding = true;
                    targetAdjust = Vec3.sum(targetAdjust, Vec3.multiply(Vec3.normalize(Vec3.subtract(targetProps.position, otherProps.position)), -penetration * PENETRATION_PULLBACK_FACTOR));
                    if (!wasColliding && false) {
                        targetColor = { x: RED.red, y: RED.green, z: RED.blue };
                    } else {
                        targetColor = { x: 200 + Math.min(-penetration / MAX_PENETRATION, 1.0) * 55, y: GRAY.green, z: GRAY.blue };
                    }
                    if (Vec3.distance(targetProps.position, lastHapticPulseLocation) > HAPTIC_PULSE_DISTANCE || !wasColliding) {
                        if (!wasColliding) {
                            velocity = { x: 0, y: 0, z: 0};
                            otherVelocity = { x: 0, y: 0, z: 0 };
                            Controller.triggerHapticPulse(HAPTIC_PULSE_FIRST_STRENGTH, HAPTIC_PULSE_FIRST_DURATION, hand);
                        } else {
                            velocity = Vec3.distance(targetProps.position, lastPosition) / FRAME_TIME;   
                            otherVelocity = Vec3.distance(otherProps.position, lastOtherPosition) / FRAME_TIME; 
                            var velocityStrength = Math.min(velocity + otherVelocity / HAPTIC_MAX_VELOCITY, 1.0);
                            var strength = HAPTIC_PULSE_MIN_STRENGTH + Math.min(-penetration / MAX_PENETRATION, 1.0) * (HAPTIC_PULSE_MAX_STRENGTH - HAPTIC_PULSE_MIN_STRENGTH);
                            Controller.triggerHapticPulse(velocityStrength * strength, HAPTIC_PULSE_DURATION, hand);
                        }
                        lastPosition = targetProps.position;
                        lastOtherPosition = otherProps.position;
                        lastHapticPulseLocation = targetProps.position;
                    }
                }     
            }
        }
        if ((wasColliding != isColliding) && !isColliding) {
            targetColor = { x: GRAY.red, y: GRAY.green, z: GRAY.blue };
        }
        //  Interpolate color toward target color 
        var currentColor = { x: copyProps.color.red,  y: copyProps.color.green,  z: copyProps.color.blue };
        var newColor = Vec3.sum(currentColor, Vec3.multiply(Vec3.subtract(targetColor, currentColor), 0.1));
        Entities.editEntity(_this.copy, { color: { red: newColor.x, green: newColor.y, blue: newColor.z } });

        var props = {
            targetPosition: Vec3.sum(targetProps.position, targetAdjust),
            targetRotation: targetProps.rotation,
            linearTimeScale: timescale,
            angularTimeScale: timescale,
            ttl: ACTION_TTL
        };
        var success = Entities.updateAction(_this.copy, _this.actionID, props);
    }

    function createTractorAction(timescale) {

        var targetProps = Entities.getEntityProperties(_this.entityID);
        var props = {
            targetPosition: targetProps.position,
            targetRotation: targetProps.rotation,
            linearTimeScale: timescale,
            angularTimeScale: timescale,
            ttl: ACTION_TTL
        };
        _this.actionID = Entities.addAction("tractor", _this.copy, props);
        return;
    }

    function createCopy() {
        var originalProps = Entities.getEntityProperties(_this.entityID);
        var props = {
            type: originalProps.type,
            modelURL: originalProps.modelURL,
            dimensions: originalProps.dimensions,
            color: GRAY,
            dynamic: true,
            damping: 0.0,
            angularDamping: 0.0,
            collidesWith: 'static',
            rotation: originalProps.rotation,
            position: originalProps.position,
            shapeType: originalProps.shapeType,
            visible: true,
            userData:JSON.stringify({
                grabbableKey:{
                    grabbable:false
                }
            })
        };
        _this.copy = Entities.addEntity(props);
    }

    function deleteCopy() {
        print("Delete copy");
        Entities.deleteEntity(_this.copy);
    }

    function makeOriginalInvisible() {
        Entities.editEntity(_this.entityID, {
            visible: false,
            collisionless: true
        });
    }

    function makeOriginalVisible() {
        Entities.editEntity(_this.entityID, {
            visible: true,
            collisionless: false
        });
    }

    function deleteTractorAction() {
        Entities.deleteAction(_this.copy, _this.actionID);
    }

    function setHand(position) {
        if (Vec3.distance(MyAvatar.getLeftPalmPosition(), position) < Vec3.distance(MyAvatar.getRightPalmPosition(), position)) {
            hand = LEFT_HAND;
        } else {
            hand = RIGHT_HAND;
        }
    }

    TouchExample.prototype = {
        preload: function(entityID) {
            _this.entityID = entityID;
        },
        startNearGrab: function(entityID, data) {
            createCopy();
            createTractorAction(TIMESCALE);
            makeOriginalInvisible();
            setHand(Entities.getEntityProperties(_this.entityID).position);
        },
        continueNearGrab: function() {
            updateTractorAction(TIMESCALE);
        },
        releaseGrab: function() {
            deleteTractorAction();
            deleteCopy();
            makeOriginalVisible();
        }
    };

    return new TouchExample();
});
