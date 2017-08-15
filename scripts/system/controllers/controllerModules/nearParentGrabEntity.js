"use strict";

//  nearParentGrabEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND, AVATAR_SELF_ID,
   getControllerJointIndex, NULL_UUID, enableDispatcherModule, disableDispatcherModule,
   propsArePhysical, Messages, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, TRIGGER_OFF_VALUE,
   makeDispatcherModuleParameters, entityIsGrabbable, makeRunningValues, NEAR_GRAB_RADIUS
*/

Script.include("/~/system/controllers/controllerDispatcherUtils.js");

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

        this.parameters = makeDispatcherModuleParameters(
            500,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);


        // XXX does handJointIndex change if the avatar changes?
        this.handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
        this.controllerJointIndex = getControllerJointIndex(this.hand);

        this.thisHandIsParent = function(props) {
            if (props.parentID !== MyAvatar.sessionUUID && props.parentID !== AVATAR_SELF_ID) {
                return false;
            }

            var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            if (props.parentJointIndex == handJointIndex) {
                return true;
            }

            var controllerJointIndex = this.controllerJointIndex;
            if (props.parentJointIndex == controllerJointIndex) {
                return true;
            }

            var controllerCRJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ?
                                                                "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" :
                                                                "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND");
            if (props.parentJointIndex == controllerCRJointIndex) {
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
                velocity: {x: 0, y: 0, z: 0},
                angularVelocity: {x: 0, y: 0, z: 0}
            };

            if (this.thisHandIsParent(targetProps)) {
                // this should never happen, but if it does, don't set previous parent to be this hand.
                // this.previousParentID[targetProps.id] = NULL;
                // this.previousParentJointIndex[targetProps.id] = -1;
            } else {
                this.previousParentID[targetProps.id] = targetProps.parentID;
                this.previousParentJointIndex[targetProps.id] = targetProps.parentJointIndex;
            }
            Entities.editEntity(targetProps.id, reparentProps);

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: targetProps.id,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
        };

        this.endNearParentingGrabEntity = function () {
            if (this.previousParentID[this.targetEntityID] === NULL_UUID) {
                Entities.editEntity(this.targetEntityID, {
                    parentID: this.previousParentID[this.targetEntityID],
                    parentJointIndex: this.previousParentJointIndex[this.targetEntityID]
                });
            } else {
                // we're putting this back as a child of some other parent, so zero its velocity
                Entities.editEntity(this.targetEntityID, {
                    parentID: this.previousParentID[this.targetEntityID],
                    parentJointIndex: this.previousParentJointIndex[this.targetEntityID],
                    velocity: {x: 0, y: 0, z: 0},
                    angularVelocity: {x: 0, y: 0, z: 0}
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
            for (var i = 0; i < nearbyEntityProperties.length; i++) {
                var props = nearbyEntityProperties[i];
                var handPosition = controllerData.controllerLocations[this.hand].position;
                var distance = Vec3.distance(props.position, handPosition);
                if (distance > NEAR_GRAB_RADIUS) {
                    break;
                }
                if (entityIsGrabbable(props)) {
                    return props;
                }
            }
            return null;
        };

        this.isReady = function (controllerData) {
            this.targetEntityID = null;
            this.grabbing = false;

            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE) {
                makeRunningValues(false, [], []);
            }

            var targetProps = this.getTargetProps(controllerData);
            if (targetProps) {
                if (propsArePhysical(targetProps)) {
                    // XXX make sure no highlights are enabled from this module
                    return makeRunningValues(false, [], []); // let nearActionGrabEntity handle it
                } else {
                    this.targetEntityID = targetProps.id;
                    // XXX highlight this.targetEntityID here
                    return makeRunningValues(true, [this.targetEntityID], []);
                }
            } else {
                // XXX make sure no highlights are enabled from this module
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData) {
            if (this.grabbing) {
                if (controllerData.triggerClicks[this.hand] == 0) {
                    this.endNearParentingGrabEntity();
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
                if (controllerData.triggerClicks[this.hand] == 1) {
                    // stop highlighting, switch to grabbing
                    // XXX stop highlight here
                    var targetProps = this.getTargetProps(controllerData);
                    if (targetProps) {
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
