"use strict";

//  nearTrigger.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, MyAvatar, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule, getGrabbableData,
   Vec3, TRIGGER_OFF_VALUE, makeDispatcherModuleParameters, makeRunningValues, NEAR_GRAB_RADIUS, unhighlightTargetEntity
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");

(function() {

    function entityWantsNearTrigger(props) {
        var grabbableData = getGrabbableData(props);
        return grabbableData.triggerable || grabbableData.wantsTrigger;
    }

    function NearTriggerEntity(hand) {
        this.hand = hand;
        this.targetEntityID = null;
        this.grabbing = false;
        this.previousParentID = {};
        this.previousParentJointIndex = {};
        this.previouslyUnhooked = {};
        this.startSent = false;

        this.parameters = makeDispatcherModuleParameters(
            120,
            this.hand === RIGHT_HAND ? ["rightHandTrigger", "rightHand"] : ["leftHandTrigger", "leftHand"],
            [],
            100);

        this.getTargetProps = function (controllerData) {
            // nearbyEntityProperties is already sorted by length from controller
            var nearbyEntityProperties = controllerData.nearbyEntityProperties[this.hand];
            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            for (var i = 0; i < nearbyEntityProperties.length; i++) {
                var props = nearbyEntityProperties[i];
                var handPosition = controllerData.controllerLocations[this.hand].position;
                var distance = Vec3.distance(props.position, handPosition);
                if (distance > NEAR_GRAB_RADIUS * sensorScaleFactor) {
                    continue;
                }
                if (entityWantsNearTrigger(props)) {
                    return props;
                }
            }
            return null;
        };

        this.startNearTrigger = function (controllerData) {
            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "startNearTrigger", args);
            unhighlightTargetEntity(this.targetEntityID);
        };

        this.continueNearTrigger = function (controllerData) {
            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "continueNearTrigger", args);
        };

        this.endNearTrigger = function (controllerData) {
            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "stopNearTrigger", args);
        };

        this.isReady = function (controllerData) {
            this.targetEntityID = null;

            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE) {
                return makeRunningValues(false, [], []);
            }

            var targetProps = this.getTargetProps(controllerData);
            if (targetProps) {
                this.targetEntityID = targetProps.id;
                return makeRunningValues(true, [this.targetEntityID], []);
            } else {
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData) {
            if (!this.startSent) {
                this.startNearTrigger(controllerData);
                this.startSent = true;
            } else if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE) {
                this.endNearTrigger(controllerData);
                this.startSent = false;
                return makeRunningValues(false, [], []);
            } else {
                this.continueNearTrigger(controllerData);
            }
            return makeRunningValues(true, [this.targetEntityID], []);
        };

        this.cleanup = function () {
            if (this.targetEntityID) {
                this.endNearTrigger();
            }
        };
    }

    var leftNearTriggerEntity = new NearTriggerEntity(LEFT_HAND);
    var rightNearTriggerEntity = new NearTriggerEntity(RIGHT_HAND);

    enableDispatcherModule("LeftNearTriggerEntity", leftNearTriggerEntity);
    enableDispatcherModule("RightNearTriggerEntity", rightNearTriggerEntity);

    function cleanup() {
        leftNearTriggerEntity.cleanup();
        rightNearTriggerEntity.cleanup();
        disableDispatcherModule("LeftNearTriggerEntity");
        disableDispatcherModule("RightNearTriggerEntity");
    }
    Script.scriptEnding.connect(cleanup);
}());
