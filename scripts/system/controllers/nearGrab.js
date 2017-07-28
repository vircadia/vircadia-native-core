"use strict";

//  nearGrab.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, MyAvatar, Controller, controllerDispatcherPlugins,
   RIGHT_HAND, LEFT_HAND, AVATAR_SELF_ID, getControllerJointIndex, getGrabbableData, NULL_UUID */

Script.include("/~/system/controllers/controllerDispatcherUtils.js");

(function() {

    var HAPTIC_PULSE_STRENGTH = 1.0;
    var HAPTIC_PULSE_DURATION = 13.0;

    var FORBIDDEN_GRAB_TYPES = ["Unknown", "Light", "PolyLine", "Zone"];
    var FORBIDDEN_GRAB_NAMES = ["Grab Debug Entity", "grab pointer"];

    function entityIsParentingGrabbable(props) {
        var grabbable = getGrabbableData(props).grabbable;
        if (!grabbable ||
            props.locked ||
            FORBIDDEN_GRAB_TYPES.indexOf(props.type) >= 0 ||
            FORBIDDEN_GRAB_NAMES.indexOf(props.name) >= 0) {
            return false;
        }
        return true;
    }

    function NearGrabParenting(hand) {
        this.priority = 5;

        this.hand = hand;
        this.grabbedThingID = null;
        this.previousParentID = {};
        this.previousParentJointIndex = {};
        this.previouslyUnhooked = {};


        // todo: does this change if the avatar changes?
        this.handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");

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

        this.startNearGrabParenting = function (controllerData, grabbedProperties) {
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

            var reparentProps = {
                parentID: AVATAR_SELF_ID,
                parentJointIndex: getControllerJointIndex(this.hand),
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
        };

        this.endNearGrabParenting = function (controllerData) {
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
            this.grabbedThingID = null;
        };

        this.isReady = function (controllerData) {
            if (controllerData.triggerClicks[this.hand] == 0) {
                return false;
            }

            var grabbedProperties = null;
            var bestDistance = 1000;
            var nearbyEntityProperties = controllerData.nearbyEntityProperties[this.hand];

            for (var i = 0; i < nearbyEntityProperties.length; i ++) {
                var props = nearbyEntityProperties[i];
                if (entityIsParentingGrabbable(props)) {
                    if (props.distanceFromController < bestDistance) {
                        bestDistance = props.distanceFromController;
                        grabbedProperties = props;
                    }
                }
            }

            if (grabbedProperties) {
                this.grabbedThingID = grabbedProperties.id;
                this.startNearGrabParenting(controllerData, grabbedProperties);
                return true;
            } else {
                return false;
            }
        };

        this.run = function (controllerData) {
            if (controllerData.triggerClicks[this.hand] == 0) {
                this.endNearGrabParenting(controllerData);
                return false;
            }
            return true;
        };
    }

    var leftNearGrabParenting = new NearGrabParenting(LEFT_HAND);
    leftNearGrabParenting.name = "leftNearGrabParenting";

    var rightNearGrabParenting = new NearGrabParenting(RIGHT_HAND);
    rightNearGrabParenting.name = "rightNearGrabParenting";
        
    if (!controllerDispatcherPlugins) {
        controllerDispatcherPlugins = {};
    }
    controllerDispatcherPlugins.leftNearGrabParenting = leftNearGrabParenting;
    controllerDispatcherPlugins.rightNearGrabParenting = rightNearGrabParenting;


    this.cleanup = function () {
        delete controllerDispatcherPlugins.leftNearGrabParenting;
        delete controllerDispatcherPlugins.rightNearGrabParenting;
    };
    Script.scriptEnding.connect(this.cleanup);
}());
