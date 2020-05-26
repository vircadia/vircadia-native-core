"use strict";

//  nearGrabEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND, getControllerJointIndex, enableDispatcherModule,
   disableDispatcherModule, Messages, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, TRIGGER_OFF_VALUE,
   makeDispatcherModuleParameters, entityIsGrabbable, makeRunningValues, NEAR_GRAB_RADIUS, findGrabbableGroupParent, Vec3,
   cloneEntity, entityIsCloneable, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, BUMPER_ON_VALUE,
   distanceBetweenPointAndEntityBoundingBox, getGrabbableData, getEnabledModuleByName, DISPATCHER_PROPERTIES, HMD,
   NEAR_GRAB_DISTANCE
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/cloneEntityUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {

    function NearGrabEntity(hand) {
        this.hand = hand;
        this.targetEntityID = null;
        this.grabbing = false;
        this.cloneAllowed = true;
        this.grabID = null;

        this.parameters = makeDispatcherModuleParameters(
            500,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.startGrab = function (targetProps) {
            if (this.grabID) {
                MyAvatar.releaseGrab(this.grabID);
            }

            var grabData = getGrabbableData(targetProps);

            var handJointIndex;
            if (HMD.mounted && HMD.isHandControllerAvailable() && grabData.grabFollowsController) {
                handJointIndex = getControllerJointIndex(this.hand);
            } else {
                handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            }

            this.targetEntityID = targetProps.id;

            var relativePosition = Entities.worldToLocalPosition(targetProps.position, MyAvatar.SELF_ID, handJointIndex);
            var relativeRotation = Entities.worldToLocalRotation(targetProps.rotation, MyAvatar.SELF_ID, handJointIndex);
            this.grabID = MyAvatar.grab(targetProps.id, handJointIndex, relativePosition, relativeRotation);
        };

        this.startNearGrabEntity = function (targetProps) {
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

            this.startGrab(targetProps);

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(targetProps.id, "startNearGrab", args);

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: targetProps.id,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));

            this.grabbing = true;
        };

        this.endGrab = function () {
            if (this.grabID) {
                MyAvatar.releaseGrab(this.grabID);
                this.grabID = null;
            }
        };

        this.endNearGrabEntity = function () {
            this.endGrab();

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "releaseGrab", args);
            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'release',
                grabbedEntity: this.targetEntityID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));

            this.grabbing = false;
            this.targetEntityID = null;
        };

        this.getTargetProps = function (controllerData) {
            // nearbyEntityProperties is already sorted by length from controller
            var nearbyEntityProperties = controllerData.nearbyEntityProperties[this.hand];
            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            var nearGrabDistance = NEAR_GRAB_DISTANCE * sensorScaleFactor;
            var nearGrabRadius = NEAR_GRAB_RADIUS * sensorScaleFactor;
            for (var i = 0; i < nearbyEntityProperties.length; i++) {
                var props = nearbyEntityProperties[i];
                var grabPosition = controllerData.controllerLocations[this.hand].position; // Is offset from hand position.
                var dist = distanceBetweenPointAndEntityBoundingBox(grabPosition, props);
                var distance = Vec3.distance(grabPosition, props.position);
                if ((dist > nearGrabDistance) ||
                    (distance > nearGrabRadius)) { // Only smallish entities can be near grabbed.
                    continue;
                }
                if (entityIsGrabbable(props) || entityIsCloneable(props)) {
                    if (!entityIsCloneable(props)) {
                        // if we've attempted to grab a non-cloneable child, roll up to the root of the tree
                        var groupRootProps = findGrabbableGroupParent(controllerData, props);
                        if (entityIsGrabbable(groupRootProps)) {
                            return groupRootProps;
                        }
                    }
                    return props;
                }
            }
            return null;
        };

        this.isReady = function (controllerData, deltaTime) {
            this.targetEntityID = null;
            this.grabbing = false;

            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE &&
                controllerData.secondaryValues[this.hand] < TRIGGER_OFF_VALUE) {
                this.cloneAllowed = true;
                return makeRunningValues(false, [], []);
            }

            var scaleModuleName = this.hand === RIGHT_HAND ? "RightScaleEntity" : "LeftScaleEntity";
            var scaleModule = getEnabledModuleByName(scaleModuleName);
            if (scaleModule && (scaleModule.grabbedThingID || scaleModule.isReady(controllerData).active)) {
                // we're rescaling -- don't start a grab.
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

        this.run = function (controllerData, deltaTime) {

            if (this.grabbing) {
                if (!controllerData.triggerClicks[this.hand] &&
                    controllerData.secondaryValues[this.hand] < TRIGGER_OFF_VALUE) {
                    this.endNearGrabEntity();
                    return makeRunningValues(false, [], []);
                }

                var props = controllerData.nearbyEntityPropertiesByID[this.targetEntityID];
                if (!props) {
                    props = Entities.getEntityProperties(this.targetEntityID, DISPATCHER_PROPERTIES);
                    if (!props) {
                        // entity was deleted
                        this.grabbing = false;
                        this.targetEntityID = null;
                        return makeRunningValues(false, [], []);
                    }
                }

                var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
                Entities.callEntityMethod(this.targetEntityID, "continueNearGrab", args);
            } else {
                // still searching
                var readiness = this.isReady(controllerData);
                if (!readiness.active) {
                    return readiness;
                }
                if (controllerData.triggerClicks[this.hand] || controllerData.secondaryValues[this.hand] > BUMPER_ON_VALUE) {
                    // switch to grab
                    var targetProps = this.getTargetProps(controllerData);
                    var targetCloneable = entityIsCloneable(targetProps);

                    if (targetCloneable) {
                        if (this.cloneAllowed) {
                            var cloneID = cloneEntity(targetProps);
                            if (cloneID !== null) {
                                var cloneProps = Entities.getEntityProperties(cloneID, DISPATCHER_PROPERTIES);
                                cloneProps.id = cloneID;
                                this.grabbing = true;
                                this.targetEntityID = cloneID;
                                this.startNearGrabEntity(cloneProps);
                                this.cloneAllowed = false; // prevent another clone call until inputs released
                            }
                        }
                    } else if (targetProps) {
                        this.grabbing = true;
                        this.startNearGrabEntity(targetProps);
                    }
                }
            }

            return makeRunningValues(true, [this.targetEntityID], []);
        };

        this.cleanup = function () {
            if (this.targetEntityID) {
                this.endNearGrabEntity();
            }
        };
    }

    var leftNearGrabEntity = new NearGrabEntity(LEFT_HAND);
    var rightNearGrabEntity = new NearGrabEntity(RIGHT_HAND);

    enableDispatcherModule("LeftNearGrabEntity", leftNearGrabEntity);
    enableDispatcherModule("RightNearGrabEntity", rightNearGrabEntity);

    function cleanup() {
        leftNearGrabEntity.cleanup();
        rightNearGrabEntity.cleanup();
        disableDispatcherModule("LeftNearGrabEntity");
        disableDispatcherModule("RightNearGrabEntity");
    }
    Script.scriptEnding.connect(cleanup);
}());
