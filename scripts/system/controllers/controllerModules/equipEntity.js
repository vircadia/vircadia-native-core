"use strict";

//  equipEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND,
   getControllerJointIndex, enableDispatcherModule, disableDispatcherModule,
   Messages, makeDispatcherModuleParameters, makeRunningValues, Settings, entityHasActions,
   Vec3, Overlays, flatten, Xform, getControllerWorldLocation, ensureDynamic, entityIsCloneable,
   cloneEntity, DISPATCHER_PROPERTIES, TEAR_AWAY_DISTANCE, Uuid
*/

Script.include("/~/system/libraries/Xform.js");
Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");
Script.include("/~/system/libraries/cloneEntityUtils.js");


var DEFAULT_SPHERE_MODEL_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/equip-Fresnel-3.fbx";
var EQUIP_SPHERE_SCALE_FACTOR = 0.65;


// Each overlayInfoSet describes a single equip hotspot.
// It is an object with the following keys:
//   timestamp - last time this object was updated, used to delete stale hotspot overlays.
//   entityID - entity assosicated with this hotspot
//   localPosition - position relative to the entity
//   hotspot - hotspot object
//   overlays - array of overlay objects created by Overlay.addOverlay()
//   currentSize - current animated scale value
//   targetSize - the target of our scale animations
//   type - "sphere" or "model".
function EquipHotspotBuddy() {
    // holds map from {string} hotspot.key to {object} overlayInfoSet.
    this.map = {};

    // array of all hotspots that are highlighed.
    this.highlightedHotspots = [];
}
EquipHotspotBuddy.prototype.clear = function() {
    var keys = Object.keys(this.map);
    for (var i = 0; i < keys.length; i++) {
        var overlayInfoSet = this.map[keys[i]];
        this.deleteOverlayInfoSet(overlayInfoSet);
    }
    this.map = {};
    this.highlightedHotspots = [];
};
EquipHotspotBuddy.prototype.highlightHotspot = function(hotspot) {
    this.highlightedHotspots.push(hotspot.key);
};
EquipHotspotBuddy.prototype.updateHotspot = function(hotspot, timestamp) {
    var overlayInfoSet = this.map[hotspot.key];
    if (!overlayInfoSet) {
        // create a new overlayInfoSet
        overlayInfoSet = {
            timestamp: timestamp,
            entityID: hotspot.entityID,
            localPosition: hotspot.localPosition,
            hotspot: hotspot,
            currentSize: 0,
            targetSize: 1,
            overlays: []
        };

        var diameter = hotspot.radius * 2;

        // override default sphere with a user specified model, if it exists.
        overlayInfoSet.overlays.push(Overlays.addOverlay("model", {
            name: "hotspot overlay",
            url: hotspot.modelURL ? hotspot.modelURL : DEFAULT_SPHERE_MODEL_URL,
            position: hotspot.worldPosition,
            rotation: {
                x: 0,
                y: 0,
                z: 0,
                w: 1
            },
            dimensions: diameter * EQUIP_SPHERE_SCALE_FACTOR,
            scale: hotspot.modelScale,
            ignoreRayIntersection: true
        }));
        overlayInfoSet.type = "model";
        this.map[hotspot.key] = overlayInfoSet;
    } else {
        overlayInfoSet.timestamp = timestamp;
    }
};
EquipHotspotBuddy.prototype.updateHotspots = function(hotspots, timestamp) {
    var _this = this;
    hotspots.forEach(function(hotspot) {
        _this.updateHotspot(hotspot, timestamp);
    });
    this.highlightedHotspots = [];
};
EquipHotspotBuddy.prototype.update = function(deltaTime, timestamp, controllerData) {

    var HIGHLIGHT_SIZE = 1.1;
    var NORMAL_SIZE = 1.0;

    var keys = Object.keys(this.map);
    for (var i = 0; i < keys.length; i++) {
        var overlayInfoSet = this.map[keys[i]];

        // this overlayInfo is highlighted.
        if (this.highlightedHotspots.indexOf(keys[i]) !== -1) {
            overlayInfoSet.targetSize = HIGHLIGHT_SIZE;
        } else {
            overlayInfoSet.targetSize = NORMAL_SIZE;
        }

        // start to fade out this hotspot.
        if (overlayInfoSet.timestamp !== timestamp) {
            overlayInfoSet.targetSize = 0;
        }

        // animate the size.
        var SIZE_TIMESCALE = 0.1;
        var tau = deltaTime / SIZE_TIMESCALE;
        if (tau > 1.0) {
            tau = 1.0;
        }
        overlayInfoSet.currentSize += (overlayInfoSet.targetSize - overlayInfoSet.currentSize) * tau;

        if (overlayInfoSet.timestamp !== timestamp && overlayInfoSet.currentSize <= 0.05) {
            // this is an old overlay, that has finished fading out, delete it!
            overlayInfoSet.overlays.forEach(Overlays.deleteOverlay);
            delete this.map[keys[i]];
        } else {
            // update overlay position, rotation to follow the object it's attached to.
            var props = controllerData.nearbyEntityPropertiesByID[overlayInfoSet.entityID];
            if (props) {
                var entityXform = new Xform(props.rotation, props.position);
                var position = entityXform.xformPoint(overlayInfoSet.localPosition);

                var dimensions;
                if (overlayInfoSet.type === "sphere") {
                    dimensions = (overlayInfoSet.hotspot.radius / 2) *  overlayInfoSet.currentSize * EQUIP_SPHERE_SCALE_FACTOR;
                } else {
                    dimensions = (overlayInfoSet.hotspot.radius / 2) * overlayInfoSet.currentSize;
                }

                overlayInfoSet.overlays.forEach(function(overlay) {
                    Overlays.editOverlay(overlay, {
                        position: position,
                        rotation: props.rotation,
                        dimensions: dimensions
                    });
                });
            } else {
                overlayInfoSet.overlays.forEach(Overlays.deleteOverlay);
                delete this.map[keys[i]];
            }
        }
    }
};

(function() {

    var ATTACH_POINT_SETTINGS = "io.highfidelity.attachPoints";

    var EQUIP_RADIUS = 1.0; // radius used for palm vs equip-hotspot for equipping.

    var HAPTIC_PULSE_STRENGTH = 1.0;
    var HAPTIC_PULSE_DURATION = 13.0;
    var HAPTIC_TEXTURE_STRENGTH = 0.1;
    var HAPTIC_TEXTURE_DURATION = 3.0;
    var HAPTIC_TEXTURE_DISTANCE = 0.002;
    var HAPTIC_DEQUIP_STRENGTH = 0.75;
    var HAPTIC_DEQUIP_DURATION = 50.0;

    var TRIGGER_SMOOTH_RATIO = 0.1; //  Time averaging of trigger - 0.0 disables smoothing
    var TRIGGER_OFF_VALUE = 0.1;
    var TRIGGER_ON_VALUE = TRIGGER_OFF_VALUE + 0.05; //  Squeezed just enough to activate search or near grab
    var BUMPER_ON_VALUE = 0.5;


    function getWearableData(props) {
        var wearable = {};
        try {
            if (!props.userDataParsed) {
                props.userDataParsed = JSON.parse(props.userData);
            }

            wearable = props.userDataParsed.wearable ? props.userDataParsed.wearable : {};
        } catch (err) {
            // don't want to spam the logs
        }
        return wearable;
    }
    function getEquipHotspotsData(props) {
        var equipHotspots = [];
        try {
            if (!props.userDataParsed) {
                props.userDataParsed = JSON.parse(props.userData);
            }

            equipHotspots = props.userDataParsed.equipHotspots ? props.userDataParsed.equipHotspots : [];
        } catch (err) {
            // don't want to spam the logs
        }
        return equipHotspots;
    }

    function getAttachPointSettings() {
        try {
            var str = Settings.getValue(ATTACH_POINT_SETTINGS);
            if (str === "false" || str === "") {
                return {};
            } else {
                return JSON.parse(str);
            }
        } catch (err) {
            print("Error parsing attachPointSettings: " + err);
            return {};
        }
    }

    function setAttachPointSettings(attachPointSettings) {
        var str = JSON.stringify(attachPointSettings);
        Settings.setValue(ATTACH_POINT_SETTINGS, str);
    }

    function getAttachPointForHotspotFromSettings(hotspot, hand) {
        var skeletonModelURL = MyAvatar.skeletonModelURL;
        var attachPointSettings = getAttachPointSettings();
        var avatarSettingsData = attachPointSettings[skeletonModelURL];
        if (avatarSettingsData) {
            var jointName = (hand === RIGHT_HAND) ? "RightHand" : "LeftHand";
            var joints = avatarSettingsData[hotspot.key];
            if (joints) {
                return joints[jointName];
            }
        }
        return undefined;
    }

    function storeAttachPointForHotspotInSettings(hotspot, hand, offsetPosition, offsetRotation) {
        var attachPointSettings = getAttachPointSettings();
        var skeletonModelURL = MyAvatar.skeletonModelURL;
        var avatarSettingsData = attachPointSettings[skeletonModelURL];
        if (!avatarSettingsData) {
            avatarSettingsData = {};
            attachPointSettings[skeletonModelURL] = avatarSettingsData;
        }
        var jointName = (hand === RIGHT_HAND) ? "RightHand" : "LeftHand";
        var joints = avatarSettingsData[hotspot.key];
        if (!joints) {
            joints = {};
            avatarSettingsData[hotspot.key] = joints;
        }
        joints[jointName] = [offsetPosition, offsetRotation];
        setAttachPointSettings(attachPointSettings);
    }

    function clearAttachPoints() {
        setAttachPointSettings({});
    }

    function EquipEntity(hand) {
        this.hand = hand;
        this.targetEntityID = null;
        this.prevHandIsUpsideDown = false;
        this.triggerValue = 0;
        this.messageGrabEntity = false;
        this.grabEntityProps = null;
        this.shouldSendStart = false;
        this.equipedWithSecondary = false;
        this.handHasBeenRightsideUp = false;

        this.parameters = makeDispatcherModuleParameters(
            300,
            this.hand === RIGHT_HAND ? ["rightHand", "rightHandEquip"] : ["leftHand", "leftHandEquip"],
            [],
            100);

        var equipHotspotBuddy = new EquipHotspotBuddy();

        this.setMessageGrabData = function(entityProperties) {
            if (entityProperties) {
                this.messageGrabEntity = true;
                this.grabEntityProps = entityProperties;
            }
        };

        // returns a list of all equip-hotspots assosiated with this entity.
        // @param {UUID} entityID
        // @returns {Object[]} array of objects with the following fields.
        //      * key {string} a string that can be used to uniquely identify this hotspot
        //      * entityID {UUID}
        //      * localPosition {Vec3} position of the hotspot in object space.
        //      * worldPosition {vec3} position of the hotspot in world space.
        //      * radius {number} radius of equip hotspot
        //      * joints {Object} keys are joint names values are arrays of two elements:
        //        offset position {Vec3} and offset rotation {Quat}, both are in the coordinate system of the joint.
        //      * modelURL {string} url for model to use instead of default sphere.
        //      * modelScale {Vec3} scale factor for model
        this.collectEquipHotspots = function(props) {
            var result = [];
            var entityID = props.id;
            var entityXform = new Xform(props.rotation, props.position);

            var equipHotspotsProps = getEquipHotspotsData(props);
            if (equipHotspotsProps && equipHotspotsProps.length > 0) {
                var i, length = equipHotspotsProps.length;
                for (i = 0; i < length; i++) {
                    var hotspot = equipHotspotsProps[i];
                    if (hotspot.position && hotspot.radius && hotspot.joints) {
                        result.push({
                            key: entityID.toString() + i.toString(),
                            entityID: entityID,
                            localPosition: hotspot.position,
                            worldPosition: entityXform.xformPoint(hotspot.position),
                            radius: hotspot.radius,
                            joints: hotspot.joints,
                            modelURL: hotspot.modelURL,
                            modelScale: hotspot.modelScale
                        });
                    }
                }
            } else {
                var wearableProps = getWearableData(props);
                var sensorToScaleFactor = MyAvatar.sensorToWorldScale;
                if (wearableProps && wearableProps.joints) {

                    result.push({
                        key: entityID.toString() + "0",
                        entityID: entityID,
                        localPosition: {
                            x: 0,
                            y: 0,
                            z: 0
                        },
                        worldPosition: entityXform.pos,
                        radius: EQUIP_RADIUS * sensorToScaleFactor,
                        joints: wearableProps.joints,
                        modelURL: null,
                        modelScale: null
                    });
                }
            }
            return result;
        };

        this.hotspotIsEquippable = function(hotspot, controllerData) {
            var props = controllerData.nearbyEntityPropertiesByID[hotspot.entityID];

            var hasParent = true;
            if (props.parentID === Uuid.NULL) {
                hasParent = false;
            }

            if (hasParent || entityHasActions(hotspot.entityID)) {
                return false;
            }

            return true;
        };

        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.updateSmoothedTrigger = function(controllerData) {
            var triggerValue = controllerData.triggerValues[this.hand];
            // smooth out trigger value
            this.triggerValue = (this.triggerValue * TRIGGER_SMOOTH_RATIO) +
                (triggerValue * (1.0 - TRIGGER_SMOOTH_RATIO));
        };

        this.triggerSmoothedGrab = function() {
            return this.triggerClicked;
        };

        this.triggerSmoothedSqueezed = function() {
            return this.triggerValue > TRIGGER_ON_VALUE;
        };

        this.triggerSmoothedReleased = function() {
            return this.triggerValue < TRIGGER_OFF_VALUE;
        };

        this.secondaryReleased = function() {
            return this.rawSecondaryValue < BUMPER_ON_VALUE;
        };

        this.secondarySmoothedSqueezed = function() {
            return this.rawSecondaryValue > BUMPER_ON_VALUE;
        };

        this.chooseNearEquipHotspots = function(candidateEntityProps, controllerData) {
            var _this = this;
            var collectedHotspots = flatten(candidateEntityProps.map(function(props) {
                return _this.collectEquipHotspots(props);
            }));
            var controllerLocation = controllerData.controllerLocations[_this.hand];
            var worldControllerPosition = controllerLocation.position;
            var equippableHotspots = collectedHotspots.filter(function(hotspot) {
                var hotspotDistance = Vec3.distance(hotspot.worldPosition, worldControllerPosition);
                return _this.hotspotIsEquippable(hotspot, controllerData) &&
                    hotspotDistance < hotspot.radius;
            });
            return equippableHotspots;
        };

        this.cloneHotspot = function(props, controllerData) {
            if (entityIsCloneable(props)) {
                var worldEntityProps = controllerData.nearbyEntityProperties[this.hand];
                var cloneID = cloneEntity(props, worldEntityProps);
                return cloneID;
            }

            return null;
        };

        this.chooseBestEquipHotspot = function(candidateEntityProps, controllerData) {
            var equippableHotspots = this.chooseNearEquipHotspots(candidateEntityProps, controllerData);
            if (equippableHotspots.length > 0) {
                // sort by distance;
                var controllerLocation = controllerData.controllerLocations[this.hand];
                var worldControllerPosition = controllerLocation.position;
                equippableHotspots.sort(function(a, b) {
                    var aDistance = Vec3.distance(a.worldPosition, worldControllerPosition);
                    var bDistance = Vec3.distance(b.worldPosition, worldControllerPosition);
                    return aDistance - bDistance;
                });
                return equippableHotspots[0];
            } else {
                return null;
            }
        };

        this.dropGestureReset = function() {
            this.prevHandIsUpsideDown = false;
        };

        this.dropGestureProcess = function (deltaTime) {
            var worldHandRotation = getControllerWorldLocation(this.handToController(), true).orientation;
            var localHandUpAxis = this.hand === RIGHT_HAND ? { x: 1, y: 0, z: 0 } : { x: -1, y: 0, z: 0 };
            var worldHandUpAxis = Vec3.multiplyQbyV(worldHandRotation, localHandUpAxis);
            var DOWN = { x: 0, y: -1, z: 0 };

            var DROP_ANGLE = Math.PI / 3;
            var HYSTERESIS_FACTOR = 1.1;
            var ROTATION_ENTER_THRESHOLD = Math.cos(DROP_ANGLE);
            var ROTATION_EXIT_THRESHOLD = Math.cos(DROP_ANGLE * HYSTERESIS_FACTOR);
            var rotationThreshold = this.prevHandIsUpsideDown ? ROTATION_EXIT_THRESHOLD : ROTATION_ENTER_THRESHOLD;

            var handIsUpsideDown = false;
            if (Vec3.dot(worldHandUpAxis, DOWN) > rotationThreshold) {
                handIsUpsideDown = true;
            }

            if (handIsUpsideDown !== this.prevHandIsUpsideDown) {
                this.prevHandIsUpsideDown = handIsUpsideDown;
                Controller.triggerHapticPulse(HAPTIC_DEQUIP_STRENGTH, HAPTIC_DEQUIP_DURATION, this.hand);
            }

            return handIsUpsideDown;
        };

        this.clearEquipHaptics = function() {
            this.prevPotentialEquipHotspot = null;
        };

        this.updateEquipHaptics = function(potentialEquipHotspot, currentLocation) {
            if (potentialEquipHotspot && !this.prevPotentialEquipHotspot ||
                !potentialEquipHotspot && this.prevPotentialEquipHotspot) {
                Controller.triggerHapticPulse(HAPTIC_TEXTURE_STRENGTH, HAPTIC_TEXTURE_DURATION, this.hand);
                this.lastHapticPulseLocation = currentLocation;
            } else if (potentialEquipHotspot &&
                       Vec3.distance(this.lastHapticPulseLocation, currentLocation) > HAPTIC_TEXTURE_DISTANCE) {
                Controller.triggerHapticPulse(HAPTIC_TEXTURE_STRENGTH, HAPTIC_TEXTURE_DURATION, this.hand);
                this.lastHapticPulseLocation = currentLocation;
            }
            this.prevPotentialEquipHotspot = potentialEquipHotspot;
        };

        this.startEquipEntity = function (controllerData) {
            this.dropGestureReset();
            this.clearEquipHaptics();
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

            var grabbedProperties = Entities.getEntityProperties(this.targetEntityID);

            // if an object is "equipped" and has a predefined offset, use it.
            if (this.grabbedHotspot) {
                var offsets = getAttachPointForHotspotFromSettings(this.grabbedHotspot, this.hand);
                if (offsets) {
                    this.offsetPosition = offsets[0];
                    this.offsetRotation = offsets[1];
                } else {
                    var handJointName = this.hand === RIGHT_HAND ? "RightHand" : "LeftHand";
                    if (this.grabbedHotspot.joints[handJointName]) {
                        this.offsetPosition = this.grabbedHotspot.joints[handJointName][0];
                        this.offsetRotation = this.grabbedHotspot.joints[handJointName][1];
                    }
                }
            }

            var handJointIndex;
            if (this.ignoreIK) {
                handJointIndex = this.controllerJointIndex;
            } else {
                handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            }

            var reparentProps = {
                parentID: MyAvatar.SELF_ID,
                parentJointIndex: handJointIndex,
                localVelocity: {x: 0, y: 0, z: 0},
                localAngularVelocity: {x: 0, y: 0, z: 0},
                localPosition: this.offsetPosition,
                localRotation: this.offsetRotation
            };

            var isClone = false;
            if (entityIsCloneable(grabbedProperties)) {
                var cloneID = this.cloneHotspot(grabbedProperties, controllerData);
                this.targetEntityID = cloneID;
                Entities.editEntity(this.targetEntityID, reparentProps);
                controllerData.nearbyEntityPropertiesByID[this.targetEntityID] = grabbedProperties;
                isClone = true;
            } else if (!grabbedProperties.locked) {
                Entities.editEntity(this.targetEntityID, reparentProps);
            } else {
                this.grabbedHotspot = null;
                this.targetEntityID = null;
                return;
            }

            // we don't want to send startEquip message until the trigger is released.  otherwise,
            // guns etc will fire right as they are equipped.
            this.shouldSendStart = true;

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'equip',
                grabbedEntity: this.targetEntityID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));

            var _this = this;
            var grabEquipCheck = function() {
                var args = [_this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
                Entities.callEntityMethod(_this.targetEntityID, "startEquip", args);
            };

            if (isClone) {
                // 100 ms seems to be sufficient time to force the check even occur after the object has been initialized.
                Script.setTimeout(grabEquipCheck, 100);
            }
        };

        this.endEquipEntity = function () {
            this.storeAttachPointInSettings();
            Entities.editEntity(this.targetEntityID, {
                parentID: Uuid.NULL,
                parentJointIndex: -1
            });

            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "releaseEquip", args);

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'release',
                grabbedEntity: this.targetEntityID,
                joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
            }));

            ensureDynamic(this.targetEntityID);
            this.targetEntityID = null;
            this.messageGrabEntity = false;
            this.grabEntityProps = null;
        };

        this.updateInputs = function (controllerData) {
            this.rawTriggerValue = controllerData.triggerValues[this.hand];
            this.triggerClicked = controllerData.triggerClicks[this.hand];
            this.rawSecondaryValue = controllerData.secondaryValues[this.hand];
            this.updateSmoothedTrigger(controllerData);
        };

        this.checkNearbyHotspots = function (controllerData, deltaTime, timestamp) {
            this.controllerJointIndex = getControllerJointIndex(this.hand);

            if (this.triggerSmoothedReleased() && this.secondaryReleased()) {
                this.waitForTriggerRelease = false;
            }

            var controllerLocation = getControllerWorldLocation(this.handToController(), true);
            var worldHandPosition = controllerLocation.position;
            var candidateEntityProps = controllerData.nearbyEntityProperties[this.hand];


            var potentialEquipHotspot = null;
            if (this.messageGrabEntity) {
                var hotspots = this.collectEquipHotspots(this.grabEntityProps);
                if (hotspots.length > -1) {
                    potentialEquipHotspot = hotspots[0];
                }
            } else {
                potentialEquipHotspot = this.chooseBestEquipHotspot(candidateEntityProps, controllerData);
            }

            if (!this.waitForTriggerRelease) {
                this.updateEquipHaptics(potentialEquipHotspot, worldHandPosition);
            }

            var nearEquipHotspots = this.chooseNearEquipHotspots(candidateEntityProps, controllerData);
            equipHotspotBuddy.updateHotspots(nearEquipHotspots, timestamp);
            if (potentialEquipHotspot) {
                equipHotspotBuddy.highlightHotspot(potentialEquipHotspot);
            }

            equipHotspotBuddy.update(deltaTime, timestamp, controllerData);

            // if the potentialHotspot is cloneable, clone it and return it
            // if the potentialHotspot os not cloneable and locked return null

            if (potentialEquipHotspot &&
                (((this.triggerSmoothedSqueezed() || this.secondarySmoothedSqueezed()) && !this.waitForTriggerRelease) ||
                 this.messageGrabEntity)) {
                this.grabbedHotspot = potentialEquipHotspot;
                this.targetEntityID = this.grabbedHotspot.entityID;
                this.startEquipEntity(controllerData);
                this.messageGrabEntity = false;
                this.equipedWithSecondary = this.secondarySmoothedSqueezed();
                return makeRunningValues(true, [potentialEquipHotspot.entityID], []);
            } else {
                return makeRunningValues(false, [], []);
            }
        };

        this.isTargetIDValid = function(controllerData) {
            var entityProperties = controllerData.nearbyEntityPropertiesByID[this.targetEntityID];
            return entityProperties && "type" in entityProperties;
        };

        this.isReady = function (controllerData, deltaTime) {
            var timestamp = Date.now();
            this.updateInputs(controllerData);
            this.handHasBeenRightsideUp = false;
            return this.checkNearbyHotspots(controllerData, deltaTime, timestamp);
        };

        this.run = function (controllerData, deltaTime) {
            var timestamp = Date.now();
            this.updateInputs(controllerData);

            if (!this.isTargetIDValid(controllerData)) {
                this.endEquipEntity();
                return makeRunningValues(false, [], []);
            }

            if (!this.targetEntityID) {
                return this.checkNearbyHotspots(controllerData, deltaTime, timestamp);
            }

            if (controllerData.secondaryValues[this.hand] && !this.equipedWithSecondary) {
                // this.secondaryReleased() will always be true when not depressed
                // so we cannot simply rely on that for release - ensure that the
                // trigger was first "prepared" by being pushed in before the release
                this.preparingHoldRelease = true;
            }

            if (this.preparingHoldRelease && !controllerData.secondaryValues[this.hand]) {
                // we have an equipped object and the secondary trigger was released
                // short-circuit the other checks and release it
                this.preparingHoldRelease = false;
                this.endEquipEntity();
                return makeRunningValues(false, [], []);
            }

            var handIsUpsideDown = this.dropGestureProcess(deltaTime);
            var dropDetected = false;
            if (this.handHasBeenRightsideUp) {
                dropDetected = handIsUpsideDown;
            }
            if (!handIsUpsideDown) {
                this.handHasBeenRightsideUp = true;
            }

            if (this.triggerSmoothedReleased() || this.secondaryReleased()) {
                if (this.shouldSendStart) {
                    // we don't want to send startEquip message until the trigger is released.  otherwise,
                    // guns etc will fire right as they are equipped.
                    var startArgs = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
                    Entities.callEntityMethod(this.targetEntityID, "startEquip", startArgs);
                    this.shouldSendStart = false;
                }
                this.waitForTriggerRelease = false;
                if (this.secondaryReleased() && this.equipedWithSecondary) {
                    this.equipedWithSecondary = false;
                }
            }

            if (dropDetected && this.prevDropDetected !== dropDetected) {
                this.waitForTriggerRelease = true;
            }

            // highlight the grabbed hotspot when the dropGesture is detected.
            if (dropDetected && this.grabbedHotspot) {
                equipHotspotBuddy.updateHotspot(this.grabbedHotspot, timestamp);
                equipHotspotBuddy.highlightHotspot(this.grabbedHotspot);
            }

            if (dropDetected && !this.waitForTriggerRelease && this.triggerSmoothedGrab()) {
                this.waitForTriggerRelease = true;
                // store the offset attach points into preferences.
                this.endEquipEntity();
                return makeRunningValues(false, [], []);
            }
            this.prevDropDetected = dropDetected;

            equipHotspotBuddy.update(deltaTime, timestamp, controllerData);

            if (!this.shouldSendStart) {
                var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
                Entities.callEntityMethod(this.targetEntityID, "continueEquip", args);
            }

            return makeRunningValues(true, [this.targetEntityID], []);
        };

        this.storeAttachPointInSettings = function() {
            if (this.grabbedHotspot && this.targetEntityID) {
                var prefProps = Entities.getEntityProperties(this.targetEntityID, ["localPosition", "localRotation"]);
                if (prefProps && prefProps.localPosition && prefProps.localRotation) {
                    storeAttachPointForHotspotInSettings(this.grabbedHotspot, this.hand,
                        prefProps.localPosition, prefProps.localRotation);
                }
            }
        };

        this.cleanup = function () {
            if (this.targetEntityID) {
                this.endEquipEntity();
            }
        };
    }

    var handleMessage = function(channel, message, sender) {
        var data;
        if (sender === MyAvatar.sessionUUID) {
            if (channel === 'Hifi-Hand-Grab') {
                try {
                    data = JSON.parse(message);
                    var equipModule = (data.hand === "left") ? leftEquipEntity : rightEquipEntity;
                    var entityProperties = Entities.getEntityProperties(data.entityID, DISPATCHER_PROPERTIES);
                    entityProperties.id = data.entityID;
                    equipModule.setMessageGrabData(entityProperties);

                } catch (e) {
                    print("WARNING: equipEntity.js -- error parsing Hifi-Hand-Grab message: " + message);
                }
            } else if (channel === 'Hifi-Hand-Drop') {
                if (message === "left") {
                    leftEquipEntity.endEquipEntity();
                } else if (message === "right") {
                    rightEquipEntity.endEquipEntity();
                } else if (message === "both") {
                    leftEquipEntity.endEquipEntity();
                    rightEquipEntity.endEquipEntity();
                }
            }
        }
    };

    Messages.subscribe('Hifi-Hand-Grab');
    Messages.subscribe('Hifi-Hand-Drop');
    Messages.messageReceived.connect(handleMessage);

    var leftEquipEntity = new EquipEntity(LEFT_HAND);
    var rightEquipEntity = new EquipEntity(RIGHT_HAND);

    enableDispatcherModule("LeftEquipEntity", leftEquipEntity);
    enableDispatcherModule("RightEquipEntity", rightEquipEntity);

    function cleanup() {
        leftEquipEntity.cleanup();
        rightEquipEntity.cleanup();
        disableDispatcherModule("LeftEquipEntity");
        disableDispatcherModule("RightEquipEntity");
        clearAttachPoints();
    }
    Script.scriptEnding.connect(cleanup);
}());
