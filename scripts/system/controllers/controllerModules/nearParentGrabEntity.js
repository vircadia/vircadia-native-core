"use strict";

//  nearParentGrabEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND, getControllerJointIndex,
   enableDispatcherModule, disableDispatcherModule, propsArePhysical, Messages, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION,
   TRIGGER_OFF_VALUE, makeDispatcherModuleParameters, entityIsGrabbable, makeRunningValues, NEAR_GRAB_RADIUS,
   findGroupParent, Vec3, cloneEntity, entityIsCloneable, propsAreCloneDynamic, HAPTIC_PULSE_STRENGTH,
   HAPTIC_PULSE_DURATION, BUMPER_ON_VALUE, findHandChildEntities, TEAR_AWAY_DISTANCE, MSECS_PER_SEC, TEAR_AWAY_CHECK_TIME,
   TEAR_AWAY_COUNT, distanceBetweenPointAndEntityBoundingBox, print, Uuid, highlightTargetEntity, unhighlightTargetEntity,
   distanceBetweenEntityLocalPositionAndBoundingBox, GRAB_POINT_SPHERE_OFFSET
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/cloneEntityUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {

    // XXX this.ignoreIK = (grabbableData.ignoreIK !== undefined) ? grabbableData.ignoreIK : true;
    // XXX this.kinematicGrab = (grabbableData.kinematic !== undefined) ? grabbableData.kinematic : NEAR_GRABBING_KINEMATIC;

    // this offset needs to match the one in libraries/display-plugins/src/display-plugins/hmd/HmdDisplayPlugin.cpp:378
    var GRAB_POINT_SPHERE_OFFSET = { x: 0.04, y: 0.13, z: 0.039 };  // x = upward, y = forward, z = lateral

    function getGrabOffset(handController) {
        var offset = GRAB_POINT_SPHERE_OFFSET;
        if (handController === Controller.Standard.LeftHand) {
            offset = {
                x: -GRAB_POINT_SPHERE_OFFSET.x,
                y: GRAB_POINT_SPHERE_OFFSET.y,
                z: GRAB_POINT_SPHERE_OFFSET.z
            };
        }

        offset.y = -GRAB_POINT_SPHERE_OFFSET.y;
        return Vec3.multiply(MyAvatar.sensorToWorldScale, offset);
    }

    function NearParentingGrabEntity(hand) {
        this.hand = hand;
        this.targetEntityID = null;
        this.grabbing = false;
        this.previousParentID = {};
        this.previousParentJointIndex = {};
        this.previouslyUnhooked = {};
        this.hapticTargetID = null;
        this.lastUnequipCheckTime = 0;
        this.autoUnequipCounter = 0;
        this.lastUnexpectedChildrenCheckTime = 0;
        this.robbed = false;
        this.highlightedEntity = null;
        this.cloneAllowed = true;

        this.parameters = makeDispatcherModuleParameters(
            140,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.thisHandIsParent = function(props) {
            if (!props) {
                return false;
            }

            if (props.parentID !== MyAvatar.sessionUUID && props.parentID !== MyAvatar.SELF_ID) {
                return false;
            }

            var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            if (props.parentJointIndex === handJointIndex) {
                return true;
            }

            if (props.parentJointIndex === getControllerJointIndex(this.hand)) {
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

        this.getOtherModule = function() {
            return this.hand === RIGHT_HAND ? leftNearParentingGrabEntity : rightNearParentingGrabEntity;
        };

        this.otherHandIsParent = function(props) {
            var otherModule = this.getOtherModule();
            return (otherModule.thisHandIsParent(props) && otherModule.grabbing);
        };

        this.startNearParentingGrabEntity = function (controllerData, targetProps) {
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);
            unhighlightTargetEntity(this.targetEntityID);
            this.highlightedEntity = null;
            var message = {
                hand: this.hand,
                entityID: this.targetEntityID
            };

            Messages.sendLocalMessage('Hifi-unhighlight-entity', JSON.stringify(message));
            var handJointIndex;
            // if (this.ignoreIK) {
            //     handJointIndex = this.controllerJointIndex;
            // } else {
            //     handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            // }
            handJointIndex = getControllerJointIndex(this.hand);

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(targetProps.id, "startNearGrab", args);

            var reparentProps = {
                parentID: MyAvatar.SELF_ID,
                parentJointIndex: handJointIndex,
                localVelocity: {x: 0, y: 0, z: 0},
                localAngularVelocity: {x: 0, y: 0, z: 0}
            };

            if (this.thisHandIsParent(targetProps)) {
                // this should never happen, but if it does, don't set previous parent to be this hand.
                this.previousParentID[targetProps.id] = null;
                this.previousParentJointIndex[targetProps.id] = -1;
            } else if (this.otherHandIsParent(targetProps)) {
                var otherModule = this.getOtherModule();
                this.previousParentID[this.grabbedThingID] = otherModule.previousParentID[this.grabbedThingID];
                this.previousParentJointIndex[this.grabbedThingID] = otherModule.previousParentJointIndex[this.grabbedThingID];
                otherModule.robbed = true;
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

        this.endNearParentingGrabEntity = function (controllerData) {
            this.hapticTargetID = null;
            var props = controllerData.nearbyEntityPropertiesByID[this.targetEntityID];
            if (this.thisHandIsParent(props) && !this.robbed) {
                if (this.previousParentID[this.targetEntityID] === Uuid.NULL || this.previousParentID === undefined) {
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
            }

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "releaseGrab", args);
            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'release',
                grabbedEntity: this.targetEntityID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
            unhighlightTargetEntity(this.targetEntityID);
            this.highlightedEntity = null;
            this.grabbing = false;
            this.targetEntityID = null;
            this.robbed = false;
        };

        this.checkForChildTooFarAway = function (controllerData) {
            var props = controllerData.nearbyEntityPropertiesByID[this.targetEntityID];
            var now = Date.now();
            if (now - this.lastUnequipCheckTime > MSECS_PER_SEC * TEAR_AWAY_CHECK_TIME) {
                this.lastUnequipCheckTime = now;
                if (props.parentID === MyAvatar.SELF_ID) {
                    var tearAwayDistance = TEAR_AWAY_DISTANCE * MyAvatar.sensorToWorldScale;
                    var controllerIndex = (this.hand === LEFT_HAND ? Controller.Standard.LeftHand : Controller.Standard.RightHand);
                    var controllerGrabOffset = getGrabOffset(controllerIndex);
                    var distance = distanceBetweenEntityLocalPositionAndBoundingBox(props, controllerGrabOffset);
                    if (distance > tearAwayDistance) {
                        this.autoUnequipCounter++;
                    } else {
                        this.autoUnequipCounter = 0;
                    }
                    if (this.autoUnequipCounter >= TEAR_AWAY_COUNT) {
                        return true;
                    }
                }
            }
            return false;
        };


        this.checkForUnexpectedChildren = function (controllerData) {
            // sometimes things can get parented to a hand and this script is unaware.  Search for such entities and
            // unhook them.

            var now = Date.now();
            var UNEXPECTED_CHILDREN_CHECK_TIME = 0.1; // seconds
            if (now - this.lastUnexpectedChildrenCheckTime > MSECS_PER_SEC * UNEXPECTED_CHILDREN_CHECK_TIME) {
                this.lastUnexpectedChildrenCheckTime = now;

                var children = findHandChildEntities(this.hand);
                var _this = this;

                children.forEach(function(childID) {
                    // we appear to be holding something and this script isn't in a state that would be holding something.
                    // unhook it.  if we previously took note of this entity's parent, put it back where it was.  This
                    // works around some problems that happen when more than one hand or avatar is passing something around.
                    if (_this.previousParentID[childID]) {
                        var previousParentID = _this.previousParentID[childID];
                        var previousParentJointIndex = _this.previousParentJointIndex[childID];

                        // The main flaw with keeping track of previous parentage in individual scripts is:
                        // (1) A grabs something (2) B takes it from A (3) A takes it from B (4) A releases it
                        // now A and B will take turns passing it back to the other.  Detect this and stop the loop here...
                        var UNHOOK_LOOP_DETECT_MS = 200;
                        if (_this.previouslyUnhooked[childID]) {
                            if (now - _this.previouslyUnhooked[childID] < UNHOOK_LOOP_DETECT_MS) {
                                previousParentID = Uuid.NULL;
                                previousParentJointIndex = -1;
                            }
                        }
                        _this.previouslyUnhooked[childID] = now;

                        Entities.editEntity(childID, {
                            parentID: previousParentID,
                            parentJointIndex: previousParentJointIndex
                        });
                    } else {
                        Entities.editEntity(childID, { parentID: Uuid.NULL });
                    }
                });
            }
        };

        this.getTargetProps = function (controllerData) {
            // nearbyEntityProperties is already sorted by length from controller
            var nearbyEntityProperties = controllerData.nearbyEntityProperties[this.hand];
            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            for (var i = 0; i < nearbyEntityProperties.length; i++) {
                var props = nearbyEntityProperties[i];
                var handPosition = controllerData.controllerLocations[this.hand].position;
                var dist = distanceBetweenPointAndEntityBoundingBox(handPosition, props);
                var distance = Vec3.distance(handPosition, props.position);
                if ((dist > TEAR_AWAY_DISTANCE) ||
                    (distance > NEAR_GRAB_RADIUS * sensorScaleFactor)) {
                    continue;
                }
                if (entityIsGrabbable(props) || entityIsCloneable(props)) {
                    // give haptic feedback
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

        this.isReady = function (controllerData, deltaTime) {
            this.targetEntityID = null;
            this.grabbing = false;

            var targetProps = this.getTargetProps(controllerData);
            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE &&
                controllerData.secondaryValues[this.hand] < TRIGGER_OFF_VALUE) {
                this.checkForUnexpectedChildren(controllerData);
                this.robbed = false;
                this.cloneAllowed = true;
                return makeRunningValues(false, [], []);
            }

            if (targetProps) {
                if ((propsArePhysical(targetProps) || propsAreCloneDynamic(targetProps)) &&
                    targetProps.parentID === Uuid.NULL) {
                    this.robbed = false;
                    return makeRunningValues(false, [], []); // let nearActionGrabEntity handle it
                } else {
                    this.targetEntityID = targetProps.id;
                    this.highlightedEntity = this.targetEntityID;
                    highlightTargetEntity(this.targetEntityID);
                    return makeRunningValues(true, [this.targetEntityID], []);
                }
            } else {
                if (this.highlightedEntity) {
                    unhighlightTargetEntity(this.highlightedEntity);
                    this.highlightedEntity = null;
                }
                this.hapticTargetID = null;
                this.robbed = false;
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData, deltaTime) {
            if (this.grabbing) {
                if (controllerData.triggerClicks[this.hand] < TRIGGER_OFF_VALUE &&
                    controllerData.secondaryValues[this.hand] < TRIGGER_OFF_VALUE) {
                    this.endNearParentingGrabEntity(controllerData);
                    return makeRunningValues(false, [], []);
                }

                var props = controllerData.nearbyEntityPropertiesByID[this.targetEntityID];
                if (!props) {
                    // entity was deleted
                    unhighlightTargetEntity(this.targetEntityID);
                    this.highlightedEntity = null;
                    this.grabbing = false;
                    this.targetEntityID = null;
                    this.hapticTargetID = null;
                    this.robbed = false;
                    return makeRunningValues(false, [], []);
                }

                if (this.checkForChildTooFarAway(controllerData)) {
                    // if the held entity moves too far from the hand, release it
                    print("nearParentGrabEntity -- autoreleasing held item because it is far from hand");
                    this.endNearParentingGrabEntity(controllerData);
                    return makeRunningValues(false, [], []);
                }

                var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
                Entities.callEntityMethod(this.targetEntityID, "continueNearGrab", args);
            } else {
                // still searching / highlighting
                var readiness = this.isReady(controllerData);
                if (!readiness.active) {
                    this.robbed = false;
                    unhighlightTargetEntity(this.highlightedEntity);
                    this.highlightedEntity = null;
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
                                var cloneProps = Entities.getEntityProperties(cloneID);
                                this.grabbing = true;
                                this.targetEntityID = cloneID;
                                this.startNearParentingGrabEntity(controllerData, cloneProps);
                                this.cloneAllowed = false; // prevent another clone call until inputs released
                            }
                        }
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

    function cleanup() {
        leftNearParentingGrabEntity.cleanup();
        rightNearParentingGrabEntity.cleanup();
        disableDispatcherModule("LeftNearParentingGrabEntity");
        disableDispatcherModule("RightNearParentingGrabEntity");
    }
    Script.scriptEnding.connect(cleanup);
}());
