//
//  Created by Eric Levin on 3/25/16
//  Copyright 2016 High Fidelity, Inc.
//
// This entity script handles the logic for untipping a cow after it collides with something
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


(function() {

    var _this = this;
    _this.COLLISION_COOLDOWN_TIME = 5000;

    _this.preload = function(entityID) {

        //set our id so other methods can get it. 
        _this.entityID = entityID;
        //load the mooing sound
        _this.mooSound = SoundCache.getSound("https://cdn-1.vircadia.com/us-e-1/Developer/Tutorials/cow/moo.wav")
        _this.mooSoundOptions = {
            volume: 0.7,
            loop: false
        };

        //variables we will use to keep track of when to reset the cow
        _this.timeSinceLastCollision = 0;
        _this.shouldUntipCow = true;
    }

    _this.collisionWithEntity = function(myID, otherID, collisionInfo) {
        //we dont actually use any of the parameters above, since we don't really care what we collided with, or the details of the collision. 

        //5 seconds after a collision, upright the cow.  protect from multiple collisions in a short timespan with the 'shouldUntipCow' variable
        if (_this.shouldUntipCow) {
            //in Hifi, preface setTimeout with Script.setTimeout
            Script.setTimeout(function() {
                _this.untipCow();
                _this.shouldUntipCow = true;
            }, _this.COLLISION_COOLDOWN_TIME);
        }

        _this.shouldUntipCow = false;

    }

    _this.untipCow = function() {
        // keep yaw but reset pitch and roll
        var cowProps = Entities.getEntityProperties(_this.entityID, ["rotation", "position"]);
        var eulerRotation = Quat.safeEulerAngles(cowProps.rotation);
        eulerRotation.x = 0;
        eulerRotation.z = 0;
        var newRotation = Quat.fromVec3Degrees(eulerRotation);

        //we zero out the velocity and angular velocity so the cow doesn't change position or spin
        Entities.editEntity(_this.entityID, {
            rotation: newRotation,
            velocity: {
                x: 0,
                y: 0,
                z: 0
            },
            angularVelocity: {
                x: 0,
                y: 0,
                z: 0
            }
        });

        //play the mooing sound when we untip!  if it isn't already playing.
        _this.mooSoundOptions.position = cowProps.position;
        if (!_this.soundInjector) {
            _this.soundInjector = Audio.playSound(_this.mooSound, _this.mooSoundOptions);
        } else {
            //if its already playing, just restart
            _this.soundInjector.setOptions(_this.mooSoundOptions);
            _this.soundInjector.restart();
        }
    }


});