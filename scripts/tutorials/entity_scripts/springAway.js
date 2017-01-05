//  springAway.js
//
//  If you attach this entity script to an object, the object will spring away from  
//  your avatar's hands.  Useful for making a beachball or basketball, because you
//  can bounce it on your hands.
//
//  You can change the force applied by the script by setting userData to contain
//  a value 'strength', which will otherwise default to DEFAULT_STRENGTH
//
//  Note that the use of dimensions.x as the size of the entity means that it 
//  needs to be spherical for this to look correct.  
//   
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){ 
    
    var UPDATE_INTERVAL_MSECS = 1000/45;    //  Update the spring/object at 45Hz
    var SETTINGS_INTERVAL_MSECS = 1000;     //  Periodically check user data for updates 
    var TOO_FAR = 5.0;                      //  If the object 
    var DEFAULT_STRENGTH = 3.0;             //  How strong/stiff is the spring?  


    var entity;
    var props;
    var checkTimer = false;
    var settingsTimer = false;
    var strength = DEFAULT_STRENGTH;
    var _this;
    
    var WANT_DEBUG = false;
    function debugPrint(string) {
        if (WANT_DEBUG) {
            print(string);
        }
    }

    function howFarAway(position) {
        return Vec3.distance(MyAvatar.position, position);
    }

    function springForce(position, center, radius) {
        //
        //  Return a vector corresponding to a normalized spring force ranging from 1 at 
        //  exact center to zero at distance radius from center.
        //
        var distance = Vec3.distance(position, center);
        return Vec3.multiply(1.0 - distance / radius, Vec3.normalize(Vec3.subtract(center, position)));
    }

    function fingerPosition(which) {
        //
        //  Get the worldspace position of either the tip of the index finger (jointName "RightHandIndex3", etc), or 
        //  fall back to the controller position if that doesn't exist.
        //
        var joint = MyAvatar.getJointPosition(which === "RIGHT" ? "RightHandIndex3" : "LeftHandIndex3");
        if (Vec3.length(joint) > 0) {
            return joint;
        } else {
            return Vec3.sum(MyAvatar.position,
                        Vec3.multiplyQbyV(MyAvatar.orientation, 
                            Controller.getPoseValue(which === "RIGHT" ? Controller.Standard.RightHand : Controller.Standard.LeftHand).translation)); 
        }  
    }

    this.preload = function(entityID) { 
        //  Load the sound and range from the entity userData fields, and note the position of the entity.
        debugPrint("springAway preload");
        entity = entityID;
        _this = this;
        checkTimer = Script.setInterval(this.maybePush, UPDATE_INTERVAL_MSECS);
        settingsTimer = Script.setInterval(this.checkSettings, SETTINGS_INTERVAL_MSECS);
    }; 

    this.maybePush = function() {
        props = Entities.getEntityProperties(entity, [ "position", "dimensions", "velocity" ]);

        // First, check if the entity is far enough away to not need to do anything with it
        
        if (howFarAway(props.position) - props.dimensions.x / 2 > TOO_FAR) {
            return;
        }

        var rightFingerPosition = fingerPosition("RIGHT");
        var leftFingerPosition = fingerPosition("LEFT");

        var addVelocity = { x: 0, y: 0, z: 0 };
        
        
        if (Vec3.distance(leftFingerPosition, props.position) < props.dimensions.x / 2) {
            addVelocity = Vec3.sum(addVelocity, Vec3.multiply(springForce(leftFingerPosition, props.position, props.dimensions.x), strength));
        }
        if (Vec3.distance(rightFingerPosition, props.position) < props.dimensions.x / 2) {
            addVelocity = Vec3.sum(addVelocity, Vec3.multiply(springForce(rightFingerPosition, props.position, props.dimensions.x), strength));
        }

        if (Vec3.length(addVelocity) > 0) {
            Entities.editEntity(entity, {
                velocity:  Vec3.sum(props.velocity, addVelocity)
            });
        }
    }

    this.checkSettings = function() {
        var dataProps = Entities.getEntityProperties(entity, [ "userData" ]);
        if (dataProps.userData) {
            var data = JSON.parse(dataProps.userData);
            if (data.strength) {
                if (!(strength === data.strength)) {
                    debugPrint("Read new spring strength: " + data.strength); 
                }
                strength = data.strength;
            }
        }
    }

    this.unload = function(entityID) { 
        debugPrint("springAway unload");
        if (checkTimer) {
            Script.clearInterval(checkTimer);
        }
        if (settingsTimer) {
            Script.clearInterval(settingsTimer);
        }
    }; 

}) 
