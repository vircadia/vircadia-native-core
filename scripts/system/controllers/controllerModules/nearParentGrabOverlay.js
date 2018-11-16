"use strict";

//  nearParentGrabOverlay.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND, getControllerJointIndex,
   enableDispatcherModule, disableDispatcherModule, Messages, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION,
   makeDispatcherModuleParameters, Overlays, makeRunningValues, Vec3, resizeTablet, getTabletWidthFromSettings,
   NEAR_GRAB_RADIUS, HMD, Uuid
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/utils.js");

(function() {

    // XXX this.ignoreIK = (grabbableData.ignoreIK !== undefined) ? grabbableData.ignoreIK : true;
    // XXX this.kinematicGrab = (grabbableData.kinematic !== undefined) ? grabbableData.kinematic : NEAR_GRABBING_KINEMATIC;

    function NearParentingGrabOverlay(hand) {
        this.hand = hand;
        this.grabbedThingID = null;
        this.previousParentID = {};
        this.previousParentJointIndex = {};
        this.previouslyUnhooked = {};
        this.robbed = false;

        this.parameters = makeDispatcherModuleParameters(
            90,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);


        // XXX does handJointIndex change if the avatar changes?
        this.handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");

        this.getOtherModule = function() {
            return (this.hand === RIGHT_HAND) ? leftNearParentingGrabOverlay : rightNearParentingGrabOverlay;
        };

        this.otherHandIsParent = function(props) {
            return this.getOtherModule().thisHandIsParent(props);
        };

        this.isGrabbedThingVisible = function() {
            return Overlays.getProperty(this.grabbedThingID, "visible");
        };

        this.thisHandIsParent = function(props) {
            if (props.parentID !== MyAvatar.sessionUUID && props.parentID !== MyAvatar.SELF_ID) {
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

        this.getGrabbedProperties = function() {
            return {
                position: Overlays.getProperty(this.grabbedThingID, "position"),
                rotation: Overlays.getProperty(this.grabbedThingID, "rotation"),
                parentID: Overlays.getProperty(this.grabbedThingID, "parentID"),
                parentJointIndex: Overlays.getProperty(this.grabbedThingID, "parentJointIndex"),
                dynamic: false,
                shapeType: "none"
            };
        };


        this.startNearParentingGrabOverlay = function (controllerData) {
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

            this.controllerJointIndex = getControllerJointIndex(this.hand);
            var handJointIndex = this.controllerJointIndex;

            var grabbedProperties = this.getGrabbedProperties();

            var reparentProps = {
                parentID: MyAvatar.SELF_ID,
                parentJointIndex: handJointIndex,
                velocity: {x: 0, y: 0, z: 0},
                angularVelocity: {x: 0, y: 0, z: 0}
            };

            if (this.thisHandIsParent(grabbedProperties)) {
                // this should never happen, but if it does, don't set previous parent to be this hand.
                // this.previousParentID[this.grabbedThingID] = NULL;
                // this.previousParentJointIndex[this.grabbedThingID] = -1;
            } else if (this.otherHandIsParent(grabbedProperties)) {
                // the other hand is parent. Steal the object and information
                var otherModule = this.getOtherModule();
                this.previousParentID[this.grabbedThingID] = otherModule.previousParentID[this.grabbedThingID];
                this.previousParentJointIndex[this.grabbedThingID] = otherModule.previousParentJointIndex[this.grabbedThingID];
                otherModule.robbed = true;
            } else {
                this.previousParentID[this.grabbedThingID] = grabbedProperties.parentID;
                this.previousParentJointIndex[this.grabbedThingID] = grabbedProperties.parentJointIndex;
            }
            Overlays.editOverlay(this.grabbedThingID, reparentProps);

            // resizeTablet to counter adjust offsets to account for change of scale from sensorToWorldMatrix
            if (HMD.tabletID && this.grabbedThingID === HMD.tabletID) {
                resizeTablet(getTabletWidthFromSettings(), reparentProps.parentJointIndex);
            }

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: this.grabbedThingID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
        };

        this.endNearParentingGrabOverlay = function () {
            var previousParentID = this.previousParentID[this.grabbedThingID];
            if ((previousParentID === Uuid.NULL || previousParentID === null) && !this.robbed) {
                Overlays.editOverlay(this.grabbedThingID, {
                    parentID: Uuid.NULL,
                    parentJointIndex: -1
                });
            } else if (!this.robbed){
                // before we grabbed it, overlay was a child of something; put it back.
                Overlays.editOverlay(this.grabbedThingID, {
                    parentID: this.previousParentID[this.grabbedThingID],
                    parentJointIndex: this.previousParentJointIndex[this.grabbedThingID]
                });

                // resizeTablet to counter adjust offsets to account for change of scale from sensorToWorldMatrix
                if (HMD.tabletID && this.grabbedThingID === HMD.tabletID) {
                    resizeTablet(getTabletWidthFromSettings(), this.previousParentJointIndex[this.grabbedThingID]);
                }
            }

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'release',
                grabbedEntity: this.grabbedThingID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));

            this.grabbedThingID = null;
        };

        this.getTargetID = function(overlays, controllerData) {
            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            for (var i = 0; i < overlays.length; i++) {
                var overlayPosition = Overlays.getProperty(overlays[i], "position");
                var handPosition = controllerData.controllerLocations[this.hand].position;
                var distance = Vec3.distance(overlayPosition, handPosition);
                if (distance <= NEAR_GRAB_RADIUS * sensorScaleFactor) {
                    if (overlays[i] !== HMD.miniTabletID || controllerData.secondaryValues[this.hand] === 0) {
                        // Don't grab mini tablet with grip.
                        return overlays[i];
                    }
                }
            }
            return null;
        };


        this.isReady = function (controllerData) {
            if ((controllerData.triggerClicks[this.hand] === 0 &&
                 controllerData.secondaryValues[this.hand] === 0)) {
                this.robbed = false;
                return makeRunningValues(false, [], []);
            }

            this.grabbedThingID = null;

            var candidateOverlays = controllerData.nearbyOverlayIDs[this.hand];
            var grabbableOverlays = candidateOverlays.filter(function(overlayID) {
                return Overlays.getProperty(overlayID, "grabbable");
            });

            var targetID = this.getTargetID(grabbableOverlays, controllerData);
            if (targetID && !this.robbed) {
                this.grabbedThingID = targetID;
                this.startNearParentingGrabOverlay(controllerData);
                return makeRunningValues(true, [this.grabbedThingID], []);
            } else {
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData) {
            if ((controllerData.triggerClicks[this.hand] === 0 && controllerData.secondaryValues[this.hand] === 0) || !this.isGrabbedThingVisible()) {
                this.endNearParentingGrabOverlay();
                this.robbed = false;
                return makeRunningValues(false, [], []);
            } else {
                // check if someone stole the target from us
                var grabbedProperties = this.getGrabbedProperties();
                if (!this.thisHandIsParent(grabbedProperties)) {
                    return makeRunningValues(false, [], []);
                }

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

    function cleanup() {
        leftNearParentingGrabOverlay.cleanup();
        rightNearParentingGrabOverlay.cleanup();
        disableDispatcherModule("LeftNearParentingGrabOverlay");
        disableDispatcherModule("RightNearParentingGrabOverlay");
    }
    Script.scriptEnding.connect(cleanup);
}());
