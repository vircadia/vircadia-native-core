//
//  tabletRezzer.js
//
//  Created by David Rowe on 9 Aug 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global */

(function () {

    "use strict";

    var // Overlay
        proxyOverlay,

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

        // Events
        MIN_HAND_CAMERA_ANGLE = 30,
        DEGREES_180 = 180,
        MIN_HAND_CAMERA_ANGLE_COS = Math.cos(Math.PI * MIN_HAND_CAMERA_ANGLE / DEGREES_180),
        updateTimer = null,
        UPDATE_INTERVAL = 250,

        LEFT_HAND = 0,
        RIGHT_HAND = 1,
        DEBUG = true;

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
    }

    function enterProxyVisible(hand) {
        proxyHand = hand;
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
            enter: null,
            update: null,
            exit: null
        },
        TABLET_OPEN: { // Tablet proper is being displayed.
            enter: null,
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
                // Compare palm directions of hands with vectors from palms to camera.
                if (shouldShowProxy(LEFT_HAND)) {
                    setState(PROXY_VISIBLE, LEFT_HAND);
                    break;
                } else if (shouldShowProxy(RIGHT_HAND)) {
                    setState(PROXY_VISIBLE, RIGHT_HAND);
                    break;
                }
                break;
            case PROXY_VISIBLE:
                // Check that palm direction of proxy hand still less than maximum angle.
                if (!shouldShowProxy(proxyHand)) {
                    setState(PROXY_HIDDEN);
                    break;
                }
                break;
            default:
                error("Missing case: " + rezzerState);
        }

        updateTimer = Script.setTimeout(update, UPDATE_INTERVAL);
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
        HMD.mountedChanged.connect(onMountedChanged);
        HMD.displayModeChanged.connect(onMountedChanged); // For the case that the HMD is already worn when the script starts.
        if (HMD.mounted) {
            update();
        }
    }

    function tearDown() {
        HMD.displayModeChanged.disconnect(onMountedChanged);
        HMD.mountedChanged.disconnect(onMountedChanged);
        if (updateTimer !== null) {
            Script.clearTimeout(updateTimer);
            updateTimer = null;
        }
    }

    setUp();
    Script.scriptEnding.connect(tearDown);

    //#endregion

}());
