//
//  miniTablet.js
//
//  Created by David Rowe on 9 Aug 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global getTabletWidthFromSettings, TRIGGER_OFF_VALUE */

(function () {

    "use strict";

    Script.include("./libraries/utils.js");
    Script.include("./libraries/controllerDispatcherUtils.js");

    var UI,
        ui = null,
        State,
        miniState = null,

        // Hands.
        NO_HAND = -1,
        LEFT_HAND = 0,
        RIGHT_HAND = 1,
        HAND_NAMES = ["LeftHand", "RightHand"],

        // Track controller grabbing state.
        HIFI_OBJECT_MANIPULATION_CHANNEL = "Hifi-Object-Manipulation",
        grabbingHand = NO_HAND,
        grabbedItem = null,

        // Miscellaneous.
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system"),
        DEBUG = false;


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


    UI = function () {

        if (!(this instanceof UI)) {
            return new UI();
        }

        var // Base overlay
            miniOverlay = null,
            MINI_MODEL = Script.resolvePath("./assets/models/miniTabletBlank.fbx"),
            MINI_DIMENSIONS = { x: 0.0637, y: 0.0965, z: 0.0045 }, // Proportional to tablet proper.
            MINI_POSITIONS = [
                {
                    x: -0.01, // Distance across hand.
                    y: 0.08, // Distance from joint.
                    z: 0.06 // Distance above palm.
                },
                {
                    x: 0.01, // Distance across hand.
                    y: 0.08, // Distance from joint.
                    z: 0.06 // Distance above palm.
                }
            ],
            DEGREES_180 = 180,
            MINI_PICTH = 40,
            MINI_ROTATIONS = [
                Quat.fromVec3Degrees({ x: 0, y: DEGREES_180 - MINI_PICTH, z: 90 }),
                Quat.fromVec3Degrees({ x: 0, y: DEGREES_180 + MINI_PICTH, z: -90 })
            ],

            // UI overlay.
            uiHand = LEFT_HAND,
            miniUIOverlay = null,
            MINI_UI_HTML = Script.resolvePath("./html/miniTablet.html"),
            MINI_UI_DIMENSIONS = { x: 0.059, y: 0.0865 },
            MINI_UI_WIDTH_PIXELS = 150,
            METERS_TO_INCHES = 39.3701,
            MINI_UI_DPI = MINI_UI_WIDTH_PIXELS / (MINI_UI_DIMENSIONS.x * METERS_TO_INCHES),
            MINI_UI_OFFSET = 0.001, // Above model surface.
            MINI_UI_LOCAL_POSITION = { x: 0.0002, y: 0.0024, z: -(MINI_DIMENSIONS.z / 2 + MINI_UI_OFFSET) },
            MINI_UI_LOCAL_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),
            miniUIOverlayEnabled = false,
            MINI_UI_OVERLAY_ENABLED_DELAY = 500,
            miniOverlayObject = null,

            // Button icons.
            MUTE_ON_ICON = Script.resourcesPath() + "icons/tablet-icons/mic-mute-a.svg",
            MUTE_OFF_ICON = Script.resourcesPath() + "icons/tablet-icons/mic-unmute-i.svg",
            GOTO_ICON = Script.resourcesPath() + "icons/tablet-icons/goto-i.svg",

            // Expansion to tablet.
            MINI_EXPAND_HANDLES = [ // Normalized coordinates in range [-0.5, 0.5] about center of mini tablet.
                { x: 0.5, y: -0.65, z: 0 },
                { x: -0.5, y: -0.65, z: 0 }
            ],
            MINI_EXPAND_DELTA_ROTATION = Quat.fromVec3Degrees({ x: -5, y: 0, z: 0 }),
            MINI_EXPAND_HANDLES_OTHER = [ // Different handles when expanding after being grabbed by other hand,
                { x: 0.5, y: -0.4, z: 0 },
                { x: -0.5, y: -0.4, z: 0 }
            ],
            MINI_EXPAND_DELTA_ROTATION_OTHER = Quat.IDENTITY,
            miniExpandHand,
            miniExpandHandles = MINI_EXPAND_HANDLES,
            miniExpandDeltaRotation = MINI_EXPAND_HANDLES_OTHER,
            miniExpandLocalPosition,
            miniExpandLocalRotation = Quat.IDENTITY,
            miniInitialWidth,
            miniTargetWidth,
            miniTargetLocalRotation,

            // Laser pointing at.
            isBodyHovered = false,

            // EventBridge.
            READY_MESSAGE = "ready", // Engine <== Dialog
            HOVER_MESSAGE = "hover", // Engine <== Dialog
            UNHOVER_MESSAGE = "unhover", // Engine <== Dialog
            MUTE_MESSAGE = "mute", // Engine <=> Dialog
            GOTO_MESSAGE = "goto", // Engine <=> Dialog
            EXPAND_MESSAGE = "expand", // Engine <== Dialog

            // Sounds.
            HOVER_SOUND = "./assets/sounds/button-hover.wav",
            HOVER_VOLUME = 0.5,
            CLICK_SOUND = "./assets/sounds/button-click.wav",
            CLICK_VOLUME = 0.8,
            hoverSound = SoundCache.getSound(Script.resolvePath(HOVER_SOUND)),
            clickSound = SoundCache.getSound(Script.resolvePath(CLICK_SOUND));


        function updateMutedStatus() {
            var isMuted = Audio.muted;
            miniOverlayObject.emitScriptEvent(JSON.stringify({
                type: MUTE_MESSAGE,
                on: isMuted,
                icon: isMuted ? MUTE_ON_ICON : MUTE_OFF_ICON
            }));
        }

        function setGotoIcon() {
            miniOverlayObject.emitScriptEvent(JSON.stringify({
                type: GOTO_MESSAGE,
                icon: GOTO_ICON
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
                    setGotoIcon();
                    break;
                case HOVER_MESSAGE:
                    if (message.target === "body") {
                        // Laser status.
                        isBodyHovered = true;
                    } else if (message.target === "button") {
                        // Audio feedback.
                        playSound(hoverSound, HOVER_VOLUME);
                    }
                    break;
                case UNHOVER_MESSAGE:
                    if (message.target === "body") {
                        // Laser status.
                        isBodyHovered = false;
                    }
                    break;
                case MUTE_MESSAGE:
                    // Toggle mute.
                    playSound(clickSound, CLICK_VOLUME);
                    Audio.muted = !Audio.muted;
                    break;
                case GOTO_MESSAGE:
                    // Goto.
                    playSound(clickSound, CLICK_VOLUME);
                    miniState.setState(miniState.MINI_EXPANDING, { hand: uiHand, goto: true });
                    break;
                case EXPAND_MESSAGE:
                    // Expand tablet;
                    playSound(clickSound, CLICK_VOLUME);
                    miniState.setState(miniState.MINI_EXPANDING, { hand: uiHand, goto: false });
                    break;
            }
        }


        function updateMiniTabletID() {
            HMD.miniTabletID = miniOverlay;
            HMD.miniTabletScreenID = miniUIOverlay;
            HMD.miniTabletHand = miniOverlay ? uiHand : -1;
        }

        function playSound(sound, volume) {
            Audio.playSound(sound, {
                position: uiHand === LEFT_HAND ? MyAvatar.getLeftPalmPosition() : MyAvatar.getRightPalmPosition(),
                volume: volume,
                localOnly: true
            });
        }


        function getUIPositionAndRotation(hand) {
            return {
                position: MINI_POSITIONS[hand],
                rotation: MINI_ROTATIONS[hand]
            };
        }

        function getMiniTabletID() {
            return miniOverlay;
        }

        function getMiniTabletProperties() {
            var properties = Overlays.getProperties(miniOverlay, ["position", "orientation"]);
            return {
                position: properties.position,
                orientation: properties.orientation
            };
        }

        function isLaserPointingAt() {
            return isBodyHovered;
        }


        function show(hand) {
            var initialScale = 0.01; // Start very small.

            uiHand = hand;

            Overlays.editOverlay(miniOverlay, {
                parentID: MyAvatar.SELF_ID,
                parentJointIndex: handJointIndex(hand),
                localPosition: Vec3.multiply(MyAvatar.sensorToWorldScale, MINI_POSITIONS[hand]),
                localRotation: MINI_ROTATIONS[hand],
                dimensions: Vec3.multiply(initialScale, MINI_DIMENSIONS),
                grabbable: true,
                visible: true
            });
            Overlays.editOverlay(miniUIOverlay, {
                localPosition: Vec3.multiply(MyAvatar.sensorToWorldScale, MINI_UI_LOCAL_POSITION),
                localRotation: MINI_UI_LOCAL_ROTATION,
                dimensions: Vec3.multiply(initialScale, MINI_UI_DIMENSIONS),
                dpi: MINI_UI_DPI / initialScale,
                visible: true
            });

            updateMiniTabletID();

            if (!miniUIOverlayEnabled) {
                // Overlay content is created the first time it is visible to the user. The initial creation displays artefacts.
                // Delay showing UI overlay until after giving it time for its content to be created.
                Script.setTimeout(function () {
                    Overlays.editOverlay(miniUIOverlay, { alpha: 1.0 });
                }, MINI_UI_OVERLAY_ENABLED_DELAY);
            }
        }

        function size(scaleFactor) {
            // Scale UI in place.
            Overlays.editOverlay(miniOverlay, {
                dimensions: Vec3.multiply(scaleFactor, MINI_DIMENSIONS)
            });
            Overlays.editOverlay(miniUIOverlay, {
                localPosition: Vec3.multiply(scaleFactor, MINI_UI_LOCAL_POSITION),
                dimensions: Vec3.multiply(scaleFactor, MINI_UI_DIMENSIONS),
                dpi: MINI_UI_DPI / scaleFactor
            });
            updateRotation();
        }

        function startExpandingTablet(hand) {
            // Expansion details.
            if (hand === uiHand) {
                miniExpandHandles = MINI_EXPAND_HANDLES;
                miniExpandDeltaRotation = MINI_EXPAND_DELTA_ROTATION;
            } else {
                miniExpandHandles = MINI_EXPAND_HANDLES_OTHER;
                miniExpandDeltaRotation = MINI_EXPAND_DELTA_ROTATION_OTHER;
            }

            // Grab details.
            var properties = Overlays.getProperties(miniOverlay, ["localPosition", "localRotation"]);
            miniExpandHand = hand;
            miniExpandLocalRotation = properties.localRotation;
            miniExpandLocalPosition = Vec3.sum(properties.localPosition,
                Vec3.multiplyQbyV(miniExpandLocalRotation,
                    Vec3.multiplyVbyV(miniExpandHandles[miniExpandHand], MINI_DIMENSIONS)));

            // Start expanding.
            miniInitialWidth = MINI_DIMENSIONS.x;
            miniTargetWidth = getTabletWidthFromSettings();
            miniTargetLocalRotation = Quat.multiply(miniExpandLocalRotation, miniExpandDeltaRotation);
        }

        function sizeAboutHandles(scaleFactor) {
            // Scale UI and move per handles.
            var tabletScaleFactor,
                dimensions,
                localRotation,
                localPosition;

            tabletScaleFactor = MyAvatar.sensorToWorldScale
                * (1 + scaleFactor * (miniTargetWidth - miniInitialWidth) / miniInitialWidth);
            dimensions = Vec3.multiply(tabletScaleFactor, MINI_DIMENSIONS);
            localRotation = Quat.mix(miniExpandLocalRotation, miniTargetLocalRotation, scaleFactor);
            localPosition =
                Vec3.sum(miniExpandLocalPosition,
                    Vec3.multiplyQbyV(miniExpandLocalRotation,
                        Vec3.multiply(-tabletScaleFactor,
                            Vec3.multiplyVbyV(miniExpandHandles[miniExpandHand], MINI_DIMENSIONS)))
                );
            localPosition = Vec3.sum(localPosition,
                Vec3.multiplyQbyV(miniExpandLocalRotation, { x: 0, y: 0.5 * -dimensions.y, z: 0 }));
            localPosition = Vec3.sum(localPosition,
                Vec3.multiplyQbyV(localRotation, { x: 0, y: 0.5 * dimensions.y, z: 0 }));
            Overlays.editOverlay(miniOverlay, {
                localPosition: localPosition,
                localRotation: localRotation,
                dimensions: dimensions
            });
            // FIXME: Temporary code change to try not displaying UI when mini tablet is expanding to become the tablet proper.
            Overlays.editOverlay(miniUIOverlay, {
                /*
                localPosition: Vec3.multiply(tabletScaleFactor, MINI_UI_LOCAL_POSITION),
                dimensions: Vec3.multiply(tabletScaleFactor, MINI_UI_DIMENSIONS),
                dpi: MINI_UI_DPI / tabletScaleFactor
                */
                visible: false
            });
        }

        function updateRotation() {
            // Update the rotation of the tablet about its face normal so that its base is horizontal.
            var COS_5_DEGREES = 0.996,
                RADIANS_TO_DEGREES = DEGREES_180 / Math.PI,
                defaultLocalRotation,
                handOrientation,
                defaultOrientation,
                faceNormal,
                desiredOrientation,
                defaultYAxis,
                desiredYAxis,
                cross,
                dot,
                deltaAngle,
                deltaRotation,
                localRotation;

            if (Overlays.getProperty(miniOverlay, "parentJointIndex") !== handJointIndex(uiHand)) {
                // Overlay has been grabbed by other hand but this script hasn't received notification yet.
                return;
            }

            defaultLocalRotation = MINI_ROTATIONS[uiHand];
            handOrientation =
                Quat.multiply(MyAvatar.orientation, MyAvatar.getAbsoluteJointRotationInObjectFrame(handJointIndex(uiHand)));
            defaultOrientation = Quat.multiply(handOrientation, defaultLocalRotation);
            faceNormal = Vec3.multiplyQbyV(defaultOrientation, Vec3.UNIT_Z);

            if (Math.abs(Vec3.dot(faceNormal, Vec3.UNIT_Y)) > COS_5_DEGREES) {
                // Don't rotate mini tablet if almost flat in the x-z plane.
                return;
            } else {
                // Rotate the tablet so that its base is parallel with the x-z plane.
                desiredOrientation = Quat.lookAt(Vec3.ZERO, Vec3.multiplyQbyV(defaultOrientation, Vec3.UNIT_Z), Vec3.UNIT_Y);
                defaultYAxis = Vec3.multiplyQbyV(defaultOrientation, Vec3.UNIT_Y);
                desiredYAxis = Vec3.multiplyQbyV(desiredOrientation, Vec3.UNIT_Y);
                cross = Vec3.cross(defaultYAxis, desiredYAxis);
                dot = Vec3.dot(defaultYAxis, desiredYAxis);
                deltaAngle = Math.atan2(Vec3.length(cross), dot) * RADIANS_TO_DEGREES;
                if (Vec3.dot(cross, Vec3.multiplyQbyV(desiredOrientation, Vec3.UNIT_Z)) > 0) {
                    deltaAngle = -deltaAngle;
                }
                deltaRotation = Quat.angleAxis(deltaAngle, Vec3.multiplyQbyV(defaultLocalRotation, Vec3.UNIT_Z));
                localRotation = Quat.multiply(deltaRotation, defaultLocalRotation);
            }
            Overlays.editOverlay(miniOverlay, {
                localRotation: localRotation
            });
        }

        function release() {
            Overlays.editOverlay(miniOverlay, {
                parentID: Uuid.NULL, // Release hold so that hand can grab tablet proper.
                grabbable: false
            });
        }

        function hide() {
            Overlays.editOverlay(miniOverlay, {
                visible: false
            });
            Overlays.editOverlay(miniUIOverlay, {
                visible: false
            });
        }

        function create() {
            miniOverlay = Overlays.addOverlay("model", {
                url: MINI_MODEL,
                dimensions: Vec3.multiply(MyAvatar.sensorToWorldScale, MINI_DIMENSIONS),
                solid: true,
                grabbable: true,
                showKeyboardFocusHighlight: false,
                displayInFront: true,
                visible: false
            });
            miniUIOverlay = Overlays.addOverlay("web3d", {
                url: MINI_UI_HTML,
                parentID: miniOverlay,
                localPosition: Vec3.multiply(MyAvatar.sensorToWorldScale, MINI_UI_LOCAL_POSITION),
                localRotation: MINI_UI_LOCAL_ROTATION,
                dimensions: Vec3.multiply(MyAvatar.sensorToWorldScale, MINI_UI_DIMENSIONS),
                dpi: MINI_UI_DPI / MyAvatar.sensorToWorldScale,
                alpha: 0, // Hide overlay while its content is being created.
                grabbable: false,
                showKeyboardFocusHighlight: false,
                displayInFront: true,
                visible: false
            });

            miniUIOverlayEnabled = false; // This and alpha = 0 hides overlay while its content is being created.

            miniOverlayObject = Overlays.getOverlayObject(miniUIOverlay);
            miniOverlayObject.webEventReceived.connect(onWebEventReceived);
        }

        function destroy() {
            if (miniOverlayObject) {
                miniOverlayObject.webEventReceived.disconnect(onWebEventReceived);
                Overlays.deleteOverlay(miniUIOverlay);
                Overlays.deleteOverlay(miniOverlay);
                miniOverlayObject = null;
                miniUIOverlay = null;
                miniOverlay = null;
                updateMiniTabletID();
            }
        }

        create();

        return {
            getUIPositionAndRotation: getUIPositionAndRotation,
            getMiniTabletID: getMiniTabletID,
            getMiniTabletProperties: getMiniTabletProperties,
            isLaserPointingAt: isLaserPointingAt,
            updateMutedStatus: updateMutedStatus,
            show: show,
            size: size,
            startExpandingTablet: startExpandingTablet,
            sizeAboutHandles: sizeAboutHandles,
            updateRotation: updateRotation,
            release: release,
            hide: hide,
            destroy: destroy
        };

    };


    State = function () {

        if (!(this instanceof State)) {
            return new State();
        }

        var
            // States.
            MINI_DISABLED = 0,
            MINI_HIDDEN = 1,
            MINI_HIDING = 2,
            MINI_SHOWING = 3,
            MINI_VISIBLE = 4,
            MINI_EXPANDING = 5,
            TABLET_OPEN = 6,
            STATE_STRINGS = ["MINI_DISABLED", "MINI_HIDDEN", "MINI_HIDING", "MINI_SHOWING", "MINI_VISIBLE", "MINI_EXPANDING",
                "TABLET_OPEN"],
            STATE_MACHINE,
            miniState = MINI_DISABLED,
            miniHand,

            // Mini tablet scaling.
            MINI_SCALE_DURATION = 250,
            MINI_SCALE_TIMEOUT = 20,
            miniScaleTimer = null,
            miniScaleStart,

            // Expansion to tablet.
            MINI_EXPAND_DURATION = 250,
            MINI_EXPAND_TIMEOUT = 20,
            miniExpandTimer = null,
            miniExpandStart,
            miniExpandedStartTime,

            // Tablet targets.
            isGoto = false,
            TABLET_ADDRESS_DIALOG = "hifi/tablet/TabletAddressDialog.qml",

            // Trigger values.
            leftTriggerOn = 0,
            rightTriggerOn = 0,
            MAX_TRIGGER_ON_TIME = 100,

            // Visibility.
            MAX_HAND_CAMERA_ANGLE = 30,
            MAX_CAMERA_HAND_ANGLE = 30,
            DEGREES_180 = 180,
            MAX_HAND_CAMERA_ANGLE_COS = Math.cos(Math.PI * MAX_HAND_CAMERA_ANGLE / DEGREES_180),
            MAX_CAMERA_HAND_ANGLE_COS = Math.cos(Math.PI * MAX_CAMERA_HAND_ANGLE / DEGREES_180),
            HIDING_DELAY = 1000, // ms
            lastVisible = [0, 0];


        function enterMiniDisabled() {
            // Stop updates.
            Script.update.disconnect(updateState);

            // Stop monitoring mute changes.
            Audio.mutedChanged.disconnect(ui.updateMutedStatus);

            // Don't keep overlays prepared if in desktop mode.
            ui.destroy();
            ui = null;
        }

        function exitMiniDisabled() {
            // Create UI so that it's ready to be displayed without seeing artefacts from creating the UI.
            ui = new UI();

            // Start monitoring mute changes.
            Audio.mutedChanged.connect(ui.updateMutedStatus);

            // Start updates.
            Script.update.connect(updateState);
        }

        function shouldShowMini(hand) {
            // Should show mini tablet if it would be oriented toward the camera.
            var show,
                pose,
                isLeftTriggerOff,
                isRightTriggerOff,
                wasLeftTriggerOff = true,
                wasRightTriggerOff = true,
                jointIndex,
                handPosition,
                handOrientation,
                uiPositionAndOrientation,
                miniPosition,
                miniOrientation,
                miniToCameraDirection,
                cameraToHand;

            // Shouldn't show mini tablet if hand isn't being controlled.
            pose = Controller.getPoseValue(hand === LEFT_HAND ? Controller.Standard.LeftHand : Controller.Standard.RightHand);
            show = pose.valid;

            // Shouldn't show mini tablet on hand if that hand's trigger or grip are pressed (i.e., laser is searching or hand 
            // is grabbing something) or the other hand's trigger is pressed unless it is pointing at the mini tablet. Allow 
            // the triggers to be pressed briefly to allow for the grabbing process.
            if (show) {
                isLeftTriggerOff = Controller.getValue(Controller.Standard.LT) < TRIGGER_OFF_VALUE
                    && Controller.getValue(Controller.Standard.LeftGrip) < TRIGGER_OFF_VALUE;
                if (!isLeftTriggerOff) {
                    if (leftTriggerOn === 0) {
                        leftTriggerOn = Date.now();
                    } else {
                        wasLeftTriggerOff = Date.now() - leftTriggerOn < MAX_TRIGGER_ON_TIME;
                    }
                } else {
                    leftTriggerOn = 0;
                }
                isRightTriggerOff = Controller.getValue(Controller.Standard.RT) < TRIGGER_OFF_VALUE
                    && Controller.getValue(Controller.Standard.RightGrip) < TRIGGER_OFF_VALUE;
                if (!isRightTriggerOff) {
                    if (rightTriggerOn === 0) {
                        rightTriggerOn = Date.now();
                    } else {
                        wasRightTriggerOff = Date.now() - rightTriggerOn < MAX_TRIGGER_ON_TIME;
                    }
                } else {
                    rightTriggerOn = 0;
                }

                show = (hand === LEFT_HAND ? wasLeftTriggerOff : wasRightTriggerOff)
                    && ((hand === LEFT_HAND ? wasRightTriggerOff : wasLeftTriggerOff) || ui.isLaserPointingAt());
            }

            // Should show mini tablet if it would be oriented toward the camera.
            if (show) {
                // Calculate current visibility of mini tablet.
                jointIndex = handJointIndex(hand);
                handPosition = Vec3.sum(MyAvatar.position,
                    Vec3.multiplyQbyV(MyAvatar.orientation, MyAvatar.getAbsoluteJointTranslationInObjectFrame(jointIndex)));
                handOrientation =
                    Quat.multiply(MyAvatar.orientation, MyAvatar.getAbsoluteJointRotationInObjectFrame(jointIndex));
                uiPositionAndOrientation = ui.getUIPositionAndRotation(hand);
                miniPosition = Vec3.sum(handPosition, Vec3.multiply(MyAvatar.sensorToWorldScale,
                    Vec3.multiplyQbyV(handOrientation, uiPositionAndOrientation.position)));
                miniOrientation = Quat.multiply(handOrientation, uiPositionAndOrientation.rotation);
                miniToCameraDirection = Vec3.normalize(Vec3.subtract(Camera.position, miniPosition));
                show = Vec3.dot(miniToCameraDirection, Quat.getForward(miniOrientation)) > MAX_HAND_CAMERA_ANGLE_COS;
                show = show || (-Vec3.dot(miniToCameraDirection, Quat.getForward(handOrientation)) > MAX_HAND_CAMERA_ANGLE_COS);
                cameraToHand = -Vec3.dot(miniToCameraDirection, Quat.getForward(Camera.orientation));
                show = show && (cameraToHand > MAX_CAMERA_HAND_ANGLE_COS);

                // Persist showing for a while after it would otherwise be hidden.
                if (show) {
                    lastVisible[hand] = Date.now();
                } else {
                    show = Date.now() - lastVisible[hand] <= HIDING_DELAY;
                }
            }

            return {
                show: show,
                cameraToHand: cameraToHand
            };
        }

        function enterMiniHidden() {
            ui.release();
            ui.hide();
        }

        function updateMiniHidden() {
            var showLeft,
                showRight;

            // Don't show mini tablet if tablet proper is already displayed, in toolbar mode, or away.
            if (HMD.showTablet || tablet.toolbarMode || MyAvatar.isAway) {
                return;
            }

            // Show mini tablet if it would be pointing at the camera.
            showLeft = shouldShowMini(LEFT_HAND);
            showRight = shouldShowMini(RIGHT_HAND);
            if (showLeft.show && showRight.show) {
                // Both hands would be pointing at camera; show the one the camera is gazing at.
                if (showLeft.cameraToHand > showRight.cameraToHand) {
                    setState(MINI_SHOWING, LEFT_HAND);
                } else {
                    setState(MINI_SHOWING, RIGHT_HAND);
                }
            } else if (showLeft.show) {
                setState(MINI_SHOWING, LEFT_HAND);
            } else if (showRight.show) {
                setState(MINI_SHOWING, RIGHT_HAND);
            }
        }

        function scaleMiniDown() {
            var scaleFactor = (Date.now() - miniScaleStart) / MINI_SCALE_DURATION;
            if (scaleFactor < 1) {
                ui.size((1 - scaleFactor) * MyAvatar.sensorToWorldScale);
                miniScaleTimer = Script.setTimeout(scaleMiniDown, MINI_SCALE_TIMEOUT);
                return;
            }
            miniScaleTimer = null;
            setState(MINI_HIDDEN);
        }

        function enterMiniHiding() {
            miniScaleStart = Date.now();
            miniScaleTimer = Script.setTimeout(scaleMiniDown, MINI_SCALE_TIMEOUT);
        }

        function updateMiniHiding() {
            if (HMD.showTablet) {
                setState(MINI_HIDDEN);
            }
        }

        function exitMiniHiding() {
            if (miniScaleTimer) {
                Script.clearTimeout(miniScaleTimer);
                miniScaleTimer = null;
            }
        }

        function scaleMiniUp() {
            var scaleFactor = (Date.now() - miniScaleStart) / MINI_SCALE_DURATION;
            if (scaleFactor < 1) {
                ui.size(scaleFactor * MyAvatar.sensorToWorldScale);
                miniScaleTimer = Script.setTimeout(scaleMiniUp, MINI_SCALE_TIMEOUT);
                return;
            }
            miniScaleTimer = null;
            ui.size(MyAvatar.sensorToWorldScale);
            setState(MINI_VISIBLE);
        }

        function checkMiniVisibility() {
            var showLeft,
                showRight;

            // Check that the mini tablet should still be visible and if so then ensure it's on the hand that the camera is
            // gazing at.
            showLeft = shouldShowMini(LEFT_HAND);
            showRight = shouldShowMini(RIGHT_HAND);
            if (showLeft.show && showRight.show) {
                if (showLeft.cameraToHand > showRight.cameraToHand) {
                    if (miniHand !== LEFT_HAND) {
                        setState(MINI_HIDING);
                    }
                } else {
                    if (miniHand !== RIGHT_HAND) {
                        setState(MINI_HIDING);
                    }
                }
            } else if (showLeft.show) {
                if (miniHand !== LEFT_HAND) {
                    setState(MINI_HIDING);
                }
            } else if (showRight.show) {
                if (miniHand !== RIGHT_HAND) {
                    setState(MINI_HIDING);
                }
            } else {
                if (grabbedItem === null || grabbingHand !== miniHand) {
                    setState(MINI_HIDING);
                } else {
                    setState(MINI_HIDDEN);
                }
            }
        }

        function enterMiniShowing(hand) {
            miniHand = hand;
            ui.show(miniHand);
            miniScaleStart = Date.now();
            miniScaleTimer = Script.setTimeout(scaleMiniUp, MINI_SCALE_TIMEOUT);
        }

        function updateMiniShowing() {
            // Hide mini tablet if tablet proper has been displayed by other means.
            if (HMD.showTablet) {
                setState(MINI_HIDDEN);
            }

            // Hide mini tablet if it should no longer be visible.
            checkMiniVisibility();
        }

        function exitMiniShowing() {
            if (miniScaleTimer) {
                Script.clearTimeout(miniScaleTimer);
                miniScaleTimer = null;
            }
        }

        function updateMiniVisible() {
            // Hide mini tablet if tablet proper has been displayed by other means.
            if (HMD.showTablet) {
                setState(MINI_HIDDEN);
                return;
            }

            // Hide mini tablet if it should no longer be visible.
            checkMiniVisibility();

            // If state hasn't changed, update mini tablet rotation.
            if (miniState === MINI_VISIBLE) {
                ui.updateRotation();
            }
        }

        function expandMini() {
            var scaleFactor = (Date.now() - miniExpandStart) / MINI_EXPAND_DURATION;
            if (scaleFactor < 1) {
                ui.sizeAboutHandles(scaleFactor);
                miniExpandTimer = Script.setTimeout(expandMini, MINI_EXPAND_TIMEOUT);
                return;
            }
            miniExpandTimer = null;
            setState(TABLET_OPEN);
        }

        function enterMiniExpanding(data) {
            isGoto = data.goto;
            ui.startExpandingTablet(data.hand);
            miniExpandStart = Date.now();
            miniExpandTimer = Script.setTimeout(expandMini, MINI_EXPAND_TIMEOUT);
        }

        function updateMiniExanding() {
            // Hide mini tablet immediately if tablet proper has been displayed by other means.
            if (HMD.showTablet) {
                setState(MINI_HIDDEN);
            }
        }

        function exitMiniExpanding() {
            if (miniExpandTimer !== null) {
                Script.clearTimeout(miniExpandTimer);
                miniExpandTimer = null;
            }
        }

        function enterTabletOpen() {
            var miniTabletProperties;

            if (isGoto) {
                tablet.loadQMLSource(TABLET_ADDRESS_DIALOG);
            } else {
                tablet.gotoHomeScreen();
            }

            ui.release();
            miniTabletProperties = ui.getMiniTabletProperties();

            Overlays.editOverlay(HMD.tabletID, {
                position: miniTabletProperties.position,
                orientation: miniTabletProperties.orientation
            });

            HMD.openTablet(true);

            miniExpandedStartTime = Date.now();
        }

        function updateTabletOpen() {
            // Give the tablet proper time to rez before hiding expanded mini tablet.
            // The mini tablet is also hidden elsewhere if the tablet proper is grabbed.
            var TABLET_OPENING_DELAY = 500;
            if (Date.now() >= miniExpandedStartTime + TABLET_OPENING_DELAY) {
                setState(MINI_HIDDEN);
            }
        }

        STATE_MACHINE = {
            MINI_DISABLED: { // Mini tablet cannot be shown because in desktop mode.
                enter: enterMiniDisabled,
                update: null,
                exit: exitMiniDisabled
            },
            MINI_HIDDEN: { // Mini tablet could be shown but isn't because hand is oriented to show it or aren't in HMD mode.
                enter: enterMiniHidden,
                update: updateMiniHidden,
                exit: null
            },
            MINI_HIDING: { // Mini tablet is reducing from MINI_VISIBLE to MINI_HIDDEN.
                enter: enterMiniHiding,
                update: updateMiniHiding,
                exit: exitMiniHiding
            },
            MINI_SHOWING: { // Mini tablet is expanding from MINI_HIDDN to MINI_VISIBLE.
                enter: enterMiniShowing,
                update: updateMiniShowing,
                exit: exitMiniShowing
            },
            MINI_VISIBLE: { // Mini tablet is visible and attached to hand.
                enter: null,
                update: updateMiniVisible,
                exit: null
            },
            MINI_EXPANDING: { // Mini tablet is expanding before showing tablet proper.
                enter: enterMiniExpanding,
                update: updateMiniExanding,
                exit: exitMiniExpanding
            },
            TABLET_OPEN: { // Tablet proper is being displayed.
                enter: enterTabletOpen,
                update: updateTabletOpen,
                exit: null
            }
        };

        function setState(state, data) {
            if (state !== miniState) {
                debug("State transition from " + STATE_STRINGS[miniState] + " to " + STATE_STRINGS[state]
                    + ( data ? " " + JSON.stringify(data) : ""));
                if (STATE_MACHINE[STATE_STRINGS[miniState]].exit) {
                    STATE_MACHINE[STATE_STRINGS[miniState]].exit(data);
                }
                if (STATE_MACHINE[STATE_STRINGS[state]].enter) {
                    STATE_MACHINE[STATE_STRINGS[state]].enter(data);
                }
                miniState = state;
            } else {
                error("Null state transition: " + state + "!");
            }
        }

        function getState() {
            return miniState;
        }

        function getHand() {
            return miniHand;
        }

        function updateState() {
            if (STATE_MACHINE[STATE_STRINGS[miniState]].update) {
                STATE_MACHINE[STATE_STRINGS[miniState]].update();
            }
        }

        function create() {
            // Nothing to do.
        }

        function destroy() {
            if (miniState !== MINI_DISABLED) {
                setState(MINI_DISABLED);
            }
        }

        create();

        return {
            MINI_DISABLED: MINI_DISABLED,
            MINI_HIDDEN: MINI_HIDDEN,
            MINI_HIDING: MINI_HIDING,
            MINI_SHOWING: MINI_SHOWING,
            MINI_VISIBLE: MINI_VISIBLE,
            MINI_EXPANDING: MINI_EXPANDING,
            TABLET_OPEN: TABLET_OPEN,
            setState: setState,
            getState: getState,
            getHand: getHand,
            destroy: destroy
        };
    };


    function onMessageReceived(channel, data, senderID, localOnly) {
        var message,
            miniHand,
            hand;

        if (channel !== HIFI_OBJECT_MANIPULATION_CHANNEL) {
            return;
        }

        try {
            message = JSON.parse(data);
        } catch (e) {
            return;
        }

        // Track grabbed state and item.
        switch (message.action) {
            case "grab":
                grabbingHand = HAND_NAMES.indexOf(message.joint);
                grabbedItem = message.grabbedEntity;
                break;
            case "release":
                grabbingHand = NO_HAND;
                grabbedItem = null;
                break;
            default:
                error("Unexpected grab message!");
                return;
        }

        if (message.grabbedEntity !== HMD.tabletID && message.grabbedEntity !== ui.getMiniTabletID()) {
            return;
        }

        if (message.action === "grab" && message.grabbedEntity === HMD.tabletID && HMD.active) {
            // Tablet may have been grabbed after it replaced expanded mini tablet.
            miniState.setState(miniState.MINI_HIDDEN);
        } else if (message.action === "grab" && miniState.getState() === miniState.MINI_VISIBLE) {
            miniHand = miniState.getHand();
            hand = message.joint === HAND_NAMES[miniHand] ? miniHand : otherHand(miniHand);
            miniState.setState(miniState.MINI_EXPANDING, { hand: hand, goto: false });
        }
    }

    function onWentAway() {
        // Mini tablet only available when user is not away.
        if (HMD.active) {
            miniState.setState(miniState.MINI_HIDDEN);
        }
    }

    function onDisplayModeChanged() {
        // Mini tablet only available when HMD is active.
        if (HMD.active) {
            miniState.setState(miniState.MINI_HIDDEN);
        } else {
            miniState.setState(miniState.MINI_DISABLED);
        }
    }


    function setUp() {
        miniState = new State();

        Messages.subscribe(HIFI_OBJECT_MANIPULATION_CHANNEL);
        Messages.messageReceived.connect(onMessageReceived);

        MyAvatar.wentAway.connect(onWentAway);
        HMD.displayModeChanged.connect(onDisplayModeChanged);
        if (HMD.active) {
            miniState.setState(miniState.MINI_HIDDEN);
        }
    }

    function tearDown() {
        miniState.setState(miniState.MINI_DISABLED);

        HMD.displayModeChanged.disconnect(onDisplayModeChanged);
        MyAvatar.wentAway.disconnect(onWentAway);

        Messages.messageReceived.disconnect(onMessageReceived);
        Messages.unsubscribe(HIFI_OBJECT_MANIPULATION_CHANNEL);

        miniState.destroy();
        miniState = null;
    }

    setUp();
    Script.scriptEnding.connect(tearDown);

}());
