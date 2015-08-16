//  handGrab.js
//  examples
//
//  Created by Sam Gondelman on 8/3/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Allow avatar to grab the closest object to each hand and throw them
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
Script.include("http://s3.amazonaws.com/hifi-public/scripts/libraries/toolBars.js");

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var nullActionID = "00000000-0000-0000-0000-000000000000";
var controllerID;
var controllerActive;
var leftHandObjectID = null;
var rightHandObjectID = null;
var leftHandActionID = nullActionID;
var rightHandActionID = nullActionID;

var TRIGGER_THRESHOLD = 0.2;
var GRAB_RADIUS = 0.15;

var LEFT_HAND_CLICK = Controller.findAction("LEFT_HAND_CLICK");
var RIGHT_HAND_CLICK = Controller.findAction("RIGHT_HAND_CLICK");
var ACTION1 = Controller.findAction("ACTION1");
var ACTION2 = Controller.findAction("ACTION2");

var rightHandGrabAction = RIGHT_HAND_CLICK;
var leftHandGrabAction = LEFT_HAND_CLICK;

var rightHandGrabValue = 0;
var leftHandGrabValue = 0;
var prevRightHandGrabValue = 0
var prevLeftHandGrabValue = 0;

var grabColor = { red: 0, green: 255, blue: 0};
var releaseColor = { red: 0, green: 0, blue: 255};

var toolBar = new ToolBar(0, 0, ToolBar.vertical, "highfidelity.toybox.toolbar", function() {
    return {
        x: 100,
        y: 380
    };
});

var BUTTON_SIZE = 32;
var SWORD_IMAGE = "https://hifi-public.s3.amazonaws.com/images/sword/sword.svg"; // TODO: replace this with a table icon
var CLEANUP_IMAGE = "http://s3.amazonaws.com/hifi-public/images/delete.png";
var tableButton = toolBar.addOverlay("image", {
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: SWORD_IMAGE,
    alpha: 1
});
var cleanupButton = toolBar.addOverlay("image", {
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: CLEANUP_IMAGE,
    alpha: 1
});

var overlays = false;
var leftHandOverlay;
var rightHandOverlay;
if (overlays) {
    leftHandOverlay = Overlays.addOverlay("sphere", {
                            position: MyAvatar.getLeftPalmPosition(),
                            size: GRAB_RADIUS,
                            color: releaseColor,
                            alpha: 0.5,
                            solid: false
                        });
    rightHandOverlay = Overlays.addOverlay("sphere", {
                            position: MyAvatar.getRightPalmPosition(),
                            size: GRAB_RADIUS,
                            color: releaseColor,
                            alpha: 0.5,
                            solid: false
                        });
}

var OBJECT_HEIGHT_OFFSET = 0.5;
var MIN_OBJECT_SIZE = 0.05;
var MAX_OBJECT_SIZE = 0.3;
var TABLE_DIMENSIONS = {
    x: 10.0,
    y: 0.2,
    z: 5.0
};

var GRAVITY = {
    x: 0.0,
    y: -2.0,
    z: 0.0
}

var LEFT = 0;
var RIGHT = 1;

var tableCreated = false;

var NUM_OBJECTS = 100;
var tableEntities = Array(NUM_OBJECTS + 1); // Also includes table

var VELOCITY_MAG = 0.3;

var entitiesToResize = [];

var MODELS = Array(
    { modelURL: "https://hifi-public.s3.amazonaws.com/ozan/props/sword/sword.fbx" },
    { modelURL: "https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Vehicles/clara/spaceshuttle.fbx" },
    { modelURL: "https://s3.amazonaws.com/hifi-public/cozza13/apartment/Stargate.fbx" },
    { modelURL: "https://dl.dropboxusercontent.com/u/17344741/kelectricguitar10/kelectricguitar10.fbx" },
    { modelURL: "https://dl.dropboxusercontent.com/u/17344741/ktoilet10/ktoilet10.fbx" },
    { modelURL: "https://hifi-public.s3.amazonaws.com/models/props/MidCenturyModernLivingRoom/Interior/BilliardsTable.fbx" },
    { modelURL: "https://hifi-public.s3.amazonaws.com/ozan/avatars/robotMedic/robotMedicRed/robotMedicRed.fst" },
    { modelURL: "https://hifi-public.s3.amazonaws.com/ozan/avatars/robotMedic/robotMedicFaceRig/robotMedic.fst" },
    { modelURL: "https://hifi-public.s3.amazonaws.com/marketplace/contents/029db3d4-da2c-4cb2-9c08-b9612ba576f5/02949063e7c4aed42ad9d1a58461f56d.fst?1427169842" },
    { modelURL: "https://hifi-public.s3.amazonaws.com/models/props/MidCenturyModernLivingRoom/Interior/Bar.fbx" },
    { modelURL: "https://hifi-public.s3.amazonaws.com/marketplace/contents/96124d04-d603-4707-a5b3-e03bf47a53b2/1431770eba362c1c25c524126f2970fb.fst?1436924721" }
    // Complex models:
    // { modelURL: "https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Architecture/sketchfab/cudillero.fbx" },
    // { modelURL: "https://hifi-public.s3.amazonaws.com/ozan/sets/musicality/musicality.fbx" },
    // { modelURL: "https://hifi-public.s3.amazonaws.com/ozan/sets/statelyHome/statelyHome.fbx" }
    );

var COLLISION_SOUNDS = Array(
    "http://public.highfidelity.io/sounds/Collisions-ballhitsandcatches/pingpong_TableBounceMono.wav",
    "http://public.highfidelity.io/sounds/Collisions-ballhitsandcatches/billiards/collision1.wav"
    );

var RESIZE_TIMER = 0.0;
var RESIZE_WAIT = 0.05; // 50 milliseconds

var leftFist = Entities.addEntity( {
                    type: "Sphere",
                    shapeType: 'sphere',
                    position: MyAvatar.getLeftPalmPosition(),
                    dimensions: { x: GRAB_RADIUS, y: GRAB_RADIUS, z: GRAB_RADIUS },
                    rotation: MyAvatar.getLeftPalmRotation(),
                    visible: false,
                    collisionsWillMove: false,
                    ignoreForCollisions: true
                });
var rightFist = Entities.addEntity( {
                    type: "Sphere",
                    shapeType: 'sphere',
                    position: MyAvatar.getRightPalmPosition(),
                    dimensions: { x: GRAB_RADIUS, y: GRAB_RADIUS, z: GRAB_RADIUS },
                    rotation: MyAvatar.getRightPalmRotation(),
                    visible: false,
                    collisionsWillMove: false,
                    ignoreForCollisions: true
                });

function letGo(hand) {
    var actionIDToRemove = (hand == LEFT) ? leftHandActionID : rightHandActionID;
    var entityIDToEdit = (hand == LEFT) ? leftHandObjectID : rightHandObjectID;
    var handVelocity = (hand == LEFT) ? MyAvatar.getLeftPalmVelocity() : MyAvatar.getRightPalmVelocity();
    var handAngularVelocity = (hand == LEFT) ? MyAvatar.getLeftPalmAngularVelocity() :
                                               MyAvatar.getRightPalmAngularVelocity();
    if (actionIDToRemove != nullActionID && entityIDToEdit != null) {
        Entities.deleteAction(entityIDToEdit, actionIDToRemove);
        // TODO: upon successful letGo, restore collision groups
        if (hand == LEFT) {
            leftHandObjectID = null;
            leftHandActionID = nullActionID;
        } else {
            rightHandObjectID = null;
            rightHandActionID = nullActionID;
        }
    }
}

function setGrabbedObject(hand) {
    var handPosition = (hand == LEFT) ? MyAvatar.getLeftPalmPosition() : MyAvatar.getRightPalmPosition();
    var entities = Entities.findEntities(handPosition, GRAB_RADIUS);
    var objectID = null;
    var minDistance = GRAB_RADIUS;
    for (var i = 0; i < entities.length; i++) {
        // Don't grab the object in your other hands, your fists, or the table
        if ((hand == LEFT && entities[i] == rightHandObjectID) ||
            (hand == RIGHT && entities[i] == leftHandObjectID) ||
            entities[i] == leftFist || entities[i] == rightFist ||
            (tableCreated && entities[i] == tableEntities[0])) {
            continue;
        } else {
            var distance = Vec3.distance(Entities.getEntityProperties(entities[i]).position, handPosition);
            if (distance <= minDistance) {
                objectID = entities[i];
                minDistance = distance;
            }
        }
    }
    if (objectID == null) {
        return false;
    }
    if (hand == LEFT) {
        leftHandObjectID = objectID;
    } else {
        rightHandObjectID = objectID;
    }
    return true;
}

function grab(hand) {
    if (!setGrabbedObject(hand)) {
        // If you don't grab an object, make a fist
        Entities.editEntity((hand == LEFT) ? leftFist : rightFist, { ignoreForCollisions: false } );
        return;
    }
    var objectID = (hand == LEFT) ? leftHandObjectID : rightHandObjectID;
    var handRotation = (hand == LEFT) ? MyAvatar.getLeftPalmRotation() : MyAvatar.getRightPalmRotation();
    var handPosition = (hand == LEFT) ? MyAvatar.getLeftPalmPosition() : MyAvatar.getRightPalmPosition();

    var objectRotation = Entities.getEntityProperties(objectID).rotation;
    var offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

    var objectPosition = Entities.getEntityProperties(objectID).position;
    var offset = Vec3.subtract(objectPosition, handPosition);
    var offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, offsetRotation)), offset);
    // print(JSON.stringify(offsetPosition));
    var actionID = Entities.addAction("hold", objectID, {
        relativePosition: { x: 0, y: 0, z: 0 },
        relativeRotation: offsetRotation,
        hand: (hand == LEFT) ? "left" : "right",
        timeScale: 0.05
    });
    if (actionID == nullActionID) {
        if (hand == LEFT) {
            leftHandObjectID = null;
        } else {
            rightHandObjectID = null;
        }
    } else {
        // TODO: upon successful grab, add to collision group so object doesn't collide with immovable entities
        if (hand == LEFT) {
            leftHandActionID = actionID;
        } else {
            rightHandActionID = actionID;
        }
    }
}

function resizeModels() {
    var newEntitiesToResize = [];
    for (var i = 0; i < entitiesToResize.length; i++) {
        var naturalDimensions = Entities.getEntityProperties(entitiesToResize[i]).naturalDimensions;
        if (naturalDimensions.x != 1.0 || naturalDimensions.y != 1.0 || naturalDimensions.z != 1.0) {
            // bigger range of sizes for models
            var dimensions = Vec3.multiply(randFloat(MIN_OBJECT_SIZE, 3.0*MAX_OBJECT_SIZE), Vec3.normalize(naturalDimensions));
            Entities.editEntity(entitiesToResize[i], {
                dimensions: dimensions,
                shapeType: "box"
            });
        } else {
            newEntitiesToResize.push(entitiesToResize[i]);
        }

    }
    entitiesToResize = newEntitiesToResize;
}

function update(deltaTime) {
    if (overlays) {
        Overlays.editOverlay(leftHandOverlay, { position: MyAvatar.getLeftPalmPosition() });
        Overlays.editOverlay(rightHandOverlay, { position: MyAvatar.getRightPalmPosition() });
    }

    // if (tableCreated && RESIZE_TIMER < RESIZE_WAIT) {
    //     RESIZE_TIMER += deltaTime;
    // } else if (tableCreated) {
    //     resizeModels();
    // }

    rightHandGrabValue = Controller.getActionValue(rightHandGrabAction);
    leftHandGrabValue = Controller.getActionValue(leftHandGrabAction);

    Entities.editEntity(leftFist, { position: MyAvatar.getLeftPalmPosition() });
    Entities.editEntity(rightFist, { position: MyAvatar.getRightPalmPosition() });

    if (rightHandGrabValue > TRIGGER_THRESHOLD &&
        prevRightHandGrabValue < TRIGGER_THRESHOLD) {
        if (overlays) {
            Overlays.editOverlay(rightHandOverlay, { color: grabColor });
        }
        grab(RIGHT);
    } else if (rightHandGrabValue < TRIGGER_THRESHOLD &&
               prevRightHandGrabValue > TRIGGER_THRESHOLD) {
        Entities.editEntity(rightFist, { ignoreForCollisions: true } );
        if (overlays) {
            Overlays.editOverlay(rightHandOverlay, { color: releaseColor });
        }
        letGo(RIGHT);
    }

    if (leftHandGrabValue > TRIGGER_THRESHOLD &&
        prevLeftHandGrabValue < TRIGGER_THRESHOLD) {
        if (overlays) {
            Overlays.editOverlay(leftHandOverlay, { color: grabColor });
        }
        grab(LEFT);
    } else if (leftHandGrabValue < TRIGGER_THRESHOLD &&
               prevLeftHandGrabValue > TRIGGER_THRESHOLD) {
        Entities.editEntity(leftFist, { ignoreForCollisions: true } );
        if (overlays) {
            Overlays.editOverlay(leftHandOverlay, { color: releaseColor });
        }
        letGo(LEFT);
    }

    prevRightHandGrabValue = rightHandGrabValue;
    prevLeftHandGrabValue = leftHandGrabValue;
}

function cleanUp() {
    letGo(RIGHT);
    letGo(LEFT);
    if (overlays) {
        Overlays.deleteOverlay(leftHandOverlay);
        Overlays.deleteOverlay(rightHandOverlay);
    }
    Entities.deleteEntity(leftFist);
    Entities.deleteEntity(rightFist);
    removeTable();
    toolBar.cleanup();
}

function onClick(event) {
    if (event.deviceID != 0) {
        return;
    }
    switch (Overlays.getOverlayAtPoint(event)) {
        case tableButton:
            if (!tableCreated) {
                createTable();
                tableCreated = true;
            }
            break;
        case cleanupButton:
            if (tableCreated) {
                removeTable();
                tableCreated = false;
            }
            break;
    }
}

randFloat = function(low, high) {
    return low + Math.random() * (high - low);
}

randInt = function(low, high) {
    return Math.floor(randFloat(low, high));
}

function createTable() {
    var tablePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(MyAvatar.orientation)));
    tableEntities[0] = Entities.addEntity( {
                            type: "Model",
                            shapeType: 'box',
                            position: tablePosition,
                            dimensions: TABLE_DIMENSIONS,
                            rotation: MyAvatar.orientation,
                            // color: { red: 102, green: 51, blue: 0 },
                            modelURL: HIFI_PUBLIC_BUCKET + 'eric/models/woodFloor.fbx',
                            collisionSoundURL: "http://public.highfidelity.io/sounds/dice/diceCollide.wav"
                        });

    for (var i = 1; i < NUM_OBJECTS + 1; i++) {
        var objectOffset = { x: TABLE_DIMENSIONS.x/2.0 * randFloat(-1, 1),
                               y: OBJECT_HEIGHT_OFFSET,
                               z: TABLE_DIMENSIONS.z/2.0 * randFloat(-1, 1) };
        var objectPosition = Vec3.sum(tablePosition, Vec3.multiplyQbyV(MyAvatar.orientation, objectOffset));
        var type;
        var randType = randInt(0, 3);
        switch (randType) {
            case 0:
                type = "Box";
                break;
            case 1:
                type = "Sphere";
                // break;
            case 2:
                type = "Model";
                break;
        }
        tableEntities[i] = Entities.addEntity( {
                                type: type,
                                position: objectPosition,
                                velocity: { x: randFloat(-VELOCITY_MAG, VELOCITY_MAG),
                                            y: randFloat(-VELOCITY_MAG, VELOCITY_MAG),
                                            z: randFloat(-VELOCITY_MAG, VELOCITY_MAG) },
                                dimensions: { x: randFloat(MIN_OBJECT_SIZE, MAX_OBJECT_SIZE),
                                              y: randFloat(MIN_OBJECT_SIZE, MAX_OBJECT_SIZE),
                                              z: randFloat(MIN_OBJECT_SIZE, MAX_OBJECT_SIZE) },
                                rotation: MyAvatar.orientation,
                                gravity: GRAVITY,
                                damping: 0.1,
                                restitution: 0.01,
                                density: 0.5,
                                collisionsWillMove: true,
                                color: { red: randInt(0, 255), green: randInt(0, 255), blue: randInt(0, 255) },
                                // collisionSoundURL: COLLISION_SOUNDS[randInt(0, COLLISION_SOUNDS.length)]
                            });
        if (type == "Model") {
            var randModel = randInt(0, MODELS.length);
            Entities.editEntity(tableEntities[i], {
                shapeType: "box",
                modelURL: MODELS[randModel].modelURL
            });
            entitiesToResize.push(tableEntities[i]);
        }
    }
}

function removeTable() {
    RESIZE_TIMER = 0.0;
    for (var i = 0; i < tableEntities.length; i++) {
        Entities.deleteEntity(tableEntities[i]);
    }
}

Script.scriptEnding.connect(cleanUp);
Script.update.connect(update);
Controller.mousePressEvent.connect(onClick);