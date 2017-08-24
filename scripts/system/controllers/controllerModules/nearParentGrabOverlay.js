"use strict";

//  nearParentGrabOverlay.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND, AVATAR_SELF_ID,
   getControllerJointIndex, NULL_UUID, enableDispatcherModule, disableDispatcherModule,
   Messages, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION,
   makeDispatcherModuleParameters, Overlays, makeRunningValues
*/

Script.include("/~/system/controllers/controllerDispatcherUtils.js");
var GRAB_RADIUS = 0.35;

(function() {

    // XXX this.ignoreIK = (grabbableData.ignoreIK !== undefined) ? grabbableData.ignoreIK : true;
    // XXX this.kinematicGrab = (grabbableData.kinematic !== undefined) ? grabbableData.kinematic : NEAR_GRABBING_KINEMATIC;

    function NearParentingGrabOverlay(hand) {
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

        this.startNearParentingGrabOverlay = function (controllerData) {
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

            var handJointIndex;
            // if (this.ignoreIK) {
            //     handJointIndex = this.controllerJointIndex;
            // } else {
            //     handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            // }
            handJointIndex = this.controllerJointIndex;

            var grabbedProperties = {
                position: Overlays.getProperty(this.grabbedThingID, "position"),
                rotation: Overlays.getProperty(this.grabbedThingID, "rotation"),
                parentID: Overlays.getProperty(this.grabbedThingID, "parentID"),
                parentJointIndex: Overlays.getProperty(this.grabbedThingID, "parentJointIndex"),
                dynamic: false,
                shapeType: "none"
            };
            
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
            Overlays.editOverlay(this.grabbedThingID, reparentProps);

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: this.grabbedThingID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
        };

        this.endNearParentingGrabOverlay = function () {
            if (this.previousParentID[this.grabbedThingID] === NULL_UUID) {
                Overlays.editOverlay(this.grabbedThingID, {
                    parentID: NULL_UUID,
                    parentJointIndex: -1
                });
            } else {
                // before we grabbed it, overlay was a child of something; put it back.
                Overlays.editOverlay(this.grabbedThingID, {
                    parentID: this.previousParentID[this.grabbedThingID],
                    parentJointIndex: this.previousParentJointIndex[this.grabbedThingID],
                });
            }

            this.grabbedThingID = null;
        };

        this.getTargetID = function(overlays, controllerData) {
            for (var i = 0; i < overlays.length; i++) {
                var overlayPosition = Overlays.getProperty(overlays[i], "position");
                var handPosition = controllerData.controllerLocations[this.hand].position;
                var distance = Vec3.distance(overlayPosition, handPosition);
                if (distance <= GRAB_RADIUS) {
                    return overlays[i];
                }
            }
            return null;
        };
            

        this.isReady = function (controllerData) {
            if (controllerData.triggerClicks[this.hand] == 0) {
                return makeRunningValues(false, [], []);
            }

            this.grabbedThingID = null;

            var candidateOverlays = controllerData.nearbyOverlayIDs[this.hand];
            var grabbableOverlays = candidateOverlays.filter(function(overlayID) {
                return Overlays.getProperty(overlayID, "grabbable");
            });

            var targetID = this.getTargetID(grabbableOverlays, controllerData);
            if (targetID) {
                this.grabbedThingID = targetID;
                this.startNearParentingGrabOverlay(controllerData);
                return makeRunningValues(true, [this.grabbedThingID], []);
            } else {
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData) {
            if (controllerData.triggerClicks[this.hand] == 0) {
                this.endNearParentingGrabOverlay();
                return makeRunningValues(false, [], []);
            } else {
                return makeRunningValues(true, [this.grabbedThingID], []);
            }
        };

        this.cleanup = function () {
            if (this.grabbedThingID) {
                this.endNearParentingGrabOverlay();
            }
        };
    }

    var leftNearParentingGrabOverlay = new NearParentingGrabOverlay(LEFT_HAND);
    var rightNearParentingGrabOverlay = new NearParentingGrabOverlay(RIGHT_HAND);

    enableDispatcherModule("LeftNearParentingGrabOverlay", leftNearParentingGrabOverlay);
    enableDispatcherModule("RightNearParentingGrabOverlay", rightNearParentingGrabOverlay);

    this.cleanup = function () {
        leftNearParentingGrabOverlay.cleanup();
        rightNearParentingGrabOverlay.cleanup();
        disableDispatcherModule("LeftNearParentingGrabOverlay");
        disableDispatcherModule("RightNearParentingGrabOverlay");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
