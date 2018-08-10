//
//  tabletRezzer.js
//
//  Created by David Rowe on 9 Aug 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global getTabletWidthFromSettings */

(function () {

    "use strict";

    Script.include("./libraries/utils.js");

    var // Overlay
        proxyOverlay = null,
        TABLET_PROXY_DIMENSIONS = { x: 0.0638, y: 0.0965, z: 0.0045 },
        TABLET_PROXY_POSITION_LEFT_HAND = {
            x: 0,
            y: 0.07, // Distance from joint.
            z: 0.07 // Distance above palm.
        },
        TABLET_PROXY_POSITION_RIGHT_HAND = {
            x: 0,
            y: 0.07, // Distance from joint.
            z: 0.07 // Distance above palm.
        },
        TABLET_PROXY_ROTATION_LEFT_HAND = Quat.fromVec3Degrees({ x: 0, y: 180, z: 90 }),
        TABLET_PROXY_ROTATION_RIGHT_HAND = Quat.fromVec3Degrees({ x: 0, y: 180, z: -90 }),

        // State machine
        PROXY_HIDDEN = 0,
        PROXY_VISIBLE = 1,
        PROXY_GRABBED = 2,
        PROXY_EXPANDING = 3,
        TABLET_OPEN = 4,
        STATE_STRINGS = ["PROXY_HIDDEN", "PROXY_VISIBLE", "PROXY_GRABBED", "PROXY_EXPANDING", "TABLET_OPEN"],
        STATE_MACHINE,
        rezzerState = PROXY_HIDDEN,
        proxyHand,
        PROXY_EXPAND_DURATION = 500,
        PROXY_EXPAND_TIMEOUT = 25,
        proxyExpandTimer = null,
        proxyExpandStart,
        proxyInitialWidth,
        proxyTargetWidth,

        // Events
        MIN_HAND_CAMERA_ANGLE = 30,
        DEGREES_180 = 180,
        MIN_HAND_CAMERA_ANGLE_COS = Math.cos(Math.PI * MIN_HAND_CAMERA_ANGLE / DEGREES_180),
        updateTimer = null,
        UPDATE_INTERVAL = 250,
        HIFI_OBJECT_MANIPULATION_CHANNEL = "Hifi-Object-Manipulation",
        avatarScale = 1,

        LEFT_HAND = 0,
        RIGHT_HAND = 1,
        HAND_NAMES = ["LeftHand", "RightHand"],
        DEBUG = false;

    // #region Utilities =======================================================================================================

    function debug(message) {
        if (!DEBUG) {
            return;
        }
        print("DEBUG: " + message);
    }

    function error(message) {
        print("ERROR: " + message);
    }

    function handJointName(hand) {
        var jointName;
        if (hand === LEFT_HAND) {
            if (Camera.mode === "first person") {
                jointName = "_CONTROLLER_LEFTHAND";
            } else if (Camera.mode === "third person") {
                jointName = "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND";
            } else {
                jointName = "LeftHand";
            }
        } else {
            if (Camera.mode === "first person") {
                jointName = "_CONTROLLER_RIGHTHAND";
            } else if (Camera.mode === "third person") {
                jointName = "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND";
            } else {
                jointName = "RightHand";
            }
        }
        return jointName;
    }

    function handJointIndex(hand) {
        return MyAvatar.getJointIndex(handJointName(hand));
    }

    // #endregion

    // #region State Machine ===================================================================================================

    function enterProxyHidden() {
        if (proxyOverlay) {
            Overlays.deleteOverlay(proxyOverlay);
            proxyOverlay = null;
        }
    }

    function enterProxyVisible(hand) {
        proxyHand = hand;
        proxyOverlay = Overlays.addOverlay("cube", {
            parentID: MyAvatar.SELF_ID,
            parentJointIndex: handJointIndex(proxyHand),
            localPosition: Vec3.multiply(avatarScale,
                proxyHand === LEFT_HAND ? TABLET_PROXY_POSITION_LEFT_HAND : TABLET_PROXY_POSITION_RIGHT_HAND),
            localRotation: proxyHand === LEFT_HAND ? TABLET_PROXY_ROTATION_LEFT_HAND : TABLET_PROXY_ROTATION_RIGHT_HAND,
            dimensions: Vec3.multiply(avatarScale, TABLET_PROXY_DIMENSIONS),
            solid: true,
            grabbable: true,
            displayInFront: true,
            visible: true
        });
    }

    function expandProxy() {
        var scaleFactor = (Date.now() - proxyExpandStart) / PROXY_EXPAND_DURATION;
        if (scaleFactor < 1) {
            Overlays.editOverlay(proxyOverlay, {
                dimensions: Vec3.multiply(
                    avatarScale * (1 + scaleFactor * (proxyTargetWidth - proxyInitialWidth) / proxyInitialWidth),
                    TABLET_PROXY_DIMENSIONS)
            });
            proxyExpandTimer = Script.setTimeout(expandProxy, PROXY_EXPAND_TIMEOUT);
            return;
        }

        proxyExpandTimer = null;
        setState(TABLET_OPEN);
    }

    function enterProxyExpanding() {
        // Detach from hand.
        Overlays.editOverlay(proxyOverlay, {
            parentID: Uuid.NULL,
            parentJointIndex: -1
        });

        // Start expanding.
        proxyInitialWidth = TABLET_PROXY_DIMENSIONS.x;
        proxyTargetWidth = getTabletWidthFromSettings();
        proxyExpandStart = Date.now();
        proxyExpandTimer = Script.setTimeout(expandProxy, PROXY_EXPAND_TIMEOUT);
    }

    function exitProxyExpanding() {
        if (proxyExpandTimer !== null) {
            Script.clearTimeout(proxyExpandTimer);
            proxyExpandTimer = null;
        }
    }

    function enterTabletOpen() {
        var proxyOverlayProperties = Overlays.getProperties(proxyOverlay, ["position", "orientation"]);

        Overlays.deleteOverlay(proxyOverlay);
        proxyOverlay = null;

        Overlays.editOverlay(HMD.tabletID, {
            position: proxyOverlayProperties.position,
            orientation: proxyOverlayProperties.orientation
        });
        HMD.openTablet(true);
    }

    STATE_MACHINE = {
        PROXY_HIDDEN: { // Tablet proxy could be shown but isn't because hand is oriented to show it or aren't in HMD mode.
            enter: enterProxyHidden,
            update: null,
            exit: null
        },
        PROXY_VISIBLE: { // Tablet proxy is visible and attached to hand.
            enter: enterProxyVisible,
            update: null,
            exit: null
        },
        PROXY_GRABBED: { // Other hand has grabbed and is holding the tablet proxy.
            enter: null,
            update: null,
            exit: null
        },
        PROXY_EXPANDING: { // Tablet proxy has been released from grab and is expanding before showing tablet proper.
            enter: enterProxyExpanding,
            update: null,
            exit: exitProxyExpanding
        },
        TABLET_OPEN: { // Tablet proper is being displayed.
            enter: enterTabletOpen,
            update: null,
            exit: null
        }
    };

    function setState(state, data) {
        if (state !== rezzerState) {
            debug("State transition from " + STATE_STRINGS[rezzerState] + " to " + STATE_STRINGS[state]);
            if (STATE_MACHINE[STATE_STRINGS[rezzerState]].exit) {
                STATE_MACHINE[STATE_STRINGS[rezzerState]].exit(data);
            }
            if (STATE_MACHINE[STATE_STRINGS[state]].enter) {
                STATE_MACHINE[STATE_STRINGS[state]].enter(data);
            }
            rezzerState = state;
        } else {
            error("Null state transition: " + state + "!");
        }
    }

    function updateState() {
        STATE_MACHINE[STATE_STRINGS[rezzerState]].update();
    }

    // #endregion

    // #region Events ==========================================================================================================

    function shouldShowProxy(hand) {
        // Should show tablet proxy if hand is oriented towards the camera.
        var pose,
            jointIndex,
            handPosition,
            handOrientation;

        pose = Controller.getPoseValue(hand === LEFT_HAND ? Controller.Standard.LeftHand : Controller.Standard.RightHand);
        if (!pose.valid) {
            return false;
        }

        jointIndex = handJointIndex(hand);
        handPosition = Vec3.sum(MyAvatar.position,
            Vec3.multiplyQbyV(MyAvatar.orientation, MyAvatar.getAbsoluteJointTranslationInObjectFrame(jointIndex)));
        handOrientation = Quat.multiply(MyAvatar.orientation, MyAvatar.getAbsoluteJointRotationInObjectFrame(jointIndex));

        return Vec3.dot(Vec3.normalize(Vec3.subtract(handPosition, Camera.position)), Quat.getForward(handOrientation))
            > MIN_HAND_CAMERA_ANGLE_COS;
    }

    function update() {
        // Assumes that is HMD.mounted.
        switch (rezzerState) {
            case PROXY_HIDDEN:
                // Don't show proxy is tablet is already displayed.
                if (HMD.showTablet) {
                    break;
                }
                // Compare palm directions of hands with vectors from palms to camera.
                if (shouldShowProxy(LEFT_HAND)) {
                    setState(PROXY_VISIBLE, LEFT_HAND);
                } else if (shouldShowProxy(RIGHT_HAND)) {
                    setState(PROXY_VISIBLE, RIGHT_HAND);
                }
                break;
            case PROXY_VISIBLE:
                // Hide proxy if tablet has been displayed by other means.
                if (HMD.showTablet) {
                    setState(PROXY_HIDDEN);
                    break;
                }
                // Check that palm direction of proxy hand still less than maximum angle.
                if (!shouldShowProxy(proxyHand)) {
                    setState(PROXY_HIDDEN);
                }
                break;
            case PROXY_GRABBED:
            case PROXY_EXPANDING:
                // Hide proxy if tablet has been displayed by other means.
                if (HMD.showTablet) {
                    setState(PROXY_HIDDEN);
                }
                break;
            case TABLET_OPEN:
                // Immediately transition back to PROXY_HIDDEN.
                setState(PROXY_HIDDEN);
                break;
            default:
                error("Missing case: " + rezzerState);
        }

        updateTimer = Script.setTimeout(update, UPDATE_INTERVAL);
    }

    function onScaleChanged() {
        avatarScale = MyAvatar.scale;
        // Clamp scale in order to work around M17434.
        avatarScale = Math.max(MyAvatar.getDomainMinScale(), Math.min(MyAvatar.getDomainMaxScale(), avatarScale));
    }

    function onMessageReceived(channel, data, senderID, localOnly) {
        var message;

        if (channel !== HIFI_OBJECT_MANIPULATION_CHANNEL) {
            return;
        }

        message = JSON.parse(data);
        if (message.grabbedEntity !== proxyOverlay) {
            return;
        }

        // Don't transition into PROXY_GRABBED unless the tablet proxy is grabbed with "other" hand. However, once it has been 
        // grabbed then the original hand can grab it back and grow it from there.
        if (message.action === "grab" && message.joint !== HAND_NAMES[proxyHand] && rezzerState === PROXY_VISIBLE) {
            setState(PROXY_GRABBED);
        } else if (message.action === "release" && rezzerState === PROXY_GRABBED) {
            setState(PROXY_EXPANDING);
        }
    }

    function onMountedChanged() {
        // Tablet proxy only available when HMD is mounted.

        if (HMD.mounted) {
            if (updateTimer === null) {
                update();
            }
            return;
        }

        if (updateTimer !== null) {
            Script.clearTimeout(updateTimer);
            updateTimer = null;
        }
        if (rezzerState !== PROXY_HIDDEN) {
            setState(PROXY_HIDDEN);
        }
    }

    // #endregion

    // #region Start-up and tear-down ==========================================================================================

    function setUp() {
        MyAvatar.scaleChanged.connect(onScaleChanged);

        Messages.subscribe(HIFI_OBJECT_MANIPULATION_CHANNEL);
        Messages.messageReceived.connect(onMessageReceived);

        HMD.mountedChanged.connect(onMountedChanged);
        HMD.displayModeChanged.connect(onMountedChanged); // For the case that the HMD is already worn when the script starts.
        if (HMD.mounted) {
            update();
        }
    }

    function tearDown() {
        setState(PROXY_HIDDEN);  // Or just tear right down? Perhaps so.

        Messages.messageReceived.disconnect(onMessageReceived);
        Messages.unsubscribe(HIFI_OBJECT_MANIPULATION_CHANNEL);

        MyAvatar.scaleChanged.disconnect(onScaleChanged);

        HMD.displayModeChanged.disconnect(onMountedChanged);
        HMD.mountedChanged.disconnect(onMountedChanged);
        if (updateTimer !== null) {
            Script.clearTimeout(updateTimer);
            updateTimer = null;
        }
        if (proxyOverlay !== null) {
            Overlays.deleteOverlay(proxyOverlay);
        }
    }

    setUp();
    Script.scriptEnding.connect(tearDown);

    // #endregion

}());
