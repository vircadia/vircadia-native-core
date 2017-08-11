"use strict";

//  nearActionGrabEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND,
   getControllerJointIndex, getGrabbableData, NULL_UUID, enableDispatcherModule, disableDispatcherModule,
   propsArePhysical, Messages, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, entityIsGrabbable,
   Quat, Vec3, MSECS_PER_SEC, getControllerWorldLocation, makeDispatcherModuleParameters
*/

Script.include("/~/system/controllers/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {

    function NearActionGrabEntity(hand) {
        this.hand = hand;
        this.grabbedThingID = null;
        this.actionID = null; // action this script created...

        this.parameters = makeDispatcherModuleParameters(
            500,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        var NEAR_GRABBING_ACTION_TIMEFRAME = 0.05; // how quickly objects move to their new position
        var ACTION_TTL = 15; // seconds
        var ACTION_TTL_REFRESH = 5;

        // XXX does handJointIndex change if the avatar changes?
        this.handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
        this.controllerJointIndex = getControllerJointIndex(this.hand);


        // handPosition is where the avatar's hand appears to be, in-world.
        this.getHandPosition = function () {
            if (this.hand === RIGHT_HAND) {
                return MyAvatar.getRightPalmPosition();
            } else {
                return MyAvatar.getLeftPalmPosition();
            }
        };

        this.getHandRotation = function () {
            if (this.hand === RIGHT_HAND) {
                return MyAvatar.getRightPalmRotation();
            } else {
                return MyAvatar.getLeftPalmRotation();
            }
        };


        this.startNearGrabAction = function (controllerData, grabbedProperties) {
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

            var grabbableData = getGrabbableData(grabbedProperties);
            this.ignoreIK = grabbableData.ignoreIK;
            this.kinematicGrab = grabbableData.kinematicGrab;

            var handRotation;
            var handPosition;
            if (this.ignoreIK) {
                var controllerID =
                    (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
                var controllerLocation = getControllerWorldLocation(controllerID, false);
                handRotation = controllerLocation.orientation;
                handPosition = controllerLocation.position;
            } else {
                handRotation = this.getHandRotation();
                handPosition = this.getHandPosition();
            }

            var objectRotation = grabbedProperties.rotation;
            this.offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

            var currentObjectPosition = grabbedProperties.position;
            var offset = Vec3.subtract(currentObjectPosition, handPosition);
            this.offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, this.offsetRotation)), offset);

            var now = Date.now();
            this.actionTimeout = now + (ACTION_TTL * MSECS_PER_SEC);

            if (this.actionID) {
                Entities.deleteAction(this.grabbedThingID, this.actionID);
            }
            this.actionID = Entities.addAction("hold", this.grabbedThingID, {
                hand: this.hand === RIGHT_HAND ? "right" : "left",
                timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
                relativePosition: this.offsetPosition,
                relativeRotation: this.offsetRotation,
                ttl: ACTION_TTL,
                kinematic: this.kinematicGrab,
                kinematicSetVelocity: true,
                ignoreIK: this.ignoreIK
            });
            if (this.actionID === NULL_UUID) {
                this.actionID = null;
                return;
            }

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: this.grabbedThingID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
        };

        // this is for when the action creation failed, before
        this.restartNearGrabAction = function (controllerData) {
            var props = Entities.getEntityProperties(this.grabbedThingID, ["position", "rotation", "userData"]);
            if (props && entityIsGrabbable(props)) {
                this.startNearGrabAction(controllerData, props);
            }
        };

        // this is for when the action is going to time-out
        this.refreshNearGrabAction = function (controllerData) {
            var now = Date.now();
            if (this.actionID && this.actionTimeout - now < ACTION_TTL_REFRESH * MSECS_PER_SEC) {
                // if less than a 5 seconds left, refresh the actions ttl
                var success = Entities.updateAction(this.grabbedThingID, this.actionID, {
                    hand: this.hand === RIGHT_HAND ? "right" : "left",
                    timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
                    relativePosition: this.offsetPosition,
                    relativeRotation: this.offsetRotation,
                    ttl: ACTION_TTL,
                    kinematic: this.kinematicGrab,
                    kinematicSetVelocity: true,
                    ignoreIK: this.ignoreIK
                });
                if (success) {
                    this.actionTimeout = now + (ACTION_TTL * MSECS_PER_SEC);
                } else {
                    print("continueNearGrabbing -- updateAction failed");
                    this.restartNearGrabAction(controllerData);
                }
            }
        };

        this.endNearGrabAction = function (controllerData) {
            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.grabbedThingID, "releaseGrab", args);

            Entities.deleteAction(this.grabbedThingID, this.actionID);
            this.actionID = null;

            this.grabbedThingID = null;
        };

        this.isReady = function (controllerData) {
            if (controllerData.triggerClicks[this.hand] == 0) {
                return false;
            }

            var grabbedProperties = null;
            // nearbyEntityProperties is already sorted by length from controller
            var nearbyEntityProperties = controllerData.nearbyEntityProperties[this.hand];
            for (var i = 0; i < nearbyEntityProperties.length; i++) {
                var props = nearbyEntityProperties[i];
                if (entityIsGrabbable(props)) {
                    grabbedProperties = props;
                    break;
                }
            }

            if (grabbedProperties) {
                if (!propsArePhysical(grabbedProperties)) {
                    return false; // let nearParentGrabEntity handle it
                }
                this.grabbedThingID = grabbedProperties.id;
                this.startNearGrabAction(controllerData, grabbedProperties);
                return true;
            } else {
                return false;
            }
        };

        this.run = function (controllerData) {
            if (controllerData.triggerClicks[this.hand] == 0) {
                this.endNearGrabAction(controllerData);
                return false;
            }

            if (!this.actionID) {
                this.restartNearGrabAction(controllerData);
            }

            this.refreshNearGrabAction(controllerData);

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.grabbedThingID, "continueNearGrab", args);

            return true;
        };
    }

    enableDispatcherModule("LeftNearActionGrabEntity", new NearActionGrabEntity(LEFT_HAND));
    enableDispatcherModule("RightNearActionGrabEntity", new NearActionGrabEntity(RIGHT_HAND));

    this.cleanup = function () {
        disableDispatcherModule("LeftNearActionGrabEntity");
        disableDispatcherModule("RightNearActionGrabEntity");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
