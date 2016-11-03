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


var tableCreated = false;

var NUM_OBJECTS = 100;
var tableEntities = Array(NUM_OBJECTS + 1); // Also includes table

var VELOCITY_MAG = 0.3;


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




function cleanUp() {
    print("CLEANUP!!!")
    if (overlays) {
        Overlays.deleteOverlay(leftHandOverlay);
        Overlays.deleteOverlay(rightHandOverlay);
    }
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
                                dynamic: true,
                                color: { red: randInt(0, 255), green: randInt(0, 255), blue: randInt(0, 255) },
                            });
        if (type == "Model") {
            var randModel = randInt(0, MODELS.length);
            Entities.editEntity(tableEntities[i], {
                shapeType: "box",
                modelURL: MODELS[randModel].modelURL
            });
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
Controller.mousePressEvent.connect(onClick);
