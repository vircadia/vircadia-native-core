"use strict";

//  nearParentGrabEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND, AVATAR_SELF_ID,
   getControllerJointIndex, NULL_UUID, enableDispatcherModule, disableDispatcherModule,
   propsArePhysical, Messages, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION,
   makeDispatcherModuleParameters, entityIsGrabbable
*/

Script.include("/~/system/controllers/controllerDispatcherUtils.js");

(function() {

    // XXX this.ignoreIK = (grabbableData.ignoreIK !== undefined) ? grabbableData.ignoreIK : true;
    // XXX this.kinematicGrab = (grabbableData.kinematic !== undefined) ? grabbableData.kinematic : NEAR_GRABBING_KINEMATIC;

    function NearParentingGrabEntity(hand) {
        this.hand = hand;
        this.grabbedThingID = null;
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

        this.startNearParentingGrabEntity = function (controllerData, grabbedProperties) {
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

            var handJointIndex;
            // if (this.ignoreIK) {
            //     handJointIndex = this.controllerJointIndex;
            // } else {
            //     handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            // }
            handJointIndex = this.controllerJointIndex;

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.grabbedThingID, "startNearGrab", args);

            var reparentProps = {
                parentID: AVATAR_SELF_ID,
                parentJointIndex: handJointIndex,
                velocity: {x: 0, y: 0, z: 0},
                angularVelocity: {x: 0, y: 0, z: 0}
            };

            if (this.thisHandIsParent(grabbedProperties)) {
                // this should never happen, but if it does, don't set previous parent to be this hand.
                // this.previousParentID[this.grabbedThingID] = NULL;
                // this.previousParentJointIndex[this.grabbedThingID] = -1;
            } else {
                this.previousParentID[this.grabbedThingID] = grabbedProperties.parentID;
                this.previousParentJointIndex[this.grabbedThingID] = grabbedProperties.parentJointIndex;
            }
            Entities.editEntity(this.grabbedThingID, reparentProps);

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: this.grabbedThingID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
        };

        this.endNearParentingGrabEntity = function (controllerData) {
            if (this.previousParentID[this.grabbedThingID] === NULL_UUID) {
                Entities.editEntity(this.grabbedThingID, {
                    parentID: this.previousParentID[this.grabbedThingID],
                    parentJointIndex: this.previousParentJointIndex[this.grabbedThingID]
                });
            } else {
                // we're putting this back as a child of some other parent, so zero its velocity
                Entities.editEntity(this.grabbedThingID, {
                    parentID: this.previousParentID[this.grabbedThingID],
                    parentJointIndex: this.previousParentJointIndex[this.grabbedThingID],
                    velocity: {x: 0, y: 0, z: 0},
                    angularVelocity: {x: 0, y: 0, z: 0}
                });
            }

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.grabbedThingID, "releaseGrab", args);

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
                if (propsArePhysical(grabbedProperties)) {
                    return false; // let nearActionGrabEntity handle it
                }
                this.grabbedThingID = grabbedProperties.id;
                this.startNearParentingGrabEntity(controllerData, grabbedProperties);
                return true;
            } else {
                return false;
            }
        };

        this.run = function (controllerData) {
            if (controllerData.triggerClicks[this.hand] == 0) {
                this.endNearParentingGrabEntity(controllerData);
                return false;
            }

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.grabbedThingID, "continueNearGrab", args);

            return true;
        };
    }

    enableDispatcherModule("LeftNearParentingGrabEntity", new NearParentingGrabEntity(LEFT_HAND));
    enableDispatcherModule("RightNearParentingGrabEntity", new NearParentingGrabEntity(RIGHT_HAND));

    this.cleanup = function () {
        disableDispatcherModule("LeftNearParentingGrabEntity");
        disableDispatcherModule("RightNearParentingGrabEntity");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
