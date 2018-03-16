//
//  sit.js
//
//  Created by Clement Brisset on 3/3/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("/~/system/libraries/utils.js");
    if (!String.prototype.startsWith) {
        String.prototype.startsWith = function(searchString, position){
            position = position || 0;
            return this.substr(position, searchString.length) === searchString;
        };
    }

    var SETTING_KEY = "com.highfidelity.avatar.isSitting";
    var ANIMATION_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/clement/production/animations/sitting_idle.fbx";
    var ANIMATION_FPS = 30;
    var ANIMATION_FIRST_FRAME = 1;
    var ANIMATION_LAST_FRAME = 10;
    var RELEASE_TIME = 500; // ms
    var RELEASE_DISTANCE = 0.2; // meters
    var MAX_IK_ERROR = 30;
    var IK_SETTLE_TIME = 250; // ms
    var DESKTOP_UI_CHECK_INTERVAL = 100;
    var DESKTOP_MAX_DISTANCE = 5;
    var SIT_DELAY = 25;
    var MAX_RESET_DISTANCE = 0.5; // meters
    var OVERRIDEN_DRIVE_KEYS = [
        DriveKeys.TRANSLATE_X,
        DriveKeys.TRANSLATE_Y,
        DriveKeys.TRANSLATE_Z,
        DriveKeys.STEP_TRANSLATE_X,
        DriveKeys.STEP_TRANSLATE_Y,
        DriveKeys.STEP_TRANSLATE_Z,
    ];

    this.entityID = null;
    this.animStateHandlerID = null;
    this.interval = null;
    this.sitDownSettlePeriod = null;
    this.lastTimeNoDriveKeys = null;
    this.sittingDown = false;

    // Preload the animation file
    this.animation = AnimationCache.prefetch(ANIMATION_URL);

    this.preload = function(entityID) {
        this.entityID = entityID;
    }
    this.unload = function() {
        if (Settings.getValue(SETTING_KEY) === this.entityID) {
            this.standUp();
        }
        if (this.interval !== null) {
            Script.clearInterval(this.interval);
            this.interval = null;
        }
        this.cleanupOverlay();
    }

    this.setSeatUser = function(user) {
        try {
            var userData = Entities.getEntityProperties(this.entityID, ["userData"]).userData;
            userData = JSON.parse(userData);

            if (user !== null) {
                userData.seat.user = user;
            } else {
                delete userData.seat.user;
            }

            Entities.editEntity(this.entityID, {
                userData: JSON.stringify(userData)
            });
        } catch (e) {
            // Do Nothing
        }
    }
    this.getSeatUser = function() {
        try {
            var properties = Entities.getEntityProperties(this.entityID, ["userData", "position"]);
            var userData = JSON.parse(properties.userData);

            // If MyAvatar return my uuid
            if (userData.seat.user === MyAvatar.sessionUUID) {
                return userData.seat.user;
            }


            // If Avatar appears to be sitting
            if (userData.seat.user) {
                var avatar = AvatarList.getAvatar(userData.seat.user);
                if (avatar &&  (Vec3.distance(avatar.position, properties.position) < RELEASE_DISTANCE)) {
                    return userData.seat.user;
                }
            }
        } catch (e) {
            // Do nothing
        }

        // Nobody on the seat
        return null;
    }

    // Is the seat used
    this.checkSeatForAvatar = function() {
        var seatUser = this.getSeatUser();

        // If MyAvatar appears to be sitting
        if (seatUser === MyAvatar.sessionUUID) {
            var properties = Entities.getEntityProperties(this.entityID, ["position"]);
            return Vec3.distance(MyAvatar.position, properties.position) < RELEASE_DISTANCE;
        }

        return seatUser !== null;
    }

    this.rolesToOverride = function() {
        return MyAvatar.getAnimationRoles().filter(function(role) {
            return !(role.startsWith("right") || role.startsWith("left"));
        });
    }

    // Handler for user changing the avatar model while sitting. There's currently an issue with changing avatar models while override role animations are applied,
    // so to avoid that problem, re-apply the role overrides once the model has finished changing.
    this.modelURLChangeFinished = function () {
        print("Sitter's model has FINISHED changing. Reapply anim role overrides.");
        var roles = this.rolesToOverride();
        for (i in roles) {
            MyAvatar.overrideRoleAnimation(roles[i], ANIMATION_URL, ANIMATION_FPS, true, ANIMATION_FIRST_FRAME, ANIMATION_LAST_FRAME);
        }
    }

    this.sitDown = function() {
        if (this.checkSeatForAvatar()) {
            print("Someone is already sitting in that chair.");
            return;
        }
        print("Sitting down (" + this.entityID + ")");
        this.sittingDown = true;

        var now = Date.now();
        this.sitDownSettlePeriod = now + IK_SETTLE_TIME;
        this.lastTimeNoDriveKeys = now;

        var previousValue = Settings.getValue(SETTING_KEY);
        Settings.setValue(SETTING_KEY, this.entityID);
        this.setSeatUser(MyAvatar.sessionUUID);
        if (previousValue === "") {
            MyAvatar.characterControllerEnabled = false;
            MyAvatar.hmdLeanRecenterEnabled = false;
            var roles = this.rolesToOverride();
            for (i in roles) {
                MyAvatar.overrideRoleAnimation(roles[i], ANIMATION_URL, ANIMATION_FPS, true, ANIMATION_FIRST_FRAME, ANIMATION_LAST_FRAME);
            }

            for (var i in OVERRIDEN_DRIVE_KEYS) {
                MyAvatar.disableDriveKey(OVERRIDEN_DRIVE_KEYS[i]);
            }

            MyAvatar.resetSensorsAndBody();
        }

        var properties = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
        var index = MyAvatar.getJointIndex("Hips");
        MyAvatar.pinJoint(index, properties.position, properties.rotation);

        this.animStateHandlerID = MyAvatar.addAnimationStateHandler(function(properties) {
            return { headType: 0 };
        }, ["headType"]);
        Script.update.connect(this, this.update);
        MyAvatar.onLoadComplete.connect(this, this.modelURLChangeFinished);
    }

    this.standUp = function() {
        print("Standing up (" + this.entityID + ")");
        MyAvatar.removeAnimationStateHandler(this.animStateHandlerID);
        Script.update.disconnect(this, this.update);
        MyAvatar.onLoadComplete.disconnect(this, this.modelURLChangeFinished);

        if (MyAvatar.sessionUUID === this.getSeatUser()) {
            this.setSeatUser(null);
        }

        if (Settings.getValue(SETTING_KEY) === this.entityID) {
            Settings.setValue(SETTING_KEY, "");

            for (var i in OVERRIDEN_DRIVE_KEYS) {
                MyAvatar.enableDriveKey(OVERRIDEN_DRIVE_KEYS[i]);
            }

            var roles = this.rolesToOverride();
            for (i in roles) {
                MyAvatar.restoreRoleAnimation(roles[i]);
            }
            MyAvatar.characterControllerEnabled = true;
            MyAvatar.hmdLeanRecenterEnabled = true;

            var index = MyAvatar.getJointIndex("Hips");
            MyAvatar.clearPinOnJoint(index);

            MyAvatar.resetSensorsAndBody();

            Script.setTimeout(function() {
                MyAvatar.bodyPitch = 0.0;
                MyAvatar.bodyRoll = 0.0;
            }, SIT_DELAY);
        }
        this.sittingDown = false;
    }

    // function called by teleport.js if it detects the appropriate userData
    this.sit = function () {
        this.sitDown();
    }

    this.createOverlay = function() {
        var text = "Click to sit";
        var textMargin = 0.05;
        var lineHeight = 0.15;

        this.overlay = Overlays.addOverlay("text3d", {
            position: { x: 0.0, y: 0.0, z: 0.0},
            dimensions: { x: 0.1, y: 0.1 },
            backgroundColor: { red: 0, green: 0, blue: 0 },
            color: { red: 255, green: 255, blue: 255 },
            topMargin: textMargin,
            leftMargin: textMargin,
            bottomMargin: textMargin,
            rightMargin: textMargin,
            text: text,
            lineHeight: lineHeight,
            alpha: 0.9,
            backgroundAlpha: 0.9,
            ignoreRayIntersection: true,
            visible: true,
            isFacingAvatar: true
        });
        var textSize = Overlays.textSize(this.overlay, text);
        var overlayDimensions = {
            x: textSize.width + 2 * textMargin,
            y: textSize.height + 2 * textMargin
        }
        var properties = Entities.getEntityProperties(this.entityID, ["position", "registrationPoint", "dimensions"]);
        var yOffset = (1.0 - properties.registrationPoint.y) * properties.dimensions.y + (overlayDimensions.y / 2.0);
        var overlayPosition = Vec3.sum(properties.position, { x: 0, y: yOffset, z: 0 });
        Overlays.editOverlay(this.overlay, {
            position: overlayPosition,
            dimensions: overlayDimensions
        });
    }
    this.cleanupOverlay = function() {
        if (this.overlay !== null) {
            Overlays.deleteOverlay(this.overlay);
            this.overlay = null;
        }
    }

    this.update = function(dt) {
        if (this.sittingDown === true) {
            var properties = Entities.getEntityProperties(this.entityID);
            var avatarDistance = Vec3.distance(MyAvatar.position, properties.position);
            var ikError = MyAvatar.getIKErrorOnLastSolve();
            var now = Date.now();
            var shouldStandUp = false;

            // Check if a drive key is pressed
            var hasActiveDriveKey = false;
            for (var i in OVERRIDEN_DRIVE_KEYS) {
                if (MyAvatar.getRawDriveKey(OVERRIDEN_DRIVE_KEYS[i]) != 0.0) {
                    hasActiveDriveKey = true;
                    break;
                }
            }

            // Only standup if user has been pushing a drive key for RELEASE_TIME
            if (hasActiveDriveKey) {
                var elapsed = now - this.lastTimeNoDriveKeys;
                shouldStandUp = elapsed > RELEASE_TIME;
            } else {
                this.lastTimeNoDriveKeys = Date.now();
            }

            // Allow some time for the IK to settle
            if (ikError > MAX_IK_ERROR && now > this.sitDownSettlePeriod) {
                shouldStandUp = true;
            }

            if (MyAvatar.sessionUUID !== this.getSeatUser()) {
                shouldStandUp = true;
            }

            if (shouldStandUp || avatarDistance > RELEASE_DISTANCE) {
                print("IK error: " + ikError + ", distance from chair: " + avatarDistance);

                // Move avatar in front of the chair to avoid getting stuck in collision hulls
                if (avatarDistance < MAX_RESET_DISTANCE) {
                    var offset = { x: 0, y: 1.0, z: -0.5 - properties.dimensions.z * properties.registrationPoint.z };
                    var position = Vec3.sum(properties.position, Vec3.multiplyQbyV(properties.rotation, offset));
                    MyAvatar.position = position;
                    print("Moving Avatar in front of the chair.");
                    // Delay standing up by 1 cycle.
                    // This leaves times for the avatar to actually move since a lot
                    // of the stand up operations are threaded
                    return;
                }

                this.standUp();
            }
        }
    }
    this.canSitDesktop = function() {
        var properties = Entities.getEntityProperties(this.entityID, ["position"]);
        var distanceFromSeat = Vec3.distance(MyAvatar.position, properties.position);
        return distanceFromSeat < DESKTOP_MAX_DISTANCE && !this.checkSeatForAvatar();
    }

    this.hoverEnterEntity = function(event) {
        if (isInEditMode() || this.interval !== null) {
            return;
        }

        var that = this;
        this.interval = Script.setInterval(function() {
            if (that.overlay === null) {
                if (that.canSitDesktop()) {
                    that.createOverlay();
                }
            } else if (!that.canSitDesktop()) {
                that.cleanupOverlay();
            }
        }, DESKTOP_UI_CHECK_INTERVAL);
    }
    this.hoverLeaveEntity = function(event) {
        if (this.interval !== null) {
            Script.clearInterval(this.interval);
            this.interval = null;
        }
        this.cleanupOverlay();
    }
    
    this.clickDownOnEntity = function (id, event) {
        if (isInEditMode()) {
            return;
        }
        if (event.isPrimaryButton && this.canSitDesktop()) {
            this.sitDown();
        }
    }
});
