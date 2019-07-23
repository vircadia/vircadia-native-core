//
//  mouseHMD.js
//
//  scripts/system/controllers/controllerModules/
//
//  Created by Dante Ruiz 2017-9-22
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Script, HMD, Reticle, Vec3, Controller */

(function() {
    var ControllerDispatcherUtils = Script.require("/~/system/libraries/controllerDispatcherUtils.js");

    function TimeLock(experation) {
        this.experation = experation;
        this.last = 0;
        this.update = function(time) {
            this.last = time || Date.now();
        };

        this.expired = function(time) {
            return ((time || Date.now()) - this.last) > this.experation;
        };
    }

    function MouseHMD() {
        var _this = this;
        this.hmdWasActive = HMD.active;
        this.mouseMoved = false;
        this.mouseActivity = new TimeLock(5000);
        this.handControllerActivity = new TimeLock(4000);
        this.parameters = ControllerDispatcherUtils.makeDispatcherModuleParameters(
            10,
            ["mouse"],
            [],
            100);

        this.onMouseMove = function() {
            _this.updateMouseActivity();
        };

        this.onMouseClick = function() {
            _this.updateMouseActivity();
        };

        this.updateMouseActivity = function(isClick) {
            if (_this.ignoreMouseActivity()) {
                return;
            }

            if (HMD.active) {
                var now = Date.now();
                _this.mouseActivity.update(now);
            }
        };

        this.adjustReticleDepth = function(controllerData) {
            if (Reticle.isPointingAtSystemOverlay(Reticle.position)) {
                var reticlePositionOnHUD = HMD.worldPointFromOverlay(Reticle.position);
                Reticle.depth = Vec3.distance(reticlePositionOnHUD, HMD.position);
            } else {
                var APPARENT_MAXIMUM_DEPTH = 100.0;
                var result = controllerData.mouseRayPointer;
                Reticle.depth = result.intersects ? result.distance : APPARENT_MAXIMUM_DEPTH;
            }
        };

        this.ignoreMouseActivity = function() {
            if (!Reticle.allowMouseCapture) {
                return true;
            }

            var pos = Reticle.position;
            if (!pos || (pos.x === -1 && pos.y === -1)) {
                return true;
            }

            if (!_this.handControllerActivity.expired()) {
                return true;
            }

            return false;
        };

        this.triggersPressed = function(controllerData, now) {
            var onValue = ControllerDispatcherUtils.TRIGGER_ON_VALUE;
            var rightHand = ControllerDispatcherUtils.RIGHT_HAND;
            var leftHand = ControllerDispatcherUtils.LEFT_HAND;
            var leftTriggerValue = controllerData.triggerValues[leftHand];
            var rightTriggerValue = controllerData.triggerValues[rightHand];

            if (leftTriggerValue > onValue || rightTriggerValue > onValue) {
                this.handControllerActivity.update(now);
                return true;
            }

            return false;
        };

        this.isReady = function(controllerData, deltaTime) {
            var now = Date.now();
            var hmdChanged = this.hmdWasActive !== HMD.active;
            this.hmdWasActive = HMD.active;
            this.triggersPressed(controllerData, now);
            if (HMD.active) {
                if (!this.mouseActivity.expired(now) && _this.handControllerActivity.expired()) {
                    Reticle.visible = true;
                    return ControllerDispatcherUtils.makeRunningValues(true, [], []);
                } else {
                    Reticle.visible = false;
                }
            } else if (hmdChanged && !Reticle.visible) {
                Reticle.visible = true;
            }

            return ControllerDispatcherUtils.makeRunningValues(false, [], []);
        };

        this.run = function(controllerData, deltaTime) {
            var now = Date.now();
            var hmdActive = HMD.active;
            if (this.mouseActivity.expired(now) || this.triggersPressed(controllerData, now) || !hmdActive) {
                if (!hmdActive) {
                    Reticle.visible = true;
                } else {
                    Reticle.visible = false;
                }

                return ControllerDispatcherUtils.makeRunningValues(false, [], []);
            }
            this.adjustReticleDepth(controllerData);
            return ControllerDispatcherUtils.makeRunningValues(true, [], []);
        };
    }

    var mouseHMD = new MouseHMD();
    ControllerDispatcherUtils.enableDispatcherModule("MouseHMD", mouseHMD);

    Controller.mouseMoveEvent.connect(mouseHMD.onMouseMove);
    Controller.mousePressEvent.connect(mouseHMD.onMouseClick);

    function cleanup() {
        ControllerDispatcherUtils.disableDispatcherModule("MouseHMD");
    }

    Script.scriptEnding.connect(cleanup);
})();
