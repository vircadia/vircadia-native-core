"use strict";

//  tabletStylusInput.js
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
	function TabletStylusInput(hand) {
        this.hand = hand;

        this.parameters = makeDispatcherModuleParameters(
            100,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);
        
        this.pointer = Pointers.createPointer(PickType.Stylus, { hand: this.hand });

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
            var rightModuleRunning = rightOverlayLaserModule ? !rightOverlayLaserModule.shouldExit(controllerData) : false;
            var leftModuleRunning = leftOverlayLaserModule ? !leftOverlayLaserModule.shouldExit(controllerData) : false;
            return leftModuleRunning || rightModuleRunning;
        };

        this.isReady = function (controllerData) {
            if (!this.overlayLaserActive(controllerData) && !this.otherModuleNeedsToRun(controllerData)) {
                return makeRunningValues(true, [], []);
            } else {
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData, deltaTime) {
            return this.isReady(controllerData);
        };
 
        this.cleanup = function () {
        	Pointers.createPointer(this.pointer);
        };
    }

    var leftTabletStylusInput = new TabletStylusInput(LEFT_HAND);
    var rightTabletStylusInput = new TabletStylusInput(RIGHT_HAND);

    enableDispatcherModule("LeftTabletStylusInput", leftTabletStylusInput);
    enableDispatcherModule("RightTabletStylusInput", rightTabletStylusInput);

    this.cleanup = function () {
        leftTabletStylusInput.cleanup();
        rightTabletStylusInput.cleanup();
        disableDispatcherModule("LeftTabletStylusInput");
        disableDispatcherModule("RightTabletStylusInput");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
