"use strict";

//  handControllerGrab.js
//
//  Created by Eric Levin on  9/2/15
//  Additions by James B. Pollack @imgntn on 9/24/2015
//  Additions By Seth Alves on 10/20/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Grabs physically moveable entities with hydra-like controllers; it works for either near or far objects.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() { // BEGIN LOCAL_SCOPE

Script.include("/~/system/libraries/utils.js");
Script.include("/~/system/libraries/Xform.js");

//
// add lines where the hand ray picking is happening
//
var WANT_DEBUG = false;
var WANT_DEBUG_STATE = false;
var WANT_DEBUG_SEARCH_NAME = null;

//
// these tune time-averaging and "on" value for analog trigger
//

var TRIGGER_SMOOTH_RATIO = 0.1; //  Time averaging of trigger - 0.0 disables smoothing
var TRIGGER_OFF_VALUE = 0.1;
var TRIGGER_ON_VALUE = TRIGGER_OFF_VALUE + 0.05; //  Squeezed just enough to activate search or near grab

var COLLIDE_WITH_AV_AFTER_RELEASE_DELAY = 0.25; // seconds

var BUMPER_ON_VALUE = 0.5;

var THUMB_ON_VALUE = 0.5;

var HAPTIC_PULSE_STRENGTH = 1.0;
var HAPTIC_PULSE_DURATION = 13.0;
var HAPTIC_TEXTURE_STRENGTH = 0.1;
var HAPTIC_TEXTURE_DURATION = 3.0;
var HAPTIC_TEXTURE_DISTANCE = 0.002;
var HAPTIC_DEQUIP_STRENGTH = 0.75;
var HAPTIC_DEQUIP_DURATION = 50.0;


var HAND_HEAD_MIX_RATIO = 0.0; //  0 = only use hands for search/move.  1 = only use head for search/move.

var PICK_WITH_HAND_RAY = true;

var EQUIP_SPHERE_COLOR = {
    red: 116,
    green: 90,
    blue: 238
};
var EQUIP_SPHERE_ALPHA = 0.15;
var EQUIP_SPHERE_SCALE_FACTOR = 0.65;

//
// distant manipulation
//

var DISTANCE_HOLDING_RADIUS_FACTOR = 3.5; // multiplied by distance between hand and object
var DISTANCE_HOLDING_ACTION_TIMEFRAME = 0.1; // how quickly objects move to their new position
var DISTANCE_HOLDING_UNITY_MASS = 1200; //  The mass at which the distance holding action timeframe is unmodified
var DISTANCE_HOLDING_UNITY_DISTANCE = 6; //  The distance at which the distance holding action timeframe is unmodified
var MOVE_WITH_HEAD = true; // experimental head-control of distantly held objects

var COLORS_GRAB_SEARCHING_HALF_SQUEEZE = {
    red: 10,
    green: 10,
    blue: 255
};

var COLORS_GRAB_SEARCHING_FULL_SQUEEZE = {
    red: 250,
    green: 10,
    blue: 10
};

var COLORS_GRAB_DISTANCE_HOLD = {
    red: 238,
    green: 75,
    blue: 214
};


var PICK_MAX_DISTANCE = 500; // max length of pick-ray

//
// near grabbing
//

var EQUIP_RADIUS = 0.1; // radius used for palm vs equip-hotspot for equipping.
// if EQUIP_HOTSPOT_RENDER_RADIUS is greater than zero, the hotspot will appear before the hand
// has reached the required position, and then grow larger once the hand is close enough to equip.
var EQUIP_HOTSPOT_RENDER_RADIUS = 0.0; // radius used for palm vs equip-hotspot for rendering hot-spots

var NEAR_GRABBING_ACTION_TIMEFRAME = 0.05; // how quickly objects move to their new position

var NEAR_GRAB_RADIUS = 0.15; // radius used for palm vs object for near grabbing.
var NEAR_GRAB_MAX_DISTANCE = 1.0; // you cannot grab objects that are this far away from your hand

var NEAR_GRAB_PICK_RADIUS = 0.25; // radius used for search ray vs object for near grabbing.

var PICK_BACKOFF_DISTANCE = 0.2; // helps when hand is intersecting the grabble object
var NEAR_GRABBING_KINEMATIC = true; // force objects to be kinematic when near-grabbed

// if an equipped item is "adjusted" to be too far from the hand it's in, it will be unequipped.
var CHECK_TOO_FAR_UNEQUIP_TIME = 0.3; // seconds, duration between checks

//
// other constants
//

var RIGHT_HAND = 1;
var LEFT_HAND = 0;

var ZERO_VEC = {
    x: 0,
    y: 0,
    z: 0
};

var ONE_VEC = {
    x: 1,
    y: 1,
    z: 1
};

var NULL_UUID = "{00000000-0000-0000-0000-000000000000}";

// these control how long an abandoned pointer line or action will hang around
var ACTION_TTL = 15; // seconds
var ACTION_TTL_REFRESH = 5;
var PICKS_PER_SECOND_PER_HAND = 60;
var MSECS_PER_SEC = 1000.0;
var GRABBABLE_PROPERTIES = [
    "position",
    "registrationPoint",
    "rotation",
    "gravity",
    "collidesWith",
    "dynamic",
    "collisionless",
    "locked",
    "name",
    "shapeType",
    "parentID",
    "parentJointIndex",
    "density",
    "dimensions",
    "userData"
];

var GRABBABLE_DATA_KEY = "grabbableKey"; // shared with grab.js
var GRAB_USER_DATA_KEY = "grabKey"; // shared with grab.js

var DEFAULT_GRABBABLE_DATA = {
    disableReleaseVelocity: false
};

// sometimes we want to exclude objects from being picked
var USE_BLACKLIST = true;
var blacklist = [];

var FORBIDDEN_GRAB_NAMES = ["Grab Debug Entity", "grab pointer"];
var FORBIDDEN_GRAB_TYPES = ['Unknown', 'Light', 'PolyLine', 'Zone'];

// states for the state machine
var STATE_OFF = 0;
var STATE_SEARCHING = 1;
var STATE_DISTANCE_HOLDING = 2;
var STATE_NEAR_GRABBING = 3;
var STATE_NEAR_TRIGGER = 4;
var STATE_FAR_TRIGGER = 5;
var STATE_HOLD = 6;
var STATE_ENTITY_TOUCHING = 7;

// "collidesWith" is specified by comma-separated list of group names
// the possible group names are:  static, dynamic, kinematic, myAvatar, otherAvatar
var COLLIDES_WITH_WHILE_GRABBED = "dynamic,otherAvatar";
var COLLIDES_WITH_WHILE_MULTI_GRABBED = "dynamic";

var HEART_BEAT_INTERVAL = 5 * MSECS_PER_SEC;
var HEART_BEAT_TIMEOUT = 15 * MSECS_PER_SEC;

var delayedDeactivateFunc;
var delayedDeactivateTimeout;
var delayedDeactivateEntityID;

var CONTROLLER_STATE_MACHINE = {};

var mostRecentSearchingHand = RIGHT_HAND;

CONTROLLER_STATE_MACHINE[STATE_OFF] = {
    name: "off",
    enterMethod: "offEnter",
    updateMethod: "off"
};
CONTROLLER_STATE_MACHINE[STATE_SEARCHING] = {
    name: "searching",
    enterMethod: "searchEnter",
    updateMethod: "search"
};
CONTROLLER_STATE_MACHINE[STATE_DISTANCE_HOLDING] = {
    name: "distance_holding",
    enterMethod: "distanceHoldingEnter",
    updateMethod: "distanceHolding"
};
CONTROLLER_STATE_MACHINE[STATE_NEAR_GRABBING] = {
    name: "near_grabbing",
    enterMethod: "nearGrabbingEnter",
    updateMethod: "nearGrabbing"
};
CONTROLLER_STATE_MACHINE[STATE_HOLD] = {
    name: "hold",
    enterMethod: "nearGrabbingEnter",
    updateMethod: "nearGrabbing"
};
CONTROLLER_STATE_MACHINE[STATE_NEAR_TRIGGER] = {
    name: "trigger",
    enterMethod: "nearTriggerEnter",
    updateMethod: "nearTrigger"
};
CONTROLLER_STATE_MACHINE[STATE_FAR_TRIGGER] = {
    name: "far_trigger",
    enterMethod: "farTriggerEnter",
    updateMethod: "farTrigger"
};
CONTROLLER_STATE_MACHINE[STATE_ENTITY_TOUCHING] = {
    name: "entityTouching",
    enterMethod: "entityTouchingEnter",
    exitMethod: "entityTouchingExit",
    updateMethod: "entityTouching"
};

function distanceBetweenPointAndEntityBoundingBox(point, entityProps) {
    var entityXform = new Xform(entityProps.rotation, entityProps.position);
    var localPoint = entityXform.inv().xformPoint(point);
    var minOffset = Vec3.multiplyVbyV(entityProps.registrationPoint, entityProps.dimensions);
    var maxOffset = Vec3.multiplyVbyV(Vec3.subtract(ONE_VEC, entityProps.registrationPoint), entityProps.dimensions);
    var localMin = Vec3.subtract(entityXform.trans, minOffset);
    var localMax = Vec3.sum(entityXform.trans, maxOffset);

    var v = {x: localPoint.x, y: localPoint.y, z: localPoint.z};
    v.x = Math.max(v.x, localMin.x);
    v.x = Math.min(v.x, localMax.x);
    v.y = Math.max(v.y, localMin.y);
    v.y = Math.min(v.y, localMax.y);
    v.z = Math.max(v.z, localMin.z);
    v.z = Math.min(v.z, localMax.z);

    return Vec3.distance(v, localPoint);
}

function angleBetween(a, b) {
    return Math.acos(Vec3.dot(Vec3.normalize(a), Vec3.normalize(b)));
}

function projectOntoEntityXYPlane(entityID, worldPos) {
    var props = entityPropertiesCache.getProps(entityID);
    var invRot = Quat.inverse(props.rotation);
    var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(worldPos, props.position));
    var invDimensions = { x: 1 / props.dimensions.x,
                          y: 1 / props.dimensions.y,
                          z: 1 / props.dimensions.z };
    var normalizedPos = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), props.registrationPoint);
    return { x: normalizedPos.x * props.dimensions.x,
             y: (1 - normalizedPos.y) * props.dimensions.y }; // flip y-axis
}

function handLaserIntersectEntity(entityID, hand) {
    var standardControllerValue = (hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
    var pose = Controller.getPoseValue(standardControllerValue);
    var worldHandPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation), MyAvatar.position);
    var worldHandRotation = Quat.multiply(MyAvatar.orientation, pose.rotation);

    var props = entityPropertiesCache.getProps(entityID);

    if (props.position) {
        var planePosition = props.position;
        var planeNormal = Vec3.multiplyQbyV(props.rotation, {x: 0, y: 0, z: 1.0});
        var rayStart = worldHandPosition;
        var rayDirection = Quat.getUp(worldHandRotation);
        var intersectionInfo = rayIntersectPlane(planePosition, planeNormal, rayStart, rayDirection);

        var intersectionPoint = planePosition;
        if (intersectionInfo.hit && intersectionInfo.distance > 0) {
            intersectionPoint = Vec3.sum(rayStart, Vec3.multiply(intersectionInfo.distance, rayDirection));
        } else {
            intersectionPoint = planePosition;
        }
        intersectionInfo.point = intersectionPoint;
        intersectionInfo.normal = planeNormal;
        intersectionInfo.searchRay = {
            origin: rayStart,
            direction: rayDirection,
            length: PICK_MAX_DISTANCE
        };
        return intersectionInfo;
    } else {
        // entity has been destroyed? or is no longer in cache
        return null;
    }
}

function rayIntersectPlane(planePosition, planeNormal, rayStart, rayDirection) {
    var rayDirectionDotPlaneNormal = Vec3.dot(rayDirection, planeNormal);
    if (rayDirectionDotPlaneNormal > 0.00001 || rayDirectionDotPlaneNormal < -0.00001) {
        var rayStartDotPlaneNormal = Vec3.dot(Vec3.subtract(planePosition, rayStart), planeNormal);
        var distance = rayStartDotPlaneNormal / rayDirectionDotPlaneNormal;
        return {hit: true, distance: distance};
    } else {
        // ray is parallel to the plane
        return {hit: false, distance: 0};
    }
}

function stateToName(state) {
    return CONTROLLER_STATE_MACHINE[state] ? CONTROLLER_STATE_MACHINE[state].name : "???";
}

function getTag() {
    return "grab-" + MyAvatar.sessionUUID;
}

function colorPow(color, power) {
    return {
        red: Math.pow(color.red / 255.0, power) * 255,
        green: Math.pow(color.green / 255.0, power) * 255,
        blue: Math.pow(color.blue / 255.0, power) * 255
    };
}

function entityHasActions(entityID) {
    return Entities.getActionIDs(entityID).length > 0;
}

function findRayIntersection(pickRay, precise, include, exclude) {
    var entities = Entities.findRayIntersection(pickRay, precise, include, exclude);
    var overlays = Overlays.findRayIntersection(pickRay);
    if (!overlays.intersects || (entities.intersects && (entities.distance <= overlays.distance))) {
        return entities;
    }
    return overlays;
}

function entityIsGrabbedByOther(entityID) {
    // by convention, a distance grab sets the tag of its action to be grab-*owner-session-id*.
    var actionIDs = Entities.getActionIDs(entityID);
    for (var actionIndex = 0; actionIndex < actionIDs.length; actionIndex++) {
        var actionID = actionIDs[actionIndex];
        var actionArguments = Entities.getActionArguments(entityID, actionID);
        var tag = actionArguments["tag"];
        if (tag == getTag()) {
            // we see a grab-*uuid* shaped tag, but it's our tag, so that's okay.
            continue;
        }
        if (tag.slice(0, 5) == "grab-") {
            // we see a grab-*uuid* shaped tag and it's not ours, so someone else is grabbing it.
            return true;
        }
    }
    return false;
}

function propsArePhysical(props) {
    if (!props.dynamic) {
        return false;
    }
    var isPhysical = (props.shapeType && props.shapeType != 'none');
    return isPhysical;
}

var USE_ATTACH_POINT_SETTINGS = true;

var ATTACH_POINT_SETTINGS = "io.highfidelity.attachPoints";

function getAttachPointSettings() {
    try {
        var str = Settings.getValue(ATTACH_POINT_SETTINGS);
        if (str === "false") {
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
    var attachPointSettings = getAttachPointSettings();
    var jointName = (hand === RIGHT_HAND) ? "RightHand" : "LeftHand";
    var joints = attachPointSettings[hotspot.key];
    if (joints) {
        return joints[jointName];
    } else {
        return undefined;
    }
}

function storeAttachPointForHotspotInSettings(hotspot, hand, offsetPosition, offsetRotation) {
    var attachPointSettings = getAttachPointSettings();
    var jointName = (hand === RIGHT_HAND) ? "RightHand" : "LeftHand";
    var joints = attachPointSettings[hotspot.key];
    if (!joints) {
        joints = {};
        attachPointSettings[hotspot.key] = joints;
    }
    joints[jointName] = [offsetPosition, offsetRotation];
    setAttachPointSettings(attachPointSettings);
}

function removeMyAvatarFromCollidesWith(origCollidesWith) {
    var collidesWithSplit = origCollidesWith.split(",");
    // remove myAvatar from the array
    for (var i = collidesWithSplit.length - 1; i >= 0; i--) {
        if (collidesWithSplit[i] === "myAvatar") {
            collidesWithSplit.splice(i, 1);
        }
    }
    return collidesWithSplit.join();
}

// If another script is managing the reticle (as is done by HandControllerPointer), we should not be setting it here,
// and we should not be showing lasers when someone else is using the Reticle to indicate a 2D minor mode.
var EXTERNALLY_MANAGED_2D_MINOR_MODE = true;
var EDIT_SETTING = "io.highfidelity.isEditting";

function isEditing() {
    var actualSettingValue = Settings.getValue(EDIT_SETTING) === "false" ? false : !!Settings.getValue(EDIT_SETTING);
    return EXTERNALLY_MANAGED_2D_MINOR_MODE && actualSettingValue;
}

function isIn2DMode() {
    // In this version, we make our own determination of whether we're aimed a HUD element,
    // because other scripts (such as handControllerPointer) might be using some other visualization
    // instead of setting Reticle.visible.
    return (EXTERNALLY_MANAGED_2D_MINOR_MODE &&
        (Reticle.pointingAtSystemOverlay || Overlays.getOverlayAtPoint(Reticle.position)));
}

function restore2DMode() {
    if (!EXTERNALLY_MANAGED_2D_MINOR_MODE) {
        Reticle.setVisible(true);
    }
}

// EntityPropertiesCache is a helper class that contains a cache of entity properties.
// the hope is to prevent excess calls to Entity.getEntityProperties()
//
// usage:
//   call EntityPropertiesCache.addEntities with all the entities that you are interested in.
//   This will fetch their properties.  Then call EntityPropertiesCache.getProps to receive an object
//   containing a cache of all the properties previously fetched.
function EntityPropertiesCache() {
    this.cache = {};
}
EntityPropertiesCache.prototype.clear = function() {
    this.cache = {};
};
EntityPropertiesCache.prototype.addEntity = function(entityID) {
    var cacheEntry = this.cache[entityID];
    if (cacheEntry && cacheEntry.refCount) {
        cacheEntry.refCount += 1;
    } else {
        this._updateCacheEntry(entityID);
    }
};
EntityPropertiesCache.prototype.addEntities = function(entities) {
    var _this = this;
    entities.forEach(function(entityID) {
        _this.addEntity(entityID);
    });
};
EntityPropertiesCache.prototype._updateCacheEntry = function(entityID) {
    var props = Entities.getEntityProperties(entityID, GRABBABLE_PROPERTIES);

    // convert props.userData from a string to an object.
    var userData = {};
    if (props.userData) {
        try {
            userData = JSON.parse(props.userData);
        } catch (err) {
            print("WARNING: malformed userData on " + entityID + ", name = " + props.name + ", error = " + err);
        }
    }
    props.userData = userData;
    props.refCount = 1;

    this.cache[entityID] = props;
};
EntityPropertiesCache.prototype.update = function() {
    // delete any cacheEntries with zero refCounts.
    var entities = Object.keys(this.cache);
    for (var i = 0; i < entities.length; i++) {
        var props = this.cache[entities[i]];
        if (props.refCount === 0) {
            delete this.cache[entities[i]];
        } else {
            props.refCount = 0;
        }
    }
};
EntityPropertiesCache.prototype.getProps = function(entityID) {
    var obj = this.cache[entityID];
    return obj ? obj : undefined;
};
EntityPropertiesCache.prototype.getGrabbableProps = function(entityID) {
    var props = this.cache[entityID];
    if (props) {
        return props.userData.grabbableKey ? props.userData.grabbableKey : DEFAULT_GRABBABLE_DATA;
    } else {
        return undefined;
    }
};
EntityPropertiesCache.prototype.getGrabProps = function(entityID) {
    var props = this.cache[entityID];
    if (props) {
        return props.userData.grabKey ? props.userData.grabKey : {};
    } else {
        return undefined;
    }
};
EntityPropertiesCache.prototype.getWearableProps = function(entityID) {
    var props = this.cache[entityID];
    if (props) {
        return props.userData.wearable ? props.userData.wearable : {};
    } else {
        return undefined;
    }
};
EntityPropertiesCache.prototype.getEquipHotspotsProps = function(entityID) {
    var props = this.cache[entityID];
    if (props) {
        return props.userData.equipHotspots ? props.userData.equipHotspots : {};
    } else {
        return undefined;
    }
};

// global cache
var entityPropertiesCache = new EntityPropertiesCache();

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

        if (hotspot.modelURL) {
            // override default sphere with a user specified model
            overlayInfoSet.overlays.push(Overlays.addOverlay("model", {
                url: hotspot.modelURL,
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
        } else {
            // default sphere overlay
            overlayInfoSet.overlays.push(Overlays.addOverlay("sphere", {
                position: hotspot.worldPosition,
                rotation: {
                    x: 0,
                    y: 0,
                    z: 0,
                    w: 1
                },
                dimensions: diameter * EQUIP_SPHERE_SCALE_FACTOR,
                color: EQUIP_SPHERE_COLOR,
                alpha: EQUIP_SPHERE_ALPHA,
                solid: true,
                visible: true,
                ignoreRayIntersection: true,
                drawInFront: false
            }));
            overlayInfoSet.type = "sphere";
        }

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
EquipHotspotBuddy.prototype.update = function(deltaTime, timestamp) {

    var HIGHLIGHT_SIZE = 1.1;
    var NORMAL_SIZE = 1.0;

    var keys = Object.keys(this.map);
    for (var i = 0; i < keys.length; i++) {
        var overlayInfoSet = this.map[keys[i]];

        // this overlayInfo is highlighted.
        if (this.highlightedHotspots.indexOf(keys[i]) != -1) {
            overlayInfoSet.targetSize = HIGHLIGHT_SIZE;
        } else {
            overlayInfoSet.targetSize = NORMAL_SIZE;
        }

        // start to fade out this hotspot.
        if (overlayInfoSet.timestamp != timestamp) {
            // because this item timestamp has expired, it might not be in the cache anymore....
            entityPropertiesCache.addEntity(overlayInfoSet.entityID);
            overlayInfoSet.targetSize = 0;
        }

        // animate the size.
        var SIZE_TIMESCALE = 0.1;
        var tau = deltaTime / SIZE_TIMESCALE;
        if (tau > 1.0) {
            tau = 1.0;
        }
        overlayInfoSet.currentSize += (overlayInfoSet.targetSize - overlayInfoSet.currentSize) * tau;

        if (overlayInfoSet.timestamp != timestamp && overlayInfoSet.currentSize <= 0.05) {
            // this is an old overlay, that has finished fading out, delete it!
            overlayInfoSet.overlays.forEach(function(overlay) {
                Overlays.deleteOverlay(overlay);
            });
            delete this.map[keys[i]];
        } else {
            // update overlay position, rotation to follow the object it's attached to.

            var props = entityPropertiesCache.getProps(overlayInfoSet.entityID);
            var entityXform = new Xform(props.rotation, props.position);
            var position = entityXform.xformPoint(overlayInfoSet.localPosition);

            var dimensions;
            if (overlayInfoSet.type == "sphere") {
                dimensions = overlayInfoSet.hotspot.radius * 2 * overlayInfoSet.currentSize * EQUIP_SPHERE_SCALE_FACTOR;
            } else {
                dimensions = overlayInfoSet.hotspot.radius * 2 * overlayInfoSet.currentSize;
            }

            overlayInfoSet.overlays.forEach(function(overlay) {
                Overlays.editOverlay(overlay, {
                    position: position,
                    rotation: props.rotation,
                    dimensions: dimensions
                });
            });
        }
    }
};

// global EquipHotspotBuddy instance
var equipHotspotBuddy = new EquipHotspotBuddy();

function MyController(hand) {
    this.hand = hand;
    if (this.hand === RIGHT_HAND) {
        this.getHandPosition = MyAvatar.getRightPalmPosition;
        // this.getHandRotation = MyAvatar.getRightPalmRotation;
    } else {
        this.getHandPosition = MyAvatar.getLeftPalmPosition;
        // this.getHandRotation = MyAvatar.getLeftPalmRotation;
    }
    this.getHandRotation = function() {
        var controllerHandInput = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        return Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(controllerHandInput).rotation);
    };

    this.actionID = null; // action this script created...
    this.grabbedEntity = null; // on this entity.
    this.state = STATE_OFF;
    this.pointer = null; // entity-id of line object
    this.entityActivated = false;

    this.triggerValue = 0; // rolling average of trigger value
    this.triggerClicked = false;
    this.rawTriggerValue = 0;
    this.rawSecondaryValue = 0;
    this.rawThumbValue = 0;

    // for visualizations
    this.overlayLine = null;

    // for lights
    this.overlayLine = null;
    this.searchSphere = null;

    this.waitForTriggerRelease = false;

    // how far from camera to search intersection?
    var DEFAULT_SEARCH_SPHERE_DISTANCE = 1000;
    this.intersectionDistance = 0.0;
    this.searchSphereDistance = DEFAULT_SEARCH_SPHERE_DISTANCE;

    this.ignoreIK = false;
    this.offsetPosition = Vec3.ZERO;
    this.offsetRotation = Quat.IDENTITY;

    this.lastPickTime = 0;
    this.lastUnequipCheckTime = 0;

    this.equipOverlayInfoSetMap = {};

    var _this = this;

    var suppressedIn2D = [STATE_OFF, STATE_SEARCHING];
    this.ignoreInput = function() {
        // We've made the decision to use 'this' for new code, even though it is fragile,
        // in order to keep/ the code uniform without making any no-op line changes.
        return (-1 !== suppressedIn2D.indexOf(this.state)) && isIn2DMode();
    };

    this.update = function(deltaTime, timestamp) {

        this.updateSmoothedTrigger();

        if (this.ignoreInput()) {
            this.turnOffVisualizations();
            return;
        }

        if (CONTROLLER_STATE_MACHINE[this.state]) {
            var updateMethodName = CONTROLLER_STATE_MACHINE[this.state].updateMethod;
            var updateMethod = this[updateMethodName];
            if (updateMethod) {
                updateMethod.call(this, deltaTime, timestamp);
            } else {
                print("WARNING: could not find updateMethod for state " + stateToName(this.state));
            }
        } else {
            print("WARNING: could not find state " + this.state + " in state machine");
        }
    };

    this.callEntityMethodOnGrabbed = function(entityMethodName) {
        var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
        Entities.callEntityMethod(this.grabbedEntity, entityMethodName, args);
    };

    this.setState = function(newState, reason) {

        if (WANT_DEBUG || WANT_DEBUG_STATE) {
            var oldStateName = stateToName(this.state);
            var newStateName = stateToName(newState);
            print("STATE (" + this.hand + "): " + newStateName + " <-- " + oldStateName + ", reason = " + reason);
        }

        // exit the old state
        if (CONTROLLER_STATE_MACHINE[this.state]) {
            var exitMethodName = CONTROLLER_STATE_MACHINE[this.state].exitMethod;
            var exitMethod = this[exitMethodName];
            if (exitMethod) {
                exitMethod.call(this);
            }
        } else {
            print("WARNING: could not find state " + this.state + " in state machine");
        }

        this.state = newState;

        // enter the new state
        if (CONTROLLER_STATE_MACHINE[newState]) {
            var enterMethodName = CONTROLLER_STATE_MACHINE[newState].enterMethod;
            var enterMethod = this[enterMethodName];
            if (enterMethod) {
                enterMethod.call(this);
            }
        } else {
            print("WARNING: could not find newState " + newState + " in state machine");
        }
    };

    this.searchSphereOn = function(location, size, color) {

        var rotation = Quat.lookAt(location, Camera.getPosition(), Vec3.UP);
        var brightColor = colorPow(color, 0.06);
        if (this.searchSphere === null) {
            var sphereProperties = {
                position: location,
                rotation: rotation,
                outerRadius: size * 1.2,
                innerColor: brightColor,
                outerColor: color,
                innerAlpha: 0.9,
                outerAlpha: 0.0,
                solid: true,
                ignoreRayIntersection: true,
                drawInFront: true, // Even when burried inside of something, show it.
                visible: true
            };
            this.searchSphere = Overlays.addOverlay("circle3d", sphereProperties);
        } else {
            Overlays.editOverlay(this.searchSphere, {
                position: location,
                rotation: rotation,
                innerColor: brightColor,
                outerColor: color,
                innerAlpha: 1.0,
                outerAlpha: 0.0,
                outerRadius: size * 1.2,
                visible: true
            });
        }
    };

    this.overlayLineOn = function(closePoint, farPoint, color) {
        if (this.overlayLine === null) {
            var lineProperties = {
                glow: 1.0,
                start: closePoint,
                end: farPoint,
                color: color,
                ignoreRayIntersection: true, // always ignore this
                drawInFront: true, // Even when burried inside of something, show it.
                visible: true,
                alpha: 1
            };
            this.overlayLine = Overlays.addOverlay("line3d", lineProperties);

        } else {
            Overlays.editOverlay(this.overlayLine, {
                lineWidth: 5,
                start: closePoint,
                end: farPoint,
                color: color,
                visible: true,
                ignoreRayIntersection: true, // always ignore this
                drawInFront: true, // Even when burried inside of something, show it.
                alpha: 1
            });
        }
    };

    this.searchIndicatorOn = function(distantPickRay) {
        var handPosition = distantPickRay.origin;
        var SEARCH_SPHERE_SIZE = 0.011;
        var SEARCH_SPHERE_FOLLOW_RATE = 0.50;

        if (this.intersectionDistance > 0) {
            //  If we hit something with our pick ray, move the search sphere toward that distance
            this.searchSphereDistance = this.searchSphereDistance * SEARCH_SPHERE_FOLLOW_RATE +
                this.intersectionDistance * (1.0 - SEARCH_SPHERE_FOLLOW_RATE);
        }

        var searchSphereLocation = Vec3.sum(distantPickRay.origin,
            Vec3.multiply(distantPickRay.direction, this.searchSphereDistance));
        this.searchSphereOn(searchSphereLocation, SEARCH_SPHERE_SIZE * this.searchSphereDistance,
            (this.triggerSmoothedGrab() || this.secondarySqueezed()) ? COLORS_GRAB_SEARCHING_FULL_SQUEEZE : COLORS_GRAB_SEARCHING_HALF_SQUEEZE);
        if (PICK_WITH_HAND_RAY) {
            this.overlayLineOn(handPosition, searchSphereLocation,
                (this.triggerSmoothedGrab() || this.secondarySqueezed()) ? COLORS_GRAB_SEARCHING_FULL_SQUEEZE : COLORS_GRAB_SEARCHING_HALF_SQUEEZE);
        }
    };

    this.evalLightWorldTransform = function(modelPos, modelRot) {

        var MODEL_LIGHT_POSITION = {
            x: 0,
            y: -0.3,
            z: 0
        };

        var MODEL_LIGHT_ROTATION = Quat.angleAxis(-90, {
            x: 1,
            y: 0,
            z: 0
        });

        return {
            p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
            q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
        };
    };

    this.lineOff = function() {
        if (this.pointer !== null) {
            Entities.deleteEntity(this.pointer);
        }
        this.pointer = null;
    };

    this.overlayLineOff = function() {
        if (this.overlayLine !== null) {
            Overlays.deleteOverlay(this.overlayLine);
        }
        this.overlayLine = null;
    };

    this.searchSphereOff = function() {
        if (this.searchSphere !== null) {
            Overlays.deleteOverlay(this.searchSphere);
            this.searchSphere = null;
            this.searchSphereDistance = DEFAULT_SEARCH_SPHERE_DISTANCE;
            this.intersectionDistance = 0.0;
        }
    };

    this.turnOffVisualizations = function() {

        this.overlayLineOff();

        this.searchSphereOff();
        restore2DMode();

    };

    this.triggerPress = function(value) {
        _this.rawTriggerValue = value;
    };

    this.triggerClick = function(value) {
        _this.triggerClicked = value;
    };

    this.secondaryPress = function(value) {
        _this.rawSecondaryValue = value;
    };

    this.updateSmoothedTrigger = function() {
        var triggerValue = this.rawTriggerValue;
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

    this.secondarySqueezed = function() {
        return _this.rawSecondaryValue > BUMPER_ON_VALUE;
    };

    this.secondaryReleased = function() {
        return _this.rawSecondaryValue < BUMPER_ON_VALUE;
    };

    // this.triggerOrsecondarySqueezed = function () {
    //     return triggerSmoothedSqueezed() || secondarySqueezed();
    // }

    // this.triggerAndSecondaryReleased = function () {
    //     return triggerSmoothedReleased() && secondaryReleased();
    // }

    this.thumbPress = function(value) {
        _this.rawThumbValue = value;
    };

    this.thumbPressed = function() {
        return _this.rawThumbValue > THUMB_ON_VALUE;
    };

    this.thumbReleased = function() {
        return _this.rawThumbValue < THUMB_ON_VALUE;
    };

    this.off = function(deltaTime, timestamp) {
        if (this.triggerSmoothedReleased()) {
            this.waitForTriggerRelease = false;
        }
        if (!this.waitForTriggerRelease && this.triggerSmoothedSqueezed()) {
            this.lastPickTime = 0;
            var controllerHandInput = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
            this.startingHandRotation = Controller.getPoseValue(controllerHandInput).rotation;
            if (this.triggerSmoothedSqueezed()) {
                this.setState(STATE_SEARCHING, "trigger squeeze detected");
                return;
            }
        }

        var candidateEntities = Entities.findEntities(this.getHandPosition(), EQUIP_HOTSPOT_RENDER_RADIUS);
        entityPropertiesCache.addEntities(candidateEntities);
        var potentialEquipHotspot = this.chooseBestEquipHotspot(candidateEntities);
        if (!this.waitForTriggerRelease) {
            this.updateEquipHaptics(potentialEquipHotspot, this.getHandPosition());
        }

        var nearEquipHotspots = this.chooseNearEquipHotspots(candidateEntities, EQUIP_HOTSPOT_RENDER_RADIUS);
        equipHotspotBuddy.updateHotspots(nearEquipHotspots, timestamp);
        if (potentialEquipHotspot) {
            equipHotspotBuddy.highlightHotspot(potentialEquipHotspot);
        }
    };

    this.clearEquipHaptics = function() {
        this.prevPotentialEquipHotspot = null;
    };

    this.updateEquipHaptics = function(potentialEquipHotspot, currentLocation) {
        if (potentialEquipHotspot && !this.prevPotentialEquipHotspot ||
            !potentialEquipHotspot && this.prevPotentialEquipHotspot) {
            Controller.triggerHapticPulse(HAPTIC_TEXTURE_STRENGTH, HAPTIC_TEXTURE_DURATION, this.hand);
            this.lastHapticPulseLocation = currentLocation;
        } else if (potentialEquipHotspot && Vec3.distance(this.lastHapticPulseLocation, currentLocation) > HAPTIC_TEXTURE_DISTANCE) {
            Controller.triggerHapticPulse(HAPTIC_TEXTURE_STRENGTH, HAPTIC_TEXTURE_DURATION, this.hand);
            this.lastHapticPulseLocation = currentLocation;
        }
        this.prevPotentialEquipHotspot = potentialEquipHotspot;
    };

    // Performs ray pick test from the hand controller into the world
    // @param {number} which hand to use, RIGHT_HAND or LEFT_HAND
    // @returns {object} returns object with two keys entityID and distance
    //
    this.calcRayPickInfo = function(hand) {

        var standardControllerValue = (hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var pose = Controller.getPoseValue(standardControllerValue);
        var worldHandPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation), MyAvatar.position);
        var worldHandRotation = Quat.multiply(MyAvatar.orientation, pose.rotation);

        var pickRay = {
            origin: PICK_WITH_HAND_RAY ? worldHandPosition : Camera.position,
            direction: PICK_WITH_HAND_RAY ? Quat.getUp(worldHandRotation) : Vec3.mix(Quat.getUp(worldHandRotation),
                Quat.getFront(Camera.orientation),
                HAND_HEAD_MIX_RATIO),
            length: PICK_MAX_DISTANCE
        };

        var result = {
            entityID: null,
            searchRay: pickRay,
            distance: PICK_MAX_DISTANCE
        };

        var now = Date.now();
        if (now - this.lastPickTime < MSECS_PER_SEC / PICKS_PER_SECOND_PER_HAND) {
            return result;
        }
        this.lastPickTime = now;

        var directionNormalized = Vec3.normalize(pickRay.direction);
        var directionBacked = Vec3.multiply(directionNormalized, PICK_BACKOFF_DISTANCE);
        var pickRayBacked = {
            origin: Vec3.subtract(pickRay.origin, directionBacked),
            direction: pickRay.direction
        };

        var intersection;
        if (USE_BLACKLIST === true && blacklist.length !== 0) {
            intersection = findRayIntersection(pickRayBacked, true, [], blacklist);
        } else {
            intersection = findRayIntersection(pickRayBacked, true);
        }

        if (intersection.intersects) {
            return {
                entityID: intersection.entityID,
                overlayID: intersection.overlayID,
                searchRay: pickRay,
                distance: Vec3.distance(pickRay.origin, intersection.intersection),
                intersection: intersection.intersection,
                normal: intersection.surfaceNormal
            };
        } else {
            return result;
        }
    };

    this.entityWantsTrigger = function(entityID) {
        var grabbableProps = entityPropertiesCache.getGrabbableProps(entityID);
        return grabbableProps && grabbableProps.wantsTrigger;
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
    this.collectEquipHotspots = function(entityID) {
        var result = [];
        var props = entityPropertiesCache.getProps(entityID);
        var entityXform = new Xform(props.rotation, props.position);
        var equipHotspotsProps = entityPropertiesCache.getEquipHotspotsProps(entityID);
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
            var wearableProps = entityPropertiesCache.getWearableProps(entityID);
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
                    radius: EQUIP_RADIUS,
                    joints: wearableProps.joints,
                    modelURL: null,
                    modelScale: null
                });
            }
        }
        return result;
    };

    this.hotspotIsEquippable = function(hotspot) {
        var props = entityPropertiesCache.getProps(hotspot.entityID);
        var grabProps = entityPropertiesCache.getGrabProps(hotspot.entityID);
        var debug = (WANT_DEBUG_SEARCH_NAME && props.name === WANT_DEBUG_SEARCH_NAME);

        var refCount = ("refCount" in grabProps) ? grabProps.refCount : 0;
        var okToEquipFromOtherHand = ((this.getOtherHandController().state == STATE_NEAR_GRABBING ||
                this.getOtherHandController().state == STATE_DISTANCE_HOLDING) &&
            this.getOtherHandController().grabbedEntity == hotspot.entityID);
        if (refCount > 0 && !okToEquipFromOtherHand) {
            if (debug) {
                print("equip is skipping '" + props.name + "': grabbed by someone else");
            }
            return false;
        }

        return true;
    };

    this.entityIsGrabbable = function(entityID) {
        var grabbableProps = entityPropertiesCache.getGrabbableProps(entityID);
        var grabProps = entityPropertiesCache.getGrabProps(entityID);
        var props = entityPropertiesCache.getProps(entityID);
        var physical = propsArePhysical(props);
        var grabbable = false;
        var debug = (WANT_DEBUG_SEARCH_NAME && props.name === WANT_DEBUG_SEARCH_NAME);

        if (physical) {
            // physical things default to grabbable
            grabbable = true;
        } else {
            // non-physical things default to non-grabbable unless they are already grabbed
            if ("refCount" in grabProps && grabProps.refCount > 0) {
                grabbable = true;
            } else {
                grabbable = false;
            }
        }

        if (grabbableProps.hasOwnProperty("grabbable")) {
            grabbable = grabbableProps.grabbable;
        }

        if (!grabbable && !grabbableProps.wantsTrigger) {
            if (debug) {
                print("grab is skipping '" + props.name + "': not grabbable.");
            }
            return false;
        }
        if (FORBIDDEN_GRAB_TYPES.indexOf(props.type) >= 0) {
            if (debug) {
                print("grab is skipping '" + props.name + "': forbidden entity type.");
            }
            return false;
        }
        if (props.locked && !grabbableProps.wantsTrigger) {
            if (debug) {
                print("grab is skipping '" + props.name + "': locked and not triggerable.");
            }
            return false;
        }
        if (FORBIDDEN_GRAB_NAMES.indexOf(props.name) >= 0) {
            if (debug) {
                print("grab is skipping '" + props.name + "': forbidden name.");
            }
            return false;
        }

        return true;
    };

    this.entityIsDistanceGrabbable = function(entityID, handPosition) {
        if (!this.entityIsGrabbable(entityID)) {
            return false;
        }

        var props = entityPropertiesCache.getProps(entityID);
        var distance = Vec3.distance(props.position, handPosition);
        var debug = (WANT_DEBUG_SEARCH_NAME && props.name === WANT_DEBUG_SEARCH_NAME);

        // we can't distance-grab non-physical
        var isPhysical = propsArePhysical(props);
        if (!isPhysical) {
            if (debug) {
                print("distance grab is skipping '" + props.name + "': not physical");
            }
            return false;
        }

        if (distance > PICK_MAX_DISTANCE) {
            // too far away, don't grab
            if (debug) {
                print("distance grab is skipping '" + props.name + "': too far away.");
            }
            return false;
        }

        if (entityIsGrabbedByOther(entityID)) {
            // don't distance grab something that is already grabbed.
            if (debug) {
                print("distance grab is skipping '" + props.name + "': already grabbed by another.");
            }
            return false;
        }

        return true;
    };

    this.entityIsNearGrabbable = function(entityID, handPosition, maxDistance) {

        if (!this.entityIsGrabbable(entityID)) {
            return false;
        }

        var props = entityPropertiesCache.getProps(entityID);
        var distance = Vec3.distance(props.position, handPosition);
        var debug = (WANT_DEBUG_SEARCH_NAME && props.name === WANT_DEBUG_SEARCH_NAME);

        if (distance > maxDistance) {
            // too far away, don't grab
            if (debug) {
                print(" grab is skipping '" + props.name + "': too far away.");
            }
            return false;
        }

        return true;
    };

    this.chooseNearEquipHotspots = function(candidateEntities, distance) {
        var equippableHotspots = flatten(candidateEntities.map(function(entityID) {
            return _this.collectEquipHotspots(entityID);
        })).filter(function(hotspot) {
            return (_this.hotspotIsEquippable(hotspot) &&
                Vec3.distance(hotspot.worldPosition, _this.getHandPosition()) < hotspot.radius + distance);
        });
        return equippableHotspots;
    };

    this.chooseBestEquipHotspot = function(candidateEntities) {
        var DISTANCE = 0;
        var equippableHotspots = this.chooseNearEquipHotspots(candidateEntities, DISTANCE);
        if (equippableHotspots.length > 0) {
            // sort by distance
            equippableHotspots.sort(function(a, b) {
                var aDistance = Vec3.distance(a.worldPosition, this.getHandPosition());
                var bDistance = Vec3.distance(b.worldPosition, this.getHandPosition());
                return aDistance - bDistance;
            });
            return equippableHotspots[0];
        } else {
            return null;
        }
    };

    this.searchEnter = function() {
        mostRecentSearchingHand = this.hand;
        var rayPickInfo = this.calcRayPickInfo(this.hand);
        if (rayPickInfo.entityID || rayPickInfo.overlayID) {
            this.intersectionDistance = rayPickInfo.distance;
            this.searchSphereDistance = this.intersectionDistance;
        }
    };

    this.search = function(deltaTime, timestamp) {
        var _this = this;
        var name;

        this.grabbedEntity = null;
        this.isInitialGrab = false;
        this.shouldResetParentOnRelease = false;

        this.checkForStrayChildren();

        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_OFF, "trigger released");
            return;
        }

        var handPosition = this.getHandPosition();

        var rayPickInfo = this.calcRayPickInfo(this.hand);

        if (rayPickInfo.entityID) {
            entityPropertiesCache.addEntity(rayPickInfo.entityID);
        }

        var candidateEntities = Entities.findEntities(handPosition, NEAR_GRAB_RADIUS);
        entityPropertiesCache.addEntities(candidateEntities);

        var potentialEquipHotspot = this.chooseBestEquipHotspot(candidateEntities);
        if (potentialEquipHotspot) {
            if (this.triggerSmoothedGrab()) {
                this.grabbedHotspot = potentialEquipHotspot;
                this.grabbedEntity = potentialEquipHotspot.entityID;
                this.setState(STATE_HOLD, "equipping '" + entityPropertiesCache.getProps(this.grabbedEntity).name + "'");
                return;
            }
        }

        var grabbableEntities = candidateEntities.filter(function(entity) {
            return _this.entityIsNearGrabbable(entity, handPosition, NEAR_GRAB_MAX_DISTANCE);
        });

        if (rayPickInfo.entityID) {
            this.intersectionDistance = rayPickInfo.distance;
            if (this.entityIsGrabbable(rayPickInfo.entityID) && rayPickInfo.distance < NEAR_GRAB_PICK_RADIUS) {
                grabbableEntities.push(rayPickInfo.entityID);
            }
        } else if (rayPickInfo.overlayID) {
            this.intersectionDistance = rayPickInfo.distance;
        } else {
            this.intersectionDistance = 0;
        }

        var entity;
        if (grabbableEntities.length > 0) {
            // sort by distance
            grabbableEntities.sort(function(a, b) {
                var aDistance = Vec3.distance(entityPropertiesCache.getProps(a).position, handPosition);
                var bDistance = Vec3.distance(entityPropertiesCache.getProps(b).position, handPosition);
                return aDistance - bDistance;
            });
            entity = grabbableEntities[0];
            name = entityPropertiesCache.getProps(entity).name;
            this.grabbedEntity = entity;
            if (this.entityWantsTrigger(entity)) {
                if (this.triggerSmoothedGrab()) {
                    this.setState(STATE_NEAR_TRIGGER, "near trigger '" + name + "'");
                    return;
                } else {
                    // potentialNearTriggerEntity = entity;
                }
            } else {
                if (this.triggerSmoothedGrab()) {
                    var props = entityPropertiesCache.getProps(entity);
                    var grabProps = entityPropertiesCache.getGrabProps(entity);
                    var refCount = grabProps.refCount ? grabProps.refCount : 0;
                    if (refCount >= 1) {
                        // if another person is holding the object, remember to restore the
                        // parent info, when we are finished grabbing it.
                        this.shouldResetParentOnRelease = true;
                        this.previousParentID = props.parentID;
                        this.previousParentJointIndex = props.parentJointIndex;
                    }

                    this.setState(STATE_NEAR_GRABBING, "near grab '" + name + "'");
                    return;
                } else {
                    // potentialNearGrabEntity = entity;
                }
            }
        }

        var pointerEvent;
        if (rayPickInfo.entityID && Entities.wantsHandControllerPointerEvents(rayPickInfo.entityID)) {
            entity = rayPickInfo.entityID;
            name = entityPropertiesCache.getProps(entity).name;

            if (Entities.keyboardFocusEntity != entity) {
                Entities.keyboardFocusEntity = entity;

                pointerEvent = {
                    type: "Move",
                    id: this.hand + 1, // 0 is reserved for hardware mouse
                    pos2D: projectOntoEntityXYPlane(entity, rayPickInfo.intersection),
                    pos3D: rayPickInfo.intersection,
                    normal: rayPickInfo.normal,
                    direction: rayPickInfo.searchRay.direction,
                    button: "None"
                };

                this.hoverEntity = entity;
                Entities.sendHoverEnterEntity(entity, pointerEvent);
            }

            // send mouse events for button highlights and tooltips.
            if (this.hand == mostRecentSearchingHand || (this.hand !== mostRecentSearchingHand &&
                                                         this.getOtherHandController().state !== STATE_SEARCHING &&
                                                         this.getOtherHandController().state !== STATE_ENTITY_TOUCHING)) {

                // most recently searching hand has priority over other hand, for the purposes of button highlighting.
                pointerEvent = {
                    type: "Move",
                    id: this.hand + 1, // 0 is reserved for hardware mouse
                    pos2D: projectOntoEntityXYPlane(entity, rayPickInfo.intersection),
                    pos3D: rayPickInfo.intersection,
                    normal: rayPickInfo.normal,
                    direction: rayPickInfo.searchRay.direction,
                    button: "None"
                };

                Entities.sendMouseMoveOnEntity(entity, pointerEvent);
                Entities.sendHoverOverEntity(entity, pointerEvent);
            }

            if (this.triggerSmoothedGrab() && !isEditing()) {
                this.grabbedEntity = entity;
                this.setState(STATE_ENTITY_TOUCHING, "begin touching entity '" + name + "'");
                return;
            }
        } else if (this.hoverEntity) {
            pointerEvent = {
                type: "Move",
                id: this.hand + 1
            };
            Entities.sendHoverLeaveEntity(this.hoverEntity, pointerEvent);
            this.hoverEntity = null;
        }

        if (rayPickInfo.entityID) {
            entity = rayPickInfo.entityID;
            name = entityPropertiesCache.getProps(entity).name;
            if (this.entityWantsTrigger(entity)) {
                if (this.triggerSmoothedGrab()) {
                    this.grabbedEntity = entity;
                    this.setState(STATE_FAR_TRIGGER, "far trigger '" + name + "'");
                    return;
                } else {
                    // potentialFarTriggerEntity = entity;
                }
            } else if (this.entityIsDistanceGrabbable(rayPickInfo.entityID, handPosition)) {
                if (this.triggerSmoothedGrab() && !isEditing()) {
                    this.grabbedEntity = entity;
                    this.setState(STATE_DISTANCE_HOLDING, "distance hold '" + name + "'");
                    return;
                } else {
                    // potentialFarGrabEntity = entity;
                }
            }
        }

        this.updateEquipHaptics(potentialEquipHotspot, handPosition);

        var nearEquipHotspots = this.chooseNearEquipHotspots(candidateEntities, EQUIP_HOTSPOT_RENDER_RADIUS);
        equipHotspotBuddy.updateHotspots(nearEquipHotspots, timestamp);
        if (potentialEquipHotspot) {
            equipHotspotBuddy.highlightHotspot(potentialEquipHotspot);
        }

        this.searchIndicatorOn(rayPickInfo.searchRay);
        Reticle.setVisible(false);
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

    this.distanceHoldingEnter = function() {
        Messages.sendLocalMessage('Hifi-Teleport-Disabler','both');
        this.clearEquipHaptics();

        // controller pose is in avatar frame
        var device = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var avatarControllerPose = Controller.getPoseValue(device);

        // transform it into world frame
        var worldControllerPosition = Vec3.sum(MyAvatar.position,
            Vec3.multiplyQbyV(MyAvatar.orientation, avatarControllerPose.translation));

        // also transform the position into room space
        var worldToSensorMat = Mat4.inverse(MyAvatar.getSensorToWorldMatrix());
        var roomControllerPosition = Mat4.transformPoint(worldToSensorMat, worldControllerPosition);

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        var now = Date.now();

        // add the action and initialize some variables
        this.currentObjectPosition = grabbedProperties.position;
        this.currentObjectRotation = grabbedProperties.rotation;
        this.currentObjectTime = now;
        this.currentCameraOrientation = Camera.orientation;

        this.grabRadius = Vec3.distance(this.currentObjectPosition, worldControllerPosition);
        this.grabRadialVelocity = 0.0;

        // compute a constant based on the initial conditions which we use below to exagerate hand motion onto the held object
        this.radiusScalar = Math.log(this.grabRadius + 1.0);
        if (this.radiusScalar < 1.0) {
            this.radiusScalar = 1.0;
        }

        // compute the mass for the purpose of energy and how quickly to move object
        this.mass = this.getMass(grabbedProperties.dimensions, grabbedProperties.density);
        var distanceToObject = Vec3.length(Vec3.subtract(MyAvatar.position, grabbedProperties.position));
        var timeScale = this.distanceGrabTimescale(this.mass, distanceToObject);

        this.actionID = NULL_UUID;
        this.actionID = Entities.addAction("spring", this.grabbedEntity, {
            targetPosition: this.currentObjectPosition,
            linearTimeScale: timeScale,
            targetRotation: this.currentObjectRotation,
            angularTimeScale: timeScale,
            tag: getTag(),
            ttl: ACTION_TTL
        });
        if (this.actionID === NULL_UUID) {
            this.actionID = null;
        }
        this.actionTimeout = now + (ACTION_TTL * MSECS_PER_SEC);

        if (this.actionID !== null) {
            this.activateEntity(this.grabbedEntity, grabbedProperties, false);
            this.callEntityMethodOnGrabbed("startDistanceGrab");
        }

        Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

        this.turnOffVisualizations();

        this.previousRoomControllerPosition = roomControllerPosition;
    };

    this.distanceHolding = function(deltaTime, timestamp) {

        if (!this.triggerClicked) {
            this.callEntityMethodOnGrabbed("releaseGrab");
            this.setState(STATE_OFF, "trigger released");
            return;
        }

        this.heartBeat(this.grabbedEntity);

        // controller pose is in avatar frame
        var device = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var avatarControllerPose = Controller.getPoseValue(device);

        // transform it into world frame
        var worldControllerPosition = Vec3.sum(MyAvatar.position,
            Vec3.multiplyQbyV(MyAvatar.orientation, avatarControllerPose.translation));
        var worldControllerRotation = Quat.multiply(MyAvatar.orientation, avatarControllerPose.rotation);

        // also transform the position into room space
        var worldToSensorMat = Mat4.inverse(MyAvatar.getSensorToWorldMatrix());
        var roomControllerPosition = Mat4.transformPoint(worldToSensorMat, worldControllerPosition);

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);

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

        this.callEntityMethodOnGrabbed("continueDistantGrab");

        var defaultMoveWithHeadData = {
            disableMoveWithHead: false
        };

        var handControllerData = getEntityCustomData('handControllerKey', this.grabbedEntity, defaultMoveWithHeadData);

        //  Update radialVelocity
        var lastVelocity = Vec3.multiply(worldHandDelta, 1.0 / deltaObjectTime);
        var delta = Vec3.normalize(Vec3.subtract(grabbedProperties.position, worldControllerPosition));
        var newRadialVelocity = Vec3.dot(lastVelocity, delta);

        var VELOCITY_AVERAGING_TIME = 0.016;
        this.grabRadialVelocity = (deltaObjectTime / VELOCITY_AVERAGING_TIME) * newRadialVelocity +
            (1.0 - (deltaObjectTime / VELOCITY_AVERAGING_TIME)) * this.grabRadialVelocity;

        var RADIAL_GRAB_AMPLIFIER = 10.0;
        if (Math.abs(this.grabRadialVelocity) > 0.0) {
            this.grabRadius = this.grabRadius + (this.grabRadialVelocity * deltaObjectTime *
                this.grabRadius * RADIAL_GRAB_AMPLIFIER);
        }

        var newTargetPosition = Vec3.multiply(this.grabRadius, Quat.getUp(worldControllerRotation));
        newTargetPosition = Vec3.sum(newTargetPosition, worldControllerPosition);

        var objectToAvatar = Vec3.subtract(this.currentObjectPosition, MyAvatar.position);
        if (handControllerData.disableMoveWithHead !== true) {
            // mix in head motion
            if (MOVE_WITH_HEAD) {
                var objDistance = Vec3.length(objectToAvatar);
                var before = Vec3.multiplyQbyV(this.currentCameraOrientation, {
                    x: 0.0,
                    y: 0.0,
                    z: objDistance
                });
                var after = Vec3.multiplyQbyV(Camera.orientation, {
                    x: 0.0,
                    y: 0.0,
                    z: objDistance
                });
                var change = Vec3.multiply(Vec3.subtract(before, after), HAND_HEAD_MIX_RATIO);
                this.currentCameraOrientation = Camera.orientation;
                this.currentObjectPosition = Vec3.sum(this.currentObjectPosition, change);
            }
        }

        var handPosition = this.getHandPosition();

        // visualizations

        var rayPickInfo = this.calcRayPickInfo(this.hand);

        this.overlayLineOn(rayPickInfo.searchRay.origin, grabbedProperties.position, COLORS_GRAB_DISTANCE_HOLD);

        var distanceToObject = Vec3.length(Vec3.subtract(MyAvatar.position, this.currentObjectPosition));
        var success = Entities.updateAction(this.grabbedEntity, this.actionID, {
            targetPosition: newTargetPosition,
            linearTimeScale: this.distanceGrabTimescale(this.mass, distanceToObject),
            targetRotation: this.currentObjectRotation,
            angularTimeScale: this.distanceGrabTimescale(this.mass, distanceToObject),
            ttl: ACTION_TTL
        });
        if (success) {
            this.actionTimeout = now + (ACTION_TTL * MSECS_PER_SEC);
        } else {
            print("continueDistanceHolding -- updateAction failed");
        }

        this.previousRoomControllerPosition = roomControllerPosition;
    };

    this.setupHoldAction = function() {
        this.actionID = Entities.addAction("hold", this.grabbedEntity, {
            hand: this.hand === RIGHT_HAND ? "right" : "left",
            timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
            relativePosition: this.offsetPosition,
            relativeRotation: this.offsetRotation,
            ttl: ACTION_TTL,
            kinematic: NEAR_GRABBING_KINEMATIC,
            kinematicSetVelocity: true,
            ignoreIK: this.ignoreIK
        });
        if (this.actionID === NULL_UUID) {
            this.actionID = null;
            return false;
        }
        var now = Date.now();
        this.actionTimeout = now + (ACTION_TTL * MSECS_PER_SEC);
        return true;
    };

    this.projectVectorAlongAxis = function(position, axisStart, axisEnd) {
        var aPrime = Vec3.subtract(position, axisStart);
        var bPrime = Vec3.subtract(axisEnd, axisStart);
        var bPrimeMagnitude = Vec3.length(bPrime);
        var dotProduct = Vec3.dot(aPrime, bPrime);
        var scalar = dotProduct / bPrimeMagnitude;
        if (scalar < 0) {
            scalar = 0;
        }
        if (scalar > 1) {
            scalar = 1;
        }
        var projection = Vec3.sum(axisStart, Vec3.multiply(scalar, Vec3.normalize(bPrime)));
        return projection;
    };

    this.dropGestureReset = function() {
        this.prevHandIsUpsideDown = false;
    };

    this.dropGestureProcess = function(deltaTime) {
        var standardControllerValue = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var pose = Controller.getPoseValue(standardControllerValue);
        var worldHandRotation = Quat.multiply(MyAvatar.orientation, pose.rotation);

        var localHandUpAxis = this.hand === RIGHT_HAND ? {
            x: 1,
            y: 0,
            z: 0
        } : {
            x: -1,
            y: 0,
            z: 0
        };
        var worldHandUpAxis = Vec3.multiplyQbyV(worldHandRotation, localHandUpAxis);
        var DOWN = {
            x: 0,
            y: -1,
            z: 0
        };

        var DROP_ANGLE = Math.PI / 6;
        var HYSTERESIS_FACTOR = 1.1;
        var ROTATION_ENTER_THRESHOLD = Math.cos(DROP_ANGLE);
        var ROTATION_EXIT_THRESHOLD = Math.cos(DROP_ANGLE * HYSTERESIS_FACTOR);
        var rotationThreshold = this.prevHandIsUpsideDown ? ROTATION_EXIT_THRESHOLD : ROTATION_ENTER_THRESHOLD;

        var handIsUpsideDown = false;
        if (Vec3.dot(worldHandUpAxis, DOWN) > rotationThreshold) {
            handIsUpsideDown = true;
        }

        if (handIsUpsideDown != this.prevHandIsUpsideDown) {
            this.prevHandIsUpsideDown = handIsUpsideDown;
            Controller.triggerHapticPulse(HAPTIC_DEQUIP_STRENGTH, HAPTIC_DEQUIP_DURATION, this.hand);
        }

        return handIsUpsideDown;
    };

    this.nearGrabbingEnter = function() {
        if (this.hand === 0) {
            Messages.sendLocalMessage('Hifi-Teleport-Disabler', 'left');

        }
        if (this.hand === 1) {
            Messages.sendLocalMessage('Hifi-Teleport-Disabler', 'right');

        }
        this.lineOff();
        this.overlayLineOff();

        this.dropGestureReset();
        this.clearEquipHaptics();

        Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

        if (this.entityActivated) {
            var saveGrabbedID = this.grabbedEntity;
            this.release();
            this.grabbedEntity = saveGrabbedID;
        }

        var otherHandController = this.getOtherHandController();
        if (otherHandController.grabbedEntity == this.grabbedEntity &&
            (otherHandController.state == STATE_NEAR_GRABBING || otherHandController.state == STATE_DISTANCE_HOLDING)) {
            otherHandController.setState(STATE_OFF, "other hand grabbed this entity");
        }

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        this.activateEntity(this.grabbedEntity, grabbedProperties, false);

        // var handRotation = this.getHandRotation();
        var handRotation = (this.hand === RIGHT_HAND) ? MyAvatar.getRightPalmRotation() : MyAvatar.getLeftPalmRotation();
        var handPosition = this.getHandPosition();

        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);
        this.ignoreIK = grabbableData.ignoreIK ? grabbableData.ignoreIK : false;

        var hasPresetPosition = false;
        if (this.state == STATE_HOLD && this.grabbedHotspot) {
            // if an object is "equipped" and has a predefined offset, use it.
            var offsets = USE_ATTACH_POINT_SETTINGS && getAttachPointForHotspotFromSettings(this.grabbedHotspot, this.hand);
            if (offsets) {
                this.offsetPosition = offsets[0];
                this.offsetRotation = offsets[1];
                hasPresetPosition = true;
            } else {
                var handJointName = this.hand === RIGHT_HAND ? "RightHand" : "LeftHand";
                if (this.grabbedHotspot.joints[handJointName]) {
                    this.offsetPosition = this.grabbedHotspot.joints[handJointName][0];
                    this.offsetRotation = this.grabbedHotspot.joints[handJointName][1];
                    hasPresetPosition = true;
                }
            }
        } else {
            var objectRotation = grabbedProperties.rotation;
            this.offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

            var currentObjectPosition = grabbedProperties.position;
            var offset = Vec3.subtract(currentObjectPosition, handPosition);
            this.offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, this.offsetRotation)), offset);
            if (this.temporaryPositionOffset) {
                this.offsetPosition = this.temporaryPositionOffset;
                // hasPresetPosition = true;
            }
        }

        var isPhysical = propsArePhysical(grabbedProperties) || entityHasActions(this.grabbedEntity);
        if (isPhysical && this.state == STATE_NEAR_GRABBING) {
            // grab entity via action
            if (!this.setupHoldAction()) {
                return;
            }
            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'grab',
                grabbedEntity: this.grabbedEntity
            }));
        } else {
            // grab entity via parenting
            this.actionID = null;
            var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            var reparentProps = {
                parentID: MyAvatar.sessionUUID,
                parentJointIndex: handJointIndex,
                velocity: {x: 0, y: 0, z: 0},
                angularVelocity: {x: 0, y: 0, z: 0}
            };
            if (hasPresetPosition) {
                reparentProps["localPosition"] = this.offsetPosition;
                reparentProps["localRotation"] = this.offsetRotation;
            }
            Entities.editEntity(this.grabbedEntity, reparentProps);

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'equip',
                grabbedEntity: this.grabbedEntity
            }));
        }

        Entities.editEntity(this.grabbedEntity, {
            velocity: {
                x: 0,
                y: 0,
                z: 0
            },
            angularVelocity: {
                x: 0,
                y: 0,
                z: 0
            },
            dynamic: false
        });

        if (this.state == STATE_NEAR_GRABBING) {
            this.callEntityMethodOnGrabbed("startNearGrab");
        } else { // this.state == STATE_HOLD
            this.callEntityMethodOnGrabbed("startEquip");
        }

        this.currentHandControllerTipPosition =
            (this.hand === RIGHT_HAND) ? MyAvatar.rightHandTipPosition : MyAvatar.leftHandTipPosition;
        this.currentObjectTime = Date.now();

        this.currentObjectPosition = grabbedProperties.position;
        this.currentObjectRotation = grabbedProperties.rotation;
        this.currentVelocity = ZERO_VEC;
        this.currentAngularVelocity = ZERO_VEC;

        this.prevDropDetected = false;
    };

    this.nearGrabbing = function(deltaTime, timestamp) {

        if (this.state == STATE_NEAR_GRABBING && !this.triggerClicked) {
            this.callEntityMethodOnGrabbed("releaseGrab");
            this.setState(STATE_OFF, "trigger released");
            return;
        }

        if (this.state == STATE_HOLD) {

            var dropDetected = this.dropGestureProcess(deltaTime);

            if (this.triggerSmoothedReleased()) {
                this.waitForTriggerRelease = false;
            }

            if (dropDetected && this.prevDropDetected != dropDetected) {
                this.waitForTriggerRelease = true;
            }

            // highlight the grabbed hotspot when the dropGesture is detected.
            if (dropDetected) {
                entityPropertiesCache.addEntity(this.grabbedHotspot.entityID);
                equipHotspotBuddy.updateHotspot(this.grabbedHotspot, timestamp);
                equipHotspotBuddy.highlightHotspot(this.grabbedHotspot);
            }

            if (dropDetected && !this.waitForTriggerRelease && this.triggerSmoothedGrab()) {
                this.callEntityMethodOnGrabbed("releaseEquip");

                // store the offset attach points into preferences.
                if (USE_ATTACH_POINT_SETTINGS && this.grabbedHotspot && this.grabbedEntity) {
                    var props = Entities.getEntityProperties(this.grabbedEntity, ["localPosition", "localRotation"]);
                    if (props && props.localPosition && props.localRotation) {
                        storeAttachPointForHotspotInSettings(this.grabbedHotspot, this.hand, props.localPosition, props.localRotation);
                    }
                }

                var grabbedEntity = this.grabbedEntity;
                this.release();
                this.grabbedEntity = grabbedEntity;
                this.setState(STATE_NEAR_GRABBING, "drop gesture detected");
                return;
            }
            this.prevDropDetected = dropDetected;
        }

        this.heartBeat(this.grabbedEntity);

        var props = Entities.getEntityProperties(this.grabbedEntity, ["localPosition", "parentID",
                                                                      "position", "rotation", "dimensions",
                                                                      "registrationPoint"]);
        if (!props.position) {
            // server may have reset, taking our equipped entity with it.  move back to "off" stte
            this.callEntityMethodOnGrabbed("releaseGrab");
            this.setState(STATE_OFF, "entity has no position property");
            return;
        }

        var now = Date.now();
        if (now - this.lastUnequipCheckTime > MSECS_PER_SEC * CHECK_TOO_FAR_UNEQUIP_TIME) {
            this.lastUnequipCheckTime = now;

            if (props.parentID == MyAvatar.sessionUUID) {
                var handPosition = this.getHandPosition();

                var TEAR_AWAY_DISTANCE = 0.1;
                var dist = distanceBetweenPointAndEntityBoundingBox(handPosition, props);
                if (dist > TEAR_AWAY_DISTANCE) {
                    print("handControllerGrab -- autoreleasing held or equipped item because it is far from hand." +
                        props.parentID + ", dist = " + dist);

                    if (this.state == STATE_NEAR_GRABBING) {
                        this.callEntityMethodOnGrabbed("releaseGrab");
                    } else { // this.state == STATE_HOLD
                        this.callEntityMethodOnGrabbed("releaseEquip");
                    }
                    this.setState(STATE_OFF, "held object too far away");
                    return;
                }
            }
        }

        // Keep track of the fingertip velocity to impart when we release the object.
        // Note that the idea of using a constant 'tip' velocity regardless of the
        // object's actual held offset is an idea intended to make it easier to throw things:
        // Because we might catch something or transfer it between hands without a good idea
        // of it's actual offset, let's try imparting a velocity which is at a fixed radius
        // from the palm.

        var handControllerPosition = (this.hand === RIGHT_HAND) ? MyAvatar.rightHandPosition : MyAvatar.leftHandPosition;

        var deltaObjectTime = (now - this.currentObjectTime) / MSECS_PER_SEC; // convert to seconds

        if (deltaObjectTime > 0.0) {
            var worldDeltaPosition = Vec3.subtract(props.position, this.currentObjectPosition);

            var previousEulers = Quat.safeEulerAngles(this.currentObjectRotation);
            var newEulers = Quat.safeEulerAngles(props.rotation);
            var worldDeltaRotation = Vec3.subtract(newEulers, previousEulers);

            this.currentVelocity = Vec3.multiply(worldDeltaPosition, 1.0 / deltaObjectTime);
            this.currentAngularVelocity = Vec3.multiply(worldDeltaRotation, Math.PI / (deltaObjectTime * 180.0));

            this.currentObjectPosition = props.position;
            this.currentObjectRotation = props.rotation;
        }

        this.currentHandControllerTipPosition = handControllerPosition;
        this.currentObjectTime = now;

        if (this.state === STATE_HOLD) {
            this.callEntityMethodOnGrabbed("continueEquip");
        }
        if (this.state == STATE_NEAR_GRABBING) {
            this.callEntityMethodOnGrabbed("continueNearGrab");
        }

        if (this.actionID && this.actionTimeout - now < ACTION_TTL_REFRESH * MSECS_PER_SEC) {
            // if less than a 5 seconds left, refresh the actions ttl
            var success = Entities.updateAction(this.grabbedEntity, this.actionID, {
                hand: this.hand === RIGHT_HAND ? "right" : "left",
                timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
                relativePosition: this.offsetPosition,
                relativeRotation: this.offsetRotation,
                ttl: ACTION_TTL,
                kinematic: NEAR_GRABBING_KINEMATIC,
                kinematicSetVelocity: true,
                ignoreIK: this.ignoreIK
            });
            if (success) {
                this.actionTimeout = now + (ACTION_TTL * MSECS_PER_SEC);
            } else {
                print("continueNearGrabbing -- updateAction failed");
                Entities.deleteAction(this.grabbedEntity, this.actionID);
                this.setupHoldAction();
            }
        }
    };

    this.nearTriggerEnter = function() {

        this.clearEquipHaptics();

        Controller.triggerShortHapticPulse(1.0, this.hand);
        this.callEntityMethodOnGrabbed("startNearTrigger");
    };

    this.farTriggerEnter = function() {
        this.clearEquipHaptics();

        this.callEntityMethodOnGrabbed("startFarTrigger");
    };

    this.nearTrigger = function(deltaTime, timestamp) {
        if (this.triggerSmoothedReleased()) {
            this.callEntityMethodOnGrabbed("stopNearTrigger");
            this.setState(STATE_OFF, "trigger released");
            return;
        }
        this.callEntityMethodOnGrabbed("continueNearTrigger");
    };

    this.farTrigger = function(deltaTime, timestamp) {
        if (this.triggerSmoothedReleased()) {
            this.callEntityMethodOnGrabbed("stopFarTrigger");
            this.setState(STATE_OFF, "trigger released");
            return;
        }

        var handPosition = this.getHandPosition();
        var pickRay = {
            origin: handPosition,
            direction: Quat.getUp(this.getHandRotation())
        };

        var now = Date.now();
        if (now - this.lastPickTime > MSECS_PER_SEC / PICKS_PER_SECOND_PER_HAND) {
            var intersection = findRayIntersection(pickRay, true);
            if (intersection.accurate || intersection.overlayID) {
                this.lastPickTime = now;
                if (intersection.entityID != this.grabbedEntity) {
                    this.callEntityMethodOnGrabbed("stopFarTrigger");
                    this.setState(STATE_OFF, "laser moved off of entity");
                    return;
                }
                if (intersection.intersects) {
                    this.intersectionDistance = Vec3.distance(pickRay.origin, intersection.intersection);
                }
                this.searchIndicatorOn(pickRay);
            }
        }

        this.callEntityMethodOnGrabbed("continueFarTrigger");
    };

    this.offEnter = function() {
        this.release();
    };

    this.entityTouchingEnter = function() {
        // test for intersection between controller laser and web entity plane.
        var intersectInfo = handLaserIntersectEntity(this.grabbedEntity, this.hand);
        if (intersectInfo) {
            var pointerEvent = {
                type: "Press",
                id: this.hand + 1, // 0 is reserved for hardware mouse
                pos2D: projectOntoEntityXYPlane(this.grabbedEntity, intersectInfo.point),
                pos3D: intersectInfo.point,
                normal: intersectInfo.normal,
                direction: intersectInfo.searchRay.direction,
                button: "Primary",
                isPrimaryHeld: true
            };

            Entities.sendMousePressOnEntity(this.grabbedEntity, pointerEvent);
            Entities.sendClickDownOnEntity(this.grabbedEntity, pointerEvent);

            this.touchingEnterTimer = 0;
            this.touchingEnterPointerEvent = pointerEvent;
            this.touchingEnterPointerEvent.button = "None";
            this.deadspotExpired = false;
        }
    };

    this.entityTouchingExit = function() {
        // test for intersection between controller laser and web entity plane.
        var intersectInfo = handLaserIntersectEntity(this.grabbedEntity, this.hand);
        if (intersectInfo) {
            var pointerEvent;
            if (this.deadspotExpired) {
                pointerEvent = {
                    type: "Release",
                    id: this.hand + 1, // 0 is reserved for hardware mouse
                    pos2D: projectOntoEntityXYPlane(this.grabbedEntity, intersectInfo.point),
                    pos3D: intersectInfo.point,
                    normal: intersectInfo.normal,
                    direction: intersectInfo.searchRay.direction,
                    button: "Primary"
                };
            } else {
                pointerEvent = this.touchingEnterPointerEvent;
                pointerEvent.button = "Primary";
                pointerEvent.isPrimaryHeld = false;
            }

            Entities.sendMouseReleaseOnEntity(this.grabbedEntity, pointerEvent);
            Entities.sendClickReleaseOnEntity(this.grabbedEntity, pointerEvent);
            Entities.sendHoverLeaveEntity(this.grabbedEntity, pointerEvent);
        }
        this.focusedEntity = null;
    };

    this.entityTouching = function(dt) {

        this.touchingEnterTimer += dt;

        entityPropertiesCache.addEntity(this.grabbedEntity);

        if (!this.triggerSmoothedGrab()) {
            this.setState(STATE_OFF, "released trigger");
            return;
        }

        // test for intersection between controller laser and web entity plane.
        var intersectInfo = handLaserIntersectEntity(this.grabbedEntity, this.hand);
        if (intersectInfo) {

            if (Entities.keyboardFocusEntity != this.grabbedEntity) {
                Entities.keyboardFocusEntity = this.grabbedEntity;
            }

            var pointerEvent = {
                type: "Move",
                id: this.hand + 1, // 0 is reserved for hardware mouse
                pos2D: projectOntoEntityXYPlane(this.grabbedEntity, intersectInfo.point),
                pos3D: intersectInfo.point,
                normal: intersectInfo.normal,
                direction: intersectInfo.searchRay.direction,
                button: "NoButtons",
                isPrimaryHeld: true
            };

            var POINTER_PRESS_TO_MOVE_DELAY = 0.15; // seconds
            var POINTER_PRESS_TO_MOVE_DEADSPOT_ANGLE = 0.05; // radians ~ 3 degrees
            if (this.deadspotExpired || this.touchingEnterTimer > POINTER_PRESS_TO_MOVE_DELAY ||
                angleBetween(pointerEvent.direction, this.touchingEnterPointerEvent.direction) > POINTER_PRESS_TO_MOVE_DEADSPOT_ANGLE) {
                Entities.sendMouseMoveOnEntity(this.grabbedEntity, pointerEvent);
                Entities.sendHoldingClickOnEntity(this.grabbedEntity, pointerEvent);
                this.deadspotExpired = true;
            }

            this.intersectionDistance = intersectInfo.distance;
            this.searchIndicatorOn(intersectInfo.searchRay);
            Reticle.setVisible(false);
        } else {
            this.setState(STATE_OFF, "grabbed entity was destroyed");
            return;
        }
    };

    this.release = function() {
        Messages.sendLocalMessage('Hifi-Teleport-Disabler','none');
        this.turnOffVisualizations();

        var noVelocity = false;
        if (this.grabbedEntity !== null) {

            //  Make a small release haptic pulse if we really were holding something
            Controller.triggerHapticPulse(HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION, this.hand);

            // If this looks like the release after adjusting something still held in the other hand, print the position
            // and rotation of the held thing to help content creators set the userData.
            var grabData = getEntityCustomData(GRAB_USER_DATA_KEY, this.grabbedEntity, {});
            if (grabData.refCount > 1) {
                var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, ["localPosition", "localRotation"]);
                if (grabbedProperties && grabbedProperties.localPosition && grabbedProperties.localRotation) {
                    print((this.hand === RIGHT_HAND ? '"LeftHand"' : '"RightHand"') + ":" +
                        '[{"x":' + grabbedProperties.localPosition.x + ', "y":' + grabbedProperties.localPosition.y +
                        ', "z":' + grabbedProperties.localPosition.z + '}, {"x":' + grabbedProperties.localRotation.x +
                        ', "y":' + grabbedProperties.localRotation.y + ', "z":' + grabbedProperties.localRotation.z +
                        ', "w":' + grabbedProperties.localRotation.w + '}]');
                }
            }

            if (this.actionID !== null) {
                Entities.deleteAction(this.grabbedEntity, this.actionID);
                // sometimes we want things to stay right where they are when we let go.
                var releaseVelocityData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);
                if (releaseVelocityData.disableReleaseVelocity === true ||
                    // this next line allowed both:
                    // (1) far-grab, pull to self, near grab, then throw
                    // (2) equip something physical and adjust it with a other-hand grab without the thing drifting
                    grabData.refCount > 1) {
                    noVelocity = true;
                }
            }
        }

        this.deactivateEntity(this.grabbedEntity, noVelocity);
        this.actionID = null;

        Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
            action: 'release',
            grabbedEntity: this.grabbedEntity,
            joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
        }));

        this.grabbedEntity = null;
        this.grabbedHotspot = null;

        if (this.triggerSmoothedGrab()) {
            this.waitForTriggerRelease = true;
        }
    };

    this.cleanup = function() {
        this.release();
    };

    this.heartBeat = function(entityID) {
        var now = Date.now();
        if (now - this.lastHeartBeat > HEART_BEAT_INTERVAL) {
            var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
            data["heartBeat"] = now;
            setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
            this.lastHeartBeat = now;
        }
    };

    this.resetAbandonedGrab = function(entityID) {
        print("cleaning up abandoned grab on " + entityID);
        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        data["refCount"] = 1;
        setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
        this.deactivateEntity(entityID, false);
    };

    this.activateEntity = function(entityID, grabbedProperties, wasLoaded) {
        if (this.entityActivated) {
            return;
        }
        this.entityActivated = true;

        if (delayedDeactivateTimeout && delayedDeactivateEntityID == entityID) {
            // we have a timeout waiting to set collisions with myAvatar back on (so that when something
            // is thrown it doesn't collide with the avatar's capsule the moment it's released).  We've
            // regrabbed the entity before the timeout fired, so cancel the timeout, run the function now
            // and adjust the grabbedProperties.  This will make the saved set of properties (the ones that
            // get re-instated after all the grabs have been released) be correct.
            Script.clearTimeout(delayedDeactivateTimeout);
            delayedDeactivateTimeout = null;
            grabbedProperties["collidesWith"] = delayedDeactivateFunc();
        }

        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        var now = Date.now();

        if (wasLoaded) {
            data["refCount"] = 1;
        } else {
            data["refCount"] = data["refCount"] ? data["refCount"] + 1 : 1;

            // zero gravity and set ignoreForCollisions in a way that lets us put them back, after all grabs are done
            if (data["refCount"] == 1) {
                data["heartBeat"] = now;
                this.lastHeartBeat = now;

                this.isInitialGrab = true;
                data["gravity"] = grabbedProperties.gravity;
                data["collidesWith"] = grabbedProperties.collidesWith;
                data["collisionless"] = grabbedProperties.collisionless;
                data["dynamic"] = grabbedProperties.dynamic;
                data["parentID"] = wasLoaded ? NULL_UUID : grabbedProperties.parentID;
                data["parentJointIndex"] = grabbedProperties.parentJointIndex;

                var whileHeldProperties = {
                    gravity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    // bummer, it isn't easy to do bitwise collisionMask operations like this:
                    // "collisionMask": COLLISION_MASK_WHILE_GRABBED | grabbedProperties.collisionMask
                    // when using string values
                    "collidesWith": COLLIDES_WITH_WHILE_GRABBED
                };
                Entities.editEntity(entityID, whileHeldProperties);
            } else if (data["refCount"] > 1) {
                if (data["heartBeat"] === undefined ||
                    now - data["heartBeat"] > HEART_BEAT_TIMEOUT) {
                    // this entity has userData suggesting it is grabbed, but nobody is updating the hearbeat.
                    // deactivate it before grabbing.
                    this.resetAbandonedGrab(entityID);
                    grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
                    return this.activateEntity(entityID, grabbedProperties, wasLoaded);
                }

                this.isInitialGrab = false;
                // if an object is being grabbed by more than one person (or the same person twice, but nevermind), switch
                // the collision groups so that it wont collide with "other" avatars.  This avoids a situation where two
                // people are holding something and one of them will be able (if the other releases at the right time) to
                // bootstrap themselves with the held object.  This happens because the meaning of "otherAvatar" in
                // the collision mask hinges on who the physics simulation owner is.
                Entities.editEntity(entityID, {
                    "collidesWith": COLLIDES_WITH_WHILE_MULTI_GRABBED
                });
            }
        }
        setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
        return data;
    };

    this.checkForStrayChildren = function() {
        // sometimes things can get parented to a hand and this script is unaware.  Search for such entities and
        // unhook them.
        var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
        var children = Entities.getChildrenIDsOfJoint(MyAvatar.sessionUUID, handJointIndex);
        children.forEach(function(childID) {
            print("disconnecting stray child of hand: (" + _this.hand + ") " + childID);
            Entities.editEntity(childID, {
                parentID: NULL_UUID
            });
        });
    };

    this.delayedDeactivateEntity = function(entityID, collidesWith) {
        // If, before the grab started, the held entity collided with myAvatar, we do the deactivation in
        // two parts.  Most of it is done in deactivateEntity(), but the final collidesWith and refcount
        // are delayed a bit.  This keeps thrown things from colliding with the avatar's capsule so often.
        // The refcount is handled in this delayed fashion so things don't get confused if someone else
        // grabs the entity before the timeout fires.
        Entities.editEntity(entityID, {
            collidesWith: collidesWith
        });
        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        if (data && data["refCount"]) {
            data["refCount"] = data["refCount"] - 1;
            if (data["refCount"] < 1) {
                data = null;
            }
        } else {
            data = null;
        }

        setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
    };

    this.deactivateEntity = function(entityID, noVelocity, delayed) {
        var deactiveProps;

        if (!this.entityActivated) {
            return;
        }
        this.entityActivated = false;

        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        var doDelayedDeactivate = false;
        if (data && data["refCount"]) {
            data["refCount"] = data["refCount"] - 1;
            if (data["refCount"] < 1) {
                deactiveProps = {
                    gravity: data["gravity"],
                    // don't set collidesWith myAvatar back right away, because thrown things tend to bounce off the
                    // avatar's capsule.
                    collidesWith: removeMyAvatarFromCollidesWith(data["collidesWith"]),
                    collisionless: data["collisionless"],
                    dynamic: data["dynamic"],
                    parentID: data["parentID"],
                    parentJointIndex: data["parentJointIndex"]
                };

                doDelayedDeactivate = (data["collidesWith"].indexOf("myAvatar") >= 0);

                if (doDelayedDeactivate) {
                    var delayedCollidesWith = data["collidesWith"];
                    var delayedEntityID = entityID;
                    delayedDeactivateFunc = function() {
                        // set collidesWith back to original value a bit later than the rest
                        delayedDeactivateTimeout = null;
                        _this.delayedDeactivateEntity(delayedEntityID, delayedCollidesWith);
                        return delayedCollidesWith;
                    };
                    delayedDeactivateTimeout =
                        Script.setTimeout(delayedDeactivateFunc, COLLIDE_WITH_AV_AFTER_RELEASE_DELAY * MSECS_PER_SEC);
                    delayedDeactivateEntityID = entityID;
                }

                // things that are held by parenting and dropped with no velocity will end up as "static" in bullet.  If
                // it looks like the dropped thing should fall, give it a little velocity.
                var props = Entities.getEntityProperties(entityID, ["parentID", "velocity", "dynamic", "shapeType"]);
                var parentID = props.parentID;

                if (!noVelocity &&
                    parentID == MyAvatar.sessionUUID &&
                    Vec3.length(data["gravity"]) > 0.0 &&
                    data["dynamic"] &&
                    data["parentID"] == NULL_UUID &&
                    !data["collisionless"]) {
                    deactiveProps["velocity"] = this.currentVelocity;
                }
                if (noVelocity) {
                    deactiveProps["velocity"] = {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    };
                    deactiveProps["angularVelocity"] = {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    };
                }

                Entities.editEntity(entityID, deactiveProps);
                data = null;
            } else if (this.shouldResetParentOnRelease) {
                // we parent-grabbed this from another parent grab.  try to put it back where we found it.
                deactiveProps = {
                    parentID: this.previousParentID,
                    parentJointIndex: this.previousParentJointIndex,
                    velocity: {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    },
                    angularVelocity: {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    }
                };
                Entities.editEntity(entityID, deactiveProps);
            } else if (noVelocity) {
                Entities.editEntity(entityID, {
                    velocity: {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    },
                    angularVelocity: {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    },
                    dynamic: data["dynamic"]
                });
            }
        } else {
            data = null;
        }
        if (!doDelayedDeactivate) {
            setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
        }
    };

    this.getOtherHandController = function() {
        return (this.hand === RIGHT_HAND) ? leftController : rightController;
    };
}

var rightController = new MyController(RIGHT_HAND);
var leftController = new MyController(LEFT_HAND);

var MAPPING_NAME = "com.highfidelity.handControllerGrab";

var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from([Controller.Standard.RT]).peek().to(rightController.triggerPress);
mapping.from([Controller.Standard.RTClick]).peek().to(rightController.triggerClick);

mapping.from([Controller.Standard.LT]).peek().to(leftController.triggerPress);
mapping.from([Controller.Standard.LTClick]).peek().to(leftController.triggerClick);

mapping.from([Controller.Standard.RB]).peek().to(rightController.secondaryPress);
mapping.from([Controller.Standard.LB]).peek().to(leftController.secondaryPress);
mapping.from([Controller.Standard.LeftGrip]).peek().to(leftController.secondaryPress);
mapping.from([Controller.Standard.RightGrip]).peek().to(rightController.secondaryPress);

mapping.from([Controller.Standard.LeftPrimaryThumb]).peek().to(leftController.thumbPress);
mapping.from([Controller.Standard.RightPrimaryThumb]).peek().to(rightController.thumbPress);

Controller.enableMapping(MAPPING_NAME);

// the section below allows the grab script to listen for messages
// that disable either one or both hands.  useful for two handed items
var handToDisable = 'none';

function update(deltaTime) {
    var timestamp = Date.now();

    if (handToDisable !== LEFT_HAND && handToDisable !== 'both') {
        leftController.update(deltaTime, timestamp);
    }
    if (handToDisable !== RIGHT_HAND && handToDisable !== 'both') {
        rightController.update(deltaTime, timestamp);
    }
    equipHotspotBuddy.update(deltaTime, timestamp);
    entityPropertiesCache.update();
}

Messages.subscribe('Hifi-Hand-Disabler');
Messages.subscribe('Hifi-Hand-Grab');
Messages.subscribe('Hifi-Hand-RayPick-Blacklist');
Messages.subscribe('Hifi-Object-Manipulation');

var handleHandMessages = function(channel, message, sender) {
    var data;
    if (sender === MyAvatar.sessionUUID) {
        if (channel === 'Hifi-Hand-Disabler') {
            if (message === 'left') {
                handToDisable = LEFT_HAND;
                leftController.turnOffVisualizations();
            }
            if (message === 'right') {
                handToDisable = RIGHT_HAND;
                rightController.turnOffVisualizations();
            }
            if (message === 'both' || message === 'none') {
                if (message === 'both') {
                    rightController.turnOffVisualizations();
                    leftController.turnOffVisualizations();

                }
                handToDisable = message;
            }
        } else if (channel === 'Hifi-Hand-Grab') {
            try {
                data = JSON.parse(message);
                var selectedController = (data.hand === 'left') ? leftController : rightController;
                var hotspotIndex = data.hotspotIndex !== undefined ? parseInt(data.hotspotIndex) : 0;
                selectedController.release();
                var wearableEntity = data.entityID;
                entityPropertiesCache.addEntity(wearableEntity);
                selectedController.grabbedEntity = wearableEntity;
                var hotspots = selectedController.collectEquipHotspots(selectedController.grabbedEntity);
                if (hotspots.length > 0) {
                    if (hotspotIndex >= hotspots.length) {
                        hotspotIndex = 0;
                    }
                    selectedController.grabbedHotspot = hotspots[hotspotIndex];
                }
                selectedController.setState(STATE_HOLD, "Hifi-Hand-Grab msg received");
                selectedController.nearGrabbingEnter();

            } catch (e) {
                print("WARNING: error parsing Hifi-Hand-Grab message");
            }

        } else if (channel === 'Hifi-Hand-RayPick-Blacklist') {
            try {
                data = JSON.parse(message);
                var action = data.action;
                var id = data.id;
                var index = blacklist.indexOf(id);

                if (action === 'add' && index === -1) {
                    blacklist.push(id);
                }
                if (action === 'remove') {
                    if (index > -1) {
                        blacklist.splice(index, 1);
                    }
                }

            } catch (e) {
                print("WARNING: error parsing Hifi-Hand-RayPick-Blacklist message");
            }
        }
    }
};

Messages.messageReceived.connect(handleHandMessages);

function cleanup() {
    rightController.cleanup();
    leftController.cleanup();
    Controller.disableMapping(MAPPING_NAME);
    Reticle.setVisible(true);
}

Script.scriptEnding.connect(cleanup);
Script.update.connect(update);

}()); // END LOCAL_SCOPE
