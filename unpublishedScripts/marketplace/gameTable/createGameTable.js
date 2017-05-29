//
//  Created by Thijs Wenker on 3/31/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Revision of James B. Pollack's work on GamesTable in 2016
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var TABLE_MODEL_URL = Script.resolvePath('assets/table/gameTable.fbx');
var MODEL_URL = Script.resolvePath('assets/table/finalFrame.fbx');
var TABLE_SCRIPT_URL = Script.resolvePath('table.js');
var ENTITY_SPAWNER_SCRIPT_URL = Script.resolvePath('entitySpawner.js');
var NEXT_GAME_BUTTON_SCRIPT_URL = Script.resolvePath('nextGameButton.js');
var RESET_BUTTON_SCRIPT_URL = Script.resolvePath('resetGameButton.js');
var TABLE_PICTURE_URL = Script.resolvePath('assets/mats/Table-default.jpg');
var NEXT_BUTTON_MODEL_URL = Script.resolvePath('assets/buttons/button-next.fbx');
var RESET_BUTTON_MODEL_URL = Script.resolvePath('assets/buttons/button-reset.fbx');

// FIXME: CHANGE TO Quat.getForward when supported
var front = Quat.getFront(MyAvatar.orientation);
var avatarHalfDown = MyAvatar.position;
var TABLE_START_POSITION = Vec3.sum(avatarHalfDown, Vec3.multiply(2, front));

var table, entitySpawner, mat;
var nextGameButton, resetGameButton;

var entitySpawnerOffset = {
    forward: 0,
    vertical: 1,
    right: 0
};

var matOffset = {
    forward: 0,
    vertical: 0.545,
    right: 0
};

var nextGameButtonOffset = {
    forward: -0.2,
    vertical: 0.45,
    right: -0.65
};

var resetGameButtonOffset = {
    forward: 0.2,
    vertical: 0.45,
    right: -0.65
};

function getOffsetFromTable(forward, vertical, right) {
    var properties = Entities.getEntityProperties(table, ['position', 'rotation']);
    var position = properties.position;
    var frontVector = Quat.getFront(properties.rotation);
    var upVector = Quat.getUp(properties.rotation);
    var rightVector = Quat.getRight(properties.rotation);
    if (forward !== undefined) {
        position = Vec3.sum(position, Vec3.multiply(frontVector, forward));
    }
    if (vertical !== undefined) {
        position = Vec3.sum(position, Vec3.multiply(upVector, vertical));
    }
    if (right !== undefined) {
        position = Vec3.sum(position, Vec3.multiply(rightVector, right));
    }

    return position;
}

function createTable() {
    table = Entities.addEntity({
        type: 'Model',
        name: 'GameTable Table 1',
        description: 'hifi:gameTable:table',
        modelURL: TABLE_MODEL_URL,
        shapeType: 'box',
        dynamic: true,
        gravity: {
            x: 0,
            y: -3.0,
            z: 0
        },
        density: 8000,
        restitution: 0,
        damping: 0.9,
        angularDamping: 0.8,
        friction: 1,
        dimensions: {
            x: 1.355,
            y: 1.1121,
            z: 1.355
        },
        velocity: {
            x: 0.0,
            y: -0.1,
            z: 0.0
        },
        script: TABLE_SCRIPT_URL,
        position: TABLE_START_POSITION,
        userData: JSON.stringify({
            grabbableKey: {
                grabbable: true
            }
        })
    });
}

function createEntitySpawner() {
    entitySpawner = Entities.addEntity({
        type: 'Zone',
        visible: false,
        name: 'GameTable Entity Spawner',
        collisionless: true,
        description: 'hifi:gameTable:entitySpawner',
        color: {
            red: 0,
            green: 255,
            blue: 0
        },
        dimensions: {
            x: 0.25,
            y: 0.25,
            z: 0.25
        },
        script: ENTITY_SPAWNER_SCRIPT_URL,
        parentID: table,
        position: getOffsetFromTable(entitySpawnerOffset.forward, entitySpawnerOffset.vertical, entitySpawnerOffset.right)
    });
}

function createMat() {
    return Entities.addEntity({
        type: 'Model',
        modelURL: MODEL_URL,
        name: 'GameTable Mat',
        description: 'hifi:gameTable:mat',
        collisionless: true,
        color: {
            red: 0,
            green: 0,
            blue: 255
        },
        restitution: 0,
        friction: 1,
        dimensions: {
            x: 1.045,
            y: 1.045,
            z: 0.025
        },
        rotation: Quat.fromPitchYawRollDegrees(90, 0, 0),
        textures: JSON.stringify({
            Picture: TABLE_PICTURE_URL
        }),
        parentID: table,
        position: getOffsetFromTable(matOffset.forward, matOffset.vertical, matOffset.right),
        userData: JSON.stringify({
            grabbableKey: {
                grabbable: false
            }
        })
    });
}

function createNextGameButton() {
    nextGameButton = Entities.addEntity({
        type: 'Model',
        modelURL: NEXT_BUTTON_MODEL_URL,
        name: 'GameTable Next Button',
        description: 'hifi:gameTable:nextGameButton',
        collisionless: true,
        color: {
            red: 0,
            green: 0,
            blue: 255
        },
        dimensions: {
            x: 0.0625,
            y: 0.1250,
            z: 0.1250
        },
        rotation: Quat.angleAxis(180, {
            x: 0,
            y: 1,
            z: 0
        }),
        parentID: table,
        script: NEXT_GAME_BUTTON_SCRIPT_URL,
        position: getOffsetFromTable(nextGameButtonOffset.forward, nextGameButtonOffset.vertical,
                                     nextGameButtonOffset.right),
        userData: JSON.stringify({
            grabbableKey: {
                wantsTrigger: true
            }
        })
    });
}

function createResetGameButton() {
    resetGameButton = Entities.addEntity({
        type: 'Model',
        modelURL: RESET_BUTTON_MODEL_URL,
        name: 'GameTable Reset Button',
        description: 'hifi:gameTable:resetGameButton',
        collisionless: true,
        color: {
            red: 255,
            green: 0,
            blue: 0
        },
        dimensions: {
            x: 0.0628,
            y: 0.1248,
            z: 0.1317
        },
        rotation: Quat.angleAxis(180, {
            x: 0,
            y: 1,
            z: 0
        }),
        parentID: table,
        script: RESET_BUTTON_SCRIPT_URL,
        position: getOffsetFromTable(resetGameButtonOffset.forward, resetGameButtonOffset.vertical,
                                     resetGameButtonOffset.right),
        userData: JSON.stringify({
            grabbableKey: {
                wantsTrigger: true
            }
        })
    });
}

function makeTable() {
    createTable();
    createMat();
    createEntitySpawner();
    createResetGameButton();
    createNextGameButton();
}

function cleanup() {
    Entities.deleteEntity(table);
    Entities.deleteEntity(mat);
    Entities.deleteEntity(entitySpawner);
    Entities.deleteEntity(nextGameButton);
    Entities.deleteEntity(resetGameButton);
}

Script.scriptEnding.connect(cleanup);

makeTable();
