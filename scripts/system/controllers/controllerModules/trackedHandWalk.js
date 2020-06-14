"use strict";

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, makeRunningValues, enableDispatcherModule, disableDispatcherModule,
   makeDispatcherModuleParameters, handsAreTracked, Controller, Vec3
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {

    function TrackedHandWalk() {
        this.gestureMappingName = 'hand-track-walk-gesture-' + Math.random();
        this.inputGestureMapping = Controller.newMapping(this.gestureMappingName);
        this.mappingName = 'hand-track-walk-' + Math.random();
        this.inputMapping = Controller.newMapping(this.mappingName);
        this.leftIndexPos = null;
        this.leftThumbPos = null;
        this.rightIndexPos = null;
        this.rightThumbPos = null;
        this.touchOnBelowDistance = 0.016;
        this.touchOffAboveDistance = 0.045;
        this.walkingForward = false;
        this.walkingBackward = false;

        this.mappingEnabled = false;

        this.parameters = makeDispatcherModuleParameters(
            80,
            ["rightHand", "leftHand"],
            [],
            100);

        this.getControlPoint = function () {
            return Vec3.multiply(Vec3.sum(this.leftIndexPos, this.rightIndexPos), 0.5);
        };

        this.updateWalking = function () {
            if (this.leftIndexPos && this.rightIndexPos) {
                var indexTipDistance = Vec3.distance(this.leftIndexPos, this.rightIndexPos);
                if (indexTipDistance < this.touchOnBelowDistance) {
                    this.walkingForward = true;
                    this.controlPoint = this.getControlPoint();
                } else if (this.walkingForward && indexTipDistance > this.touchOffAboveDistance) {
                    this.walkingForward = false;
                } // else don't change walkingForward
            }

            if (this.leftThumbPos && this.rightThumbPos) {
                var thumbTipDistance = Vec3.distance(this.leftThumbPos, this.rightThumbPos);
                if (thumbTipDistance < this.touchOnBelowDistance) {
                    this.walkingBackward = true;
                    this.controlPoint = this.getControlPoint();
                } else if (this.walkingBackward && thumbTipDistance > this.touchOffAboveDistance) {
                    this.walkingBackward = false;
                } // else don't change this.walkingBackward
            }

            if ((this.walkingForward || this.walkingBackward) && !this.mappingEnabled) {
                Controller.enableMapping(this.mappingName);
                this.mappingEnabled = true;
            } else if (!(this.walkingForward || this.walkingBackward) && this.mappingEnabled) {
                this.inputMapping.disable();
                this.mappingEnabled = false;
            } // else don't change mappingEnabled
        };

        this.leftIndexChanged = function (pose) {
            if (pose.valid) {
                this.leftIndexPos = pose.translation;
            } else {
                this.leftIndexPos = null;
            }
            this.updateWalking();
        };

        this.leftThumbChanged = function (pose) {
            if (pose.valid) {
                this.leftThumbPos = pose.translation;
            } else {
                this.leftThumbPos = null;
            }
            this.updateWalking();
        };

        this.rightIndexChanged = function (pose) {
            if (pose.valid) {
                this.rightIndexPos = pose.translation;
            } else {
                this.rightIndexPos = null;
            }
            this.updateWalking();
        };

        this.rightThumbChanged = function (pose) {
            if (pose.valid) {
                this.rightThumbPos = pose.translation;
            } else {
                this.rightThumbPos = null;
            }
            this.updateWalking();
        };

        this.isReady = function (controllerData) {
            return makeRunningValues(handsAreTracked() && (this.walkingForward || this.walkingBackward), [], []);
        };

        this.run = function (controllerData) {
            return this.isReady(controllerData);
        };

        this.setup = function () {
            var _this = this;
            this.inputGestureMapping.from(Controller.Standard.LeftHandIndex4).peek().to(function (pose) {
                _this.leftIndexChanged(pose);
            });
            this.inputGestureMapping.from(Controller.Standard.LeftHandThumb4).peek().to(function (pose) {
                _this.leftThumbChanged(pose);
            });
            this.inputGestureMapping.from(Controller.Standard.RightHandIndex4).peek().to(function (pose) {
                _this.rightIndexChanged(pose);
            });
            this.inputGestureMapping.from(Controller.Standard.RightHandThumb4).peek().to(function (pose) {
                _this.rightThumbChanged(pose);
            });

            this.inputMapping.from(function() {
                if (_this.walkingForward) {
                    // var currentPoint = _this.getControlPoint();
                    // return currentPoint.z - _this.controlPoint.z;
                    return -0.5;
                } else if (_this.walkingBackward) {
                    // var currentPoint = _this.getControlPoint();
                    // return currentPoint.z - _this.controlPoint.z;
                    return 0.5;
                } else {
                    // return Controller.getActionValue(Controller.Standard.TranslateZ);
                    return 0.0;
                }
            }).to(Controller.Actions.TranslateZ);

            // this.inputMapping.from(function() {
            //     if (_this.walkingForward) {
            //         var currentPoint = _this.getControlPoint();
            //         return currentPoint.x - _this.controlPoint.x;
            //     } else {
            //         return Controller.getActionValue(Controller.Standard.Yaw);
            //     }
            // }).to(Controller.Actions.Yaw);

            Controller.enableMapping(this.gestureMappingName);
        };

        this.cleanUp = function () {
            this.inputGestureMapping.disable();
            this.inputMapping.disable();
        };
    }

    var trackedHandWalk = new TrackedHandWalk();
    trackedHandWalk.setup();
    enableDispatcherModule("TrackedHandWalk", trackedHandWalk);

    function cleanup() {
        trackedHandWalk.cleanUp();
        disableDispatcherModule("TrackedHandWalk");
    }
    Script.scriptEnding.connect(cleanup);
}());
