"use strict";

//  nearGrab.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, HMD, Camera, MyAvatar, Controller, controllerDispatcherPlugins */


(function() {

    var LEFT_HAND = 0;
    var RIGHT_HAND = 1;

    var HAPTIC_PULSE_STRENGTH = 1.0;
    var HAPTIC_PULSE_DURATION = 13.0;

    var FORBIDDEN_GRAB_TYPES = ["Unknown", "Light", "PolyLine", "Zone"];
    var FORBIDDEN_GRAB_NAMES = ["Grab Debug Entity", "grab pointer"];

    var NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
    var AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

    function getControllerJointIndex(hand) {
        if (HMD.isHandControllerAvailable()) {
            var controllerJointIndex = -1;
            if (Camera.mode === "first person") {
                controllerJointIndex = MyAvatar.getJointIndex(hand === RIGHT_HAND ?
                                                              "_CONTROLLER_RIGHTHAND" :
                                                              "_CONTROLLER_LEFTHAND");
            } else if (Camera.mode === "third person") {
                controllerJointIndex = MyAvatar.getJointIndex(hand === RIGHT_HAND ?
                                                              "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" :
                                                              "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND");
            }

            return controllerJointIndex;
        }

        return MyAvatar.getJointIndex("Head");
    }

    function propsArePhysical(props) {
        if (!props.dynamic) {
            return false;
        }
        var isPhysical = (props.shapeType && props.shapeType != 'none');
        return isPhysical;
    }

    function entityIsGrabbable(props) {
        var grabbableProps = {};
        var userDataParsed = null;
        try {
            userDataParsed = JSON.parse(props.userData);
        } catch (err) {
        }
        if (userDataParsed && userDataParsed.grabbable) {
            grabbableProps = userDataParsed.grabbable;
        }
        var grabbable = propsArePhysical(props);
        if (grabbableProps.hasOwnProperty("grabbable")) {
            grabbable = grabbableProps.grabbable;
        }

        if (!grabbable) {
            return false;
        }
        if (FORBIDDEN_GRAB_TYPES.indexOf(props.type) >= 0) {
            return false;
        }
        if (props.locked) {
            return false;
        }
        if (FORBIDDEN_GRAB_NAMES.indexOf(props.name) >= 0) {
            return false;
        }
        return true;
    }


    function NearGrab(hand) {
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

        this.startNearGrab = function (controllerData, grabbedProperties) {
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

        this.endNearGrab = function (controllerData) {
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
                if (entityIsGrabbable(props)) {
                    if (props.distanceFromController < bestDistance) {
                        bestDistance = props.distanceFromController;
                        grabbedProperties = props;
                    }
                }
            }

            if (grabbedProperties) {
                this.grabbedThingID = grabbedProperties.id;
                this.startNearGrab(controllerData, grabbedProperties);
                return true;
            } else {
                return false;
            }
        };

        this.run = function (controllerData) {
            if (controllerData.triggerClicks[this.hand] == 0) {
                this.endNearGrab(controllerData);
                return false;
            }
            return true;
        };
    }

    var leftNearGrab = new NearGrab(LEFT_HAND);
    leftNearGrab.name = "leftNearGrab";

    var rightNearGrab = new NearGrab(RIGHT_HAND);
    rightNearGrab.name = "rightNearGrab";
        
    if (!controllerDispatcherPlugins) {
        controllerDispatcherPlugins = {};
    }
    controllerDispatcherPlugins.leftNearGrab = leftNearGrab;
    controllerDispatcherPlugins.rightNearGrab = rightNearGrab;


    this.cleanup = function () {
        delete controllerDispatcherPlugins.leftNearGrab;
        delete controllerDispatcherPlugins.rightNearGrab;
    };
    Script.scriptEnding.connect(this.cleanup);
}());
