//
//  fingerPaint.js
//
//  Created by David Rowe on 15 Feb 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    var tablet,
        button,
        BUTTON_NAME = "PAINT",
        isFingerPainting = false,
        leftHand,
        rightHand,
        leftBrush,
        rightBrush,
        CONTROLLER_MAPPING_NAME = "com.highfidelity.fingerPaint",
        isTabletDisplayed = false,
        HIFI_POINT_INDEX_MESSAGE_CHANNEL = "Hifi-Point-Index",
        HIFI_GRAB_DISABLE_MESSAGE_CHANNEL = "Hifi-Grab-Disable";

    function paintBrush(name) {
        // Paints in 3D.

        var brushName = name,
            BRUSH_COLOR = { red: 250, green: 0, blue: 0 },
            ERASE_SEARCH_DISTANCE = 0.1;  // m

        function startLine(position, width) {
        }

        function drawLine(position, width) {
        }

        function finishLine(position, width) {
        }

        function cancelLine() {
        }

        function eraseClosestLine(position) {
        }

        function tearDown() {
        }

        return {
            startLine: startLine,
            drawLine: drawLine,
            finishLine: finishLine,
            cancelLine: cancelLine,
            eraseClosestLine: eraseClosestLine,
            tearDown: tearDown
        };
    }

    function handController(name) {
        // Translates controller data into application events.
        var handName = name,
            isEnabled = false,

            triggerPressedCallback,
            triggerPressingCallback,
            triggerReleasedCallback,
            gripPressedCallback,

            triggerValue = 0.0,
            isTriggerPressed = false,
            TRIGGER_SMOOTH_RATIO = 0.1,
            TRIGGER_OFF = 0.05,
            TRIGGER_ON = 0.1,
            TRIGGER_START_WIDTH = 0.15,
            TRIGGER_FINISH_WIDTH = 1.0,
            TRIGGER_RANGE_WIDTH = TRIGGER_FINISH_WIDTH - TRIGGER_START_WIDTH,
            START_LINE_WIDTH = 0.005,
            FINISH_LINE_WIDTH = 0.010,
            RANGE_LINE_WIDTH = FINISH_LINE_WIDTH - START_LINE_WIDTH,

            gripValue = 0.0,
            isGripPressed = false,
            GRIP_SMOOTH_RATIO = 0.1,
            GRIP_OFF = 0.05,
            GRIP_ON = 0.1;

        function setEnabled(enabled) {
            isEnabled = enabled;
        }

        function onTriggerPress(value) {
            var wasTriggerPressed,
                fingerTipPosition,
                lineWidth;

            if (!isEnabled) {
                return;
            }

            triggerValue = triggerValue * TRIGGER_SMOOTH_RATIO + value * (1.0 - TRIGGER_SMOOTH_RATIO);

            wasTriggerPressed = isTriggerPressed;
            if (isTriggerPressed) {
                isTriggerPressed = triggerValue > TRIGGER_OFF;
            } else {
                isTriggerPressed = triggerValue > TRIGGER_ON;
            }

            if (wasTriggerPressed || isTriggerPressed) {
                fingerTipPosition = MyAvatar.getJointPosition(handName === "left" ? "LeftHandIndex3" : "RightHandIndex3");
                if (triggerValue < TRIGGER_START_WIDTH) {
                    lineWidth = START_LINE_WIDTH;
                } else {
                    lineWidth = START_LINE_WIDTH
                        + (triggerValue - TRIGGER_START_WIDTH) / TRIGGER_RANGE_WIDTH * RANGE_LINE_WIDTH;
                }

                if (!wasTriggerPressed && isTriggerPressed) {
                    triggerPressedCallback(fingerTipPosition, lineWidth);
                } else if (wasTriggerPressed && isTriggerPressed) {
                    triggerPressingCallback(fingerTipPosition, lineWidth);
                } else {
                    triggerReleasedCallback(fingerTipPosition, lineWidth);
                }
            }
        }

        function onGripPress(value) {
            var fingerTipPosition;

            if (!isEnabled) {
                return;
            }

            gripValue = gripValue * GRIP_SMOOTH_RATIO + value * (1.0 - GRIP_SMOOTH_RATIO);

            if (isGripPressed) {
                isGripPressed = gripValue > GRIP_OFF;
            } else {
                isGripPressed = gripValue > GRIP_ON;
                if (isGripPressed) {
                    fingerTipPosition = MyAvatar.getJointPosition(handName === "left" ? "LeftHandIndex3" : "RightHandIndex3");
                    gripPressedCallback(fingerTipPosition);
                }
            }
        }

        function setUp(onTriggerPressed, onTriggerPressing, onTriggerReleased, onGripPressed) {
            triggerPressedCallback = onTriggerPressed;
            triggerPressingCallback = onTriggerPressing;
            triggerReleasedCallback = onTriggerReleased;
            gripPressedCallback = onGripPressed;
        }

        function tearDown() {
            // Nothing to do.
        }

        return {
            setEnabled: setEnabled,
            onTriggerPress: onTriggerPress,
            onGripPress: onGripPress,
            setUp: setUp,
            tearDown: tearDown
        };
    }

    function updateHandFunctions() {
        // Update other scripts' hand functions.
        var enabled = !isFingerPainting || isTabletDisplayed;

        Messages.sendMessage(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL, JSON.stringify({
            holdEnabled: enabled,
            nearGrabEnabled: enabled,
            farGrabEnabled: enabled
        }));
        Messages.sendMessage(HIFI_POINT_INDEX_MESSAGE_CHANNEL, JSON.stringify({
            pointIndex: !enabled
        }));
    }

    function onButtonClicked() {
        var wasFingerPainting = isFingerPainting;

        isFingerPainting = !isFingerPainting;
        button.editProperties({ isActive: isFingerPainting });

        leftHand.setEnabled(isFingerPainting);
        rightHand.setEnabled(isFingerPainting);

        if (wasFingerPainting) {
            leftBrush.cancelLine();
            rightBrush.cancelLine();
        }

        updateHandFunctions();
    }

    function onTabletScreenChanged(type, url) {
        var TABLET_SCREEN_CLOSED = "Closed";

        isTabletDisplayed = type !== TABLET_SCREEN_CLOSED;
        updateHandFunctions();
    }

    function setUp() {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (!tablet) {
            return;
        }

        // Tablet button.
        button = tablet.addButton({
            icon: "icons/tablet-icons/bubble-i.svg",
            activeIcon: "icons/tablet-icons/bubble-a.svg",
            text: BUTTON_NAME,
            isActive: isFingerPainting
        });
        button.clicked.connect(onButtonClicked);

        // Track whether tablet is displayed or not.
        tablet.screenChanged.connect(onTabletScreenChanged);

        // Connect controller API to handController objects.
        leftHand = handController("left");
        rightHand = handController("right");
        var controllerMapping = Controller.newMapping(CONTROLLER_MAPPING_NAME);
        controllerMapping.from(Controller.Standard.LT).to(leftHand.onTriggerPress);
        controllerMapping.from(Controller.Standard.LeftGrip).to(leftHand.onGripPress);
        controllerMapping.from(Controller.Standard.RT).to(rightHand.onTriggerPress);
        controllerMapping.from(Controller.Standard.RightGrip).to(rightHand.onGripPress);
        Controller.enableMapping(CONTROLLER_MAPPING_NAME);

        // Connect handController outputs to paintBrush objects.
        leftBrush = paintBrush("left");
        leftHand.setUp(leftBrush.startLine, leftBrush.drawLine, leftBrush.finishLine, leftBrush.eraseClosestLine);
        rightBrush = paintBrush("right");
        rightHand.setUp(rightBrush.startLine, rightBrush.drawLine, rightBrush.finishLine, rightBrush.eraseClosestLine);

        // Messages channels for enabling/disabling other scripts' functions.
        Messages.subscribe(HIFI_POINT_INDEX_MESSAGE_CHANNEL);
        Messages.subscribe(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL);
    }

    function tearDown() {
        if (!tablet) {
            return;
        }

        button.clicked.disconnect(onButtonClicked);
        tablet.removeButton(button);

        tablet.screenChanged.disconnect(onTabletScreenChanged);

        Controller.disableMapping(CONTROLLER_MAPPING_NAME);

        leftBrush.tearDown();
        leftBrush = null;
        leftHand.tearDown();
        leftHand = null;

        rightBrush.tearDown();
        rightBrush = null;
        rightHand.tearDown();
        rightHand = null;

        Messages.unsubscribe(HIFI_POINT_INDEX_MESSAGE_CHANNEL);
        Messages.unsubscribe(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());