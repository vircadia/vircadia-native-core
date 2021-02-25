"use strict";

//
//  away.js
//
//  Created by Howard Stearns 11/3/15
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// Goes into "paused" when the '.' key (and automatically when started in HMD), and normal when pressing any key.
// See MAIN CONTROL, below, for what "paused" actually does.

/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function() { // BEGIN LOCAL_SCOPE

var BASIC_TIMER_INTERVAL = 50; // 50ms = 20hz
var OVERLAY_WIDTH = 1920;
var OVERLAY_HEIGHT = 1080;
var OVERLAY_DATA = {
    width: OVERLAY_WIDTH,
    height: OVERLAY_HEIGHT,
    imageURL: Script.resolvePath("assets/images/Overlay-Viz-blank.png"),
    emissive: true,
    drawInFront: true,
    alpha: 1
};
var AVATAR_MOVE_FOR_ACTIVE_DISTANCE = 0.8; // meters -- no longer away if avatar moves this far while away

var CAMERA_MATRIX = -7;

var OVERLAY_DATA_HMD = {
    localPosition: {x: 0, y: 0, z: -1 * MyAvatar.sensorToWorldScale},
    localRotation: {x: 0, y: 0, z: 0, w: 1},
    width: OVERLAY_WIDTH,
    height: OVERLAY_HEIGHT,
    url: Script.resolvePath("assets/images/Overlay-Viz-blank.png"),
    color: {red: 255, green: 255, blue: 255},
    alpha: 1,
    scale: 2 * MyAvatar.sensorToWorldScale,
    emissive: true,
    drawInFront: true,
    parentID: MyAvatar.SELF_ID,
    parentJointIndex: CAMERA_MATRIX,
    ignorePickIntersection: true
};

var AWAY_INTRO = {
    url: "qrc:///avatar/animations/afk_texting.fbx",
    playbackRate: 30.0,
    loopFlag: true,
    startFrame: 1.0,
    endFrame: 489.0
};

// MAIN CONTROL
var isEnabled = true;
var wasMuted; // unknonwn?
var isAway = false; // we start in the un-away state
var eventMappingName = "io.highfidelity.away"; // goActive on hand controller button events, too.
var eventMapping = Controller.newMapping(eventMappingName);
var avatarPosition = MyAvatar.position;
var wasHmdMounted = HMD.mounted;
var previousBubbleState = Users.getIgnoreRadiusEnabled();

var enterAwayStateWhenFocusLostInVR = HMD.enterAwayStateWhenFocusLostInVR;

// some intervals we may create/delete
var avatarMovedInterval;


// prefetch the kneel animation and hold a ref so it's always resident in memory when we need it.
var _animation = AnimationCache.prefetch(AWAY_INTRO.url);

function playAwayAnimation() {
    MyAvatar.overrideAnimation(AWAY_INTRO.url,
            AWAY_INTRO.playbackRate,
            AWAY_INTRO.loopFlag,
            AWAY_INTRO.startFrame,
            AWAY_INTRO.endFrame);
}

function stopAwayAnimation() {
    MyAvatar.restoreAnimation();
}

// OVERLAY
var overlay = Overlays.addOverlay("image", OVERLAY_DATA);
var overlayHMD = Overlays.addOverlay("image3d", OVERLAY_DATA_HMD);

function showOverlay() {
    if (HMD.active) {
        // make sure desktop version is hidden
        Overlays.editOverlay(overlay, { visible: false });
        Overlays.editOverlay(overlayHMD, { visible: true });
    } else {
        // make sure HMD is hidden
        Overlays.editOverlay(overlayHMD, { visible: false });

        // Update for current screen size, keeping overlay proportions constant.
        var screen = Controller.getViewportDimensions();

        // keep the overlay it's natural size and always center it...
        Overlays.editOverlay(overlay, {
            visible: true,
            x: ((screen.x - OVERLAY_WIDTH) / 2),
            y: ((screen.y - OVERLAY_HEIGHT) / 2)
        });
    }
}

function hideOverlay() {
    Overlays.editOverlay(overlay, {visible: false});
    Overlays.editOverlay(overlayHMD, {visible: false});
}

hideOverlay();

function maybeMoveOverlay() {
    if (isAway) {
        // if we switched from HMD to Desktop, make sure to hide our HUD overlay and show the
        // desktop overlay
        if (!HMD.active) {
            showOverlay(); // this will also recenter appropriately
        }

        if (HMD.active) {

            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            var localPosition = {x: 0, y: 0, z: -1 * sensorScaleFactor};
            Overlays.editOverlay(overlayHMD, { visible: true, localPosition: localPosition, scale: 2 * sensorScaleFactor });

            // make sure desktop version is hidden
            Overlays.editOverlay(overlay, { visible: false });

            // also remember avatar position
            avatarPosition = MyAvatar.position;

        }
    }
}

function ifAvatarMovedGoActive() {
    var newAvatarPosition = MyAvatar.position;
    if (Vec3.distance(newAvatarPosition, avatarPosition) > AVATAR_MOVE_FOR_ACTIVE_DISTANCE) {
        goActive();
    }
    avatarPosition = newAvatarPosition;
}

function goAway(fromStartup) {
    if (!isEnabled || isAway) {
        return;
    }
 
    // If we're entering away mode from some other state than startup, then we create our move timer immediately.
    // However if we're just stating up, we need to delay this process so that we don't think the initial teleport
    // is actually a move.
    if (fromStartup === undefined || fromStartup === false) {
        avatarMovedInterval = Script.setInterval(ifAvatarMovedGoActive, BASIC_TIMER_INTERVAL);
    } else {
        var WAIT_FOR_MOVE_ON_STARTUP = 3000; // 3 seconds
        Script.setTimeout(function() {
            avatarMovedInterval = Script.setInterval(ifAvatarMovedGoActive, BASIC_TIMER_INTERVAL);
        }, WAIT_FOR_MOVE_ON_STARTUP);
    }

    previousBubbleState = Users.getIgnoreRadiusEnabled();
    if (!previousBubbleState) {
        Users.toggleIgnoreRadius();
    }
    UserActivityLogger.privacyShieldToggled(Users.getIgnoreRadiusEnabled());
    UserActivityLogger.toggledAway(true);
    MyAvatar.isAway = true;
}

function goActive() {
    if (!isAway) {
        return;
    }

    UserActivityLogger.toggledAway(false);
    MyAvatar.isAway = false;

    if (Users.getIgnoreRadiusEnabled() !== previousBubbleState) {
        Users.toggleIgnoreRadius();
        UserActivityLogger.privacyShieldToggled(Users.getIgnoreRadiusEnabled());
    }

    if (!Window.hasFocus()) {
        Window.setFocus();
    }
}

MyAvatar.wentAway.connect(setAwayProperties);
MyAvatar.wentActive.connect(setActiveProperties);

function setAwayProperties() {
    isAway = true;
    wasMuted = Audio.muted;
    if (!wasMuted) {
        Audio.muted = !Audio.muted;
    }
    MyAvatar.setEnableMeshVisible(false); // just for our own display, without changing point of view
    playAwayAnimation(); // animation is still seen by others
    showOverlay();

    HMD.requestShowHandControllers();

    // tell the Reticle, we want to stop capturing the mouse until we come back
    Reticle.allowMouseCapture = false;
    // Allow users to find their way to other applications, our menus, etc.
    // For desktop, that means we want the reticle visible.
    // For HMD, the hmd preview will show the system mouse because of allowMouseCapture,
    // but we want to turn off our Reticle so that we don't get two in preview and a stuck one in headset.
    Reticle.visible = !HMD.active;
    wasHmdMounted = HMD.mounted; // always remember the correct state

    avatarPosition = MyAvatar.position;
}

function setActiveProperties() {
    isAway = false;
    if (Audio.muted && !wasMuted) {
        Audio.muted = false;
    }
    MyAvatar.setEnableMeshVisible(true); // IWBNI we respected Developer->Avatar->Draw Mesh setting.
    stopAwayAnimation();

    HMD.requestHideHandControllers();

    // update the UI sphere to be centered about the current HMD orientation.
    HMD.centerUI();

    // forget about any IK joint limits
    MyAvatar.clearIKJointLimitHistory();

    // update the avatar hips to point in the same direction as the HMD orientation.
    MyAvatar.centerBody();

    hideOverlay();

    // tell the Reticle, we are ready to capture the mouse again and it should be visible
    Reticle.allowMouseCapture = true;
    Reticle.visible = true;
    if (HMD.active) {
        Reticle.position = HMD.getHUDLookAtPosition2D();
    }
    wasHmdMounted = HMD.mounted; // always remember the correct state

    Script.clearInterval(avatarMovedInterval);
}

var wasHmdActive = HMD.active;
var wasMouseCaptured = Reticle.mouseCaptured;

function maybeGoAway() {
    // If our active state change (went to or from HMD mode), and we are now in the HMD, go into away
    if (HMD.active !== wasHmdActive) {
        wasHmdActive = !wasHmdActive;
        if (wasHmdActive) {
            goAway();
            return;
        }
    }

    // If the mouse has gone from captured, to non-captured state, then it likely means the person is still in the HMD,
    // but tabbed away from the application (meaning they don't have mouse control) and they likely want to go into
    // an away state
    if (Reticle.mouseCaptured !== wasMouseCaptured) {
        wasMouseCaptured = !wasMouseCaptured;
        if (!wasMouseCaptured) {
            if (enterAwayStateWhenFocusLostInVR) {
                goAway();
                return;
            }
        }
    }

    // If you've removed your HMD from your head, and we can detect it, we will also go away...
    if (HMD.mounted !== wasHmdMounted) {
        wasHmdMounted = HMD.mounted;
        print("HMD mounted changed...");

        // We're putting the HMD on... switch to those devices
        if (HMD.mounted) {
            print("NOW mounted...");
        } else {
            print("HMD NOW un-mounted...");

            if (HMD.active) {
                goAway();
                return;
            }
        }
    }
}

function setEnabled(value) {
    if (!value) {
        goActive();
    }
    isEnabled = value;
}

function checkAudioToggled() {
    if (isAway && !Audio.muted) {
        goActive();
    }
}


var CHANNEL_AWAY_ENABLE = "Hifi-Away-Enable";
var handleMessage = function(channel, message, sender) {
    if (channel === CHANNEL_AWAY_ENABLE && sender === MyAvatar.sessionUUID) {
        print("away.js | Got message on Hifi-Away-Enable: ", message);
        if (message === 'enable') {
            setEnabled(true);
        } else if (message === 'toggle') {
            toggleAway();
        }
    }
};
Messages.subscribe(CHANNEL_AWAY_ENABLE);
Messages.messageReceived.connect(handleMessage);

function toggleAway() {
    if (!isAway) {
        goAway();
    } else {
        goActive();
    }
}

var maybeIntervalTimer = Script.setInterval(function() {
    maybeMoveOverlay();
    maybeGoAway();
    checkAudioToggled();
}, BASIC_TIMER_INTERVAL);


Controller.mousePressEvent.connect(goActive);
// Note peek() so as to not interfere with other mappings.
eventMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(goActive);
eventMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(goActive);
eventMapping.from(Controller.Standard.LeftSecondaryThumb).peek().to(goActive);
eventMapping.from(Controller.Standard.RightSecondaryThumb).peek().to(goActive);
eventMapping.from(Controller.Standard.LT).peek().to(goActive);
eventMapping.from(Controller.Standard.LB).peek().to(goActive);
eventMapping.from(Controller.Standard.LS).peek().to(goActive);
eventMapping.from(Controller.Standard.LeftGrip).peek().to(goActive);
eventMapping.from(Controller.Standard.RT).peek().to(goActive);
eventMapping.from(Controller.Standard.RB).peek().to(goActive);
eventMapping.from(Controller.Standard.RS).peek().to(goActive);
eventMapping.from(Controller.Standard.RightGrip).peek().to(goActive);
eventMapping.from(Controller.Standard.Back).peek().to(goActive);
eventMapping.from(Controller.Standard.Start).peek().to(goActive);
Controller.enableMapping(eventMappingName);

function awayStateWhenFocusLostInVRChanged(enabled) {
    enterAwayStateWhenFocusLostInVR = enabled;
}

Script.scriptEnding.connect(function () {
    Script.clearInterval(maybeIntervalTimer);
    goActive();
    HMD.awayStateWhenFocusLostInVRChanged.disconnect(awayStateWhenFocusLostInVRChanged);
    Controller.disableMapping(eventMappingName);
    Controller.mousePressEvent.disconnect(goActive);
    Messages.messageReceived.disconnect(handleMessage);
    Messages.unsubscribe(CHANNEL_AWAY_ENABLE);
});

HMD.awayStateWhenFocusLostInVRChanged.connect(awayStateWhenFocusLostInVRChanged);

if (HMD.active && !HMD.mounted) {
    print("Starting script, while HMD is active and not mounted...");
    goAway(true);
}


}()); // END LOCAL_SCOPE
