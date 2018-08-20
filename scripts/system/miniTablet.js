//
//  miniTablet.js
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

    var // Base overlay
        proxyOverlay = null,
        PROXY_MODEL = Script.resolvePath("./assets/models/tinyTablet.fbx"),
        PROXY_DIMENSIONS = { x: 0.0637, y: 0.0965, z: 0.0046 }, // Proportional to tablet proper.
        PROXY_POSITION_LEFT_HAND = {
            x: 0,
            y: 0.07, // Distance from joint.
            z: 0.07 // Distance above palm.
        },
        PROXY_POSITION_RIGHT_HAND = {
            x: 0,
            y: 0.07, // Distance from joint.
            z: 0.07 // Distance above palm.
        },
        /*
        // Aligned cross-palm.
        PROXY_ROTATION_LEFT_HAND = Quat.fromVec3Degrees({ x: 0, y: 180, z: 90 }),
        PROXY_ROTATION_RIGHT_HAND = Quat.fromVec3Degrees({ x: 0, y: 180, z: -90 }),
        */
        // Aligned with palm.
        PROXY_ROTATION_LEFT_HAND = Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),
        PROXY_ROTATION_RIGHT_HAND = Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),

        // UI overlay.
        proxyUIOverlay = null,
        PROXY_UI_HTML = Script.resolvePath("./html/miniTablet.html"),
        PROXY_UI_DIMENSIONS = { x: 0.0577, y: 0.0905 },
        PROXY_UI_WIDTH_PIXELS = 150,
        METERS_TO_INCHES = 39.3701,
        PROXY_UI_DPI = PROXY_UI_WIDTH_PIXELS / (PROXY_UI_DIMENSIONS.x * METERS_TO_INCHES),
        PROXY_UI_OFFSET = 0.001, // Above model surface.
        PROXY_UI_LOCAL_POSITION = { x: 0, y: 0, z: -(PROXY_DIMENSIONS.z / 2 + PROXY_UI_OFFSET) },
        PROXY_UI_LOCAL_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),
        proxyUIOverlayEnabled = false,
        PROXY_UI_OVERLAY_ENABLED_DELAY = 500,
        proxyOverlayObject = null,

        MUTE_ON_ICON = Script.resourcesPath() + "icons/tablet-icons/mic-mute-a.svg",
        MUTE_OFF_ICON = Script.resourcesPath() + "icons/tablet-icons/mic-unmute-i.svg",
        BUBBLE_ON_ICON = Script.resourcesPath() + "icons/tablet-icons/bubble-a.svg",
        BUBBLE_OFF_ICON = Script.resourcesPath() + "icons/tablet-icons/bubble-i.svg",

        // State machine
        PROXY_DISABLED = 0,
        PROXY_HIDDEN = 1,
        PROXY_VISIBLE = 2,
        PROXY_EXPANDING = 3,
        TABLET_OPEN = 4,
        STATE_STRINGS = ["PROXY_DISABLED", "PROXY_HIDDEN", "PROXY_VISIBLE", "PROXY_EXPANDING", "TABLET_OPEN"],
        STATE_MACHINE,
        rezzerState = PROXY_DISABLED,
        proxyHand,
        PROXY_EXPAND_HANDLES = [
            { x: 0.5, y: -0.4, z: 0 },
            { x: -0.5, y: -0.4, z: 0 }
        ],
        proxyExpandHand,
        proxyExpandLocalPosition,
        proxyExpandLocalRotation = Quat.IDENTITY,
        PROXY_EXPAND_DURATION = 250,
        PROXY_EXPAND_TIMEOUT = 25,
        proxyExpandTimer = null,
        proxyExpandStart,
        proxyInitialWidth,
        proxyTargetWidth,
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system"),

        // EventBridge
        READY_MESSAGE = "ready", // Engine <== Dialog
        HOVER_MESSAGE = "hover", // Engine <== Dialog
        MUTE_MESSAGE = "mute", // Engine <=> Dialog
        BUBBLE_MESSAGE = "bubble", // Engine <=> Dialog
        EXPAND_MESSAGE = "expand", // Engine <== Dialog

        // Events
        MIN_HAND_CAMERA_ANGLE = 30,
        DEGREES_180 = 180,
        MIN_HAND_CAMERA_ANGLE_COS = Math.cos(Math.PI * MIN_HAND_CAMERA_ANGLE / DEGREES_180),
        updateTimer = null,
        UPDATE_INTERVAL = 300,
        HIFI_OBJECT_MANIPULATION_CHANNEL = "Hifi-Object-Manipulation",
        avatarScale = 1,

        // Sounds
        HOVER_SOUND = "./assets/sounds/button-hover.wav",
        HOVER_VOLUME = 0.5,
        CLICK_SOUND = "./assets/sounds/button-click.wav",
        CLICK_VOLUME = 0.8,
        hoverSound = SoundCache.getSound(Script.resolvePath(HOVER_SOUND)),
        clickSound = SoundCache.getSound(Script.resolvePath(CLICK_SOUND)),

        // Hands
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

    function otherHand(hand) {
        return hand === LEFT_HAND ? RIGHT_HAND : LEFT_HAND;
    }

    function playSound(sound, volume) {
        Audio.playSound(sound, {
            position: proxyHand === LEFT_HAND ? MyAvatar.getLeftPalmPosition() : MyAvatar.getRightPalmPosition(),
            volume: volume,
            localOnly: true
        });
    }

    // #endregion

    // #region Communications ==================================================================================================

    function updateMutedStatus() {
        var isMuted = Audio.muted;
        proxyOverlayObject.emitScriptEvent(JSON.stringify({
            type: MUTE_MESSAGE,
            on: isMuted,
            icon: isMuted ? MUTE_ON_ICON : MUTE_OFF_ICON
        }));
    }

    function updateBubbleStatus() {
        var isBubbleOn = Users.getIgnoreRadiusEnabled();
        proxyOverlayObject.emitScriptEvent(JSON.stringify({
            type: BUBBLE_MESSAGE,
            on: isBubbleOn,
            icon: isBubbleOn ? BUBBLE_ON_ICON : BUBBLE_OFF_ICON
        }));
    }

    function onWebEventReceived(data) {
        var message;

        try {
            message = JSON.parse(data);
        } catch (e) {
            console.error("EventBridge message error");
            return;
        }

        switch (message.type) {
            case READY_MESSAGE:
                // Send initial button statuses.
                updateMutedStatus();
                updateBubbleStatus();
                break;
            case HOVER_MESSAGE:
                // Audio feedback.
                playSound(hoverSound, HOVER_VOLUME);
                break;
            case MUTE_MESSAGE:
                // Toggle mute.
                playSound(clickSound, CLICK_VOLUME);
                Audio.muted = !Audio.muted;
                break;
            case BUBBLE_MESSAGE:
                // Toggle bubble.
                playSound(clickSound, CLICK_VOLUME);
                Users.toggleIgnoreRadius();
                break;
            case EXPAND_MESSAGE:
                // Expand tablet;
                playSound(clickSound, CLICK_VOLUME);
                setState(PROXY_EXPANDING, proxyHand);
                break;
        }
    }

    // #endregion

    // #region UI ==============================================================================================================

    function createUI() {
        proxyOverlay = Overlays.addOverlay("model", {
            url: PROXY_MODEL,
            dimensions: Vec3.multiply(avatarScale, PROXY_DIMENSIONS),
            solid: true,
            grabbable: true,
            showKeyboardFocusHighlight: false,
            displayInFront: true,
            visible: false
        });
        proxyUIOverlay = Overlays.addOverlay("web3d", {
            url: PROXY_UI_HTML,
            parentID: proxyOverlay,
            localPosition: Vec3.multiply(avatarScale, PROXY_UI_LOCAL_POSITION),
            localRotation: PROXY_UI_LOCAL_ROTATION,
            dimensions: Vec3.multiply(avatarScale, PROXY_UI_DIMENSIONS),
            dpi: PROXY_UI_DPI / avatarScale,
            alpha: 0, // Hide overlay while its content is being created.
            grabbable: false,
            showKeyboardFocusHighlight: false,
            displayInFront: true,
            visible: false
        });

        proxyUIOverlayEnabled = false; // This and alpha = 0 hides overlay while its content is being created.

        proxyOverlayObject = Overlays.getOverlayObject(proxyUIOverlay);
        proxyOverlayObject.webEventReceived.connect(onWebEventReceived);
    }

    function showUI(hand) {
        Overlays.editOverlay(proxyOverlay, {
            parentID: MyAvatar.SELF_ID,
            parentJointIndex: handJointIndex(proxyHand),
            localPosition: Vec3.multiply(avatarScale,
                proxyHand === LEFT_HAND ? PROXY_POSITION_LEFT_HAND : PROXY_POSITION_RIGHT_HAND),
            localRotation: proxyHand === LEFT_HAND ? PROXY_ROTATION_LEFT_HAND : PROXY_ROTATION_RIGHT_HAND,
            dimensions: Vec3.multiply(avatarScale, PROXY_DIMENSIONS),
            visible: true
        });
        Overlays.editOverlay(proxyUIOverlay, {
            localPosition: Vec3.multiply(avatarScale, PROXY_UI_LOCAL_POSITION),
            dimensions: Vec3.multiply(avatarScale, PROXY_UI_DIMENSIONS),
            dpi: PROXY_UI_DPI / avatarScale,
            visible: true
        });

        if (!proxyUIOverlayEnabled) {
            // Overlay content is created the first time it is visible to the user. The initial creation displays artefacts.
            // Delay showing UI overlay until after giving it time for its content to be created.
            Script.setTimeout(function () {
                Overlays.editOverlay(proxyUIOverlay, { alpha: 1.0 });
            }, PROXY_UI_OVERLAY_ENABLED_DELAY);
        }
    }

    function sizeUI(scaleFactor) {
        Overlays.editOverlay(proxyOverlay, {
            localPosition:
                Vec3.sum(proxyExpandLocalPosition,
                    Vec3.multiplyQbyV(proxyExpandLocalRotation,
                        Vec3.multiply(-scaleFactor,
                            Vec3.multiplyVbyV(PROXY_EXPAND_HANDLES[proxyExpandHand], PROXY_DIMENSIONS)))
                ),
            dimensions: Vec3.multiply(scaleFactor, PROXY_DIMENSIONS)
        });
        Overlays.editOverlay(proxyUIOverlay, {
            localPosition: Vec3.multiply(scaleFactor, PROXY_UI_LOCAL_POSITION),
            dimensions: Vec3.multiply(scaleFactor, PROXY_UI_DIMENSIONS),
            dpi: PROXY_UI_DPI / scaleFactor
        });
    }

    function hideUI() {
        Overlays.editOverlay(proxyOverlay, {
            parentID: Uuid.NULL, // Release hold so that hand can grab tablet proper.
            visible: false
        });
        Overlays.editOverlay(proxyUIOverlay, {
            visible: false
        });
    }

    function destroyUI() {
        if (proxyOverlayObject) {
            proxyOverlayObject.webEventReceived.disconnect(onWebEventReceived);
            Overlays.deleteOverlay(proxyUIOverlay);
            Overlays.deleteOverlay(proxyOverlay);
            proxyOverlayObject = null;
            proxyUIOverlay = null;
            proxyOverlay = null;
        }
    }

    // #endregion

    // #region State Machine ===================================================================================================

    function enterProxyDisabled() {
        // Stop updates.
        if (updateTimer !== null) {
            Script.clearTimeout(updateTimer);
            updateTimer = null;
        }

        // Stop monitoring mute and bubble changes.
        Audio.mutedChanged.disconnect(updateMutedStatus);
        Users.ignoreRadiusEnabledChanged.disconnect(updateBubbleStatus);

        // Don't keep overlays prepared if in desktop mode.
        destroyUI();
    }

    function exitProxyDisabled() {
        // Create UI so that it's ready to be displayed without seeing artefacts from creating the UI.
        createUI();

        // Start monitoring mute and bubble changes.
        Audio.mutedChanged.connect(updateMutedStatus);
        Users.ignoreRadiusEnabledChanged.connect(updateBubbleStatus);

        // Start updates.
        updateTimer = Script.setTimeout(updateState, UPDATE_INTERVAL);
    }

    function shouldShowProxy(hand) {
        // Should show tablet proxy if hand is oriented toward the camera and the camera is oriented toward the proxy tablet.
        var pose,
            jointIndex,
            handPosition,
            handOrientation,
            cameraToHandDirection;

        pose = Controller.getPoseValue(hand === LEFT_HAND ? Controller.Standard.LeftHand : Controller.Standard.RightHand);
        if (!pose.valid) {
            return false;
        }

        jointIndex = handJointIndex(hand);
        handPosition = Vec3.sum(MyAvatar.position,
            Vec3.multiplyQbyV(MyAvatar.orientation, MyAvatar.getAbsoluteJointTranslationInObjectFrame(jointIndex)));
        handOrientation = Quat.multiply(MyAvatar.orientation, MyAvatar.getAbsoluteJointRotationInObjectFrame(jointIndex));
        cameraToHandDirection = Vec3.normalize(Vec3.subtract(handPosition, Camera.position));

        return Vec3.dot(cameraToHandDirection, Quat.getForward(handOrientation)) > MIN_HAND_CAMERA_ANGLE_COS
            && Vec3.dot(cameraToHandDirection, Quat.getForward(Camera.orientation)) > MIN_HAND_CAMERA_ANGLE_COS;
    }

    function enterProxyHidden() {
        hideUI();
    }

    function updateProxyHidden() {
        // Don't show proxy if tablet is already displayed or in toolbar mode.
        if (HMD.showTablet || tablet.toolbarMode) {
            return;
        }
        // Compare palm directions of hands with vectors from palms to camera.
        if (shouldShowProxy(LEFT_HAND)) {
            setState(PROXY_VISIBLE, LEFT_HAND);
        } else if (shouldShowProxy(RIGHT_HAND)) {
            setState(PROXY_VISIBLE, RIGHT_HAND);
        }
    }

    function enterProxyVisible(hand) {
        proxyHand = hand;
        showUI(proxyHand);
    }

    function updateProxyVisible() {
        // Hide proxy if tablet has been displayed by other means.
        if (HMD.showTablet) {
            setState(PROXY_HIDDEN);
            return;
        }
        // Check that palm direction of proxy hand still less than maximum angle.
        if (!shouldShowProxy(proxyHand)) {
            setState(PROXY_HIDDEN);
        }
    }

    function expandProxy() {
        var scaleFactor = (Date.now() - proxyExpandStart) / PROXY_EXPAND_DURATION;
        var tabletScaleFactor = avatarScale * (1 + scaleFactor * (proxyTargetWidth - proxyInitialWidth) / proxyInitialWidth);
        if (scaleFactor < 1) {
            sizeUI(tabletScaleFactor);
            proxyExpandTimer = Script.setTimeout(expandProxy, PROXY_EXPAND_TIMEOUT);
            return;
        }

        proxyExpandTimer = null;
        setState(TABLET_OPEN);
    }

    function enterProxyExpanding(hand) {
        // Grab details.
        var properties = Overlays.getProperties(proxyOverlay, ["localPosition", "localRotation"]);
        proxyExpandHand = hand;
        proxyExpandLocalRotation = properties.localRotation;
        proxyExpandLocalPosition = Vec3.sum(properties.localPosition,
            Vec3.multiplyQbyV(proxyExpandLocalRotation,
                Vec3.multiplyVbyV(PROXY_EXPAND_HANDLES[proxyExpandHand], PROXY_DIMENSIONS)));

        // Start expanding.
        proxyInitialWidth = PROXY_DIMENSIONS.x;
        proxyTargetWidth = getTabletWidthFromSettings();
        proxyExpandStart = Date.now();
        proxyExpandTimer = Script.setTimeout(expandProxy, PROXY_EXPAND_TIMEOUT);
    }

    function updateProxyExanding() {
        // Hide proxy if tablet has been displayed by other means.
        if (HMD.showTablet) {
            setState(PROXY_HIDDEN);
        }
    }

    function exitProxyExpanding() {
        if (proxyExpandTimer !== null) {
            Script.clearTimeout(proxyExpandTimer);
            proxyExpandTimer = null;
        }
    }

    function enterTabletOpen() {
        var proxyOverlayProperties = Overlays.getProperties(proxyOverlay, ["position", "orientation"]);

        hideUI();

        Overlays.editOverlay(HMD.tabletID, {
            position: proxyOverlayProperties.position,
            orientation: proxyOverlayProperties.orientation
        });
        HMD.openTablet(true);
    }

    function updateTabletOpen() {
        // Immediately transition back to PROXY_HIDDEN.
        setState(PROXY_HIDDEN);
    }

    STATE_MACHINE = {
        PROXY_DISABLED: { // Tablet proxy cannot be shown because in desktop mode.
            enter: enterProxyDisabled,
            update: null,
            exit: exitProxyDisabled
        },
        PROXY_HIDDEN: { // Tablet proxy could be shown but isn't because hand is oriented to show it or aren't in HMD mode.
            enter: enterProxyHidden,
            update: updateProxyHidden,
            exit: null
        },
        PROXY_VISIBLE: { // Tablet proxy is visible and attached to hand.
            enter: enterProxyVisible,
            update: updateProxyVisible,
            exit: null
        },
        PROXY_EXPANDING: { // Tablet proxy has been grabbed and is expanding before showing tablet proper.
            enter: enterProxyExpanding,
            update: updateProxyExanding,
            exit: exitProxyExpanding
        },
        TABLET_OPEN: { // Tablet proper is being displayed.
            enter: enterTabletOpen,
            update: updateTabletOpen,
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
        updateTimer = Script.setTimeout(updateState, UPDATE_INTERVAL);
    }

    // #endregion

    // #region Events ==========================================================================================================

    function onScaleChanged() {
        avatarScale = MyAvatar.scale;
        // Clamp scale in order to work around M17434.
        avatarScale = Math.max(MyAvatar.getDomainMinScale(), Math.min(MyAvatar.getDomainMaxScale(), avatarScale));
    }

    function onMessageReceived(channel, data, senderID, localOnly) {
        var message, hand;

        if (channel !== HIFI_OBJECT_MANIPULATION_CHANNEL) {
            return;
        }

        message = JSON.parse(data);
        if (message.grabbedEntity !== proxyOverlay) {
            return;
        }

        if (message.action === "grab" && rezzerState === PROXY_VISIBLE) {
            hand = message.joint === HAND_NAMES[proxyHand] ? proxyHand : otherHand(proxyHand);
            setState(PROXY_EXPANDING, hand);
        }
    }

    function onDisplayModeChanged() {
        // Tablet proxy only available when HMD is active.
        if (HMD.active) {
            setState(PROXY_HIDDEN);
        } else {
            setState(PROXY_DISABLED);
        }
    }

    // #endregion

    // #region Start-up and tear-down ==========================================================================================

    function setUp() {
        MyAvatar.scaleChanged.connect(onScaleChanged);

        Messages.subscribe(HIFI_OBJECT_MANIPULATION_CHANNEL);
        Messages.messageReceived.connect(onMessageReceived);

        HMD.displayModeChanged.connect(onDisplayModeChanged);
        if (HMD.active) {
            setState(PROXY_HIDDEN);
        }
    }

    function tearDown() {
        if (updateTimer !== null) {
            Script.clearTimeout(updateTimer);
            updateTimer = null;
        }

        setState(PROXY_DISABLED);

        HMD.displayModeChanged.disconnect(onDisplayModeChanged);

        Messages.messageReceived.disconnect(onMessageReceived);
        Messages.unsubscribe(HIFI_OBJECT_MANIPULATION_CHANNEL);

        MyAvatar.scaleChanged.disconnect(onScaleChanged);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);

    // #endregion

}());
