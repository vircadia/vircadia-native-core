"use strict";

//  nearParentGrabEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND, AVATAR_SELF_ID, getControllerJointIndex, NULL_UUID,
   enableDispatcherModule, disableDispatcherModule, propsArePhysical, Messages, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION,
   TRIGGER_OFF_VALUE, makeDispatcherModuleParameters, entityIsGrabbable, makeRunningValues, NEAR_GRAB_RADIUS, findGroupParent,
   Vec3, cloneEntity, entityIsCloneable, propsAreCloneDynamic, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, BUMPER_ON_VALUE
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/cloneEntityUtils.js");

(function() {

    // XXX this.ignoreIK = (grabbableData.ignoreIK !== undefined) ? grabbableData.ignoreIK : true;
    // XXX this.kinematicGrab = (grabbableData.kinematic !== undefined) ? grabbableData.kinematic : NEAR_GRABBING_KINEMATIC;

    function NearParentingGrabEntity(hand) {
        this.hand = hand;
        this.targetEntityID = null;
        this.grabbing = false;
        this.previousParentID = {};
        this.previousParentJointIndex = {};
        this.previouslyUnhooked = {};
        this.hapticTargetID = null;

        this.parameters = makeDispatcherModuleParameters(
            500,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);


        // XXX does handJointIndex change if the avatar changes?
        this.handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
        this.controllerJointIndex = getControllerJointIndex(this.hand);

        this.getOtherModule = function() {
            return (this.hand === RIGHT_HAND) ? leftNearParentingGrabEntity : rightNearParentingGrabEntity;
        };

        this.otherHandIsParent = function(props) {
            return this.getOtherModule().thisHandIsParent(props);
        };

        this.thisHandIsParent = function(props) {
            if (props.parentID !== MyAvatar.sessionUUID && props.parentID !== AVATAR_SELF_ID) {
                return false;
            }

            var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            if (props.parentJointIndex === handJointIndex) {
                return true;
            }

            var controllerJointIndex = this.controllerJointIndex;
            if (props.parentJointIndex === controllerJointIndex) {
                return true;
            }

            var controllerCRJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ?
                "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" :
                "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND");

            if (props.parentJointIndex === controllerCRJointIndex) {
                return true;
            }

            return false;
        };

        this.startNearParentingGrabEntity = function (controllerData, targetProps) {
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

            var handJointIndex;
            // if (this.ignoreIK) {
            //     handJointIndex = this.controllerJointIndex;
            // } else {
            //     handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            // }
            handJointIndex = this.controllerJointIndex;

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(targetProps.id, "startNearGrab", args);

            var reparentProps = {
                parentID: AVATAR_SELF_ID,
                parentJointIndex: handJointIndex,
                localVelocity: {x: 0, y: 0, z: 0},
                localAngularVelocity: {x: 0, y: 0, z: 0}
            };

            if (this.thisHandIsParent(targetProps)) {
                // this should never happen, but if it does, don't set previous parent to be this hand.
                // this.previousParentID[targetProps.id] = NULL;
                // this.previousParentJointIndex[targetProps.id] = -1;
            } else if (this.otherHandIsParent(targetProps)) {
                // the other hand is parent. Steal the object and information
                var otherModule = this.getOtherModule();
                this.previousParentID[targetProps.id] = otherModule.previousParentID[targetProps.id];
                this.previousParentJointIndex[targetProps.id] = otherModule.previousParentJointIndex[targetProps.id];
                otherModule.endNearParentingGrabEntity();
            } else {
                this.previousParentID[targetProps.id] = targetProps.parentID;
                this.previousParentJointIndex[targetProps.id] = targetProps.parentJointIndex;
            }

            this.targetEntityID = targetProps.id;
            Entities.editEntity(targetProps.id, reparentProps);

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: targetProps.id,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
            this.grabbing = true;
        };

        this.endNearParentingGrabEntity = function () {
            if (this.previousParentID[this.targetEntityID] === NULL_UUID || this.previousParentID === undefined) {
                Entities.editEntity(this.targetEntityID, {
                    parentID: this.previousParentID[this.targetEntityID],
                    parentJointIndex: this.previousParentJointIndex[this.targetEntityID]
                });
            } else {
                // we're putting this back as a child of some other parent, so zero its velocity
                Entities.editEntity(this.targetEntityID, {
                    parentID: this.previousParentID[this.targetEntityID],
                    parentJointIndex: this.previousParentJointIndex[this.targetEntityID],
                    localVelocity: {x: 0, y: 0, z: 0},
                    localAngularVelocity: {x: 0, y: 0, z: 0}
                });
            }

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "releaseGrab", args);
            this.grabbing = false;
            this.targetEntityID = null;
        };

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
                if (entityIsGrabbable(props)) {
                    // give haptic feedback
                    if (props.id !== this.hapticTargetID) {
                        Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);
                        this.hapticTargetID = props.id;
                    }
                    // if we've attempted to grab a child, roll up to the root of the tree
                    var groupRootProps = findGroupParent(controllerData, props);
                    if (entityIsGrabbable(groupRootProps)) {
                        return groupRootProps;
                    }
                    return props;
                }
            }
            return null;
        };

        this.isReady = function (controllerData, deltaTime) {
            this.targetEntityID = null;
            this.grabbing = false;

            var targetProps = this.getTargetProps(controllerData);
            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE &&
                controllerData.secondaryValues[this.hand] < TRIGGER_OFF_VALUE) {
                return makeRunningValues(false, [], []);
            }

            if (targetProps) {
                if (propsArePhysical(targetProps) || propsAreCloneDynamic(targetProps)) {
                    return makeRunningValues(false, [], []); // let nearActionGrabEntity handle it
                } else {
                    this.targetEntityID = targetProps.id;
                    return makeRunningValues(true, [this.targetEntityID], []);
                }
            } else {
                this.hapticTargetID = null;
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData, deltaTime) {
            if (this.grabbing) {
                if (controllerData.triggerClicks[this.hand] < TRIGGER_OFF_VALUE &&
                    controllerData.secondaryValues[this.hand] <  TRIGGER_OFF_VALUE) {
                    this.endNearParentingGrabEntity();
                    this.hapticTargetID = null;
                    return makeRunningValues(false, [], []);
                }

                var props = Entities.getEntityProperties(this.targetEntityID);
                if (!this.thisHandIsParent(props)) {
                    this.grabbing = false;
                    this.targetEntityID = null;
                    this.hapticTargetID = null;
                    return makeRunningValues(false, [], []);
                }

                var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
                Entities.callEntityMethod(this.targetEntityID, "continueNearGrab", args);
            } else {
                // still searching / highlighting
                var readiness = this.isReady (controllerData);
                if (!readiness.active) {
                    return readiness;
                }
                if (controllerData.triggerClicks[this.hand] || controllerData.secondaryValues[this.hand] > BUMPER_ON_VALUE) {
                    // switch to grab
                    var targetProps = this.getTargetProps(controllerData);
                    var targetCloneable = entityIsCloneable(targetProps);

                    if (targetCloneable) {
                        var worldEntityProps = controllerData.nearbyEntityProperties[this.hand];
                        var cloneID = cloneEntity(targetProps, worldEntityProps);
                        var cloneProps = Entities.getEntityProperties(cloneID);

                        this.grabbing = true;
                        this.targetEntityID = cloneID;
                        this.startNearParentingGrabEntity(controllerData, cloneProps);

                    } else if (targetProps) {
                        this.grabbing = true;
                        this.startNearParentingGrabEntity(controllerData, targetProps);
                    }
                }
            }

            return makeRunningValues(true, [this.targetEntityID], []);
        };

        this.cleanup = function () {
            if (this.targetEntityID) {
                this.endNearParentingGrabEntity();
            }
        };
    }

    var leftNearParentingGrabEntity = new NearParentingGrabEntity(LEFT_HAND);
    var rightNearParentingGrabEntity = new NearParentingGrabEntity(RIGHT_HAND);

    enableDispatcherModule("LeftNearParentingGrabEntity", leftNearParentingGrabEntity);
    enableDispatcherModule("RightNearParentingGrabEntity", rightNearParentingGrabEntity);

    this.cleanup = function () {
        leftNearParentingGrabEntity.cleanup();
        rightNearParentingGrabEntity.cleanup();
        disableDispatcherModule("LeftNearParentingGrabEntity");
        disableDispatcherModule("RightNearParentingGrabEntity");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
