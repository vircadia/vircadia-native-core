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

(function() {
    var ControllerDispatcherUtils = Script.require("/~/system/libraries/controllerDispatcherUtils.js");

    var WEIGHTING = 1 / 20; // simple moving average over last 20 samples
    var ONE_MINUS_WEIGHTING = 1 - WEIGHTING;
    var AVERAGE_MOUSE_VELOCITY_FOR_SEEK_TO = 20;
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

        this.ignoreMouseActivity = function() {
            if (!Reticle.allowMouseCapture) {
                return true;
            }

            var pos = Reticle.position;
            if (!pos || (pos.x == -1 && pos.y == -1)) {
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
            this.triggersPressed(controllerData, now);
            if ((HMD.active && !this.mouseActivity.expired(now)) && _this.handControllerActivity.expired()) {
                Reticle.visible = true;
                return ControllerDispatcherUtils.makeRunningValues(true, [], []);
            }
            if (HMD.active) {
                Reticle.visble = false;
            }

            return ControllerDispatcherUtils.makeRunningValues(false, [], []);
        };
        
        this.run = function(controllerData, deltaTime) {
            var now = Date.now();
            if (this.mouseActivity.expired(now) || this.triggersPressed(controllerData, now)) {
                Reticle.visible = false;
                return ControllerDispatcherUtils.makeRunningValues(false, [], []);
            }
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
