//
//  createTableWithItems.js
//
//  Created by James B. Pollack @imgntn on 1/7/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This script shows how to hook up a model entity to your avatar to act as a doppelganger.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var table, wearable;

var TABLE_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/james/doppelganger/table.FBX';
var TABLE_DIMENSIONS = {
    x: 0.76,
    y: 1.06,
    z: 0.76
};

function createTable() {
    var avatarRot = Quat.fromPitchYawRollDegrees(0, MyAvatar.bodyYaw, 0.0);
    var position, rotation;
    var ids = Entities.findEntities(MyAvatar.position, 20);
    var hasBase = false;
    for (var i = 0; i < ids.length; i++) {
        var entityID = ids[i];
        var props = Entities.getEntityProperties(entityID, "name");
        var name = props.name;
        if (name === "Hifi-Dressing-Room-Base") {
            var details = Entities.getEntityProperties(entityID, ["position", "dimensions", "rotation"]);
            var rightVector = Quat.getRight(details.rotation);
            var rightDistance = 1.5;
            position = Vec3.sum(Vec3.multiply(rightDistance, rightVector), details.position);
            position.y = details.position.y += TABLE_DIMENSIONS.y / 2
            rotation = details.rotation;
            hasBase = true;
        }
    }

    if (hasBase === false) {
        position = Vec3.sum(MyAvatar.position, Vec3.multiply(1.5, Quat.getFront(avatarRot)));
        rotation = avatarRot;
    }

    var tableProperties = {
        name: 'Hifi-Dressing-Room-Table',
        type: 'Model',
        shapeType:'box',
        modelURL: TABLE_MODEL_URL,
        dimensions: TABLE_DIMENSIONS,
        position: position,
        rotation: rotation,
        collisionsWillMove: false,
        ignoreForCollisions: false,
        userData: JSON.stringify({
            grabbableKey: {
                grabbable: false
            }
        })
    }
    print('TABLE PROPS', JSON.stringify(tableProperties))
    table = Entities.addEntity(tableProperties);
}


function createWearable() {
    var tableProperties = Entities.getEntityProperties(table);
    var properties = {
        type: 'Model',
        modelURL: 'https://s3.amazonaws.com/hifi-public/tony/cowboy-hat.fbx',
        name: 'Hifi-Wearable',
        dimensions: {
            x: 0.25,
            y: 0.25,
            z: 0.25
        },
        color: {
            red: 0,
            green: 255,
            blue: 0
        },
        position: {
            x: tableProperties.position.x,
            y: tableProperties.position.y + tableProperties.dimensions.y / 1.5,
            z: tableProperties.position.z
        },
        userData: JSON.stringify({
            "grabbableKey": {
                "invertSolidWhileHeld": false
            },
            "wearable": {
                "joints": ["head", "Head", "hair", "neck"]
            },
            handControllerKey: {
                disableReleaseVelocity: true,
                disableMoveWithHead: true,
            }
        })
    }
    wearable = Entities.addEntity(properties);
}

function init() {
    createTable();
    createWearable();
}

function cleanup() {
    Entities.deleteEntity(table);
    Entities.deleteEntity(wearable);

}
init();
Script.scriptEnding.connect(cleanup)