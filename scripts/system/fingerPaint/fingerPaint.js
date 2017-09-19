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
        UNDO_STACK_SIZE = 10, 
        _undoStack = [];
        _isFingerPainting = false,
        _isTabletFocused = false,
        _shouldRestoreTablet = false,
        MAX_LINE_WIDTH = 0.036,
        _leftHand = null,
        _rightHand = null,
        _leftBrush = null,
        _rightBrush = null,
        _isBrushColored = false,
        _isLeftHandDominant = false,
        _isMouseDrawing = false,
        _savedSettings = null,
        _isTabletDisplayed = false,
        CONTROLLER_MAPPING_NAME = "com.highfidelity.fingerPaint",
        HIFI_POINT_INDEX_MESSAGE_CHANNEL = "Hifi-Point-Index",
        HIFI_GRAB_DISABLE_MESSAGE_CHANNEL = "Hifi-Grab-Disable",
        HIFI_POINTER_DISABLE_MESSAGE_CHANNEL = "Hifi-Pointer-Disable",
        SCRIPT_PATH = Script.resolvePath(''),
        CONTENT_PATH = SCRIPT_PATH.substr(0, SCRIPT_PATH.lastIndexOf('/')),
        ANIMATION_SCRIPT_PATH = Script.resolvePath("content/animatedBrushes/animatedBrushScript.js"),
        APP_URL = CONTENT_PATH + "/html/main.html";

    Script.include("../libraries/controllers.js");
    Script.include("content/js/ColorUtils.js");
    Script.include("content/animatedBrushes/animatedBrushesList.js");

    var _inkSourceOverlay = null;
    // Set path for finger paint hand animations 
    var RIGHT_ANIM_URL = Script.resourcesPath() + 'avatar/animations/touch_point_closed_right.fbx';
    var LEFT_ANIM_URL = Script.resourcesPath() + 'avatar/animations/touch_point_closed_left.fbx';
    var RIGHT_ANIM_URL_OPEN = Script.resourcesPath() + 'avatar/animations/touch_point_open_right.fbx';
    var LEFT_ANIM_URL_OPEN = Script.resourcesPath() + 'avatar/animations/touch_point_open_left.fbx'; 

    function paintBrush(name) {
        // Paints in 3D.
        var brushName = name,
            _strokeColor = {
                red: _savedSettings.currentColor.red, 
                green: _savedSettings.currentColor.green, 
                blue: _savedSettings.currentColor.blue
            },
            _dynamicColor = null,
            ERASE_SEARCH_RADIUS = 0.1,  // m
            STROKE_DIMENSIONS = { x: 10, y: 10, z: 10 },
            _isDrawingLine = false,
            _isTriggerWidthEnabled = _savedSettings.currentTriggerWidthEnabled,
            _entityID,
            _basePosition,
            _strokePoints,
            _strokeNormals,
            _strokeColors,
            _strokeWidths,
            _timeOfLastPoint,
            _isContinuousLine = _savedSettings.currentIsContinuous,
            _lastPosition = null,
            _shouldKeepDrawing = false,
            _texture = CONTENT_PATH + "/" + _savedSettings.currentTexture.brushName,
            _dynamicEffects = _savedSettings.currentDynamicBrushes;
            //'https://upload.wikimedia.org/wikipedia/commons/thumb/9/93/Caris_Tessellation.svg/1024px-Caris_Tessellation.svg.png', // Daantje
            _strokeWidthMultiplier = _savedSettings.currentStrokeWidth * 2 + 0.1,
            _isUvModeStretch = _savedSettings.currentTexture.brushType == "stretch",
            MIN_STROKE_LENGTH = 0.005,  // m
            MIN_STROKE_INTERVAL = 66,  // ms
            MAX_POINTS_PER_LINE = 52;  // Quick fix for polyline points disappearing issue.
                
        function calculateStrokeNormal() {
            if (!_isMouseDrawing) {
                var controllerPose = _isLeftHandDominant 
                                    ? getControllerWorldLocation(Controller.Standard.LeftHand, true)
                                    : getControllerWorldLocation(Controller.Standard.RightHand, true);
                var fingerTipRotation = controllerPose.rotation;
                return Quat.getUp(fingerTipRotation);
            
            } else {
                return Vec3.multiplyQbyV(Camera.getOrientation(), Vec3.UNIT_NEG_Z);
            }
            
        }
        
        function setStrokeColor(red, green, blue) {
            _strokeColor.red = red;
            _strokeColor.green = green;
            _strokeColor.blue = blue;
        }
        
        function getStrokeColor() {
            return _strokeColor;
        }

        function calculateValueInRange(value, min, max, increment) {
            var delta = max - min;
            value += increment;
            if (value > max) {
                return min;
            } else {
                return value;
            }
        }

        function attacthColorToProperties(properties) {
            //colored brushes should always be white and no effects should be applied
            if (_isBrushColored) {
                properties.color = {red: 255, green: 255, blue: 255};
                return;
            }

            var isAnyDynamicEffectEnabled = false;
            if ("dynamicHue" in _dynamicEffects && _dynamicEffects.dynamicHue) {
                isAnyDynamicEffectEnabled = true;
                var hueIncrement = 359.0 / MAX_POINTS_PER_LINE;
                _dynamicColor.hue = calculateValueInRange(_dynamicColor.hue, 0, 359, hueIncrement);
            } 

            if ("dynamicSaturation" in _dynamicEffects && _dynamicEffects.dynamicSaturation) {
                isAnyDynamicEffectEnabled = true;
                _dynamicColor.saturation = _dynamicColor.saturation == 0.5 ? 1.0 : 0.5;
                //saturation along the full line
                //var saturationIncrement = 1.0 / 70.0;
                //_dynamicColor.saturation = calculateValueInRange(_dynamicColor.saturation, 0, 1, saturationIncrement);
            } 

            if ("dynamicValue" in _dynamicEffects && _dynamicEffects.dynamicValue) {
                isAnyDynamicEffectEnabled = true;
                _dynamicColor.value = _dynamicColor.value == 0.6 ? 1.0 : 0.6;
                //value along the full line
                //var saturationIncrement = 1.0 / 70.0;
                //_dynamicColor.saturation = calculateValueInRange(_dynamicColor.saturation, 0, 1, saturationIncrement);
            } 


            if (!isAnyDynamicEffectEnabled) {
                properties.color = _strokeColor;
                return;
            }

            var newRgbColor = hsv2rgb(_dynamicColor);
            _strokeColors.push({
                x: newRgbColor.red/255.0,
                y: newRgbColor.green/255.0, 
                z: newRgbColor.blue/255.0}
            );
            properties.strokeColors = _strokeColors;
        }

        function setDynamicEffects(dynamicEffects) {
            _dynamicEffects = dynamicEffects;
        }

        function isDrawingLine() {
            return _isDrawingLine;
        }
        
        function setStrokeWidthMultiplier(strokeWidthMultiplier) {
            _strokeWidthMultiplier = strokeWidthMultiplier;
        }

        function setIsContinuousLine(isContinuousLine) {
            _isContinuousLine = isContinuousLine;
        }

        function setIsTriggerWidthEnabled(isTriggerWidthEnabled) {
            _isTriggerWidthEnabled = isTriggerWidthEnabled;
        }
        
        function setUVMode(isUvModeStretch) {
            _isUvModeStretch = isUvModeStretch;
        }
        
        function getStrokeWidth() {
            return _strokeWidthMultiplier;
        }

        function getEntityID() {
            return _entityID;
        }
                
        function setTexture(texture) {
            _texture = texture;
        }
        
        function undo() {
            var undo = _undoStack.pop();
            if (_undoStack.length == 0) {
                var undoDisableEvent = {type: "undoDisable", value: true};
                tablet.emitScriptEvent(JSON.stringify(undoDisableEvent));
            }
            if (undo.type == "deleted") {
                var prevEntityId = undo.data.id;
                var newEntity = Entities.addEntity(undo.data);
                //restoring a deleted entity will create a new entity with a new id therefore we need to update
                //the created elements id in the undo stack. For the delete elements though, it is not necessary
                //to update the id since you can't delete the same entity twice
                for (var i = 0; i < _undoStack.length; i++) {
                    if (_undoStack[i].type == "created" && _undoStack[i].data == prevEntityId) {
                        _undoStack[i].data = newEntity;
                    } 
                }
            } else {
                Entities.deleteEntity(undo.data);
            }
        }

        function calculateLineWidth(width) {
            if (_isTriggerWidthEnabled) {
                return width * _strokeWidthMultiplier;    
            } else {
                return MAX_LINE_WIDTH * _strokeWidthMultiplier; //MAX_LINE_WIDTH
            }
        }

        function startLine(position, width) {
            // Start drawing a polyline.
            if (_isTabletFocused)
                return;

            width = calculateLineWidth(width);
            
            if (_isDrawingLine) {
                print("ERROR: startLine() called when already drawing line");
                // Nevertheless, continue on and start a new line.
            }

            if (_shouldKeepDrawing) {
                _strokePoints = [Vec3.distance(_basePosition, _strokePoints[_strokePoints.length - 1])];
            } else {
                _strokePoints = [Vec3.ZERO];
            }

            _basePosition = position;
            _strokeNormals = [calculateStrokeNormal()];
            _strokeColors = [];
            _strokeWidths = [width];
            _timeOfLastPoint = Date.now();

            var newEntityProperties = {
                type: "PolyLine",
                name: "fingerPainting",
                shapeType: "box",
                position: position,
                linePoints: _strokePoints,
                normals: _strokeNormals,
                strokeWidths: _strokeWidths,
                textures: _texture, // Daantje
                isUVModeStretch: _isUvModeStretch,
                dimensions: STROKE_DIMENSIONS,
            };
            _dynamicColor = rgb2hsv(_strokeColor);
            attacthColorToProperties(newEntityProperties);
            _entityID = Entities.addEntity(newEntityProperties);
            _isDrawingLine = true;
            addAnimationToBrush(_entityID);
            _lastPosition = position;
        }

        function drawLine(position, width) {
            // Add a stroke to the polyline if stroke is a sufficient length.
            var localPosition,
                distanceToPrevious,
                MAX_DISTANCE_TO_PREVIOUS = 1.0;
            
            width = calculateLineWidth(width);
                
            if (!_isDrawingLine) {
                print("ERROR: drawLine() called when not drawing line");
                return;
            }

            localPosition = Vec3.subtract(position, _basePosition);
            distanceToPrevious = Vec3.distance(localPosition, _strokePoints[_strokePoints.length - 1]);

            if (distanceToPrevious > MAX_DISTANCE_TO_PREVIOUS) {
                // Ignore occasional spurious finger tip positions.
                return;
            }

            if (distanceToPrevious >= MIN_STROKE_LENGTH
                    && (Date.now() - _timeOfLastPoint) >= MIN_STROKE_INTERVAL
                    && _strokePoints.length < MAX_POINTS_PER_LINE) {
                _strokePoints.push(localPosition);
                _strokeNormals.push(calculateStrokeNormal());
                _strokeWidths.push(width);
                _timeOfLastPoint = Date.now();

                var editItemProperties = {
                    color: _strokeColor,
                    linePoints: _strokePoints,
                    normals: _strokeNormals,
                    strokeWidths: _strokeWidths
                };
                attacthColorToProperties(editItemProperties);
                Entities.editEntity(_entityID, editItemProperties);

            } else if (_isContinuousLine && _strokePoints.length >= MAX_POINTS_PER_LINE) {
                finishLine(position, width);
                _shouldKeepDrawing = true;
                startLine(_lastPosition, width);
            }
            _lastPosition = position;
        }

        function finishLine(position, width) {
            // Finish drawing polyline; delete if it has only 1 point.
            var userData = Entities.getEntityProperties(_entityID).userData;
            if (userData && JSON.parse(userData).animations && !_isBrushColored) {
                Entities.editEntity(_entityID, {
                    script: ANIMATION_SCRIPT_PATH,
                });    
            }
            width = calculateLineWidth(width);
            
            if (!_isDrawingLine) {
                print("ERROR: finishLine() called when not drawing line");
                return;
            }

            if (_strokePoints.length === 1) {
                // Delete "empty" line.
                Entities.deleteEntity(_entityID);
            }

            _isDrawingLine = false;
            _shouldKeepDrawing = false;
            addElementToUndoStack({type: "created", data: _entityID});
        }

        function cancelLine() {
            // Cancel any line being drawn.
            if (_isDrawingLine) {
                Entities.deleteEntity(_entityID);
                _isDrawingLine = false;
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
                    _basePosition = properties.position;
                    for (j = 0, pointsLength = properties.linePoints.length; j < pointsLength; j += 1) {
                        distance = Vec3.distance(position, Vec3.sum(_basePosition, properties.linePoints[j]));
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
                addElementToUndoStack({type: "deleted", data: Entities.getEntityProperties(foundID)});
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
            tearDown: tearDown,
            setStrokeColor: setStrokeColor,
            setStrokeWidthMultiplier: setStrokeWidthMultiplier,
            setTexture: setTexture,
            isDrawingLine: isDrawingLine,
            undo: undo,
            getStrokeColor: getStrokeColor,
            getStrokeWidth: getStrokeWidth,
            getEntityID: getEntityID,
            setUVMode: setUVMode,
            setIsTriggerWidthEnabled: setIsTriggerWidthEnabled,
            setDynamicEffects: setDynamicEffects,
            setIsContinuousLine: setIsContinuousLine
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
                
                if ((handName === "left" && _isLeftHandDominant) || (handName === "right" && !_isLeftHandDominant)){
                    if (!wasTriggerPressed && isTriggerPressed) {
                        // TEST DAANTJE changes to a random color everytime you start a new line                    
                        triggerPressedCallback(fingerTipPosition, lineWidth);
                    } else if (wasTriggerPressed && isTriggerPressed) {
                        triggerPressingCallback(fingerTipPosition, lineWidth);
                    } else {
                        triggerReleasedCallback(fingerTipPosition, lineWidth);                       
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
                    
                    if ((handName === "left" && _isLeftHandDominant) || (handName === "right" && !_isLeftHandDominant)){
                        gripPressedCallback(fingerTipPosition);
                    }
                }
            }
        }

        function checkTabletHasFocus() {
            var controllerPose = _isLeftHandDominant 
                                    ? getControllerWorldLocation(Controller.Standard.LeftHand, true)
                                    : getControllerWorldLocation(Controller.Standard.RightHand, true);
            
            var fingerTipRotation = controllerPose.rotation;
            var fingerTipPosition = controllerPose.position;
            var pickRay = {
                origin: fingerTipPosition,
                direction: Quat.getUp(fingerTipRotation)
            }
            if (_inkSourceOverlay) {
                var overlays = Overlays.findRayIntersection(pickRay, false, [HMD.tabletID], [_inkSourceOverlay.overlayID]);
                if (overlays.intersects && HMD.tabletID == overlays.overlayID) {
                    if (!_isTabletFocused) {
                        _isTabletFocused = true;
                        Overlays.editOverlay(_inkSourceOverlay, {visible: false});
                        updateHandAnimations();
                        pauseProcessing();
                    } 
                } else {
                    if (_isTabletFocused) {
                        _isTabletFocused = false;
                        Overlays.editOverlay(_inkSourceOverlay, {visible: true});
                        resumeProcessing();
                        updateHandFunctions();
                    }            
                };
            }
        }

        function onUpdate() {
            if (HMD.tabletID 
                && (((_leftBrush == null || _rightBrush == null) 
                || (!_leftBrush.isDrawingLine() && !_rightBrush.isDrawingLine())))) {

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
        var enabled = !_isFingerPainting || _isTabletDisplayed;

        Messages.sendMessage(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL, JSON.stringify({
            holdEnabled: enabled,
            nearGrabEnabled: enabled,
            farGrabEnabled: enabled
        }), true);
        
        
        Messages.sendMessage(HIFI_POINTER_DISABLE_MESSAGE_CHANNEL, JSON.stringify({
            pointerEnabled: false
        }), true);      
        
        Messages.sendMessage(HIFI_POINT_INDEX_MESSAGE_CHANNEL, JSON.stringify({
            pointIndex: !enabled
        }), true);
    }
    
    function updateHandAnimations(){
        var ANIM_URL = (_isLeftHandDominant? LEFT_ANIM_URL: RIGHT_ANIM_URL );
        var ANIM_OPEN = (_isLeftHandDominant? LEFT_ANIM_URL_OPEN: RIGHT_ANIM_URL_OPEN );
        var handLiteral = (_isLeftHandDominant? "left": "right" );

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
        var strokeColor = _leftBrush.getStrokeColor();
        var strokeWidth = _leftBrush.getStrokeWidth()*0.06;
        if (_inkSourceOverlay == null){
            _inkSourceOverlay = Overlays.addOverlay(
                "sphere", 
                { 
                    parentID: MyAvatar.sessionUUID, 
                    parentJointIndex: MyAvatar.getJointIndex(handLiteral === "left" ? "LeftHandIndex4" : "RightHandIndex4"),
                    localPosition: { x: 0, y: 0, z: 0 }, 
                    size: strokeWidth, 
                    color: strokeColor, 
                    solid: true 
                });
        } else {
            Overlays.editOverlay(
                _inkSourceOverlay, 
                {
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
        _leftHand = handController("left");
        _rightHand = handController("right");
        
          // Connect handController outputs to paintBrush objects.
        _leftBrush = paintBrush("left");
        _leftHand.setUp(_leftBrush.startLine, _leftBrush.drawLine, _leftBrush.finishLine, _leftBrush.eraseClosestLine);
        _rightBrush = paintBrush("right");
        _rightHand.setUp(_rightBrush.startLine, _rightBrush.drawLine, _rightBrush.finishLine, _rightBrush.eraseClosestLine);

        var controllerMapping = Controller.newMapping(CONTROLLER_MAPPING_NAME);
        controllerMapping.from(Controller.Standard.LT).to(_leftHand.onTriggerPress);
        controllerMapping.from(Controller.Standard.LeftGrip).to(_leftHand.onGripPress);
        controllerMapping.from(Controller.Standard.RT).to(_rightHand.onTriggerPress);
        controllerMapping.from(Controller.Standard.RightGrip).to(_rightHand.onGripPress);
        Controller.enableMapping(CONTROLLER_MAPPING_NAME);

        //Change to finger paint hand animation
        updateHandAnimations();
        
        // Messages channels for enabling/disabling other scripts' functions.
        Messages.subscribe(HIFI_POINT_INDEX_MESSAGE_CHANNEL);
        Messages.subscribe(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL);
        Messages.subscribe(HIFI_POINTER_DISABLE_MESSAGE_CHANNEL);

        // Update hand controls.
        Script.update.connect(_leftHand.onUpdate);
        Script.update.connect(_rightHand.onUpdate);
    }

    function disableProcessing() {
        if (_leftHand && _rightHand) {
            Script.update.disconnect(_leftHand.onUpdate);
            Script.update.disconnect(_rightHand.onUpdate);

            Controller.disableMapping(CONTROLLER_MAPPING_NAME);

            Messages.sendLocalMessage("Hifi-Hand-Disabler", "none");
            
            _leftBrush.tearDown();
            _leftBrush = null;
            _leftHand.tearDown();
            _leftHand = null;

            _rightBrush.tearDown();
            _rightBrush = null;
            _rightHand.tearDown();
            _rightHand = null;

            Messages.unsubscribe(HIFI_POINT_INDEX_MESSAGE_CHANNEL);
            Messages.unsubscribe(HIFI_GRAB_DISABLE_MESSAGE_CHANNEL);
            Messages.unsubscribe(HIFI_POINTER_DISABLE_MESSAGE_CHANNEL);
            
            //Restores and clears hand animations
            restoreAllHandAnimations();
            
            //clears Overlay sphere
            Overlays.deleteOverlay(_inkSourceOverlay);
            _inkSourceOverlay = null;
        }
    }

    //Load last fingerpaint settings
    function restoreLastValues() {
        _savedSettings = new Object();
        _savedSettings.currentColor = Settings.getValue("currentColor", {red: 250, green: 0, blue: 0, origin: "custom"}),
        _savedSettings.currentStrokeWidth = Settings.getValue("currentStrokeWidth", 0.25);
        _savedSettings.currentTexture = Settings.getValue("currentTexture", {brushID: 0});
        _savedSettings.currentDrawingHand = Settings.getValue("currentDrawingHand", MyAvatar.getDominantHand() == "left");
        _savedSettings.currentAnimatedBrushes = Settings.getValue("currentAnimatedBrushes", []);
        _savedSettings.customColors = Settings.getValue("customColors", []);
        _savedSettings.currentTab = Settings.getValue("currentTab", 0);
        _savedSettings.currentTriggerWidthEnabled = Settings.getValue("currentTriggerWidthEnabled", true);
        _savedSettings.currentDynamicBrushes = Settings.getValue("currentDynamicBrushes", new Object());
        _savedSettings.currentIsContinuous = Settings.getValue("currentIsContinuous", false);
        _savedSettings.currentIsBrushColored = Settings.getValue("currentIsBrushColored", false);
        _savedSettings.currentHeadersCollapsedStatus = Settings.getValue("currentHeadersCollapsedStatus", new Object());
        _savedSettings.undoDisable = _undoStack.length == 0;
        //set some global variables
        _isLeftHandDominant = _savedSettings.currentDrawingHand;
        _isBrushColored = _savedSettings.currentIsBrushColored;
    }

    function onButtonClicked() {
        restoreLastValues();
        var wasFingerPainting = _isFingerPainting;
        _isFingerPainting = !_isFingerPainting;

        if (!_isFingerPainting) {
            tablet.gotoHomeScreen();
        }
        button.editProperties({ isActive: _isFingerPainting });

        if (wasFingerPainting) {
            _leftBrush.cancelLine();
            _rightBrush.cancelLine();
        }

        if (_isFingerPainting) {
            tablet.gotoWebScreen(APP_URL + "?" + encodeURIComponent(JSON.stringify(_savedSettings)));
            HMD.openTablet();
            enableProcessing();
            _savedSettings = null;
        }

        updateHandFunctions();

        if (!_isFingerPainting) {
            disableProcessing();
            Controller.mousePressEvent.disconnect(mouseStartLine);
            Controller.mouseMoveEvent.disconnect(mouseDrawLine);
            Controller.mouseReleaseEvent.disconnect(mouseFinishLine);
        } else {
            Controller.mousePressEvent.connect(mouseStartLine);
            Controller.mouseMoveEvent.connect(mouseDrawLine);
            Controller.mouseReleaseEvent.connect(mouseFinishLine);
        }
    }
    
    function onTabletShownChanged() {
        if (_shouldRestoreTablet && tablet.tabletShown) {
            _shouldRestoreTablet = false;
            _isTabletFocused = false; 
            _isFingerPainting = false;
            HMD.openTablet();
            onButtonClicked();
            HMD.openTablet();
        }
    }

    function onTabletScreenChanged(type, url) {
        var TABLET_SCREEN_CLOSED = "Closed";
        var TABLET_SCREEN_HOME = "Home";
        var TABLET_SCREEN_WEB = "Web";
            
        _isTabletDisplayed = type !== TABLET_SCREEN_CLOSED;
        _isFingerPainting = type === TABLET_SCREEN_WEB && url.indexOf("fingerPaint/html/main.html") > -1;
        
        if (!_isFingerPainting) {
            disableProcessing();
            updateHandFunctions();
        }
        button.editProperties({ isActive: _isFingerPainting });
    }


    function onWebEventReceived(event){

        if (typeof event === "string") {
            event = JSON.parse(event);
        }

        switch (event.type) {
            case "appReady":
                _isTabletFocused = false; //make sure we can set the focus on the tablet again
                break;

            case "reloadPage":
                restoreLastValues();
                tablet.gotoWebScreen(APP_URL + "?" + encodeURIComponent(JSON.stringify(_savedSettings)));
                break;

            case "changeTab":
                Settings.setValue("currentTab", event.currentTab);
                break;

            case "changeColor":
                if (!_isBrushColored) {
                    var setStrokeColorEvent = {type: "changeStrokeColor", value: event};
                    tablet.emitScriptEvent(JSON.stringify(setStrokeColorEvent));
                    
                    Settings.setValue("currentColor", event);
                    _leftBrush.setStrokeColor(event.red, event.green, event.blue);
                    _rightBrush.setStrokeColor(event.red, event.green, event.blue);
                    Overlays.editOverlay(_inkSourceOverlay, {
                        color: {red: event.red, green: event.green, blue: event.blue} 
                    });
                } 
                break;

            case "switchTriggerPressureWidth":
                Settings.setValue("currentTriggerWidthEnabled", event.enabled);
                _leftBrush.setIsTriggerWidthEnabled(event.enabled);
                _rightBrush.setIsTriggerWidthEnabled(event.enabled);
                break;

            case "addCustomColor":
                var customColors = Settings.getValue("customColors", []);
                customColors.push({red: event.red, green: event.green, blue: event.blue});
                if (customColors.length > event.maxColors) {
                    customColors.splice(0, 1); //remove first color
                }
                Settings.setValue("customColors", customColors);
                break;

            case "switchCollapsed":
                var collapsedStatus = Settings.getValue("currentHeadersCollapsedStatus", new Object());
                collapsedStatus[event.sectionId] = event.isCollapsed;
                Settings.setValue("currentHeadersCollapsedStatus", collapsedStatus);
                break;

            case "changeBrush":
                Settings.setValue("currentTexture", event);
                Settings.setValue("currentIsBrushColored", event.isColored);

                if (event.brushType === "repeat") {
                    _leftBrush.setUVMode(false);
                    _rightBrush.setUVMode(false);

                } else if (event.brushType === "stretch") {
                    _leftBrush.setUVMode(true);
                    _rightBrush.setUVMode(true);
                } 

                _isBrushColored = event.isColored;
                event.brushName = CONTENT_PATH + "/" + event.brushName;
                _leftBrush.setTexture(event.brushName);
                _rightBrush.setTexture(event.brushName);
                break;

            case "changeLineWidth":
                Settings.setValue("currentStrokeWidth", event.brushWidth);
                var dim = event.brushWidth * 2 + 0.1;
                _leftBrush.setStrokeWidthMultiplier(dim);
                _rightBrush.setStrokeWidthMultiplier(dim);
                Overlays.editOverlay(_inkSourceOverlay, {
                    size: dim * 0.06 
                });
                break;

            case "undo":
                //The undo is called only on the right brush because the undo stack is global, meaning that
                //calling undo on both the left and right brush would cause the stack to pop twice.
                //Using the leftBrush instead of the rightBrush would have the exact same effect.
                _rightBrush.undo();
                break;

            case "changeBrushHand":
                Settings.setValue("currentDrawingHand", event.DrawingHand == "left");
                _isLeftHandDominant = event.DrawingHand == "left";
                _isTabletFocused = false;
                updateHandAnimations();
                break;

            case "switchAnimatedBrush":
                var animatedBrushes = Settings.getValue("currentAnimatedBrushes", []);
                var brushSettingsIndex = animatedBrushes.indexOf(event.animatedBrushID);
                if (!event.enabled && brushSettingsIndex > -1) { //already exists so we are disabling it
                    animatedBrushes.splice(brushSettingsIndex, 1);
                } else if (event.enabled && brushSettingsIndex == -1) { //doesn't exist yet so we are just adding it
                    animatedBrushes.push(event.animatedBrushID);
                }
                Settings.setValue("currentAnimatedBrushes", animatedBrushes);
                print("Setting animated brushes: " + JSON.stringify(animatedBrushes));
                AnimatedBrushesInfo[event.animatedBrushName].isEnabled = event.enabled;
                AnimatedBrushesInfo[event.animatedBrushName].settings = event.settings;
                break;

            case "switchDynamicBrush":
                var dynamicBrushes = Settings.getValue("currentDynamicBrushes", new Object());
                dynamicBrushes[event.dynamicBrushId] = event.enabled;
                Settings.setValue("currentDynamicBrushes", dynamicBrushes);
                _rightBrush.setDynamicEffects(dynamicBrushes);
                _leftBrush.setDynamicEffects(dynamicBrushes);
                break;

            case "switchIsContinuous":
                Settings.setValue("currentIsContinuous", event.enabled);
                _rightBrush.setIsContinuousLine(event.enabled);
                _leftBrush.setIsContinuousLine(event.enabled);
                break;

            default:
                break;
        }
    }    

    function addAnimationToBrush(entityID) {
        Object.keys(AnimatedBrushesInfo).forEach(function(animationName) {
            if (AnimatedBrushesInfo[animationName].isEnabled) {
                var prevUserData = Entities.getEntityProperties(entityID).userData;
                //preserve other possible user data
                prevUserData = prevUserData == "" ? new Object() : JSON.parse(prevUserData); 
                if (prevUserData.animations == null) {
                    prevUserData.animations = {};
                }

                prevUserData.animations[animationName] = animatedBrushFactory(animationName, 
                    AnimatedBrushesInfo[animationName].settings, entityID);

                Entities.editEntity(entityID, {userData: JSON.stringify(prevUserData)});    
            }
        });
    }

    function addElementToUndoStack(item)
    {
        var undoDisableEvent = {type: "undoDisable", value: false};
        tablet.emitScriptEvent(JSON.stringify(undoDisableEvent));

        if (_undoStack.length + 1 > UNDO_STACK_SIZE) {
            _undoStack.splice(0, 1);
        }
        _undoStack.push(item);
    }

    function onHmdChanged(isHMDActive) { 
        var wasHMDActive = Settings.getValue("wasHMDActive", null);        
        if (isHMDActive != wasHMDActive) {
            Settings.setValue("wasHMDActive", isHMDActive);            
            if (wasHMDActive == null) {
                return;
            } else {
                if (_isFingerPainting) {
                    _shouldRestoreTablet = true;

                    //Make sure the tablet is being shown when we try to change the window
                    while (!tablet.tabletShown) {
                        HMD.openTablet();
                    }
                } 
            }
        }
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
            isActive: _isFingerPainting
        });

        button.clicked.connect(onButtonClicked);
        // Track whether tablet is displayed or not.
        tablet.screenChanged.connect(onTabletScreenChanged);
        tablet.tabletShownChanged.connect(onTabletShownChanged);
        HMD.displayModeChanged.connect(onHmdChanged);
    }

    function tearDown() {
        if (!tablet) {
            return;
        }

        if (_isFingerPainting) {
            _isFingerPainting = false;
            updateHandFunctions();
            disableProcessing();
        }

        tablet.screenChanged.disconnect(onTabletScreenChanged);
        tablet.tabletShownChanged.disconnect(onTabletShownChanged);
        button.clicked.disconnect(onButtonClicked);
        tablet.removeButton(button);
    }
   
    function getFingerPosition(x, y) {
        var pickRay = Camera.computePickRay(x, y);
        return Vec3.sum(pickRay.origin, Vec3.multiply(pickRay.direction, 5));
    }

    function mouseDrawLine(event){
        if (_rightBrush && _rightBrush.isDrawingLine()) {
            _rightBrush.drawLine(getFingerPosition(event.x, event.y), MAX_LINE_WIDTH);
        } 
    }

    function mouseStartLine(event){
        if (event.isLeftButton && _rightBrush && !_rightBrush.isDrawingLine()) {
            _isMouseDrawing = true;
            _rightBrush.startLine(getFingerPosition(event.x, event.y), MAX_LINE_WIDTH);
        }
        //Note: won't work until findRayIntersection works with polylines
        //
        //else if (event.isMiddleButton) {
        //    var pickRay = Camera.computePickRay(event.x, event.y);
        //    var entityToDelete = Entities.findRayIntersection(pickRay, false, [Entities.findEntities(MyAvatar.position, 1000)], []);
        //    print("Entity to DELETE: " + JSON.stringify(entityToDelete));
        //    var line3d = Overlays.addOverlay("line3d", {
        //        start: pickRay.origin, 
        //        end: Vec3.sum(pickRay.origin, Vec3.multiply(pickRay.direction, 100)),
        //        color: { red: 255, green: 0, blue: 255},
        //        lineWidth: 5
        //    });
        //    if (entityToDelete.intersects) {
        //        print("Entity to DELETE Properties: " + JSON.stringify(Entities.getEntityProperties(entityToDelete.entityID)));
        //        //Entities.deleteEntity(entityToDelete.entityID);
        //    }
        //}
    }

    function mouseFinishLine(event){
        _isMouseDrawing = false;
        if (_rightBrush && _rightBrush.isDrawingLine()) {
            _rightBrush.finishLine(getFingerPosition(event.x, event.y), MAX_LINE_WIDTH);
        } 
    }

    setUp();
    Script.scriptEnding.connect(tearDown);    
}());