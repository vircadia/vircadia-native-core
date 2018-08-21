"use strict";

//  nearActionGrabEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND,
   getControllerJointIndex, getGrabbableData, enableDispatcherModule, disableDispatcherModule,
   propsArePhysical, Messages, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, entityIsGrabbable,
   Quat, Vec3, MSECS_PER_SEC, getControllerWorldLocation, makeDispatcherModuleParameters, makeRunningValues,
   TRIGGER_OFF_VALUE, NEAR_GRAB_RADIUS, findGroupParent, entityIsCloneable, propsAreCloneDynamic, cloneEntity,
   HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, BUMPER_ON_VALUE, unhighlightTargetEntity, Uuid
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");
Script.include("/~/system/libraries/cloneEntityUtils.js");

(function() {

    function NearActionGrabEntity(hand) {
        this.hand = hand;
        this.targetEntityID = null;
        this.actionID = null; // action this script created...
        this.hapticTargetID = null;

        this.parameters = makeDispatcherModuleParameters(
            140,
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


        this.startNearGrabAction = function (controllerData, targetProps) {
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

            var grabbableData = getGrabbableData(targetProps);
            this.ignoreIK = grabbableData.ignoreIK;
            this.kinematicGrab = grabbableData.kinematic;

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

            var objectRotation = targetProps.rotation;
            this.offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

            var currentObjectPosition = targetProps.position;
            var offset = Vec3.subtract(currentObjectPosition, handPosition);
            this.offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, this.offsetRotation)), offset);

            var now = Date.now();
            this.actionTimeout = now + (ACTION_TTL * MSECS_PER_SEC);

            if (this.actionID) {
                Entities.deleteAction(this.targetEntityID, this.actionID);
            }
            this.actionID = Entities.addAction("hold", this.targetEntityID, {
                hand: this.hand === RIGHT_HAND ? "right" : "left",
                timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
                relativePosition: this.offsetPosition,
                relativeRotation: this.offsetRotation,
                ttl: ACTION_TTL,
                kinematic: this.kinematicGrab,
                kinematicSetVelocity: true,
                ignoreIK: this.ignoreIK
            });
            if (this.actionID === Uuid.NULL) {
                this.actionID = null;
                return;
            }

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: this.targetEntityID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "startNearGrab", args);
            unhighlightTargetEntity(this.targetEntityID);
            var message = {
                hand: this.hand,
                entityID: this.targetEntityID
            };

            Messages.sendLocalMessage('Hifi-unhighlight-entity', JSON.stringify(message));
        };

        // this is for when the action is going to time-out
        this.refreshNearGrabAction = function (controllerData) {
            var now = Date.now();
            if (this.actionID && this.actionTimeout - now < ACTION_TTL_REFRESH * MSECS_PER_SEC) {
                // if less than a 5 seconds left, refresh the actions ttl
                var success = Entities.updateAction(this.targetEntityID, this.actionID, {
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
                }
            }
        };

        this.endNearGrabAction = function () {
            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "releaseGrab", args);

            Entities.deleteAction(this.targetEntityID, this.actionID);
            this.actionID = null;

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'release',
                grabbedEntity: this.targetEntityID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));

            this.targetEntityID = null;
        };

        this.getTargetProps = function (controllerData) {
            // nearbyEntityProperties is already sorted by distance from controller
            var nearbyEntityProperties = controllerData.nearbyEntityProperties[this.hand];
            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            for (var i = 0; i < nearbyEntityProperties.length; i++) {
                var props = nearbyEntityProperties[i];
                if (props.distance > NEAR_GRAB_RADIUS * sensorScaleFactor) {
                    break;
                }
                if (entityIsGrabbable(props) || entityIsCloneable(props)) {
                    if (props.id !== this.hapticTargetID) {
                        Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);
                        this.hapticTargetID = props.id;
                    }
                    if (!entityIsCloneable(props)) {
                        // if we've attempted to grab a non-cloneable child, roll up to the root of the tree
                        var groupRootProps = findGroupParent(controllerData, props);
                        if (entityIsGrabbable(groupRootProps)) {
                            return groupRootProps;
                        }
                    }
                    return props;
                }
            }
            return null;
        };

        this.isReady = function (controllerData) {
            this.targetEntityID = null;

            var targetProps = this.getTargetProps(controllerData);
            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE &&
                controllerData.secondaryValues[this.hand] < TRIGGER_OFF_VALUE) {
                return makeRunningValues(false, [], []);
            }

            if (targetProps) {
                if ((!propsArePhysical(targetProps) && !propsAreCloneDynamic(targetProps)) ||
                    targetProps.parentID !== Uuid.NULL) {
                    return makeRunningValues(false, [], []); // let nearParentGrabEntity handle it
                } else {
                    this.targetEntityID = targetProps.id;
                    return makeRunningValues(true, [this.targetEntityID], []);
                }
            } else {
                this.hapticTargetID = null;
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData) {
            if (this.actionID) {
                if (controllerData.triggerClicks[this.hand] < TRIGGER_OFF_VALUE &&
                    controllerData.secondaryValues[this.hand] < TRIGGER_OFF_VALUE) {
                    this.endNearGrabAction();
                    this.hapticTargetID = null;
                    return makeRunningValues(false, [], []);
                }

                this.refreshNearGrabAction(controllerData);
                var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
                Entities.callEntityMethod(this.targetEntityID, "continueNearGrab", args);
            } else {

                // still searching / highlighting
                var readiness = this.isReady (controllerData);
                if (!readiness.active) {
                    return readiness;
                }

                var targetProps = this.getTargetProps(controllerData);
                if (targetProps) {
                    if (controllerData.triggerClicks[this.hand] ||
                        controllerData.secondaryValues[this.hand] > BUMPER_ON_VALUE) {
                        // switch to grabbing
                        var targetCloneable = entityIsCloneable(targetProps);
                        if (targetCloneable) {
                            var cloneID = cloneEntity(targetProps);
                            var cloneProps = Entities.getEntityProperties(cloneID);
                            this.targetEntityID = cloneID;
                            this.startNearGrabAction(controllerData, cloneProps);
                        } else {
                            this.startNearGrabAction(controllerData, targetProps);
                        }
                    }
                }
            }

            return makeRunningValues(true, [this.targetEntityID], []);
        };

        this.cleanup = function () {
            if (this.targetEntityID) {
                this.endNearGrabAction();
            }
        };
    }

    var leftNearActionGrabEntity = new NearActionGrabEntity(LEFT_HAND);
    var rightNearActionGrabEntity = new NearActionGrabEntity(RIGHT_HAND);

    enableDispatcherModule("LeftNearActionGrabEntity", leftNearActionGrabEntity);
    enableDispatcherModule("RightNearActionGrabEntity", rightNearActionGrabEntity);

    function cleanup() {
        leftNearActionGrabEntity.cleanup();
        rightNearActionGrabEntity.cleanup();
        disableDispatcherModule("LeftNearActionGrabEntity");
        disableDispatcherModule("RightNearActionGrabEntity");
    }
    Script.scriptEnding.connect(cleanup);
}());
