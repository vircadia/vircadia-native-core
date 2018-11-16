"use strict";

//  farParentGrabEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Controller, RIGHT_HAND, LEFT_HAND, Mat4, MyAvatar, Vec3, Quat, getEnabledModuleByName, makeRunningValues,
   Entities, enableDispatcherModule, disableDispatcherModule, entityIsGrabbable, makeDispatcherModuleParameters, MSECS_PER_SEC,
   HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, TRIGGER_OFF_VALUE, TRIGGER_ON_VALUE, ZERO_VEC, getControllerWorldLocation,
   projectOntoEntityXYPlane, ContextOverlay, HMD, Picks, makeLaserLockInfo, makeLaserParams, AddressManager,
   getEntityParents, Selection, DISPATCHER_HOVERING_LIST, unhighlightTargetEntity, Messages, Uuid, findGroupParent,
   worldPositionToRegistrationFrameMatrix, DISPATCHER_PROPERTIES, findFarGrabJointChildEntities
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
    var MARGIN = 25;

    function TargetObject(entityID, entityProps) {
        this.entityID = entityID;
        this.entityProps = entityProps;
        this.targetEntityID = null;
        this.targetEntityProps = null;

        this.getTargetEntity = function() {
            var parentPropsLength = this.parentProps.length;
            if (parentPropsLength !== 0) {
                var targetEntity = {
                    id: this.parentProps[parentPropsLength - 1].id,
                    props: this.parentProps[parentPropsLength - 1]};
                this.targetEntityID = targetEntity.id;
                this.targetEntityProps = targetEntity.props;
                return targetEntity;
            }
            this.targetEntityID = this.entityID;
            this.targetEntityProps = this.entityProps;
            return {
                id: this.entityID,
                props: this.entityProps};
        };
    }

    function FarParentGrabEntity(hand) {
        this.hand = hand;
        this.targetEntityID = null;
        this.targetObject = null;
        this.previouslyUnhooked = {};
        this.previousParentID = {};
        this.previousParentJointIndex = {};
        this.potentialEntityWithContextOverlay = false;
        this.entityWithContextOverlay = false;
        this.contextOverlayTimer = false;
        this.highlightedEntity = null;
        this.reticleMinX = MARGIN;
        this.reticleMaxX = 0;
        this.reticleMinY = MARGIN;
        this.reticleMaxY = 0;
        this.lastUnexpectedChildrenCheckTime = 0;

        var FAR_GRAB_JOINTS = [65527, 65528]; // FARGRAB_LEFTHAND_INDEX, FARGRAB_RIGHTHAND_INDEX

        var DISTANCE_HOLDING_RADIUS_FACTOR = 3.5; // multiplied by distance between hand and object
        var DISTANCE_HOLDING_ACTION_TIMEFRAME = 0.1; // how quickly objects move to their new position
        var DISTANCE_HOLDING_UNITY_MASS = 1200; //  The mass at which the distance holding action timeframe is unmodified
        var DISTANCE_HOLDING_UNITY_DISTANCE = 6; //  The distance at which the distance holding action timeframe is unmodified

        this.parameters = makeDispatcherModuleParameters(
            540,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100,
            makeLaserParams(this.hand, false));


        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.distanceGrabTimescale = function(mass, distance) {
            var timeScale = DISTANCE_HOLDING_ACTION_TIMEFRAME * mass /
                DISTANCE_HOLDING_UNITY_MASS * distance /
                DISTANCE_HOLDING_UNITY_DISTANCE;
            if (timeScale < DISTANCE_HOLDING_ACTION_TIMEFRAME) {
                timeScale = DISTANCE_HOLDING_ACTION_TIMEFRAME;
            }
            return timeScale;
        };

        this.getMass = function(dimensions, density) {
            return (dimensions.x * dimensions.y * dimensions.z) * density;
        };

        this.thisFarGrabJointIsParent = function(isParentProps) {
            if (!isParentProps) {
                return false;
            }

            if (isParentProps.parentID !== MyAvatar.sessionUUID && isParentProps.parentID !== MyAvatar.SELF_ID) {
                return false;
            }

            if (isParentProps.parentJointIndex === FAR_GRAB_JOINTS[this.hand]) {
                return true;
            }

            return false;
        };

        this.startFarParentGrab = function (controllerData, grabbedProperties) {
            var controllerLocation = controllerData.controllerLocations[this.hand];
            var worldControllerPosition = controllerLocation.position;
            var worldControllerRotation = controllerLocation.orientation;
            // transform the position into room space
            var worldToSensorMat = Mat4.inverse(MyAvatar.getSensorToWorldMatrix());
            var roomControllerPosition = Mat4.transformPoint(worldToSensorMat, worldControllerPosition);

            var now = Date.now();

            // add the action and initialize some variables
            this.currentObjectPosition = grabbedProperties.position;
            this.currentObjectRotation = grabbedProperties.rotation;
            this.currentObjectTime = now;

            this.grabRadius = this.grabbedDistance;
            this.grabRadialVelocity = 0.0;

            // offset between controller vector at the grab radius and the entity position
            var targetPosition = Vec3.multiply(this.grabRadius, Quat.getUp(worldControllerRotation));
            targetPosition = Vec3.sum(targetPosition, worldControllerPosition);
            this.offsetPosition = Vec3.subtract(this.currentObjectPosition, targetPosition);

            // compute a constant based on the initial conditions which we use below to exaggerate hand motion
            // onto the held object
            this.radiusScalar = Math.log(this.grabRadius + 1.0);
            if (this.radiusScalar < 1.0) {
                this.radiusScalar = 1.0;
            }

            // compute the mass for the purpose of energy and how quickly to move object
            this.mass = this.getMass(grabbedProperties.dimensions, grabbedProperties.density);

            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);
            unhighlightTargetEntity(this.targetEntityID);
            var message = {
                hand: this.hand,
                entityID: this.targetEntityID
            };

            Messages.sendLocalMessage('Hifi-unhighlight-entity', JSON.stringify(message));

            var newTargetPosLocal = MyAvatar.worldToJointPoint(grabbedProperties.position);
            MyAvatar.setJointTranslation(FAR_GRAB_JOINTS[this.hand], newTargetPosLocal);

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(grabbedProperties.id, "startNearGrab", args);

            var reparentProps = {
                parentID: MyAvatar.SELF_ID,
                parentJointIndex: FAR_GRAB_JOINTS[this.hand],
                localVelocity: {x: 0, y: 0, z: 0},
                localAngularVelocity: {x: 0, y: 0, z: 0}
            };

            if (this.thisFarGrabJointIsParent(grabbedProperties)) {
                // this should never happen, but if it does, don't set previous parent to be this hand.
                this.previousParentID[grabbedProperties.id] = null;
                this.previousParentJointIndex[grabbedProperties.id] = -1;
            } else {
                this.previousParentID[grabbedProperties.id] = grabbedProperties.parentID;
                this.previousParentJointIndex[grabbedProperties.id] = grabbedProperties.parentJointIndex;
            }

            this.targetEntityID = grabbedProperties.id;
            Entities.editEntity(grabbedProperties.id, reparentProps);

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: grabbedProperties.id,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
            this.grabbing = true;

            this.previousRoomControllerPosition = roomControllerPosition;
        };

        this.continueDistanceHolding = function(controllerData) {
            var controllerLocation = controllerData.controllerLocations[this.hand];
            var worldControllerPosition = controllerLocation.position;
            var worldControllerRotation = controllerLocation.orientation;

            // also transform the position into room space
            var worldToSensorMat = Mat4.inverse(MyAvatar.getSensorToWorldMatrix());
            var roomControllerPosition = Mat4.transformPoint(worldToSensorMat, worldControllerPosition);

            var grabbedProperties = Entities.getEntityProperties(this.targetEntityID, DISPATCHER_PROPERTIES);
            var now = Date.now();
            var deltaObjectTime = (now - this.currentObjectTime) / MSECS_PER_SEC; // convert to seconds
            this.currentObjectTime = now;

            // the action was set up when this.distanceHolding was called.  update the targets.
            var radius = Vec3.distance(this.currentObjectPosition, worldControllerPosition) *
                this.radiusScalar * DISTANCE_HOLDING_RADIUS_FACTOR;
            if (radius < 1.0) {
                radius = 1.0;
            }

            var roomHandDelta = Vec3.subtract(roomControllerPosition, this.previousRoomControllerPosition);
            var worldHandDelta = Mat4.transformVector(MyAvatar.getSensorToWorldMatrix(), roomHandDelta);
            var handMoved = Vec3.multiply(worldHandDelta, radius);
            this.currentObjectPosition = Vec3.sum(this.currentObjectPosition, handMoved);

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "continueDistanceGrab", args);

            //  Update radialVelocity
            var lastVelocity = Vec3.multiply(worldHandDelta, 1.0 / deltaObjectTime);
            var delta = Vec3.normalize(Vec3.subtract(grabbedProperties.position, worldControllerPosition));
            var newRadialVelocity = Vec3.dot(lastVelocity, delta);

            var VELOCITY_AVERAGING_TIME = 0.016;
            var blendFactor = deltaObjectTime / VELOCITY_AVERAGING_TIME;
            if (blendFactor < 0.0) {
                blendFactor = 0.0;
            } else if (blendFactor > 1.0) {
                blendFactor = 1.0;
            }
            this.grabRadialVelocity = blendFactor * newRadialVelocity + (1.0 - blendFactor) * this.grabRadialVelocity;

            var RADIAL_GRAB_AMPLIFIER = 10.0;
            if (Math.abs(this.grabRadialVelocity) > 0.0) {
                this.grabRadius = this.grabRadius + (this.grabRadialVelocity * deltaObjectTime *
                                                     this.grabRadius * RADIAL_GRAB_AMPLIFIER);
            }

            // don't let grabRadius go all the way to zero, because it can't come back from that
            var MINIMUM_GRAB_RADIUS = 0.1;
            if (this.grabRadius < MINIMUM_GRAB_RADIUS) {
                this.grabRadius = MINIMUM_GRAB_RADIUS;
            }
            var newTargetPosition = Vec3.multiply(this.grabRadius, Quat.getUp(worldControllerRotation));
            newTargetPosition = Vec3.sum(newTargetPosition, worldControllerPosition);
            newTargetPosition = Vec3.sum(newTargetPosition, this.offsetPosition);

            // MyAvatar.setJointTranslation(FAR_GRAB_JOINTS[this.hand], MyAvatar.worldToJointPoint(newTargetPosition));

            // var newTargetPosLocal = Mat4.transformPoint(MyAvatar.getSensorToWorldMatrix(), newTargetPosition);
            var newTargetPosLocal = MyAvatar.worldToJointPoint(newTargetPosition);
            MyAvatar.setJointTranslation(FAR_GRAB_JOINTS[this.hand], newTargetPosLocal);

            this.previousRoomControllerPosition = roomControllerPosition;
        };

        this.endFarParentGrab = function (controllerData) {
            this.hapticTargetID = null;
            // var endProps = controllerData.nearbyEntityPropertiesByID[this.targetEntityID];
            var endProps = Entities.getEntityProperties(this.targetEntityID, DISPATCHER_PROPERTIES);
            if (this.thisFarGrabJointIsParent(endProps)) {
                Entities.editEntity(this.targetEntityID, {
                    parentID: this.previousParentID[this.targetEntityID],
                    parentJointIndex: this.previousParentJointIndex[this.targetEntityID],
                    localVelocity: {x: 0, y: 0, z: 0},
                    localAngularVelocity: {x: 0, y: 0, z: 0}
                });
            }

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "releaseGrab", args);
            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'release',
                grabbedEntity: this.targetEntityID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));
            unhighlightTargetEntity(this.targetEntityID);
            this.grabbing = false;
            this.targetEntityID = null;
            this.potentialEntityWithContextOverlay = false;
            MyAvatar.clearJointData(FAR_GRAB_JOINTS[this.hand]);
        };

        this.updateRecommendedArea = function() {
            var dims = Controller.getViewportDimensions();
            this.reticleMaxX = dims.x - MARGIN;
            this.reticleMaxY = dims.y - MARGIN;
        };

        this.calculateNewReticlePosition = function(intersection) {
            this.updateRecommendedArea();
            var point2d = HMD.overlayFromWorldPoint(intersection);
            point2d.x = Math.max(this.reticleMinX, Math.min(point2d.x, this.reticleMaxX));
            point2d.y = Math.max(this.reticleMinY, Math.min(point2d.y, this.reticleMaxY));
            return point2d;
        };

        this.notPointingAtEntity = function(controllerData) {
            var intersection = controllerData.rayPicks[this.hand];
            var entityProperty = Entities.getEntityProperties(intersection.objectID, DISPATCHER_PROPERTIES);
            var entityType = entityProperty.type;
            var hudRayPick = controllerData.hudRayPicks[this.hand];
            var point2d = this.calculateNewReticlePosition(hudRayPick.intersection);
            if ((intersection.type === Picks.INTERSECTED_ENTITY && entityType === "Web") ||
                intersection.type === Picks.INTERSECTED_OVERLAY || Window.isPointOnDesktopWindow(point2d)) {
                return true;
            }
            return false;
        };

        this.distanceRotate = function(otherFarGrabModule) {
            this.distanceRotating = true;
            this.distanceHolding = false;

            var worldControllerRotation = getControllerWorldLocation(this.handToController(), true).orientation;
            var controllerRotationDelta =
                Quat.multiply(worldControllerRotation, Quat.inverse(this.previousWorldControllerRotation));
            // Rotate entity by twice the delta rotation.
            controllerRotationDelta = Quat.multiply(controllerRotationDelta, controllerRotationDelta);

            // Perform the rotation in the translation controller's action update.
            otherFarGrabModule.currentObjectRotation = Quat.multiply(controllerRotationDelta,
                otherFarGrabModule.currentObjectRotation);

            this.previousWorldControllerRotation = worldControllerRotation;
        };

        this.prepareDistanceRotatingData = function(controllerData) {
            var intersection = controllerData.rayPicks[this.hand];

            var controllerLocation = getControllerWorldLocation(this.handToController(), true);
            var worldControllerPosition = controllerLocation.position;
            var worldControllerRotation = controllerLocation.orientation;

            var grabbedProperties = Entities.getEntityProperties(intersection.objectID, DISPATCHER_PROPERTIES);
            this.currentObjectPosition = grabbedProperties.position;
            this.grabRadius = intersection.distance;

            // Offset between controller vector at the grab radius and the entity position.
            var targetPosition = Vec3.multiply(this.grabRadius, Quat.getUp(worldControllerRotation));
            targetPosition = Vec3.sum(targetPosition, worldControllerPosition);
            this.offsetPosition = Vec3.subtract(this.currentObjectPosition, targetPosition);

            // Initial controller rotation.
            this.previousWorldControllerRotation = worldControllerRotation;
        };

        this.destroyContextOverlay = function(controllerData) {
            if (this.entityWithContextOverlay) {
                ContextOverlay.destroyContextOverlay(this.entityWithContextOverlay);
                this.entityWithContextOverlay = false;
                this.potentialEntityWithContextOverlay = false;
            }
        };

        this.checkForUnexpectedChildren = function (controllerData) {
            // sometimes things can get parented to a hand and this script is unaware.  Search for such entities and
            // unhook them.

            var now = Date.now();
            var UNEXPECTED_CHILDREN_CHECK_TIME = 0.1; // seconds
            if (now - this.lastUnexpectedChildrenCheckTime > MSECS_PER_SEC * UNEXPECTED_CHILDREN_CHECK_TIME) {
                this.lastUnexpectedChildrenCheckTime = now;

                var children = findFarGrabJointChildEntities(this.hand);
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

        this.targetIsNull = function() {
            var properties = Entities.getEntityProperties(this.targetEntityID, DISPATCHER_PROPERTIES);
            if (Object.keys(properties).length === 0 && this.distanceHolding) {
                return true;
            }
            return false;
        };

        this.getTargetProps = function (controllerData) {
            var targetEntity = controllerData.rayPicks[this.hand].objectID;
            if (targetEntity) {
                var gtProps = Entities.getEntityProperties(targetEntity, DISPATCHER_PROPERTIES);
                if (entityIsGrabbable(gtProps)) {
                    // give haptic feedback
                    if (gtProps.id !== this.hapticTargetID) {
                        Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);
                        this.hapticTargetID = gtProps.id;
                    }
                    // if we've attempted to grab a child, roll up to the root of the tree
                    var groupRootProps = findGroupParent(controllerData, gtProps);
                    if (entityIsGrabbable(groupRootProps)) {
                        return groupRootProps;
                    }
                    return gtProps;
                }
            }
            return null;
        };

        this.isReady = function (controllerData) {
            if (HMD.active) {
                if (this.notPointingAtEntity(controllerData)) {
                    return makeRunningValues(false, [], []);
                }

                this.distanceHolding = false;
                this.distanceRotating = false;

                if (controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE) {
                    var targetProps = this.getTargetProps(controllerData);
                    if (targetProps && (targetProps.dynamic && targetProps.parentID === Uuid.NULL)) {
                        return makeRunningValues(false, [], []); // let farActionGrabEntity handle it
                    } else {
                        this.prepareDistanceRotatingData(controllerData);
                        return makeRunningValues(true, [], []);
                    }
                } else {
                    this.checkForUnexpectedChildren(controllerData);
                    this.destroyContextOverlay();
                    return makeRunningValues(false, [], []);
                }
            }
            return makeRunningValues(false, [], []);
        };

        this.run = function (controllerData) {
            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE ||
                this.notPointingAtEntity(controllerData) || this.targetIsNull()) {
                this.endFarParentGrab(controllerData);
                Selection.removeFromSelectedItemsList(DISPATCHER_HOVERING_LIST, "entity", this.highlightedEntity);
                this.highlightedEntity = null;
                return makeRunningValues(false, [], []);
            }
            this.intersectionDistance = controllerData.rayPicks[this.hand].distance;

            var otherModuleName = this.hand === RIGHT_HAND ? "LeftFarParentGrabEntity" : "RightFarParentGrabEntity";
            var otherFarGrabModule = getEnabledModuleByName(otherModuleName);

            // gather up the readiness of the near-grab modules
            var nearGrabNames = [
                this.hand === RIGHT_HAND ? "RightScaleAvatar" : "LeftScaleAvatar",
                this.hand === RIGHT_HAND ? "RightFarTriggerEntity" : "LeftFarTriggerEntity",
                this.hand === RIGHT_HAND ? "RightNearActionGrabEntity" : "LeftNearActionGrabEntity",
                this.hand === RIGHT_HAND ? "RightNearParentingGrabEntity" : "LeftNearParentingGrabEntity",
                this.hand === RIGHT_HAND ? "RightNearParentingGrabOverlay" : "LeftNearParentingGrabOverlay",
                this.hand === RIGHT_HAND ? "RightNearTabletHighlight" : "LeftNearTabletHighlight"
            ];

            var nearGrabReadiness = [];
            for (var i = 0; i < nearGrabNames.length; i++) {
                var nearGrabModule = getEnabledModuleByName(nearGrabNames[i]);
                var ready = nearGrabModule ? nearGrabModule.isReady(controllerData) : makeRunningValues(false, [], []);
                nearGrabReadiness.push(ready);
            }

            if (this.targetEntityID) {
                // if we are doing a distance grab and the object or tablet gets close enough to the controller,
                // stop the far-grab so the near-grab or equip can take over.
                for (var k = 0; k < nearGrabReadiness.length; k++) {
                    if (nearGrabReadiness[k].active && (nearGrabReadiness[k].targets[0] === this.targetEntityID ||
                                                        HMD.tabletID && nearGrabReadiness[k].targets[0] === HMD.tabletID)) {
                        this.endFarParentGrab(controllerData);
                        return makeRunningValues(false, [], []);
                    }
                }

                this.continueDistanceHolding(controllerData);
            } else {
                // if we are doing a distance search and this controller moves into a position
                // where it could near-grab something, stop searching.
                for (var j = 0; j < nearGrabReadiness.length; j++) {
                    if (nearGrabReadiness[j].active) {
                        this.endFarParentGrab(controllerData);
                        return makeRunningValues(false, [], []);
                    }
                }

                var rayPickInfo = controllerData.rayPicks[this.hand];
                if (rayPickInfo.type === Picks.INTERSECTED_ENTITY) {
                    if (controllerData.triggerClicks[this.hand]) {
                        var entityID = rayPickInfo.objectID;
                        Selection.removeFromSelectedItemsList(DISPATCHER_HOVERING_LIST, "entity", this.highlightedEntity);
                        this.highlightedEntity = null;
                        var targetProps = Entities.getEntityProperties(entityID, DISPATCHER_PROPERTIES);
                        if (targetProps.href !== "") {
                            AddressManager.handleLookupString(targetProps.href);
                            return makeRunningValues(false, [], []);
                        }

                        this.targetObject = new TargetObject(entityID, targetProps);
                        this.targetObject.parentProps = getEntityParents(targetProps);

                        if (this.contextOverlayTimer) {
                            Script.clearTimeout(this.contextOverlayTimer);
                        }
                        this.contextOverlayTimer = false;
                        if (entityID === this.entityWithContextOverlay) {
                            this.destroyContextOverlay();
                        } else {
                            Selection.removeFromSelectedItemsList("contextOverlayHighlightList", "entity", entityID);
                        }

                        var targetEntity = this.targetObject.getTargetEntity();
                        entityID = targetEntity.id;
                        targetProps = targetEntity.props;

                        if (targetProps.dynamic || this.targetObject.entityProps.dynamic) {
                            // let farActionGrabEntity handle it
                            return makeRunningValues(false, [], []);
                        }

                        if (entityIsGrabbable(targetProps) || entityIsGrabbable(this.targetObject.entityProps)) {

                            if (!this.distanceRotating) {
                                this.targetEntityID = entityID;
                                this.grabbedDistance = rayPickInfo.distance;
                            }

                            if (otherFarGrabModule.targetEntityID === this.targetEntityID &&
                                otherFarGrabModule.distanceHolding) {
                                this.prepareDistanceRotatingData(controllerData);
                                this.distanceRotate(otherFarGrabModule);
                            } else {
                                this.distanceHolding = true;
                                this.distanceRotating = false;
                                this.startFarParentGrab(controllerData, targetProps);
                            }
                        }
                    } else {
                        var targetEntityID = rayPickInfo.objectID;
                        if (this.highlightedEntity !== targetEntityID) {
                            Selection.removeFromSelectedItemsList(DISPATCHER_HOVERING_LIST, "entity", this.highlightedEntity);
                            var selectionTargetProps = Entities.getEntityProperties(targetEntityID, DISPATCHER_PROPERTIES);

                            var selectionTargetObject = new TargetObject(targetEntityID, selectionTargetProps);
                            selectionTargetObject.parentProps = getEntityParents(selectionTargetProps);
                            var selectionTargetEntity = selectionTargetObject.getTargetEntity();

                            if (entityIsGrabbable(selectionTargetEntity.props) ||
                                entityIsGrabbable(selectionTargetObject.entityProps)) {

                                Selection.addToSelectedItemsList(DISPATCHER_HOVERING_LIST, "entity", rayPickInfo.objectID);
                            }
                            this.highlightedEntity = rayPickInfo.objectID;
                        }

                        if (!this.entityWithContextOverlay) {
                            var _this = this;

                            if (_this.potentialEntityWithContextOverlay !== rayPickInfo.objectID) {
                                if (_this.contextOverlayTimer) {
                                    Script.clearTimeout(_this.contextOverlayTimer);
                                }
                                _this.contextOverlayTimer = false;
                                _this.potentialEntityWithContextOverlay = rayPickInfo.objectID;
                            }

                            if (!_this.contextOverlayTimer) {
                                _this.contextOverlayTimer = Script.setTimeout(function () {
                                    if (!_this.entityWithContextOverlay &&
                                        _this.contextOverlayTimer &&
                                        _this.potentialEntityWithContextOverlay === rayPickInfo.objectID) {
                                        var cotProps = Entities.getEntityProperties(rayPickInfo.objectID,
                                                                                    DISPATCHER_PROPERTIES);
                                        var pointerEvent = {
                                            type: "Move",
                                            id: _this.hand + 1, // 0 is reserved for hardware mouse
                                            pos2D: projectOntoEntityXYPlane(rayPickInfo.objectID,
                                                                            rayPickInfo.intersection, cotProps),
                                            pos3D: rayPickInfo.intersection,
                                            normal: rayPickInfo.surfaceNormal,
                                            direction: Vec3.subtract(ZERO_VEC, rayPickInfo.surfaceNormal),
                                            button: "Secondary"
                                        };
                                        if (ContextOverlay.createOrDestroyContextOverlay(rayPickInfo.objectID, pointerEvent)) {
                                            _this.entityWithContextOverlay = rayPickInfo.objectID;
                                        }
                                    }
                                    _this.contextOverlayTimer = false;
                                }, 500);
                            }
                        }
                    }
                } else if (this.distanceRotating) {
                    this.distanceRotate(otherFarGrabModule);
                } else if (this.highlightedEntity) {
                    Selection.removeFromSelectedItemsList(DISPATCHER_HOVERING_LIST, "entity", this.highlightedEntity);
                    this.highlightedEntity = null;
                }
            }
            return this.exitIfDisabled(controllerData);
        };

        this.exitIfDisabled = function(controllerData) {
            var moduleName = this.hand === RIGHT_HAND ? "RightDisableModules" : "LeftDisableModules";
            var disableModule = getEnabledModuleByName(moduleName);
            if (disableModule) {
                if (disableModule.disableModules) {
                    this.endFarParentGrab(controllerData);
                    Selection.removeFromSelectedItemsList(DISPATCHER_HOVERING_LIST, "entity", this.highlightedEntity);
                    this.highlightedEntity = null;
                    return makeRunningValues(false, [], []);
                }
            }
            var grabbedThing = (this.distanceHolding || this.distanceRotating) ? this.targetObject.entityID : null;
            var offset = this.calculateOffset(controllerData);
            var laserLockInfo = makeLaserLockInfo(grabbedThing, false, this.hand, offset);
            return makeRunningValues(true, [], [], laserLockInfo);
        };

        this.calculateOffset = function(controllerData) {
            if (this.distanceHolding || this.distanceRotating) {
                var targetProps = Entities.getEntityProperties(this.targetObject.entityID,
                                                               [ "position", "rotation", "registrationPoint", "dimensions" ]);
                return worldPositionToRegistrationFrameMatrix(targetProps, controllerData.rayPicks[this.hand].intersection);
            }
            return undefined;
        };
    }

    var leftFarParentGrabEntity = new FarParentGrabEntity(LEFT_HAND);
    var rightFarParentGrabEntity = new FarParentGrabEntity(RIGHT_HAND);

    enableDispatcherModule("LeftFarParentGrabEntity", leftFarParentGrabEntity);
    enableDispatcherModule("RightFarParentGrabEntity", rightFarParentGrabEntity);

    function cleanup() {
        disableDispatcherModule("LeftFarParentGrabEntity");
        disableDispatcherModule("RightFarParentGrabEntity");
    }
    Script.scriptEnding.connect(cleanup);
}());
