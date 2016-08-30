//
//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


(function() {

    var _this = this;
    _this.COLLISION_COOLDOWN_TIME = 5000;

    var startPosition = {
        x: 1100.6343,
        y: 460.5366,
        z: -65.2142
    };

    var startRotation = Quat.fromPitchYawRollDegrees(3.1471, -170.4121, -0.0060)

    _this.preload = function(entityID) {

        //set our id so other methods can get it. 
        _this.entityID = entityID;

        //variables we will use to keep track of when to reset the cow
        _this.timeSinceLastCollision = 0;
        _this.shouldUntip = true;
    }

    _this.collisionWithEntity = function(myID, otherID, collisionInfo) {
        //we dont actually use any of the parameters above, since we don't really care what we collided with, or the details of the collision. 
        //5 seconds after a collision, upright the target.  protect from multiple collisions in a short timespan with the 'shouldUntip' variable
        if (_this.shouldUntip) {
            //in Hifi, preface setTimeout with Script.setTimeout
            Script.setTimeout(function() {
                _this.untip();
                _this.shouldUntip = true;
            }, _this.COLLISION_COOLDOWN_TIME);
        }

        _this.shouldUntip = false;

    }

    _this.untip = function() {
        var props = Entities.getEntityProperties(this.entityID);
        var rotation = Quat.safeEulerAngles(props.rotation)
        if (rotation.x > 3 || rotation.x < -3 || rotation.z > 3 || rotation.z < -3) {
            print('home target - too much pitch or roll, fix it');

            //we zero out the velocity and angular velocity
            Entities.editEntity(_this.entityID, {
                position: startPosition,
                rotation: startRotation,
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
        }



    }


});