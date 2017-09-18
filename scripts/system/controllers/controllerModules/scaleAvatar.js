//  handControllerGrab.js
//
//  Created by Dante Ruiz on  9/11/17
//
//  Grabs physically moveable entities with hydra-like controllers; it works for either near or far objects.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Vec3, MyAvatar, RIGHT_HAND */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function () {
    var dispatcherUtils = Script.require("/~/system/libraries/controllerDispatcherUtils.js");

    function ScaleAvatar(hand) {
        this.hand = hand;
        this.scalingStartAvatarScale = 0;
        this.scalingStartDistance = 0;

        this.parameters = dispatcherUtils.makeDispatcherModuleParameters(
            120,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100
        );

        this.otherHand = function() {
            return this.hand === dispatcherUtils.RIGHT_HAND ? dispatcherUtils.LEFT_HAND : dispatcherUtils.RIGHT_HAND;
        };

        this.getOtherModule = function() {
            var otherModule = this.hand === dispatcherUtils.RIGHT_HAND ? leftScaleAvatar : rightScaleAvatar;
            return otherModule;
        };

        this.triggersPressed = function(controllerData) {
            if (controllerData.triggerClicks[this.hand] &&
                controllerData.secondaryValues[this.hand] > dispatcherUtils.BUMPER_ON_VALUE) {
                return true;
            }
            return false;
        };

        this.isReady = function(controllerData) {
            var otherModule = this.getOtherModule();
            if (this.triggersPressed(controllerData) && otherModule.triggersPressed(controllerData)) {
                this.scalingStartAvatarScale = MyAvatar.scale;
                this.scalingStartDistance = Vec3.length(Vec3.subtract(controllerData.controllerLocations[this.hand].position,
                    controllerData.controllerLocations[this.otherHand()].position));
                return dispatcherUtils.makeRunningValues(true, [], []);
            }
            return dispatcherUtils.makeRunningValues(false, [], []);
        };

        this.run = function(controllerData) {
            var otherModule = this.getOtherModule();
            if (this.triggersPressed(controllerData) && otherModule.triggersPressed(controllerData)) {
                if (this.hand === dispatcherUtils.RIGHT_HAND) {
                    var scalingCurrentDistance =
                        Vec3.length(Vec3.subtract(controllerData.controllerLocations[this.hand].position,
                        controllerData.controllerLocations[this.otherHand()].position));

                    var newAvatarScale = (scalingCurrentDistance / this.scalingStartDistance) * this.scalingStartAvatarScale;
                    MyAvatar.scale = newAvatarScale;
                }
                return dispatcherUtils.makeRunningValues(true, [], []);
            }
            return dispatcherUtils.makeRunningValues(false, [], []);
        };
    }

    var leftScaleAvatar = new ScaleAvatar(dispatcherUtils.LEFT_HAND);
    var rightScaleAvatar = new ScaleAvatar(dispatcherUtils.RIGHT_HAND);

    dispatcherUtils.enableDispatcherModule("LeftScaleAvatar", leftScaleAvatar);
    dispatcherUtils.enableDispatcherModule("RightScaleAvatar", rightScaleAvatar);

    this.cleanup = function() {
        dispatcherUtils.disableDispatcherModule("LeftScaleAvatar");
        dispatcherUtils.disableDispatcherModule("RightScaleAvatar");
    };
})();
