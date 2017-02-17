(function() {
    Script.include("/~/system/libraries/utils.js");

    var ROLE = "fly";
    var ANIMATION_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/clement/production/animations/sitting_idle.fbx";
    var ANIMATION_FPS = 30;
    var ANIMATION_FIRST_FRAME = 1;
    var ANIMATION_LAST_FRAME = 10;
    var RELEASE_KEYS = ['w', 'a', 's', 'd', 'UP', 'LEFT', 'DOWN', 'RIGHT'];
    var RELEASE_TIME = 500; // ms
    var RELEASE_DISTANCE = 0.2; // meters
    var MAX_IK_ERROR = 15;
    var DESKTOP_UI_CHECK_INTERVAL = 250;
    var DESKTOP_MAX_DISTANCE = 5;

    this.entityID = null;
    this.timers = {};
    this.animStateHandlerID = null;

    this.preload = function(entityID) {
        this.entityID = entityID;
    }
    this.unload = function() {
        if (MyAvatar.getParentID() === this.entityID) {
            this.sitUp(this.entityID);
        }
        if (this.interval) {
            Script.clearInterval(this.interval);
            this.interval = null;
        }
        this.cleanupOverlay();
    }

    this.checkSeatForAvatar = function() {
        var avatarIdentifiers = AvatarList.getAvatarIdentifiers();
        for (var i in avatarIdentifiers) {
            var avatar = AvatarList.getAvatar(avatarIdentifiers[i]);
            if (avatar && avatar.getParentID() === this.entityID) {
                return true;
            }
        }
        return false;
    }

    this.sitDown = function() {
        if (this.checkSeatForAvatar()) {
            print("Someone is already sitting in that chair.");
            return;
        }

        MyAvatar.overrideRoleAnimation(ROLE, ANIMATION_URL, ANIMATION_FPS, true, ANIMATION_FIRST_FRAME, ANIMATION_LAST_FRAME);
        MyAvatar.setParentID(this.entityID);
        MyAvatar.characterControllerEnabled = false;
        MyAvatar.hmdLeanRecenterEnabled = false;

        var properties = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
        var index = MyAvatar.getJointIndex("Hips");
        MyAvatar.pinJoint(index, properties.position, properties.rotation);

        this.animStateHandlerID = MyAvatar.addAnimationStateHandler(function(props) {
            return { headType: 0 };
        }, ["headType"]);

        Script.update.connect(this, this.update);
        Controller.keyPressEvent.connect(this, this.keyPressed);
        Controller.keyReleaseEvent.connect(this, this.keyReleased);
        for (var i in RELEASE_KEYS) {
            Controller.captureKeyEvents({ text: RELEASE_KEYS[i] });
        }
    }

    this.sitUp = function() {
        MyAvatar.restoreRoleAnimation(ROLE);
        MyAvatar.setParentID("");
        MyAvatar.characterControllerEnabled = true;
        MyAvatar.hmdLeanRecenterEnabled = true;

        var index = MyAvatar.getJointIndex("Hips");
        MyAvatar.clearPinOnJoint(index);

        MyAvatar.removeAnimationStateHandler(this.animStateHandlerID);

        Script.update.disconnect(this, this.update);
        Controller.keyPressEvent.disconnect(this, this.keyPressed);
        Controller.keyReleaseEvent.disconnect(this, this.keyReleased);
        for (var i in RELEASE_KEYS) {
            Controller.releaseKeyEvents({ text: RELEASE_KEYS[i] });
        }
    }

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
        var props = Entities.getEntityProperties(this.entityID, ["position", "registrationPoint", "dimensions"]);
        var yOffset = (1.0 - props.registrationPoint.y) * props.dimensions.y + (overlayDimensions.y / 2.0);
        var overlayPosition = Vec3.sum(props.position, { x: 0, y: yOffset, z: 0 });
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
        if (MyAvatar.getParentID() === this.entityID) {
            var properties = Entities.getEntityProperties(this.entityID, ["position"]);
            var avatarDistance = Vec3.distance(MyAvatar.position, properties.position);
            var ikError = MyAvatar.getIKErrorOnLastSolve();
            print("IK error: " + ikError);
            if (avatarDistance > RELEASE_DISTANCE || ikError > MAX_IK_ERROR) {
                this.sitUp(this.entityID);
            }
        }
    }
    this.keyPressed = function(event) {
        if (isInEditMode()) {
            return;
        }

        if (RELEASE_KEYS.indexOf(event.text) !== -1) {
            var that = this;
            this.timers[event.text] = Script.setTimeout(function() {
                that.sitUp();
            }, RELEASE_TIME);
        }
    }
    this.keyReleased = function(event) {
        if (RELEASE_KEYS.indexOf(event.text) !== -1) {
            if (this.timers[event.text]) {
                Script.clearTimeout(this.timers[event.text]);
                delete this.timers[event.text];
            }
        }
    }

    this.canSitDesktop = function() {
        var props = Entities.getEntityProperties(this.entityID, ["position"]);
        var distanceFromSeat = Vec3.distance(MyAvatar.position, props.position);
        return distanceFromSeat < DESKTOP_MAX_DISTANCE && !this.checkSeatForAvatar();
    }

    this.hoverEnterEntity = function(event) {
        if (isInEditMode() || (MyAvatar.getParentID() === this.entityID)) {
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
        if (this.interval) {
            Script.clearInterval(this.interval);
            this.interval = null;
        }
        this.cleanupOverlay();
    }

    this.clickDownOnEntity = function () {
        if (isInEditMode() || (MyAvatar.getParentID() === this.entityID)) {
            return;
        }
        if (this.canSitDesktop()) {
            this.sitDown();
        }
    }
});
