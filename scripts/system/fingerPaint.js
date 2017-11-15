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
        shouldPointFingers = false,
        leftHand = null,
        rightHand = null,
        leftBrush = null,
        rightBrush = null,
        CONTROLLER_MAPPING_NAME = "com.highfidelity.fingerPaint",
        isTabletDisplayed = false,
        HIFI_POINT_INDEX_MESSAGE_CHANNEL = "Hifi-Point-Index",
        HIFI_GRAB_DISABLE_MESSAGE_CHANNEL = "Hifi-Grab-Disable",
        HIFI_POINTER_DISABLE_MESSAGE_CHANNEL = "Hifi-Pointer-Disable";
        HOW_TO_EXIT_MESSAGE = "Press B on your controller to exit FingerPainting mode";

    function paintBrush(name) {
        // Paints in 3D.
        var brushName = name,
            STROKE_COLOR = { red: 250, green: 0, blue: 0 },
            ERASE_SEARCH_RADIUS = 0.1,  // m
            isDrawingLine = false,
            entityID,
            basePosition,
            strokePoints,
            strokeNormals,
            strokeWidths,
            timeOfLastPoint,
            MIN_STROKE_LENGTH = 0.005,  // m
            MIN_STROKE_INTERVAL = 66,  // ms
            MAX_POINTS_PER_LINE = 70;  // Hard-coded limit in PolyLineEntityItem.h.

        function strokeNormal() {
            return Vec3.multiplyQbyV(Camera.getOrientation(), Vec3.UNIT_NEG_Z);
        }

        function startLine(position, width) {
            // Start drawing a polyline.

            if (isDrawingLine) {
                print("ERROR: startLine() called when already drawing line");
                // Nevertheless, continue on and start a new line.
            }

            basePosition = position;

            strokePoints = [Vec3.ZERO];
            strokeNormals = [strokeNormal()];
            strokeWidths = [width];
            timeOfLastPoint = Date.now();

            entityID = Entities.addEntity({
                type: "PolyLine",
                name: "fingerPainting",
                color: STROKE_COLOR,
                position: position,
                linePoints: strokePoints,
                normals: strokeNormals,
                strokeWidths: strokeWidths,
                dimensions: { x: 10, y: 10, z: 10 }
            });

            isDrawingLine = true;
        }

        function drawLine(position, width) {
            // Add a stroke to the polyline if stroke is a sufficient length.
            var localPosition,
                distanceToPrevious,
                MAX_DISTANCE_TO_PREVIOUS = 1.0;

            if (!isDrawingLine) {
                print("ERROR: drawLine() called when not drawing line");
                return;
            }

            localPosition = Vec3.subtract(position, basePosition);
            distanceToPrevious = Vec3.distance(localPosition, strokePoints[strokePoints.length - 1]);

            if (distanceToPrevious > MAX_DISTANCE_TO_PREVIOUS) {
                // Ignore occasional spurious finger tip positions.
                return;
            }

            if (distanceToPrevious >= MIN_STROKE_LENGTH
                    && (Date.now() - timeOfLastPoint) >= MIN_STROKE_INTERVAL
                    && strokePoints.length < MAX_POINTS_PER_LINE) {
                strokePoints.push(localPosition);
                strokeNormals.push(strokeNormal());
                strokeWidths.push(width);
                timeOfLastPoint = Date.now();

                Entities.editEntity(entityID, {
                    linePoints: strokePoints,
                    normals: strokeNormals,
                    strokeWidths: strokeWidths
                });
            }
        }

        function finishLine(position, width) {
            // Finish drawing polyline; delete if it has only 1 point.

            if (!isDrawingLine) {
                print("ERROR: finishLine() called when not drawing line");
                return;
            }

            if (strokePoints.length === 1) {
                // Delete "empty" line.
                Entities.deleteEntity(entityID);
            }

            isDrawingLine = false;
        }

        function cancelLine() {
            // Cancel any line being drawn.
            if (isDrawingLine) {
                Entities.deleteEntity(entityID);
                isDrawingLine = false;
            }
        }

        function eraseClosestLine(position) {
            // Erase closest line that is within search radius of finger tip.
            var entities,
                entitiesLength,
                properties,
                i,
                pointsLength,
                j,
                distance,
                found = false,
                foundID,
                foundDistance = ERASE_SEARCH_RADIUS;

            // Find entities with bounding box within search radius.
            entities = Entities.findEntities(position, ERASE_SEARCH_RADIUS);

            // Fine polyline entity with closest point within search radius.
            for (i = 0, entitiesLength = entities.length; i < entitiesLength; i += 1) {
                properties = Entities.getEntityProperties(entities[i], ["type", "position", "linePoints"]);
                if (properties.type === "PolyLine") {
                    basePosition = properties.position;
                    for (j = 0, pointsLength = properties.linePoints.length; j < pointsLength; j += 1) {
                        distance = Vec3.distance(position, Vec3.sum(basePosition, properties.linePoints[j]));
                        if (distance <= foundDistance) {
                            found = true;
                            foundID = entities[i];
                            foundDistance = distance;
                        }
                    }
                }
            }

            // Delete found entity.
            if (found) {
                Entities.deleteEntity(foundID);
            }
        }

        function tearDown() {
            cancelLine();
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

            triggerPressedCallback,
            triggerPressingCallback,
            triggerReleasedCallback,
            gripPressedCallback,

            rawTriggerValue = 0.0,
            triggerValue = 0.0,
            isTriggerPressed = false,
            TRIGGER_SMOOTH_RATIO = 0.1,
            TRIGGER_OFF = 0.05,
            TRIGGER_ON = 0.1,
            TRIGGER_START_WIDTH_RAMP = 0.15,
            TRIGGER_FINISH_WIDTH_RAMP = 1.0,
            TRIGGER_RAMP_WIDTH = TRIGGER_FINISH_WIDTH_RAMP - TRIGGER_START_WIDTH_RAMP,
            MIN_LINE_WIDTH = 0.005,
            MAX_LINE_WIDTH = 0.03,
            RAMP_LINE_WIDTH = MAX_LINE_WIDTH - MIN_LINE_WIDTH,

            rawGripValue = 0.0,
            gripValue = 0.0,
            isGripPressed = false,
            GRIP_SMOOTH_RATIO = 0.1,
            GRIP_OFF = 0.05,
            GRIP_ON = 0.1;

        function onTriggerPress(value) {
            // Controller values are only updated when they change so store latest for use in update.
            rawTriggerValue = value;
        }

        function updateTriggerPress(value) {
            var wasTriggerPressed,
                fingerTipPosition,
                lineWidth;

            triggerValue = triggerValue * TRIGGER_SMOOTH_RATIO + rawTriggerValue * (1.0 - TRIGGER_SMOOTH_RATIO);

            wasTriggerPressed = isTriggerPressed;
            if (isTriggerPressed) {
                isTriggerPressed = triggerValue > TRIGGER_OFF;
            } else {
                isTriggerPressed = triggerValue > TRIGGER_ON;
            }

            if (wasTriggerPressed || isTriggerPressed) {
                fingerTipPosition = MyAvatar.getJointPosition(handName === "left" ? "LeftHandIndex4" : "RightHandIndex4");
                if (triggerValue < TRIGGER_START_WIDTH_RAMP) {
                    lineWidth = MIN_LINE_WIDTH;
                } else {
                    lineWidth = MIN_LINE_WIDTH
                        + (triggerValue - TRIGGER_START_WIDTH_RAMP) / TRIGGER_RAMP_WIDTH * RAMP_LINE_WIDTH;
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
            // Controller values are only updated when they change so store latest for use in update.
            rawGripValue = value;
        }

        function updateGripPress() {
            var fingerTipPosition;

            gripValue = gripValue * GRIP_SMOOTH_RATIO + rawGripValue * (1.0 - GRIP_SMOOTH_RATIO);

            if (isGripPressed) {
                isGripPressed = gripValue > GRIP_OFF;
            } else {
                isGripPressed = gripValue > GRIP_ON;
                if (isGripPressed) {
                    fingerTipPosition = MyAvatar.getJointPosition(handName === "left" ? "LeftHandIndex4" : "RightHandIndex4");
                    gripPressedCallback(fingerTipPosition);
                }
            }
        }

        function onUpdate() {
            updateTriggerPress();
            updateGripPress();
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
            onTriggerPress: onTriggerPress,
            onGripPress: onGripPress,
            onUpdate: onUpdate,
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
        }), true);
        Messages.sendMessage(HIFI_POINTER_DISABLE_MESSAGE_CHANNEL, JSON.stringify({
            pointerEnabled: enabled
        }), true);

        var newShouldPointFingers = !enabled;
        if (newShouldPointFingers !== shouldPointFingers) {
            Messages.sendMessage(HIFI_POINT_INDEX_MESSAGE_CHANNEL, JSON.stringify({
                pointIndex: newShouldPointFingers
            }), true);
            shouldPointFingers = newShouldPointFingers;
        }
    }

    function howToExitTutorial() {
        HMD.requestShowHandControllers();
        setControllerPartLayer('button_b', 'highlight');
        messageWindow = Window.alert(HOW_TO_EXIT_MESSAGE);
        setControllerPartLayer('button_b', 'blank');
        HMD.requestHideHandControllers();
        Settings.setValue("FingerPaintTutorialComplete", true);
    }

    function enableProcessing() {
        // Connect controller API to handController objects.
        leftHand = handController("left");
        rightHand = handController("right");
        var controllerMapping = Controller.newMapping(CONTROLLER_MAPPING_NAME);
        controllerMapping.from(Controller.Standard.LT).to(leftHand.onTriggerPress);
        controllerMapping.from(Controller.Standard.LeftGrip).to(leftHand.onGripPress);
        controllerMapping.from(Controller.Standard.RT).to(rightHand.onTriggerPress);
        controllerMapping.from(Controller.Standard.RightGrip).to(rightHand.onGripPress);
        controllerMapping.from(Controller.Standard.B).to(onButtonClicked);
        Controller.enableMapping(CONTROLLER_MAPPING_NAME);
        
        if (!Settings.getValue("FingerPaintTutorialComplete")) {
            howToExitTutorial();
        }

        // Connect handController outputs to paintBrush objects.
        leftBrush = paintBrush("left");
        leftHand.setUp(leftBrush.startLine, leftBrush.drawLine, leftBrush.finishLine, leftBrush.eraseClosestLine);
        rightBrush = paintBrush("right");
        rightHand.setUp(rightBrush.startLine, rightBrush.drawLine, rightBrush.finishLine, rightBrush.eraseClosestLine);

        // Messages channels for enabling/disabling other scripts' functions.
        Messages.subscribe(HIFI_POINT_INDEX_MESSAGE_CHANNEL);
        Messages.subscribe(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL);
        Messages.subscribe(HIFI_POINTER_DISABLE_MESSAGE_CHANNEL);

        // Update hand controls.
        Script.update.connect(leftHand.onUpdate);
        Script.update.connect(rightHand.onUpdate);
    }

    function disableProcessing() {
        Script.update.disconnect(leftHand.onUpdate);
        Script.update.disconnect(rightHand.onUpdate);

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
        Messages.unsubscribe(HIFI_POINTER_DISABLE_MESSAGE_CHANNEL);
    }

    function onButtonClicked() {
        var wasFingerPainting = isFingerPainting;

        isFingerPainting = !isFingerPainting;
        button.editProperties({ isActive: isFingerPainting });

        print("Finger painting: " + isFingerPainting ? "on" : "off");

        if (wasFingerPainting) {
            leftBrush.cancelLine();
            rightBrush.cancelLine();
        }

        if (isFingerPainting) {
            enableProcessing();
        }

        updateHandFunctions();

        if (!isFingerPainting) {
            disableProcessing();
        }
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
            icon: "icons/tablet-icons/finger-paint-i.svg",
            activeIcon: "icons/tablet-icons/finger-paint-a.svg",
            text: BUTTON_NAME,
            isActive: isFingerPainting
        });
        button.clicked.connect(onButtonClicked);

        // Track whether tablet is displayed or not.
        tablet.screenChanged.connect(onTabletScreenChanged);
    }

    function tearDown() {
        if (!tablet) {
            return;
        }

        if (isFingerPainting) {
            isFingerPainting = false;
            updateHandFunctions();
            disableProcessing();
        }

        tablet.screenChanged.disconnect(onTabletScreenChanged);

        button.clicked.disconnect(onButtonClicked);
        tablet.removeButton(button);
    }
    
    /**
     * A controller is made up of parts, and each part can have multiple "layers,"
     * which are really just different texures. For example, the "trigger" part
     * has "normal" and "highlight" layers.
     */
    function setControllerPartLayer(part, layer) {
        data = {};
        data[part] = layer;
        Messages.sendLocalMessage('Controller-Set-Part-Layer', JSON.stringify(data));
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
