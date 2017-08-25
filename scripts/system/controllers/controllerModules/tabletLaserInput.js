"use strict"

//  tabletLaserInput.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND,
   NULL_UUID, enableDispatcherModule, disableDispatcherModule, makeRunningValues,
   Messages, Quat, Vec3, getControllerWorldLocation, makeDispatcherModuleParameters, Overlays, ZERO_VEC,
   AVATAR_SELF_ID, HMD, INCHES_TO_METERS, DEFAULT_REGISTRATION_POINT, Settings, getGrabPointSphereOffset
*/



Script.include("/~/system/controllers/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
     var halfPath = {
        type: "line3d",
        color: COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawInFront: true, // Even when burried inside of something, show it.
        parentID: AVATAR_SELF_ID
    };
    var halfEnd = {
        type: "sphere",
        solid: true,
        color: COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
        alpha: 0.9,
        ignoreRayIntersection: true,
        drawInFront: true, // Even when burried inside of something, show it.
        visible: true
    };
    var fullPath = {
        type: "line3d",
        color: COLORS_GRAB_SEARCHING_FULL_SQUEEZE,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawInFront: true, // Even when burried inside of something, show it.
        parentID: AVATAR_SELF_ID
    };
    var fullEnd = {
        type: "sphere",
        solid: true,
        color: COLORS_GRAB_SEARCHING_FULL_SQUEEZE,
        alpha: 0.9,
        ignoreRayIntersection: true,
        drawInFront: true, // Even when burried inside of something, show it.
        visible: true
    };
    var holdPath = {
        type: "line3d",
        color: COLORS_GRAB_DISTANCE_HOLD,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawInFront: true, // Even when burried inside of something, show it.
        parentID: AVATAR_SELF_ID
    };
    
    var renderStates = [{name: "half", path: halfPath, end: halfEnd},
                        {name: "full", path: fullPath, end: fullEnd},
                        {name: "hold", path: holdPath}];

    var defaultRenderStates = [{name: "half", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: halfPath},
                               {name: "full", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: fullPath},
                               {name: "hold", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: holdPath}];
    

    // triggered when stylus presses a web overlay/entity
    var HAPTIC_STYLUS_STRENGTH = 1.0;
    var HAPTIC_STYLUS_DURATION = 20.0;

    function stylusTargetHasKeyboardFocus(laserTarget) {
        if (laserTarget && laserTarget !== NULL_UUID) {
            return Overlays.keyboardFocusOverlay === laserTarget;
        }
    }

    function setKeyboardFocusOnStylusTarget(laserTarget) {
        if (laserTarget && laserTarget !== NULL_UUID) {
            Overlays.keyboardFocusOverlay = laserTarget;
            Entities.keyboardFocusEntity = NULL_UUID;
        }
    }

    function sendHoverEnterEventToStylusTarget(hand, laserTarget) {
        if (!laserTarget) {
            return;
        }
        var pointerEvent = {
            type: "Move",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "None"
        };

        if (laserTarget.overlayID && laserTarget.overlayID !== NULL_UUID) {
            Overlays.sendHoverEnterOverlay(laserTarget.overlayID, pointerEvent);
        }
    }

    function sendHoverOverEventToStylusTarget(hand, laserTarget) {
        
        if (!laserTarget) {
            return;
        }
        var pointerEvent = {
            type: "Move",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "None"
        };

        if (laserTarget.overlayID && laserTarget.overlayID !== NULL_UUID) {
            Overlays.sendMouseMoveOnOverlay(laserTarget.overlayID, pointerEvent);
            Overlays.sendHoverOverOverlay(laserTarget.overlayID, pointerEvent);
        }
    }

    function sendTouchStartEventToStylusTarget(hand, laserTarget) {
        if (!laserTarget) {
            return;
        }
        
        var pointerEvent = {
            type: "Press",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "Primary",
            isPrimaryHeld: true
        };

        if (laserTarget.overlayID && laserTarget.overlayID !== NULL_UUID) {
            Overlays.sendMousePressOnOverlay(laserTarget.overlayID, pointerEvent);
        }
    }

    function sendTouchEndEventToStylusTarget(hand, laserTarget) {
        if (!laserTarget) {
            return;
        }
        var pointerEvent = {
            type: "Release",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "Primary"
        };

        if (laserTarget.overlayID && laserTarget.overlayID !== NULL_UUID) {
            Overlays.sendMouseReleaseOnOverlay(laserTarget.overlayID, pointerEvent);
        }
    }

    function sendTouchMoveEventToStylusTarget(hand, laserTarget) {
        if (!laserTarget) {
            return;
        }
        var pointerEvent = {
            type: "Move",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "Primary",
            isPrimaryHeld: true
        };
        
        if (laserTarget.overlayID && laserTarget.overlayID !== NULL_UUID) {
            Overlays.sendMouseReleaseOnOverlay(laserTarget.overlayID, pointerEvent);
        }
    }

    // will return undefined if overlayID does not exist.
    function calculateLaserTargetFromOverlay(laserTip, overlayID) {
        var overlayPosition = Overlays.getProperty(overlayID, "position");
        if (overlayPosition === undefined) {
            return;
        }

        // project stylusTip onto overlay plane.
        var overlayRotation = Overlays.getProperty(overlayID, "rotation");
        if (overlayRotation === undefined) {
            return;
        }
        var normal = Vec3.multiplyQbyV(overlayRotation, {x: 0, y: 0, z: 1});
        var distance = Vec3.dot(Vec3.subtract(laserTip, overlayPosition), normal);
        var position = Vec3.subtract(laserTip, Vec3.multiply(normal, distance));

        // calclulate normalized position
        var invRot = Quat.inverse(overlayRotation);
        var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(position, overlayPosition));
        var dpi = Overlays.getProperty(overlayID, "dpi");

        var dimensions;
        if (dpi) {
            // Calculate physical dimensions for web3d overlay from resolution and dpi; "dimensions" property
            // is used as a scale.
            var resolution = Overlays.getProperty(overlayID, "resolution");
            if (resolution === undefined) {
                return;
            }
            resolution.z = 1;  // Circumvent divide-by-zero.
            var scale = Overlays.getProperty(overlayID, "dimensions");
            if (scale === undefined) {
                return;
            }
            scale.z = 0.01;    // overlay dimensions are 2D, not 3D.
            dimensions = Vec3.multiplyVbyV(Vec3.multiply(resolution, INCHES_TO_METERS / dpi), scale);
        } else {
            dimensions = Overlays.getProperty(overlayID, "dimensions");
            if (dimensions === undefined) {
                return;
            }
            if (!dimensions.z) {
                dimensions.z = 0.01;    // sometimes overlay dimensions are 2D, not 3D.
            }
        }
        var invDimensions = { x: 1 / dimensions.x, y: 1 / dimensions.y, z: 1 / dimensions.z };
        var normalizedPosition = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), DEFAULT_REGISTRATION_POINT);

        // 2D position on overlay plane in meters, relative to the bounding box upper-left hand corner.
        var position2D = { x: normalizedPosition.x * dimensions.x,
                           y: (1 - normalizedPosition.y) * dimensions.y }; // flip y-axis

        return {
            entityID: null,
            overlayID: overlayID,
            distance: distance,
            position: position,
            position2D: position2D,
            normal: normal,
            normalizedPosition: normalizedPosition,
            dimensions: dimensions,
            valid: true
        };
    }

    function distance2D(a, b) {
        var dx = (a.x - b.x);
        var dy = (a.y - b.y);
        return Math.sqrt(dx * dx + dy * dy);
    }

    function TabletLaserInput(hand) {
        this.hand = hand;
        this.previousLaserClikcedTarget = false;
        this.laserPressingTarget = false;
        this.tabletScreenID = HMD.tabletScreenID;
        this.mode = "none";
        this.laserTargetID = null;
        this.laserTarget = null;
        this.pressEnterLaserTarget = null;
        this.hover = false;


        this.parameters = makeDispatcherModuleParameters(
            200,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.getOtherHandController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.LeftHand : Controller.Standard.RightHand;
        };

        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.stealTouchFocus = function(laserTarget) {
            this.requestTouchFocus(laserTarget);
        };

        this.requestTouchFocus = function(laserTarget) {
            sendHoverEnterEventToStylusTarget(this.hand, this.laserTarget);
        };

        this.hasTouchFocus = function(laserTarget) {
            return (this.laserTargetID === HMD.tabletScreenID);
        };

        this.relinquishTouchFocus = function() {
            // send hover leave event.
            var pointerEvent = { type: "Move", id: this.hand + 1 };
            Overlays.sendMouseMoveOnOverlay(this.tabletScreenID, pointerEvent);
            Overlays.sendHoverOverOverlay(this.tabletScreenID, pointerEvent);
            Overlays.sendHoverLeaveOverlay(this.tabletScreenID, pointerEvent);
        };

        this.updateLaserPointer = function(controllerData) {
            var RADIUS = 0.005;
            var dim = { x: RADIUS, y: RADIUS, z: RADIUS };
            
            if (this.mode === "full") {
                this.fullEnd.dimensions = dim;
                LaserPointers.editRenderState(this.laserPointer, this.mode, {path: fullPath, end: this.fullEnd});
            } else if (this.mode === "half") {
                this.halfEnd.dimensions = dim;
                LaserPointers.editRenderState(this.laserPointer, this.mode, {path: halfPath, end: this.halfEnd});
            }

            LaserPointers.enableLaserPointer(this.laserPointer);
            LaserPointers.setRenderState(this.laserPointer, this.mode);
        };

        this.processControllerTriggers = function(controllerData) {
            var rayPickIntersection = controllerData.rayPicks[this.hand].intersection;
            this.laserTarget = calculateLaserTargetFromOverlay(rayPickIntersection, HMD.tabletScreenID);
            if (controllerData.triggerClicks[this.hand]) {
                this.mode = "full";
                this.laserPressingTarget = true;
                this.hover = false;
            } else if (controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE) {
                this.mode = "half";
                this.laserPressingTarget = false;
                this.hover = true;
                this.requestTouchFocus(HMD.tabletScreenID);
            } else {
                this.mode = "none";
                this.laserPressingTarget = false;
                this.hover = false;
                this.relinquishTouchFocus();
                
            }
            this.homeButtonTouched = false;
        };

        this.hovering = function() {
            if (this.hasTouchFocus(this.laserTargetID)) {
                sendHoverOverEventToStylusTarget(this.hand, this.laserTarget);
            }
        };

        this.laserPressEnter = function () {
            this.stealTouchFocus(this.laserTarget.overlayID);
            sendTouchStartEventToStylusTarget(this.hand, this.laserTarget);
            Controller.triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION, this.hand);

            this.touchingEnterTimer = 0;
            this.pressEnterLaserTarget = this.laserTarget;
            this.deadspotExpired = false;

            var TOUCH_PRESS_TO_MOVE_DEADSPOT = 0.0381;
            this.deadspotRadius = TOUCH_PRESS_TO_MOVE_DEADSPOT;
        };

        this.laserPressExit = function () {

            if (this.laserTarget === undefined) {
                return;
            }

            // special case to handle home button.
            if (this.laserTargetID === HMD.homeButtonID) {
                Messages.sendLocalMessage("home", this.laserTargetID);
            }

            // send press event
            if (this.deadspotExpired) {
                sendTouchEndEventToStylusTarget(this.hand, this.laserTarget);
            } else {
                print(this.pressEnterLaserTarget);
                sendTouchEndEventToStylusTarget(this.hand, this.pressEnterLaserTarget);
            }
        };

        this.laserPressing = function (controllerData, dt) {

            this.touchingEnterTimer += dt;

            if (this.laserTarget) {
               
                var POINTER_PRESS_TO_MOVE_DELAY = 0.33; // seconds
                if (this.deadspotExpired || this.touchingEnterTimer > POINTER_PRESS_TO_MOVE_DELAY ||
                    distance2D(this.laserTarget.position2D,
                               this.pressEnterLaserTarget.position2D) > this.deadspotRadius) {
                    sendTouchMoveEventToStylusTarget(this.hand, this.laserTarget);
                    this.deadspotExpired = true;
                }
                
            } else {
                this.laserPressingTarget = false;
            }
        };

        this.pointingAtTablet = function(target) {
            return (target === HMD.tabletID);
        };

        this.pointingAtTabletScreen = function(target) {
            return (target === HMD.tabletScreenID);
        }

        this.pointingAtHomeButton = function(target) {
            return (target === HMD.homeButtonID);
        }

        this.isReady = function (controllerData) {
            var target = controllerData.rayPicks[this.hand].objectID;
            if (this.pointingAtTabletScreen(target) || this.pointingAtHomeButton(target) || this.pointingAtTablet(target)) {
                if (controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE) {
                    this.tabletScrenID = HMD.tabletScreenID;
                    return makeRunningValues(true, [], []);
                }
            }
            return makeRunningValues(false, [], []);
        };

        this.reset = function() {
            this.laserPressExit();
            this.hover = false;
            this.pressEnterLaserTarget = null;
            this.laserTarget = null;
            this.laserTargetID = null;
            this.laserPressingTarget = false;
            this.previousLaserClickedTarget = null;
            this.mode = "none";
        };

        this.run = function (controllerData, deltaTime) {
            var target = controllerData.rayPicks[this.hand].objectID;

            
            if (!this.pointingAtTabletScreen(target) && !this.pointingAtHomeButton(target) && !this.pointingAtTablet(target)) {
                LaserPointers.disableLaserPointer(this.laserPointer);
                this.laserPressExit();
                this.relinquishTouchFocus();
                this.reset();
                this.updateLaserPointer();
                return makeRunningValues(false, [], []);
            }

            this.processControllerTriggers(controllerData);
            this.updateLaserPointer(controllerData);
            var intersection = LaserPointers.getPrevRayPickResult(this.laserPointer);
            this.laserTargetID = intersection.objectID;
            this.laserTarget = calculateLaserTargetFromOverlay(intersection.intersection, intersection.objectID);
            if (this.laserTarget === undefined) {
                return makeRunningValues(true, [], []);
            }

            if (!this.previousLaserClickedTarget && this.laserPressingTarget) {
                this.laserPressEnter();
            }
            if (this.previousLaserClickedTarget && !this.laserPressingTarget) {
                this.laserPressExit();
            }
            this.previousLaserClickedTarget = this.laserPressingTarget;

            if (this.laserPressingTarget) {
                this.laserPressing(controllerData, deltaTime);
            }
            
            if (this.hover) {
                this.hovering();
            }

            return makeRunningValues(true, [], []);
        };

        this.cleanup = function () {
            LaserPointers.disableLaserPointer(this.laserPointer);
            LaserPointers.removeLaserPointer(this.laserPointer);
        };

        this.halfEnd = halfEnd;
        this.fullEnd = fullEnd;
        this.laserPointer = LaserPointers.createLaserPointer({
            joint: (this.hand == RIGHT_HAND) ? "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" : "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
            filter: RayPick.PICK_OVERLAYS,
            maxDistance: PICK_MAX_DISTANCE,
            posOffset: getGrabPointSphereOffset(this.handToController()),
            renderStates: renderStates,
            faceAvatar: true,
            defaultRenderStates: defaultRenderStates
        });

        LaserPointers.setIgnoreOverlays(this.laserPointer, [HMD.tabletID]);
    };
    
    var leftTabletLaserInput = new TabletLaserInput(LEFT_HAND);
    var rightTabletLaserInput = new TabletLaserInput(RIGHT_HAND);

    enableDispatcherModule("LeftTabletLaserInput", leftTabletLaserInput);
    enableDispatcherModule("RightTabletLaserInput", rightTabletLaserInput);

    this.cleanup = function () {
        leftTabletStylusInput.cleanup();
        rightTabletStylusInput.cleanup();
        disableDispatcherModule("LeftTabletLaserInput");
        disableDispatcherModule("RightTabletLaserInput");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
