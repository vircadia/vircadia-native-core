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
        //undo vars
        UNDO_STACK_SIZE = 5, 
        undoStack = [];
        isFingerPainting = false,
        isTabletFocused = false, //starts with true cause you need to be hovering the tablet in order to open the app
        tabletDebugFocusLine = null,
        //animated brush vars
        lastFrameTime = Date.now(),
        frameDeltaSeconds = null, //time that passed between frames in ms;
        //end of dynamic brush vars
        leftHand = null,
        rightHand = null,
        leftBrush = null,
        rightBrush = null,
        isBrushColored = false,
        isLeftHandDominant = false,
        savedSettings = null,
        CONTROLLER_MAPPING_NAME = "com.highfidelity.fingerPaint",
        isTabletDisplayed = false,
        HIFI_POINT_INDEX_MESSAGE_CHANNEL = "Hifi-Point-Index",
        HIFI_GRAB_DISABLE_MESSAGE_CHANNEL = "Hifi-Grab-Disable",
        HIFI_POINTER_DISABLE_MESSAGE_CHANNEL = "Hifi-Pointer-Disable",
        SCRIPT_PATH = Script.resolvePath(''),
        CONTENT_PATH = SCRIPT_PATH.substr(0, SCRIPT_PATH.lastIndexOf('/')),
        ANIMATION_SCRIPT_PATH = Script.resolvePath("content/brushes/dynamicBrushes/dynamicBrushScript.js"),
        APP_URL = CONTENT_PATH + "/html/main.html";

    // Set up the qml ui
    //var qml = Script.resolvePath('PaintWindow.qml');
    Script.include("../libraries/controllers.js");
    Script.include("content/brushes/dynamicBrushes/dynamicBrushesList.js");
    //var window = null;

    //var inkSource = null;
	var inkSourceOverlay = null;
	// Set path for finger paint hand animations 
	var RIGHT_ANIM_URL = Script.resourcesPath() + 'avatar/animations/touch_point_closed_right.fbx';
    var LEFT_ANIM_URL = Script.resourcesPath() + 'avatar/animations/touch_point_closed_left.fbx';
	var RIGHT_ANIM_URL_OPEN = Script.resourcesPath() + 'avatar/animations/touch_point_open_right.fbx';
    var LEFT_ANIM_URL_OPEN = Script.resourcesPath() + 'avatar/animations/touch_point_open_left.fbx'; 

    function paintBrush(name) {
        // Paints in 3D.
        var brushName = name,
            STROKE_COLOR = savedSettings.currentColor,
            ERASE_SEARCH_RADIUS = 0.1,  // m
            STROKE_DIMENSIONS = { x: 10, y: 10, z: 10 },
            isDrawingLine = false,
            entityID,
            basePosition,
            strokePoints,
            strokeNormals,
            strokeWidths,
            timeOfLastPoint,
            texture = savedSettings.currentTexture,
            //'https://upload.wikimedia.org/wikipedia/commons/thumb/9/93/Caris_Tessellation.svg/1024px-Caris_Tessellation.svg.png', // Daantje
            strokeWidthMultiplier = savedSettings.currentStrokeWidth,
			IS_UV_MODE_STRETCH = true,
            MIN_STROKE_LENGTH = 0.005,  // m
            MIN_STROKE_INTERVAL = 66,  // ms
            MAX_POINTS_PER_LINE = 70;  // Hard-coded limit in PolyLineEntityItem.h.
                
        function strokeNormal() {
            return Vec3.multiplyQbyV(Camera.getOrientation(), Vec3.UNIT_NEG_Z);
        }
        
        function changeStrokeColor(red, green, blue) {
            STROKE_COLOR.red = red;
            STROKE_COLOR.green = green;
            STROKE_COLOR.blue = blue;
        }
        
        function getStrokeColor() {
            return STROKE_COLOR;
        }

        function isDrawing() {
            return isDrawingLine;
        }
        
        function changeStrokeWidthMultiplier(multiplier) {
            strokeWidthMultiplier = multiplier;
        }
		
		function changeUVMode(isUVModeStretch) {
            IS_UV_MODE_STRETCH = isUVModeStretch;
        }
        
        function getStrokeWidth() {
            return strokeWidthMultiplier;
        }

        function getEntityID() {
            return entityID;
        }
		        
        function changeTexture(textureURL) {
            texture = textureURL;
        }
        
        function undoErasing() {
            var undo = undoStack.pop();
            if (undo) {
                Entities.addEntity(undo);
            }
        }

        function startLine(position, width) {
            // Start drawing a polyline.
            if (isTabletFocused)
                return;

            width = width * strokeWidthMultiplier;
            
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
                textures: texture, // Daantje
				isUVModeStretch: IS_UV_MODE_STRETCH,
                dimensions: STROKE_DIMENSIONS,
                shapeType: "box",
                collisionless: true,
            });
            isDrawingLine = true;
            addAnimationToBrush(entityID);
        }

        function drawLine(position, width) {
            // Add a stroke to the polyline if stroke is a sufficient length.
            var localPosition,
                distanceToPrevious,
                MAX_DISTANCE_TO_PREVIOUS = 1.0;

            width = width * strokeWidthMultiplier;    
                
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
                    color: STROKE_COLOR,
                    linePoints: strokePoints,
                    normals: strokeNormals,
                    strokeWidths: strokeWidths
                });
            }
        }

        function finishLine(position, width) {
            // Finish drawing polyline; delete if it has only 1 point.
            //stopDynamicBrush = true;
            print("Before adding script: " + JSON.stringify(Entities.getEntityProperties(entityID)));
            var userData = Entities.getEntityProperties(entityID).userData;
            if (userData && JSON.parse(userData).animations) {
                Entities.editEntity(entityID, {
                    script: ANIMATION_SCRIPT_PATH,
                });    
            }
            print("After adding script: " + JSON.stringify(Entities.getEntityProperties(entityID)));
            //setIsDrawingFingerPaint(entityID, false);
            print("already stopped drawing");
            width = width * strokeWidthMultiplier;
            
            if (!isDrawingLine) {
                print("ERROR: finishLine() called when not drawing line");
                return;
            }

            if (strokePoints.length === 1) {
                // Delete "empty" line.
                Entities.deleteEntity(entityID);
            }

            isDrawingLine = false;
            print("After adding script 3: " + JSON.stringify(Entities.getEntityProperties(entityID)));
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
                addElementToUndoStack(Entities.getEntityProperties(foundID));
                Entities.deleteEntity(foundID);
                /*Entities.editEntity(entityID, {
                    color: «,
                    normals: strokeNormals,
                    strokeWidths: strokeWidths
                });*/
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
            tearDown: tearDown,
            changeStrokeColor: changeStrokeColor,
            changeStrokeWidthMultiplier: changeStrokeWidthMultiplier,
            changeTexture: changeTexture,
            isDrawing: isDrawing,
            undoErasing: undoErasing,
            getStrokeColor: getStrokeColor,
            getStrokeWidth: getStrokeWidth,
            getEntityID: getEntityID,
			changeUVMode: changeUVMode
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
            
            var LASER_ALPHA = 0.5;
            var LASER_TRIGGER_COLOR_XYZW = {x: 250 / 255, y: 10 / 255, z: 10 / 255, w: LASER_ALPHA};
            var SYSTEM_LASER_DIRECTION = {x: 0, y: 0, z: -1};
            var LEFT_HUD_LASER = 1;
            var RIGHT_HUD_LASER = 2;
            var BOTH_HUD_LASERS = LEFT_HUD_LASER + RIGHT_HUD_LASER;
            if (isLeftHandDominant){
                HMD.setHandLasers(RIGHT_HUD_LASER, true, LASER_TRIGGER_COLOR_XYZW, SYSTEM_LASER_DIRECTION);
                
                HMD.disableHandLasers(LEFT_HUD_LASER);
            }else{
                HMD.setHandLasers(LEFT_HUD_LASER, true, LASER_TRIGGER_COLOR_XYZW, SYSTEM_LASER_DIRECTION);
                HMD.disableHandLasers(RIGHT_HUD_LASER);
                
            }
            HMD.disableExtraLaser();
            
            
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
                
                opositeHandPosition = MyAvatar.getJointPosition(handName === "left" ? "RightHandMiddle1" : "LeftHandMiddle1");
                
                if (triggerValue < TRIGGER_START_WIDTH_RAMP) {
                    lineWidth = MIN_LINE_WIDTH;
                } else {
                    lineWidth = MIN_LINE_WIDTH
                        + (triggerValue - TRIGGER_START_WIDTH_RAMP) / TRIGGER_RAMP_WIDTH * RAMP_LINE_WIDTH;
                }
                
                if ((handName === "left" && isLeftHandDominant) || (handName === "right" && !isLeftHandDominant)){
                    if (!wasTriggerPressed && isTriggerPressed) {
                        
                        // TEST DAANTJE changes to a random color everytime you start a new line
                        //leftBrush.changeStrokeColor(Math.random()*255, Math.random()*255, Math.random()*255);
                        //rightBrush.changeStrokeColor(Math.random()*255, Math.random()*255, Math.random()*255);
                        // TEST Stroke line width
                        //var dim = Math.random()*4 + +0.5;
                        //var dim2 = Math.floor( Math.random()*40 + 5);
                        //leftBrush.changeStrokeWidthMultiplier(dim);
                        //rightBrush.changeStrokeWidthMultiplier(dim);
                        
                        triggerPressedCallback(fingerTipPosition, lineWidth);
                    } else if (wasTriggerPressed && isTriggerPressed) {
                        triggerPressingCallback(fingerTipPosition, lineWidth);
                    } else {
                        triggerReleasedCallback(fingerTipPosition, lineWidth);
                        
                        /* // define condition to switch dominant hands
                        if (Vec3.length(Vec3.subtract(fingerTipPosition, opositeHandPosition)) < 0.1){
                            isLeftHandDominant = !isLeftHandDominant;
                            
                            // Test DAANTJE changes texture
                            // if (Math.random() > 0.5) {
                                // leftBrush.changeTexture(null);
                                // rightBrush.changeTexture(null);
                            // }else {
                                // leftBrush.changeTexture('http://i.imgur.com/SSWDJtd.png');
                                // rightBrush.changeTexture('https://upload.wikimedia.org/wikipedia/commons/thumb/9/93/Caris_Tessellation.svg/1024px-Caris_Tessellation.svg.png');
                            // }
                            
                        } */
                        
                    }
                    
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
                    
                    if ((handName === "left" && isLeftHandDominant) || (handName === "right" && !isLeftHandDominant)){
                        gripPressedCallback(fingerTipPosition);
                    }
                }
            }
        }

        function checkTabletHasFocus() {
            var controllerPose = isLeftHandDominant ? getControllerWorldLocation(Controller.Standard.LeftHand, true)
                                                    : getControllerWorldLocation(Controller.Standard.RightHand, true);
            
            var fingerTipRotation = controllerPose.rotation;//Quat.inverse(MyAvatar.orientation);
            var fingerTipPosition = controllerPose.position;//MyAvatar.getJointPosition(handName === "left" ? "LeftHandIndex4" : "RightHandIndex4");
            var pickRay = {
                origin: fingerTipPosition,
                direction: Quat.getUp(fingerTipRotation)
            }
            var overlays = Overlays.findRayIntersection(pickRay, false, [HMD.tabletID], [inkSourceOverlay.overlayID]);
            //print(JSON.stringify(overlays));
            if (overlays.intersects && HMD.tabletID == overlays.overlayID) {
                if (!isTabletFocused) {
                    isTabletFocused = true;
                    //isFingerPainting = false;
                    Overlays.editOverlay(inkSourceOverlay, {visible: false});

                    updateHandAnimations();
                    pauseProcessing();
                    print("Hovering tablet!");
                } 
            } else {
                if (isTabletFocused) {
                    print("Unhovering tablet!");
                    isTabletFocused = false;
                    //isFingerPainting = true;
                    Overlays.editOverlay(inkSourceOverlay, {visible: true});
                    resumeProcessing();
                    updateHandFunctions();
                    //updateHandAnimations();
                    print("Current hand " + handName);
                    print("isFingerPainting " + isFingerPainting);
                    print("inkSourceOverlay " + JSON.stringify(inkSourceOverlay));
                    print("inkSourceOverlay " + JSON.stringify(inkSourceOverlay));
                }            
            };
        }

        function onUpdate() {
            
            //update ink Source
            // var strokeColor = leftBrush.getStrokeColor();
            // var strokeWidth = leftBrush.getStrokeWidth()*0.06;
            
            // var position = MyAvatar.getJointPosition(isLeftHandDominant ? "LeftHandIndex4" : "RightHandIndex4");
            // if (inkSource){
                
                
                // Entities.editEntity(inkSource, {
                    // color : strokeColor,
                    // position : position,
                    // dimensions : {
                        // x: strokeWidth, 
                        // y: strokeWidth, 
                        // z: strokeWidth} 
                
                // });
            // } else{
                // var inkSourceProps = {
                    // type: "Sphere",
                    // name: "inkSource",
                    // color: strokeColor,
                    // position: position,
                    // ignoreForCollisions: true,
                    
                    // dimensions: {x: strokeWidth, y:strokeWidth, z:strokeWidth}
                // }
                // inkSource = Entities.addEntity(inkSourceProps);
            // }

            /*frameDeltaSeconds = Date.now() - lastFrameTime;
            lastFrameTime = Date.now();
            var nearbyEntities = Entities.findEntities(MyAvatar.position, 5.0);
            for (var i = 0; i < nearbyEntities.length; i++) {
                //Entities.editEntity(nearbyEntities[i], {userData: ""}); //clear user data (good to use in case we need to clear it)

                var userData = Entities.getEntityProperties(nearbyEntities[i]).userData;
                if (userData != "") {
                    playAnimations(nearbyEntities[i], JSON.parse(userData).animations);
                }
            }*/
            /*if (Entities.getEntityProperties(nearbyEntities[i]).name == "fingerPainting")
                    Entities.deleteEntity(nearbyEntities[i]);*/
            if ((leftBrush == null || rightBrush == null) || (!leftBrush.isDrawing() && !rightBrush.isDrawing()))  {
                checkTabletHasFocus();
            }
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
            //Entities
            //if (inkSource){
            //    Entities.deleteEntity(inkSource);
			//	inkSource = null;
            //}
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
            pointerEnabled: false
        }), true);
        
        //    Messages.sendMessage(HIFI_POINTER_DISABLE_MESSAGE_CHANNEL, JSON.stringify({
        //    pointerEnabled: enabled
        //}), true);
        //}), true);
        
        
        
        Messages.sendMessage(HIFI_POINT_INDEX_MESSAGE_CHANNEL, JSON.stringify({
            pointIndex: !enabled
        }), true);
    }
	
	function updateHandAnimations(){
		var ANIM_URL = (isLeftHandDominant? LEFT_ANIM_URL: RIGHT_ANIM_URL );
		var ANIM_OPEN = (isLeftHandDominant? LEFT_ANIM_URL_OPEN: RIGHT_ANIM_URL_OPEN );
		var handLiteral = (isLeftHandDominant? "left": "right" );

		//Clear previous hand animation override
		restoreAllHandAnimations();
		
		//"rightHandGraspOpen","rightHandGraspClosed",
		MyAvatar.overrideRoleAnimation(handLiteral + "HandGraspOpen", ANIM_OPEN, 30, false, 19, 20);
		MyAvatar.overrideRoleAnimation(handLiteral + "HandGraspClosed", ANIM_URL, 30, false, 19, 20);

		//"rightIndexPointOpen","rightIndexPointClosed",
		MyAvatar.overrideRoleAnimation(handLiteral + "IndexPointOpen", ANIM_OPEN, 30, false, 19, 20);
		MyAvatar.overrideRoleAnimation(handLiteral + "IndexPointClosed", ANIM_URL, 30, false, 19, 20);

		//"rightThumbRaiseOpen","rightThumbRaiseClosed",
		MyAvatar.overrideRoleAnimation(handLiteral + "ThumbRaiseOpen", ANIM_OPEN, 30, false, 19, 20);
		MyAvatar.overrideRoleAnimation(handLiteral + "ThumbRaiseClosed", ANIM_URL, 30, false, 19, 20);

		//"rightIndexPointAndThumbRaiseOpen","rightIndexPointAndThumbRaiseClosed", 
		MyAvatar.overrideRoleAnimation(handLiteral + "IndexPointAndThumbRaiseOpen", ANIM_OPEN, 30, false, 19, 20);
		MyAvatar.overrideRoleAnimation(handLiteral + "IndexPointAndThumbRaiseClosed", ANIM_URL, 30, false, 19, 20);
		
		//turn off lasers and other interactions
		Messages.sendLocalMessage("Hifi-Hand-Disabler", "none");
		Messages.sendLocalMessage("Hifi-Hand-Disabler", handLiteral);
		
		
		
		//update ink Source
        var strokeColor = leftBrush.getStrokeColor();
        var strokeWidth = leftBrush.getStrokeWidth()*0.06;
		if (inkSourceOverlay == null){
			inkSourceOverlay = Overlays.addOverlay("sphere", { parentID: MyAvatar.sessionUUID, parentJointIndex: MyAvatar.getJointIndex(handLiteral === "left" ? "LeftHandIndex4" : "RightHandIndex4"), localPosition: { x: 0, y: 0, z: 0 }, size: strokeWidth, color: strokeColor , solid: true });
		} else {
			Overlays.editOverlay(inkSourceOverlay, {
                parentJointIndex: MyAvatar.getJointIndex(handLiteral === "left" ? "LeftHandIndex4" : "RightHandIndex4"),
				localPosition: { x: 0, y: 0, z: 0 },
				size: strokeWidth, 
				color: strokeColor 
            });
		}

	}
	
	function restoreAllHandAnimations(){
		//"rightHandGraspOpen","rightHandGraspClosed",
		MyAvatar.restoreRoleAnimation("rightHandGraspOpen");
		MyAvatar.restoreRoleAnimation("rightHandGraspClosed");

		//"rightIndexPointOpen","rightIndexPointClosed",
		MyAvatar.restoreRoleAnimation("rightIndexPointOpen");
		MyAvatar.restoreRoleAnimation("rightIndexPointClosed");

		//"rightThumbRaiseOpen","rightThumbRaiseClosed",
		MyAvatar.restoreRoleAnimation("rightThumbRaiseOpen");
		MyAvatar.restoreRoleAnimation("rightThumbRaiseClosed");

		//"rightIndexPointAndThumbRaiseOpen","rightIndexPointAndThumbRaiseClosed", 
		MyAvatar.restoreRoleAnimation("rightIndexPointAndThumbRaiseOpen");
		MyAvatar.restoreRoleAnimation("rightIndexPointAndThumbRaiseClosed");
		
		//"leftHandGraspOpen","leftHandGraspClosed",
		MyAvatar.restoreRoleAnimation("leftHandGraspOpen");
		MyAvatar.restoreRoleAnimation("leftHandGraspClosed");

		//"leftIndexPointOpen","leftIndexPointClosed",
		MyAvatar.restoreRoleAnimation("leftIndexPointOpen");
		MyAvatar.restoreRoleAnimation("leftIndexPointClosed");

		//"leftThumbRaiseOpen","leftThumbRaiseClosed",
		MyAvatar.restoreRoleAnimation("leftThumbRaiseOpen");
		MyAvatar.restoreRoleAnimation("leftThumbRaiseClosed");

		//"leftIndexPointAndThumbRaiseOpen","leftIndexPointAndThumbRaiseClosed", 
		MyAvatar.restoreRoleAnimation("leftIndexPointAndThumbRaiseOpen");
		MyAvatar.restoreRoleAnimation("leftIndexPointAndThumbRaiseClosed");
	}

    function pauseProcessing() {
        //Script.update.disconnect(leftHand.onUpdate);
        //Script.update.disconnect(rightHand.onUpdate);

        //Controller.disableMapping(CONTROLLER_MAPPING_NAME);

        Messages.sendLocalMessage("Hifi-Hand-Disabler", "none");

        Messages.unsubscribe(HIFI_POINT_INDEX_MESSAGE_CHANNEL);
        Messages.unsubscribe(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL);
        Messages.unsubscribe(HIFI_POINTER_DISABLE_MESSAGE_CHANNEL);
        
        
        //Restores and clears hand animations
        restoreAllHandAnimations();
    }

     function resumeProcessing() {
        //Change to finger paint hand animation
        updateHandAnimations();
        
        // Messages channels for enabling/disabling other scripts' functions.
        Messages.subscribe(HIFI_POINT_INDEX_MESSAGE_CHANNEL);
        Messages.subscribe(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL);
        Messages.subscribe(HIFI_POINTER_DISABLE_MESSAGE_CHANNEL);
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
        Controller.enableMapping(CONTROLLER_MAPPING_NAME);

        // Connect handController outputs to paintBrush objects.
        leftBrush = paintBrush("left");
        leftHand.setUp(leftBrush.startLine, leftBrush.drawLine, leftBrush.finishLine, leftBrush.eraseClosestLine);
        rightBrush = paintBrush("right");
        rightHand.setUp(rightBrush.startLine, rightBrush.drawLine, rightBrush.finishLine, rightBrush.eraseClosestLine);

		//Change to finger paint hand animation
		updateHandAnimations();
		
        // Messages channels for enabling/disabling other scripts' functions.
        Messages.subscribe(HIFI_POINT_INDEX_MESSAGE_CHANNEL);
        Messages.subscribe(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL);
        Messages.subscribe(HIFI_POINTER_DISABLE_MESSAGE_CHANNEL);

        // Update hand controls.
        Script.update.connect(leftHand.onUpdate);
        Script.update.connect(rightHand.onUpdate);
        
		
        // enable window palette
        /*window = new OverlayWindow({
            title: 'Paint Window',
            source: qml,
            width: 600, height: 600,
        });*/
        
        // 75
        //50
        //window.setPosition(75, 100);
        //window.closed.connect(function() { 
        //Script.stop(); 
        //}); uncomment for qml interface

        /*window.fromQml.connect(function(message){
            if (message[0] === "color"){
                leftBrush.changeStrokeColor(message[1], message[2], message[3]);
                rightBrush.changeStrokeColor(message[1], message[2], message[3]);
				Overlays.editOverlay(inkSourceOverlay, {
				    color: {red: message[1], green: message[2], blue: message[3]} 
                });
                return;
            }
            if (message[0] === "width"){
                print("changing brush width " + message[1]);
                var dim = message[1]*2 +0.1;
                print("changing brush dim " + dim);
                //var dim2 = Math.floor( Math.random()*40 + 5);
                leftBrush.changeStrokeWidthMultiplier(dim);
                rightBrush.changeStrokeWidthMultiplier(dim);
				Overlays.editOverlay(inkSourceOverlay, {
				    size: dim * 0.06 
				
                });
                return;
            }
            if (message[0] === "brush"){
                print("changing brush to qml " + message[1]);

                //var dim2 = Math.floor( Math.random()*40 + 5);
                leftBrush.changeTexture(message[1]);
                rightBrush.changeTexture(message[1]);
				
				if (message[1] === "content/brushes/paintbrush1.png") {
					leftBrush.changeUVMode(true);
					rightBrush.changeUVMode(true);
				}else if (message[1] === "content/brushes/paintbrush3.png") {
					leftBrush.changeUVMode(true);
					rightBrush.changeUVMode(true);
				}else{
					leftBrush.changeUVMode(false);
					rightBrush.changeUVMode(false);
				}
                return;
            }
            if (message[0] === "undo"){
                leftBrush.undoErasing();
                rightBrush.undoErasing();
                return;
            }
            if (message[0] === "hand"){
                isLeftHandDominant = !isLeftHandDominant;
				updateHandAnimations();
                return;
            }
        });*/ //uncomment for qml interface
    }

    function disableProcessing() {
        Script.update.disconnect(leftHand.onUpdate);
        Script.update.disconnect(rightHand.onUpdate);

        Controller.disableMapping(CONTROLLER_MAPPING_NAME);

		Messages.sendLocalMessage("Hifi-Hand-Disabler", "none");
		
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
        
		
		//Restores and clears hand animations
		restoreAllHandAnimations();
		
		//clears Overlay sphere
		Overlays.deleteOverlay(inkSourceOverlay);
		inkSourceOverlay = null;
		
        // disable window palette
        //window.close(); //uncomment for qml interface
    }

    //Load last fingerpaint settings
    function restoreLastValues() {
        savedSettings = new Object();
        savedSettings.currentColor = Settings.getValue("currentColor", {red: 250, green: 0, blue: 0}),
        savedSettings.currentStrokeWidth = Settings.getValue("currentStrokeWidth", 0.25);
        savedSettings.currentTexture = Settings.getValue("currentTexture", null);
        savedSettings.currentDrawingHand = Settings.getValue("currentDrawingHand", false);
        savedSettings.currentDynamicBrushes = Settings.getValue("currentDynamicBrushes", []);
        savedSettings.customColors = Settings.getValue("customColors", []);
        savedSettings.currentTab = Settings.getValue("currentTab", 0);

        print("Restoring data: " + JSON.stringify(savedSettings));
        isLeftHandDominant = savedSettings.currentDrawingHand;
    }

    function onButtonClicked() {
        restoreLastValues();

        isTabletFocused = false; //should always start false so onUpdate updates this variable to true in the beggining
        var wasFingerPainting = isFingerPainting;

        isFingerPainting = !isFingerPainting;
        button.editProperties({ isActive: isFingerPainting });

        print("Finger painting: " + isFingerPainting ? "on" : "off");

        if (wasFingerPainting) {
            leftBrush.cancelLine();
            rightBrush.cancelLine();
        }

        if (isFingerPainting) {
        	tablet.gotoWebScreen(APP_URL);
            enableProcessing();
        }

        updateHandFunctions();

        if (!isFingerPainting) {
            disableProcessing();
        }
    }

    function onTabletScreenChanged(type, url) {
        var TABLET_SCREEN_CLOSED = "Closed";
        var TABLET_SCREEN_HOME = "Home";

        isTabletDisplayed = type !== TABLET_SCREEN_CLOSED;
        if (type === TABLET_SCREEN_HOME) {
            if (isFingerPainting) {
                isFingerPainting = false;
                disableProcessing();
            }
            button.editProperties({ isActive: isFingerPainting });
            print("Closing the APP");
        }
        updateHandFunctions();
    }


    function onWebEventReceived(event){
        print("Received Web Event: " + event);

        if (typeof event === "string") {
            event = JSON.parse(event);
        }
        switch (event.type) {
            case "ready":
                print("Setting up the tablet");
                tablet.emitScriptEvent(JSON.stringify(savedSettings));
                break;
            case "changeTab":
                Settings.setValue("currentTab", event.currentTab);
                break;
            case "changeColor":
                if (!isBrushColored) {
                    Settings.setValue("currentColor", event);
                    print("changing color...");
                    leftBrush.changeStrokeColor(event.red, event.green, event.blue);
                    rightBrush.changeStrokeColor(event.red, event.green, event.blue);
                    Overlays.editOverlay(inkSourceOverlay, {
                        color: {red: event.red, green: event.green, blue: event.blue} 
                    });
                    
                } 
                break;
            case "addCustomColor":
                print("Adding custom color");
                var customColors = Settings.getValue("customColors", []);
                customColors.push({red: event.red, green: event.green, blue: event.blue});
                if (customColors.length > event.maxColors) {
                    customColors.splice(0, 1); //remove first color
                }
                Settings.setValue("customColors", customColors);
                break;


            case "changeBrush":
                print("abrushType: " + event.brushType);
                Settings.setValue("currentTexture", event);
                if (event.brushType === "repeat") {
                    print("brushType: " + event.brushType);
                    leftBrush.changeUVMode(false);
                    rightBrush.changeUVMode(false);
                } else if (event.brushType === "stretch") {
                    print("brushType: " + event.brushType);
                    leftBrush.changeUVMode(true);
                    rightBrush.changeUVMode(true);
                } 
                isBrushColored = event.isColored;
                if (event.isColored) {
                    leftBrush.changeStrokeColor(255, 255, 255);
                    rightBrush.changeStrokeColor(255, 255, 255);
                    Overlays.editOverlay(inkSourceOverlay, {
                        color: {red: 255, green: 255, blue: 255} 
                    });
                }
                /*leftBrush.changeUVMode(false);
                rightBrush.changeUVMode(false);
                if (event.brushName.indexOf("256x256") != -1) {
                    leftBrush.changeUVMode(true);
                    rightBrush.changeUVMode(true);
                }*/
                print("changing brush to " + event.brushName);
                event.brushName = CONTENT_PATH + "/" + event.brushName;
                leftBrush.changeTexture(event.brushName);
                rightBrush.changeTexture(event.brushName);
                break;

            case "changeLineWidth":
                Settings.setValue("currentStrokeWidth", event.brushWidth);
                var dim = event.brushWidth*2 +0.1;
                print("changing brush width dim to " + dim);
                //var dim2 = Math.floor( Math.random()*40 + 5);
                leftBrush.changeStrokeWidthMultiplier(dim);
                rightBrush.changeStrokeWidthMultiplier(dim);
                Overlays.editOverlay(inkSourceOverlay, {
                    size: dim * 0.06 
                });
                break;

            case "undo":
                print("Going to undo");
                /**
                The undo is called only on the right brush because the undo stack is global, meaning that
                calling undoErasing on both the left and right brush would cause the stack to pop twice.
                Using the leftBrush instead of the rightBrush would have the exact same effect.
                */
                rightBrush.undoErasing();
                break;

            case "changeBrushHand":
                Settings.setValue("currentDrawingHand", event.DrawingHand == "left");
                isLeftHandDominant = event.DrawingHand == "left";
                updateHandAnimations();
                break;

            case "switchDynamicBrush":
                var dynamicBrushes = Settings.getValue("currentDynamicBrushes", []);
                var brushSettingsIndex = dynamicBrushes.indexOf(event.dynamicBrushID);
                if (brushSettingsIndex > -1) { //already exists so we are disabling it
                    dynamicBrushes.splice(brushSettingsIndex, 1);
                } else { //doesn't exist yet so we are just adding it
                    dynamicBrushes.push(event.dynamicBrushID);
                }
                Settings.setValue("currentDynamicBrushes", dynamicBrushes);
                DynamicBrushesInfo[event.dynamicBrushName].isEnabled = event.enabled;
                DynamicBrushesInfo[event.dynamicBrushName].settings = event.settings;
                //print("SEtting dynamic brush" + JSON.stringify(DynamicBrushesInfo[event.dynamicBrushName]));
                break;

            default:
                break;
        }
    }    

    function addAnimationToBrush(entityID) {
            print("Brushes INfo 0" + JSON.stringify(DynamicBrushesInfo));
            Object.keys(DynamicBrushesInfo).forEach(function(animationName) {
                print(animationName + "Brushes INfo 0" + JSON.stringify(DynamicBrushesInfo));
                if (DynamicBrushesInfo[animationName].isEnabled) {
                    var prevUserData = Entities.getEntityProperties(entityID).userData;
                    prevUserData = prevUserData == "" ? new Object() : JSON.parse(prevUserData); //preserve other possible user data
                    if (prevUserData.animations == null) {
                        prevUserData.animations = {};
                    }
                    prevUserData.animations[animationName] = dynamicBrushFactory(animationName, DynamicBrushesInfo[animationName].settings);
                    Entities.editEntity(entityID, {userData: JSON.stringify(prevUserData)});    
                    //Entities.editEntity(entityID, {script: Script.resolvePath("content/brushes/dynamicBrushes/dynamicBrushScript.js")});    
                }
            });
            print("Brushes INfo 1" + JSON.stringify(DynamicBrushesInfo));
    }

    function addElementToUndoStack(item)
    {
        if (undoStack.length + 1 > UNDO_STACK_SIZE) {
            undoStack.splice(0, 1);
        }
        undoStack.push(item);
    }

    function setUp() {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (!tablet) {
            return;
        }
        tablet.webEventReceived.connect(onWebEventReceived);
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

    setUp();
    Script.scriptEnding.connect(tearDown);    
}());