"use strict";

//  stylusInput.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND,
   enableDispatcherModule, disableDispatcherModule, makeRunningValues,
   Messages, Quat, Vec3, getControllerWorldLocation, makeDispatcherModuleParameters, Overlays, ZERO_VEC,
   HMD, INCHES_TO_METERS, DEFAULT_REGISTRATION_POINT, Settings, getGrabPointSphereOffset,
   getEnabledModuleByName
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
    var TouchEventUtils = Script.require("/~/system/libraries/touchEventUtils.js");
    function isNearStylusTarget(stylusTargets, maxNormalDistance) {
        var stylusTargetIDs = [];
        for (var index = 0; index < stylusTargets.length; index++) {
            var stylusTarget = stylusTargets[index];
            if (stylusTarget.distance <= maxNormalDistance) {
                stylusTargetIDs.push(stylusTarget.id);
            }
        }
        return stylusTargetIDs;
    }

    function getOverlayDistance(controllerPosition, overlayID) {
        var position = Overlays.getProperty(overlayID, "position");
        return {
            id: overlayID,
            distance: Vec3.distance(position, controllerPosition)
        };
    }

	function TabletStylusInput(hand) {
        this.hand = hand;

        this.parameters = makeDispatcherModuleParameters(
            100,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.pointer = Pointers.createPointer(PickType.Stylus, {
            hand: this.hand,
            filter: Picks.PICK_OVERLAYS,
            hover: true,
            enabled: true
        });

        this.disable = false;

        this.otherModuleNeedsToRun = function(controllerData) {
            var grabOverlayModuleName = this.hand === RIGHT_HAND ? "RightNearParentingGrabOverlay" : "LeftNearParentingGrabOverlay";
            var grabOverlayModule = getEnabledModuleByName(grabOverlayModuleName);
            var grabOverlayModuleReady = grabOverlayModule ? grabOverlayModule.isReady(controllerData) : makeRunningValues(false, [], []);
            var farGrabModuleName = this.hand === RIGHT_HAND ? "RightFarActionGrabEntity" : "LeftFarActionGrabEntity";
            var farGrabModule = getEnabledModuleByName(farGrabModuleName);
            var farGrabModuleReady = farGrabModule ? farGrabModule.isReady(controllerData) : makeRunningValues(false, [], []);
            return grabOverlayModuleReady.active || farGrabModuleReady.active;
        };

        this.overlayLaserActive = function(controllerData) {
            var rightOverlayLaserModule = getEnabledModuleByName("RightOverlayLaserInput");
            var leftOverlayLaserModule = getEnabledModuleByName("LeftOverlayLaserInput");
            var rightModuleRunning = rightOverlayLaserModule ? rightOverlayLaserModule.isReady(controllerData).active : false;
            var leftModuleRunning = leftOverlayLaserModule ? leftOverlayLaserModule.isReady(controllerData).active : false;
            return leftModuleRunning || rightModuleRunning;
        };

        this.processStylus = function(controllerData) {
            if (this.overlayLaserActive(controllerData) || this.otherModuleNeedsToRun(controllerData)) {
                Pointers.setRenderState(this.pointer, "disabled");
                return false;
            }

            var sensorScaleFactor = MyAvatar.sensorToWorldScale;

            // build list of stylus targets, near the stylusTip
            var stylusTargets = [];
            var candidateEntities = controllerData.nearbyEntityProperties;
            var candidateOverlays = controllerData.nearbyOverlayIDs;
            var controllerPosition = controllerData.controllerLocations[this.hand].position;
            var i, props, stylusTarget;

            /*for (i = 0; i < candidateEntities.length; i++) {
                props = candidateEntities[i];
                if (props && props.type === "Web") {
                    stylusTarget = TouchEventUtils.calculateTouchTargetFromEntity({position: controllerPosition}, candidateEntities[i]);
                    if (stylusTarget) {
                        stylusTargets.push(stylusTarget);
                    }
                }
            }*/


            for (i = 0; i < candidateOverlays.length; i++) {
                if (candidateOverlays[i] !== HMD.tabletID &&
                    Overlays.getProperty(candidateOverlays[i], "visible")) {
                    stylusTarget = getOverlayDistance(controllerPosition, candidateOverlays[i]);
                    if (stylusTarget) {
                        stylusTargets.push(stylusTarget);
                    }
                }
            }



            // add the tabletScreen, if it is valid
            if (HMD.tabletScreenID && HMD.tabletScreenID !== Uuid.NULL &&
                Overlays.getProperty(HMD.tabletScreenID, "visible")) {
                stylusTarget = getOverlayDistance(controllerPosition, HMD.tabletScreenID);
                if (stylusTarget) {
                    stylusTargets.push(stylusTarget);
                }
            }

            // add the tablet home button.
            if (HMD.homeButtonID && HMD.homeButtonID !== Uuid.NULL &&
                Overlays.getProperty(HMD.homeButtonID, "visible")) {
                stylusTarget = getOverlayDistance(controllerPosition, HMD.homeButtonID);
                if (stylusTarget) {
                    stylusTargets.push(stylusTarget);
                }
            }

            var WEB_DISPLAY_STYLUS_DISTANCE = 0.5;
            var nearStylusTarget = isNearStylusTarget(stylusTargets, WEB_DISPLAY_STYLUS_DISTANCE * sensorScaleFactor);

            if (nearStylusTarget.length !== 0) {
                print(this.hand + "---" + this.disable);
                if (!this.disable) {
                    Pointers.setRenderState(this.pointer,"events on");
                    Pointers.setIncludeItems(this.pointer, nearStylusTarget);
                } else {
                    Pointers.setRenderState(this.pointer,"events off");
                }
                return true;
            } else {
                Pointers.setRenderState(this.pointer, "disabled");
                Pointers.setIncludeItems(this.pointer, []);
                return false;
            }

           // var nearestStylusTarget = calculateNearestStylusTarget(stylusTargets);
        };

        this.isReady = function (controllerData) {
            if (this.processStylus(controllerData)) {
                return makeRunningValues(true, [], []);
            } else {
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData, deltaTime) {
            return this.isReady(controllerData);
        };
 
        this.cleanup = function () {
        	//Pointers.createPointer(this.pointer);
        };
    }

    function mouseHoverEnter(overlayID, event) {
        if (event.id === leftTabletStylusInput.pointer && !rightTabletStylusInput.disable && !leftTabletStylusInput.disable) {
            rightTabletStylusInput.disable = true;
        } else if (event.id === rightTabletStylusInput.pointer && !leftTabletStylusInput.disable && !rightTabletStylusInput.disable) {
            leftTabletStylusInput.disable = true;
        }
    }

    function mouseHoverLeave(overlayID, event) {
        if (event.id === leftTabletStylusInput.pointer) {
            rightTabletStylusInput.disable = false;
        } else if (event.id === rightTabletStylusInput.pointer) {
            leftTabletStylusInput.disable = false;
        }
    }

    var leftTabletStylusInput = new TabletStylusInput(LEFT_HAND);
    var rightTabletStylusInput = new TabletStylusInput(RIGHT_HAND);

    enableDispatcherModule("LeftTabletStylusInput", leftTabletStylusInput);
    enableDispatcherModule("RightTabletStylusInput", rightTabletStylusInput);

    Overlays.hoverEnterOverlay.connect(mouseHoverEnter);
    Overlays.hoverLeaveOverlay.connect(mouseHoverLeave);

    this.cleanup = function () {
        leftTabletStylusInput.cleanup();
        rightTabletStylusInput.cleanup();
        disableDispatcherModule("LeftTabletStylusInput");
        disableDispatcherModule("RightTabletStylusInput");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
