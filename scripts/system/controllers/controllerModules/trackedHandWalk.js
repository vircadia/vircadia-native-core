"use strict";

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, RIGHT_HAND, LEFT_HAND, makeRunningValues, enableDispatcherModule, disableDispatcherModule,
   makeDispatcherModuleParameters, handsAreTracked, Controller, Vec3
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {

    function TrackedHandWalk(hand) {
        this.mappingName = 'hand-track-walk-' + Math.random();
        this.inputMapping = Controller.newMapping(this.mappingName);
        this.leftIndexPos = null;
        this.rightIndexPos = null;
        this.pinchOnBelowDistance = 0.016;
        this.pinchOffAboveDistance = 0.04;
        this.walking = false;

        this.parameters = makeDispatcherModuleParameters(
            80,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.updateWalking = function () {
            if (this.leftIndexPos && this.rightIndexPos) {
                var tipDistance = Vec3.distance(this.leftIndexPos, this.rightIndexPos);
                if (tipDistance < this.pinchOnBelowDistance) {
                    this.walking = true;
                } else if (this.walking && tipDistance > this.pinchOffAboveDistance) {
                    this.walking = false;
                }
            }
        };

        this.leftIndexChanged = function (pose) {
            if (pose.valid) {
                this.leftIndexPos = pose.translation;
            } else {
                this.leftIndexPos = null;
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

        this.isReady = function (controllerData) {
            if (!handsAreTracked()) {
                return makeRunningValues(false, [], []);
            }
            if (this.walking) {
                return makeRunningValues(true, [], []);
            }
        };

        this.run = function (controllerData) {
            return this.isReady(controllerData);
        };

        this.setup = function () {
            var _this = this;
            this.inputMapping.from(Controller.Standard.LeftHandIndex4).peek().to(function (pose) {
                _this.leftIndexChanged(pose);
            });
            this.inputMapping.from(Controller.Standard.RightHandIndex4).peek().to(function (pose) {
                _this.rightIndexChanged(pose);
            });

            this.inputMapping.from(function() {
                if (_this.walking) {
                    return -1;
                } else {
                    return Controller.getActionValue(Controller.Standard.TranslateZ);
                }
            }).to(Controller.Actions.TranslateZ);

            Controller.enableMapping(this.mappingName);
        };

        this.cleanUp = function () {
            this.inputMapping.disable();
        };
    }

    var trackedHandWalk = new TrackedHandWalk(LEFT_HAND);
    trackedHandWalk.setup();
    enableDispatcherModule("TrackedHandWalk", trackedHandWalk);

    function cleanup() {
        trackedHandWalk.cleanUp();
        disableDispatcherModule("TrackedHandWalk");
    }
    Script.scriptEnding.connect(cleanup);
}());
