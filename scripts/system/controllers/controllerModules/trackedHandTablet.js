"use strict";

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, makeRunningValues, enableDispatcherModule, disableDispatcherModule,
   makeDispatcherModuleParameters, handsAreTracked, Controller, Vec3, Tablet, HMD, MyAvatar
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {

    function TrackedHandTablet() {
        this.mappingName = 'hand-track-tablet-' + Math.random();
        this.inputMapping = Controller.newMapping(this.mappingName);
        this.leftIndexPos = null;
        this.leftThumbPos = null;
        this.rightIndexPos = null;
        this.rightThumbPos = null;
        this.touchOnBelowDistance = 0.016;
        this.touchOffAboveDistance = 0.045;

        this.gestureCompleted = false;
        this.previousGestureCompleted = false;

        this.parameters = makeDispatcherModuleParameters(
            70,
            ["rightHand", "leftHand"],
            [],
            100);

        this.checkForGesture = function () {
            if (this.leftThumbPos && this.leftIndexPos && this.rightThumbPos && this.rightIndexPos) {
                var leftTipDistance = Vec3.distance(this.leftThumbPos, this.leftIndexPos);
                var rightTipDistance = Vec3.distance(this.rightThumbPos, this.rightIndexPos);
                if (leftTipDistance < this.touchOnBelowDistance && rightTipDistance < this.touchOnBelowDistance) {
                    this.gestureCompleted = true;
                } else if (leftTipDistance > this.touchOffAboveDistance || rightTipDistance > this.touchOffAboveDistance) {
                    this.gestureCompleted = false;
                } // else don't change gestureCompleted
            } else {
                this.gestureCompleted = false;
            }

            if (this.gestureCompleted && !this.previousGestureCompleted) {
                var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
                if (HMD.showTablet) {
                    HMD.closeTablet(false);
                } else if (!HMD.showTablet && !tablet.toolbarMode && !MyAvatar.isAway) {
                    tablet.gotoHomeScreen();
                    HMD.openTablet(false);
                }
            }

            this.previousGestureCompleted = this.gestureCompleted;
        };

        this.leftIndexChanged = function (pose) {
            if (pose.valid) {
                this.leftIndexPos = pose.translation;
            } else {
                this.leftIndexPos = null;
            }
            this.checkForGesture();
        };

        this.leftThumbChanged = function (pose) {
            if (pose.valid) {
                this.leftThumbPos = pose.translation;
            } else {
                this.leftThumbPos = null;
            }
            this.checkForGesture();
        };

        this.rightIndexChanged = function (pose) {
            if (pose.valid) {
                this.rightIndexPos = pose.translation;
            } else {
                this.rightIndexPos = null;
            }
            this.checkForGesture();
        };

        this.rightThumbChanged = function (pose) {
            if (pose.valid) {
                this.rightThumbPos = pose.translation;
            } else {
                this.rightThumbPos = null;
            }
            this.checkForGesture();
        };

        this.isReady = function (controllerData) {
            return makeRunningValues(handsAreTracked() && this.gestureCompleted, [], []);
        };

        this.run = function (controllerData) {
            return this.isReady(controllerData);
        };

        this.setup = function () {
            var _this = this;
            this.inputMapping.from(Controller.Standard.LeftHandIndex4).peek().to(function (pose) {
                _this.leftIndexChanged(pose);
            });
            this.inputMapping.from(Controller.Standard.LeftHandThumb4).peek().to(function (pose) {
                _this.leftThumbChanged(pose);
            });
            this.inputMapping.from(Controller.Standard.RightHandIndex4).peek().to(function (pose) {
                _this.rightIndexChanged(pose);
            });
            this.inputMapping.from(Controller.Standard.RightHandThumb4).peek().to(function (pose) {
                _this.rightThumbChanged(pose);
            });

            Controller.enableMapping(this.mappingName);
        };

        this.cleanUp = function () {
            this.inputMapping.disable();
        };
    }

    var trackedHandWalk = new TrackedHandTablet();
    trackedHandWalk.setup();
    enableDispatcherModule("TrackedHandTablet", trackedHandWalk);

    function cleanup() {
        trackedHandWalk.cleanUp();
        disableDispatcherModule("TrackedHandTablet");
    }
    Script.scriptEnding.connect(cleanup);
}());
