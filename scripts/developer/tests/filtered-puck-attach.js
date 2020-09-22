//
//  Created by Anthony J. Thibault on 2017/06/20
//  Modified by Robbie Uvanni to support multiple pucks and easier placement of pucks on entities, on 2017/08/01
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  When this script is running, a new app button, named "PUCKTACH", will be added to the toolbar/tablet.
//  Click this app to bring up the puck attachment panel.
//

/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
/* global Xform */
Script.include("/~/system/libraries/Xform.js");

(function() { // BEGIN LOCAL_SCOPE

var TABLET_BUTTON_NAME = "PUCKATTACH";
var TABLET_APP_URL = Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/html/filtered-puck-attach.html?2");
var NUM_TRACKED_OBJECTS = 16;

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var tabletButton = tablet.addButton({
    text: TABLET_BUTTON_NAME,
    icon: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/puck-i.svg"),
    activeIcon: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/puck-a.svg")
});

var shown = false;
function onScreenChanged(type, url) {
    if (type === "Web" && url === TABLET_APP_URL) {
        tabletButton.editProperties({isActive: true});
        if (!shown) {
            // hook up to event bridge
            tablet.webEventReceived.connect(onWebEventReceived);
            shownChanged(true);
        }
        shown = true;
    } else {
        tabletButton.editProperties({isActive: false});
        if (shown) {
            // disconnect from event bridge
            tablet.webEventReceived.disconnect(onWebEventReceived);
            shownChanged(false);
        }
        shown = false;
    }
}
tablet.screenChanged.connect(onScreenChanged);

function pad(num, size) {
    var tempString = "000000000" + num;
    return tempString.substr(tempString.length - size);
}
function indexToTrackedObjectName(index) {
    return "TrackedObject" + pad(index, 2);
}
function getAvailableTrackedObjects() {
    var available = [];
    var i;
    for (i = 0; i < NUM_TRACKED_OBJECTS; i++) {
        var key = indexToTrackedObjectName(i);
        var pose = Controller.getPoseValue(Controller.Standard[key]);
        if (pose && pose.valid) {
            available.push(i);
        }
    }
    return available;
}
function sendAvailableTrackedObjects() {
    tablet.emitScriptEvent(JSON.stringify({
        pucks: getAvailableTrackedObjects(),
        selectedPuck: ((lastPuck === undefined) ? -1 : lastPuck.name)
    }));
}

function getRelativePosition(origin, rotation, offset) {
    var relativeOffset = Vec3.multiplyQbyV(rotation, offset);
    var worldPosition = Vec3.sum(origin, relativeOffset);
    return worldPosition;
}
function getPropertyForEntity(entityID, propertyName) {
    return Entities.getEntityProperties(entityID, [propertyName])[propertyName];
}
function entityExists(entityID) {
    return Object.keys(Entities.getEntityProperties(entityID)).length > 0;
}

var VIVE_PUCK_MODEL = Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/vive_tracker_puck_y180z180.obj");
var VIVE_PUCK_DIMENSIONS = { x: 0.0945, y: 0.0921, z: 0.0423 }; // 1/1000th scale of model
var VIVE_PUCK_SEARCH_DISTANCE = 1.5; // metres
var VIVE_PUCK_SPAWN_DISTANCE = 0.5; // metres
var VIVE_PUCK_TRACKED_OBJECT_MAX_DISTANCE = 10.0; // metres
var VIVE_PUCK_NAME = "Tracked Puck";

var trackedPucks = { };
var lastPuck;

var DEFAULT_ROTATION_SMOOTHING_CONSTANT = 1.0; // no smoothing
var DEFAULT_TRANSLATION_SMOOTHING_CONSTANT = 1.0; // no smoothing
var DEFAULT_TRANSLATION_ACCELERATION_LIMIT = 1000; // only extreme accelerations are smoothed
var DEFAULT_ROTATION_ACCELERATION_LIMIT = 10000; // only extreme accelerations are smoothed
var DEFAULT_TRANSLATION_SNAP_THRESHOLD = 0; // no snapping
var DEFAULT_ROTATION_SNAP_THRESHOLD = 0; // no snapping

function buildMappingJson() {
    var obj = {name: "com.highfidelity.testing.filteredPuckAttach", channels: []};
    var i;
    for (i = 0; i < NUM_TRACKED_OBJECTS; i++) {
        obj.channels.push({
            from: "Vive." + indexToTrackedObjectName(i),
            to: "Standard." + indexToTrackedObjectName(i),
            filters: [
                {
                    type: "accelerationLimiter",
                    translationAccelerationLimit: DEFAULT_TRANSLATION_ACCELERATION_LIMIT,
                    rotationAccelerationLimit: DEFAULT_ROTATION_ACCELERATION_LIMIT,
                    translationSnapThreshold: DEFAULT_TRANSLATION_SNAP_THRESHOLD,
                    rotationSnapThreshold: DEFAULT_ROTATION_SNAP_THRESHOLD,
                },
                {
                    type: "exponentialSmoothing",
                    translation: DEFAULT_TRANSLATION_SMOOTHING_CONSTANT,
                    rotation: DEFAULT_ROTATION_SMOOTHING_CONSTANT
                }
            ]
        });
    }
    return obj;
}

var mappingJson = buildMappingJson();

var mapping;
function mappingChanged() {
    if (mapping) {
        mapping.disable();
    }
    mapping = Controller.parseMapping(JSON.stringify(mappingJson));
    mapping.enable();
}

function shownChanged(newShown) {
    if (newShown) {
        mappingChanged();
    } else {
        mapping.disable();
    }
}

function setTranslationAccelerationLimit(value) {
    var i;
    for (i = 0; i < NUM_TRACKED_OBJECTS; i++) {
        mappingJson.channels[i].filters[0].translationAccelerationLimit = value;
    }
    mappingChanged();
}

function setTranslationSnapThreshold(value) {
    // convert from mm
    var MM_PER_M = 1000;
    var meters = value / MM_PER_M;
    var i;
    for (i = 0; i < NUM_TRACKED_OBJECTS; i++) {
        mappingJson.channels[i].filters[0].translationSnapThreshold = meters;
    }
    mappingChanged();
}

function setRotationAccelerationLimit(value) {
    var i;
    for (i = 0; i < NUM_TRACKED_OBJECTS; i++) {
        mappingJson.channels[i].filters[0].rotationAccelerationLimit = value;
    }
    mappingChanged();
}

function setRotationSnapThreshold(value) {
    // convert from degrees
    var PI_IN_DEGREES = 180;
    var radians = value * (Math.pi / PI_IN_DEGREES);
    var i;
    for (i = 0; i < NUM_TRACKED_OBJECTS; i++) {
        mappingJson.channels[i].filters[0].translationSnapThreshold = radians;
    }
    mappingChanged();
}

function setTranslationSmoothingConstant(value) {
    var i;
    for (i = 0; i < NUM_TRACKED_OBJECTS; i++) {
        mappingJson.channels[i].filters[1].translation = value;
    }
    mappingChanged();
}

function setRotationSmoothingConstant(value) {
    var i;
    for (i = 0; i < NUM_TRACKED_OBJECTS; i++) {
        mappingJson.channels[i].filters[1].rotation = value;
    }
    mappingChanged();
}


function createPuck(puck) {
    // create a puck entity and add it to our list of pucks
    var action = indexToTrackedObjectName(puck.puckno);
    var pose = Controller.getPoseValue(Controller.Standard[action]);

    if (pose && pose.valid) {
        var spawnOffset = Vec3.multiply(Vec3.FRONT, VIVE_PUCK_SPAWN_DISTANCE);
        var spawnPosition = getRelativePosition(MyAvatar.position, MyAvatar.orientation, spawnOffset);

        // should be an overlay
        var puckEntityProperties = {
            name: "Tracked Puck",
            type: "Model",
            modelURL: VIVE_PUCK_MODEL,
            dimensions: VIVE_PUCK_DIMENSIONS,
            position: spawnPosition,
            userData: '{ "grabbableKey": { "grabbable": true, "kinematic": false } }'
        };

        var puckEntityID = Entities.addEntity(puckEntityProperties);

        // if we've already created this puck, destroy it
        if (trackedPucks.hasOwnProperty(puck.puckno)) {
            destroyPuck(puck.puckno);
        }
        // if we had an unfinalized puck, destroy it
        if (lastPuck !== undefined) {
            destroyPuck(lastPuck.name);
        }
        // create our new unfinalized puck
        trackedPucks[puck.puckno] = {
            puckEntityID: puckEntityID,
            trackedEntityID: ""
        };
        lastPuck = trackedPucks[puck.puckno];
        lastPuck.name = Number(puck.puckno);
    }
}
function finalizePuck(puckName) {
    // find nearest entity and change its parent to the puck

    if (!trackedPucks.hasOwnProperty(puckName)) {
        print('2');
        return;
    }
    if (lastPuck === undefined) {
        print('3');
        return;
    }
    if (lastPuck.name !== Number(puckName)) {
        print('1');
        return;
    }

    var puckPosition = getPropertyForEntity(lastPuck.puckEntityID, "position");
    var foundEntities = Entities.findEntities(puckPosition, VIVE_PUCK_SEARCH_DISTANCE);

    var foundEntity;
    var leastDistance = Number.MAX_VALUE;

    for (var i = 0; i < foundEntities.length; i++) {
        var entity = foundEntities[i];

        if (getPropertyForEntity(entity, "name") !== VIVE_PUCK_NAME) {
            var entityPosition = getPropertyForEntity(entity, "position");
            var d = Vec3.distance(entityPosition, puckPosition);

            if (d < leastDistance) {
                leastDistance = d;
                foundEntity = entity;
            }
        }
    }

    if (foundEntity) {
        lastPuck.trackedEntityID = foundEntity;
        // remember the userdata and collisionless flag for the tracked entity since
        // we're about to remove it and make it ungrabbable and collisionless
        lastPuck.trackedEntityUserData = getPropertyForEntity(foundEntity, "userData");
        lastPuck.trackedEntityCollisionFlag = getPropertyForEntity(foundEntity, "collisionless");
        // update properties of the tracked entity
        Entities.editEntity(lastPuck.trackedEntityID, {
            "parentID": lastPuck.puckEntityID,
            "userData": '{ "grabbableKey": { "grabbable": false } }',
            "collisionless": 1
        });
        // remove reference to puck since it is now calibrated and finalized
        lastPuck = undefined;
    }
}
function updatePucks() {
    // for each puck, update its position and orientation
    for (var puckName in trackedPucks) {
        if (!trackedPucks.hasOwnProperty(puckName)) {
            continue;
        }
        var action = indexToTrackedObjectName(puckName);
        var pose = Controller.getPoseValue(Controller.Standard[action]);
        if (pose && pose.valid) {
            var puck = trackedPucks[puckName];
            if (puck.trackedEntityID) {
                if (entityExists(puck.trackedEntityID)) {
                    var avatarXform = new Xform(MyAvatar.orientation, MyAvatar.position);
                    var puckXform = new Xform(pose.rotation, pose.translation);
                    var finalXform = Xform.mul(avatarXform, puckXform);

                    var d = Vec3.distance(MyAvatar.position, finalXform.pos);
                    if (d > VIVE_PUCK_TRACKED_OBJECT_MAX_DISTANCE) {
                        print('tried to move tracked object too far away: ' + d);
                        return;
                    }

                    Entities.editEntity(puck.puckEntityID, {
                        position: finalXform.pos,
                        rotation: finalXform.rot
                    });

                    // in case someone grabbed both entities and destroyed the
                    // child/parent relationship
                    Entities.editEntity(puck.trackedEntityID, {
                        parentID: puck.puckEntityID
                    });
                } else {
                    destroyPuck(puckName);
                }
            }
        }
    }
}
function destroyPuck(puckName) {
    // unparent entity and delete its parent
    if (!trackedPucks.hasOwnProperty(puckName)) {
        return;
    }

    var puck = trackedPucks[puckName];
    var puckEntityID = puck.puckEntityID;
    var trackedEntityID = puck.trackedEntityID;

    // remove the puck as a parent entity and restore the tracked entities
    // former userdata and collision flag
    Entities.editEntity(trackedEntityID, {
        "parentID": "{00000000-0000-0000-0000-000000000000}",
        "userData": puck.trackedEntityUserData,
        "collisionless": puck.trackedEntityCollisionFlag
    });

    delete trackedPucks[puckName];

    // in some cases, the entity deletion may occur before the parent change
    // has been processed, resulting in both the puck and the tracked entity
    // to be deleted so we wait 100ms before deleting the puck, assuming
    // that the parent change has occured
    var DELETE_TIMEOUT = 100; // ms
    Script.setTimeout(function() {
        // delete the puck
        Entities.deleteEntity(puckEntityID);
    }, DELETE_TIMEOUT);
}
function destroyPucks() {
    // remove all pucks and unparent entities
    for (var puckName in trackedPucks) {
        if (trackedPucks.hasOwnProperty(puckName)) {
            destroyPuck(puckName);
        }
    }
}

function onWebEventReceived(msg) {
    var obj = {};

    try {
        obj = JSON.parse(msg);
    } catch (err) {
        return;
    }

    switch (obj.cmd) {
    case "ready":
        sendAvailableTrackedObjects();
        break;
    case "create":
        createPuck(obj);
        break;
    case "finalize":
        finalizePuck(obj.puckno);
        break;
    case "destroy":
        destroyPuck(obj.puckno);
        break;
    case "translation-acceleration-limit":
        setTranslationAccelerationLimit(Number(obj.val));
        break;
    case "translation-snap-threshold":
        setTranslationSnapThreshold(Number(obj.val));
        break;
    case "rotation-acceleration-limit":
        setRotationAccelerationLimit(Number(obj.val));
        break;
    case "rotation-snap-threshold":
        setRotationSnapThreshold(Number(obj.val));
        break;
    case "translation-smoothing-constant":
        setTranslationSmoothingConstant(Number(obj.val));
        break;
    case "rotation-smoothing-constant":
        setRotationSmoothingConstant(Number(obj.val));
        break;
    }
}

Script.update.connect(updatePucks);
Script.scriptEnding.connect(function () {
    tablet.removeButton(tabletButton);
    destroyPucks();
    if (shown) {
        tablet.webEventReceived.disconnect(onWebEventReceived);
        tablet.gotoHomeScreen();
    }
    tablet.screenChanged.disconnect(onScreenChanged);
    if (mapping) {
        mapping.disable();
    }
});
tabletButton.clicked.connect(function () {
    if (shown) {
        tablet.gotoHomeScreen();
    } else {
        tablet.gotoWebScreen(TABLET_APP_URL);
    }
});
}()); // END LOCAL_SCOPE
