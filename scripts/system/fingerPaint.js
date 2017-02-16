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
            triggerPressed,     // Callback.
            triggerPressing,    // ""
            triggerReleased,    // ""
            gripPressed,        // ""
            isEnabled = false;

        function setEnabled(enabled) {
            isEnabled = enabled;
        }

        function onTriggerPress(value) {
            if (!isEnabled) {
                return;
            }
        }

        function onGripPress(value) {
            if (!isEnabled) {
                return;
            }
        }

        function tearDown() {
        }

        return {
            setEnabled: setEnabled,
            onTriggerPress: onTriggerPress,
            onGripPress: onGripPress,
            triggerPressed: triggerPressed,
            triggerPressing: triggerPressing,
            triggerReleased: triggerReleased,
            gripPressed: gripPressed,
            tearDown: tearDown
        };
    }

    function updateHandControllerGrab() {
        // Send message to handControllerGrab.js to handle.
        var enabled = !isFingerPainting || isTabletDisplayed;

        Messages.sendMessage(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL, JSON.stringify({
            holdEnabled: enabled,
            nearGrabEnabled: enabled,
            farGrabEnabled: enabled
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

        updateHandControllerGrab();
    }

    function onTabletScreenChanged(type, url) {
        var TABLET_SCREEN_CLOSED = "Closed";

        isTabletDisplayed = type !== TABLET_SCREEN_CLOSED;
        updateHandControllerGrab();
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
        leftHand.triggerPressed = leftBrush.startLine;
        leftHand.triggerPressing = leftBrush.drawLine;
        leftHand.trigerRelease = leftBrush.finishLine;
        leftHand.gripPressed = leftBrush.eraseLine;
        rightBrush = paintBrush("right");
        rightHand.triggerPressed = rightBrush.startLine;
        rightHand.triggerPressing = rightBrush.drawLine;
        rightHand.trigerRelease = rightBrush.finishLine;
        rightHand.gripPressed = rightBrush.eraseLine;

        // Messages channel for disabling/enabling laser pointers and grabbing.
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

        Messages.unsubscribe(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());