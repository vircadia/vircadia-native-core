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

var TABLET_BUTTON_NAME = "PUCKTACH";
var TABLET_APP_URL = "http://content.highfidelity.com/seefo/production/puck-attach/puck-attach.html";

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var tabletButton = tablet.addButton({
    text: TABLET_BUTTON_NAME,
    icon: "https://s3.amazonaws.com/hifi-public/tony/icons/puck-i.svg",
    activeIcon: "https://s3.amazonaws.com/hifi-public/tony/icons/puck-a.svg"
});

var shown = false;
function onScreenChanged(type, url) {
    if (type === "Web" && url === TABLET_APP_URL) {
        tabletButton.editProperties({isActive: true});
        if (!shown) {
            // hook up to event bridge
            tablet.webEventReceived.connect(onWebEventReceived);

            Script.setTimeout(function () {
                // send available tracked objects to the html running in the tablet.
                var availableTrackedObjects = getAvailableTrackedObjects();
                tablet.emitScriptEvent(JSON.stringify(availableTrackedObjects));
            }, 1000); // wait 1 sec before sending..
        }
        shown = true;
    } else {
        tabletButton.editProperties({isActive: false});
        if (shown) {
            // disconnect from event bridge
            tablet.webEventReceived.disconnect(onWebEventReceived);
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
    var NUM_TRACKED_OBJECTS = 16;
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

function getRelativePosition(origin, rotation, offset) {
    var relativeOffset = Vec3.multiplyQbyV(rotation, offset);
    var worldPosition = Vec3.sum(origin, relativeOffset);
    return worldPosition;
}
function getPropertyForEntity(entityID, propertyName) {
    return Entities.getEntityProperties(entityID, [propertyName])[propertyName];
}

var VIVE_PUCK_MODEL = "http://content.highfidelity.com/seefo/production/puck-attach/vive_tracker_puck.obj";
var VIVE_PUCK_DIMENSIONS = { x: 0.0945, y: 0.0921, z: 0.0423 }; // 1/1000th scale of model
var VIVE_PUCK_SEARCH_DISTANCE = 1.5; // metres
var VIVE_PUCK_SPAWN_DISTANCE = 1.0; // metres
var VIVE_PUCK_NAME = "Tracked Puck";

var trackedPucks = { };
var lastPuck = { };

function createPuck(puck) {
    // create a puck entity and add it to our list of pucks
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
    
    if (trackedPucks.hasOwnProperty(puck.puckno)) {
        destroyPuck(puck.puckno);
    }
    
    trackedPucks[puck.puckno] = {
        puckEntityID: puckEntityID,
        trackedEntityID: ""
    };
    lastPuck = trackedPucks[puck.puckno];
}
function finalizePuck() {
    // find nearest entity and change its parent to the puck
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
        Entities.editEntity(lastPuck.trackedEntityID, { "parentID": lastPuck.puckEntityID });
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
                var avatarXform = new Xform(MyAvatar.orientation, MyAvatar.position);
                var puckXform = new Xform(pose.rotation, pose.translation);
                var finalXform = Xform.mul(avatarXform, puckXform);

                Entities.editEntity(puck.puckEntityID, {
                    position: finalXform.pos,
                    rotation: finalXform.rot,
                });
                
                // in case someone grabbed both entities and destroyed the 
                // child/parent relationship
                Entities.editEntity(puck.trackedEntityID, {
                    parentID: puck.puckEntityID
                });
            } 
        }
    }
}
function destroyPuck(puckName) {
    // unparent entity and delete its parent
    var puckEntityID = trackedPucks[puckName].puckEntityID;
    var trackedEntityID = trackedPucks[puckName].trackedEntityID;

    // remove the puck as a parent entity and then delete the puck
    Entities.editEntity(trackedEntityID, { "parentID": "{00000000-0000-0000-0000-000000000000}" });
    Entities.deleteEntity(puckEntityID);
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
    case "create":
        createPuck(obj);
        break;
    case "finalize":
        finalizePuck();
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
});
tabletButton.clicked.connect(function () {
    if (shown) {
        tablet.gotoHomeScreen();
    } else {
        tablet.gotoWebScreen(TABLET_APP_URL);
    }
});
}()); // END LOCAL_SCOPE