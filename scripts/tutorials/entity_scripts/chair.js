//  chair.js
//
//  Restrictions right now:  
//  Chair objects need to be set as not colliding with avatars, so that they pull avatar 
//  avatar into collision with them.  Also they need to be at or above standing height 
//  (like a stool).  
//    
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){ 
    
    var UPDATE_INTERVAL_MSECS = 1000;       //  Update the spring/object at 45Hz
    var SETTINGS_INTERVAL_MSECS = 1000;     //  Periodically check user data for updates 
    var DEFAULT_SIT_DISTANCE = 1.0;         //  How strong/stiff is the spring?  
    var HYSTERESIS = 1.2;

    var SITTING = 0;
    var LEAVING = 1; 
    var STANDING = 2;

    var state = STANDING;
    var sitDistance = DEFAULT_SIT_DISTANCE;
    var standDistance = DEFAULT_SIT_DISTANCE / 3;
    var entity;
    var props;
    var checkTimer = false;
    var settingsTimer = false;
    var sitting = false;

    var _this;
    
    var WANT_DEBUG = true;
    function debugPrint(string) {
        if (WANT_DEBUG) {
            print(string);
        }
    }

    function howFarAway(position) {
        return Vec3.distance(MyAvatar.position, position);
    }

    function isSeatOpen(position, distance) {
        closest = true;
        AvatarList.getAvatarIdentifiers().forEach(function(avatarSessionUUID) {
            var avatar = AvatarList.getAvatar(avatarSessionUUID);
            if (Vec3.distance(avatar.position, position) < distance) {
                closest = false;
            } 
        });
        return closest;
    }

    function enterSitPose() {
        var rot;
        var UPPER_LEG_ANGLE = 240;
        var LOWER_LEG_ANGLE = -80;
        rot = Quat.safeEulerAngles(MyAvatar.getJointRotation("RightUpLeg"));
        MyAvatar.setJointData("RightUpLeg", Quat.fromPitchYawRollDegrees(UPPER_LEG_ANGLE, rot.y, rot.z), MyAvatar.getJointTranslation("RightUpLeg"));
        rot = Quat.safeEulerAngles(MyAvatar.getJointRotation("RightLeg"));
        MyAvatar.setJointData("RightLeg", Quat.fromPitchYawRollDegrees(LOWER_LEG_ANGLE, rot.y, rot.z), MyAvatar.getJointTranslation("RightLeg"));
        rot = Quat.safeEulerAngles(MyAvatar.getJointRotation("LeftUpLeg"));
        MyAvatar.setJointData("LeftUpLeg", Quat.fromPitchYawRollDegrees(UPPER_LEG_ANGLE, rot.y, rot.z), MyAvatar.getJointTranslation("LeftUpLeg"));
        rot = Quat.safeEulerAngles(MyAvatar.getJointRotation("LeftLeg"));
        MyAvatar.setJointData("LeftLeg", Quat.fromPitchYawRollDegrees(LOWER_LEG_ANGLE, rot.y, rot.z), MyAvatar.getJointTranslation("LeftLeg"));
    }
    function leaveSitPose() {
         MyAvatar.clearJointData("RightUpLeg");
         MyAvatar.clearJointData("LeftUpLeg");
         MyAvatar.clearJointData("RightLeg");
         MyAvatar.clearJointData("LeftLeg");
    }

    function moveToSeat(position, rotation) {
        var eulers = Quat.safeEulerAngles(MyAvatar.orientation);
        eulers.y = Quat.safeEulerAngles(rotation).y;
        MyAvatar.position = position;
        MyAvatar.orientation = Quat.fromPitchYawRollDegrees(eulers.x, eulers.y, eulers.z);
    }

    this.preload = function(entityID) { 
        //  Load the sound and range from the entity userData fields, and note the position of the entity.
        debugPrint("chair preload");
        entity = entityID;
        _this = this;
        checkTimer = Script.setInterval(this.maybeSitOrStand, UPDATE_INTERVAL_MSECS);
        settingsTimer = Script.setInterval(this.checkSettings, SETTINGS_INTERVAL_MSECS);
    }; 

    this.maybeSitOrStand = function() {
        props = Entities.getEntityProperties(entity, [ "position", "rotation" ]);
        // First, check if the entity is far enough away to not need to do anything with it
        var howFar = howFarAway(props.position);
        if ((state === STANDING) && (howFar < sitDistance) && isSeatOpen(props.position, standDistance)) {
            moveToSeat(props.position, props.rotation);
            //MyAvatar.characterControllerEnabled = true;
            enterSitPose();
            state = SITTING;
            debugPrint("Sitting");
        } else if ((state === SITTING) && (howFar > standDistance)) {
            leaveSitPose();
            state = LEAVING;
            MyAvatar.characterControllerEnabled = true;
            debugPrint("Leaving");
        } else if ((state === LEAVING) && (howFar > sitDistance * HYSTERESIS)) {
            state = STANDING;
            debugPrint("Standing");
        }
    }

    this.clickDownOnEntity = function(entityID, mouseEvent) { 
        //  If entity is clicked, jump to seat
        
        props = Entities.getEntityProperties(entity, [ "position", "rotation" ]);
        if ((state === STANDING) && isSeatOpen(props.position, standDistance)) {
            moveToSeat(props.position, props.rotation);
            //MyAvatar.characterControllerEnabled = false;
            enterSitPose();
            state = SITTING;
            debugPrint("Sitting");
        }
    }

    this.checkSettings = function() {
        var dataProps = Entities.getEntityProperties(entity, [ "userData" ]);
        if (dataProps.userData) {
            var data = JSON.parse(dataProps.userData);
            if (data.sitDistance) {
                if (!(sitDistance === data.sitDistance)) {
                    debugPrint("Read new sit distance: " + data.sitDistance); 
                }
                sitDistance = data.sitDistance;
            }
            if (data.standDistance) {
                if (!(standDistance === data.standDistance)) {
                    debugPrint("Read new stand distance: " + data.standDistance); 
                }
                standDistance = data.standDistance;
            }
        }
    }

    this.unload = function(entityID) { 
        debugPrint("chair unload");
        if (checkTimer) {
            Script.clearInterval(checkTimer);
        }
        if (settingsTimer) {
            Script.clearInterval(settingsTimer);
        }
    }; 

}) 
