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
    
    var CHECK_INTERVAL_MSECS = 250;         //  When sitting, check for need to stand up
    var SETTINGS_INTERVAL_MSECS = 1000;     //  Periodically check user data for updates 
    var DEFAULT_SIT_DISTANCE = 1.0;         //  How far away from the chair can you sit?
    var HYSTERESIS = 1.1;

    var sitTarget = { x: 0, y: 0, z: 0 };   //  Offset where your butt should go relative
                                            //  to the object's center.
    var SITTING = 0;
    var STANDING = 1;

    var state = STANDING;
    var sitDistance = DEFAULT_SIT_DISTANCE;

    var entity;
    var props;
    var checkTimer = false;
    var settingsTimer = false;
    var sitting = false;

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

    function isSeatOpen(position, distance) {
        closest = true;
        AvatarList.getAvatarIdentifiers().forEach(function(avatarSessionUUID) {
            var avatar = AvatarList.getAvatar(avatarSessionUUID);
            if (avatarSessionUUID && Vec3.distance(avatar.position, position) < distance) {
                debugPrint("Seat Occupied!");
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

    function sitDown(position, rotation) {
        var eulers = Quat.safeEulerAngles(MyAvatar.orientation);
        eulers.y = Quat.safeEulerAngles(rotation).y;
        MyAvatar.position = Vec3.sum(position, Vec3.multiplyQbyV(props.rotation, sitTarget));
        MyAvatar.orientation = Quat.fromPitchYawRollDegrees(eulers.x, eulers.y, eulers.z);

        enterSitPose();
        state = SITTING;
    }

    this.preload = function(entityID) { 
        //  Load the sound and range from the entity userData fields, and note the position of the entity.
        debugPrint("chair preload");
        entity = entityID;
        _this = this;
        settingsTimer = Script.setInterval(this.checkSettings, SETTINGS_INTERVAL_MSECS);
    }; 

    this.maybeStand = function() {
        props = Entities.getEntityProperties(entity, [ "position", "rotation" ]);
        // First, check if the entity is far enough away to not need to do anything with it
        var howFar = howFarAway(props.position);
        if ((state === SITTING) && (howFar > sitDistance * HYSTERESIS)) {
            leaveSitPose();
            Script.clearInterval(checkTimer);
            checkTimer = null;
            state = STANDING;
            debugPrint("Standing");
        } 
    }

    this.clickDownOnEntity = function(entityID, mouseEvent) { 
        //  If entity is clicked, sit
        props = Entities.getEntityProperties(entity, [ "position", "rotation" ]);
        if ((state === STANDING) && isSeatOpen(props.position, sitDistance)) {
            sitDown(props.position, props.rotation);
            checkTimer = Script.setInterval(this.maybeStand, CHECK_INTERVAL_MSECS);
            debugPrint("Sitting from mouse click");
        }
    }

    this.startFarTrigger = function() {
        //  If entity is far clicked, sit
        props = Entities.getEntityProperties(entity, [ "position", "rotation" ]);
        if ((state === STANDING) && isSeatOpen(props.position, sitDistance)) {
            sitDown(props.position, props.rotation);
            checkTimer = Script.setInterval(this.maybeStand, CHECK_INTERVAL_MSECS);
            debugPrint("Sitting from far trigger");
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
            if (data.sitTarget) {
                if (data.sitTarget.y && (data.sitTarget.y != sitTarget.y)) {
                    debugPrint("Read new sitTarget.y: " + data.sitTarget.y); 
                    sitTarget.y = data.sitTarget.y;
                }
                if (data.sitTarget.x && (data.sitTarget.x != sitTarget.x)) {
                    debugPrint("Read new sitTarget.x: " + data.sitTarget.x); 
                    sitTarget.x = data.sitTarget.x;
                }
                if (data.sitTarget.z && (data.sitTarget.z != sitTarget.z)) {
                    debugPrint("Read new sitTarget.z: " + data.sitTarget.z); 
                    sitTarget.z = data.sitTarget.z;
                }
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
