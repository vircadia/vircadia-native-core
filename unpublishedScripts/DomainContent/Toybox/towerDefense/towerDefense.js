//
//  towerDefense.js
//
//  Created by Clement on 12/1/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

print("============= Script Starting =============");

function findSurfaceBelowPosition(pos) {
    var result = Entities.findRayIntersection({
        origin: pos,
        direction: { x: 0, y: -1, z: 0 }
    });
    if (result.intersects) {
        return result.intersection;
    }
    return pos;
}

var GAME_STATES = {
    IDLE: 0,
    BUILD: 1,
    FIGHT: 2,
    GAMEOVER: 3,
};

var BASE_POSITION = findSurfaceBelowPosition(MyAvatar.position);
var BUILD_TIME_SECONDS = 60;
var BUTTON_POSITION = { x: 0, y: 0, z: 0 };
var BASES_SIZE = 15;
var TARGET_SIZE = 2;
var BASES_HEIGHT = 6;
var BASES_TRANSPARENCY = 0.2;
var BASES = [
    {
        position: { x: -15, y: 0, z: 0 },
        color: { red: 255, green: 0, blue: 0 },
        spawnerPosition: { x: -15 + BASES_SIZE/2, y: 0, z: -BASES_SIZE/2 },
    },
    {
        position: { x: 15, y: 0, z: 0 },
        color: { red: 0, green: 255, blue: 0 },
        spawnerPosition: { x: 15 - BASES_SIZE/2, y: 0, z: BASES_SIZE/2 },
    },
    {
        position: { x: 0, y: 0, z: -15 },
        color: { red: 0, green: 0, blue: 255 },
        spawnerPosition: { x: BASES_SIZE/2, y: 0, z: -15 + BASES_SIZE/2 },
    },
    {
        position: { x: 0, y: 0, z: 15 },
        color: { red: 255, green: 0, blue: 255 },
        spawnerPosition: { x: -BASES_SIZE/2, y: 0, z: 15 - BASES_SIZE/2 },
    },
];


var CHANNEL_NAME = "tower-defense-" //+ Math.floor(Math.random() * 9999);
print(CHANNEL_NAME);

var QUERY_RADIUS = 200;

var buttonID;
var bases = [];
var entityIDs = [];
var spawnerIDs = [];

var teamEntities = {
    0: {},
    1: {},
    2: {},
    3: {},
};

var currentState = GAME_STATES.IDLE;

Messages.subscribe(CHANNEL_NAME);

if (this.EntityViewer) {
    EntityViewer.setPosition(Vec3.sum(BASE_POSITION, BUTTON_POSITION));
    EntityViewer.setCenterRadius(QUERY_RADIUS);
}

var BEGIN_BUILDING_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/letTheGamesBegin.wav"));
var BEGIN_FIGHTING_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/fight.wav"));
var GAME_OVER_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/gameOver.wav"));
var TEN_SECONDS_REMAINING_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/tenSecondsRemaining.wav"));

setup();


function setup() {
    var buttonProperties = {
        type: 'Box',
        name: 'TD.StartButton',
        position: Vec3.sum(BASE_POSITION, BUTTON_POSITION),
        dimensions: { x: 1, y: 1, z: 1 },
        color: { red: 0, green: 255, blue: 0 },
        script: Script.resolvePath("towerButton.js"),
        userData: JSON.stringify({
            grabbableKey: { 
                wantsTrigger: true
            },
            gameChannel: CHANNEL_NAME 
        }),
    }
    buttonID = Entities.addEntity(buttonProperties);

    for (var i in BASES) {
        var position = Vec3.sum(BASE_POSITION, BASES[i].position);
        var arenaProperties = {
            name: 'TD.Arena',
            type: 'Box',
            position: position,
            registrationPoint: { x: 0.5, y: 0, z: 0.5 },
            dimensions: { x: BASES_SIZE, y: BASES_HEIGHT, z: BASES_SIZE },
            color: { red: 255, green: 255, blue: 255 },
            script: Script.resolvePath("teamAreaEntity.js"),
            visible: false,
            collisionless: true,
            userData: JSON.stringify({
                gameChannel: CHANNEL_NAME,
            })
        };
        // Base block
        var arenaID = Entities.addEntity(arenaProperties)
        bases.push(arenaID);
        teamEntities[i].arenaID = arenaID;


        const ROOF_HEIGHT = 0.2;
        position.y += BASES_HEIGHT - ROOF_HEIGHT;
        var roofProperties = {
            name: 'TD.Roof',
            type: 'Box',
            position: position,
            registrationPoint: { x: 0.5, y: 0, z: 0.5 },
            dimensions: { x: BASES_SIZE, y: ROOF_HEIGHT, z: BASES_SIZE },
            color: BASES[i].color,
            script: Script.resolvePath('roofEntity.js'),
            userData: JSON.stringify({
                gameChannel: CHANNEL_NAME,
            })
        }
        // Player area
        var roofID = Entities.addEntity(roofProperties)
        bases.push(roofID);
        teamEntities[i].roofID = roofID;
    }
}

function cleanup() {
    for (var i = 0; i < 4; ++i) {
        var t = teamEntities[i];
        Entities.deleteEntity(t.targetID);
        Entities.deleteEntity(t.spawnerID);
        if (t.bowIDs !== undefined) {
            for (var j = 0; j < t.bowIDs.length; ++j) {
                Entities.deleteEntity(t.bowIDs[j]);
            }
        }
        Entities.deleteEntity(t.roofID);
        Entities.deleteEntity(t.arenaID);
    }
    while (bases.length > 0) {
        Entities.deleteEntity(bases.pop());
    }
    Entities.deleteEntity(buttonID);
    print("Size of entityIDs:", entityIDs.length);
    for (var i = 0; i < entityIDs.length; ++i) {
        print("Deleting: ", entityIDs[i]);
        Entities.deleteEntity(entityIDs[i]);
    }
    entityIDs = [];
    for (var i = 0; i < spawnerIDs.length; ++i) {
        Entities.deleteEntity(spawnerIDs[i]);
    }
    spawnerIDs = [];

    print("============= Script Stopping =============");
    Script.stop();
}

function parseJSON(json) {
    try {
        return JSON.parse(json);
    } catch (e) {
        return undefined;
    }
}

Messages.messageReceived.connect(function(channel, messageJSON, senderID) {
    print("Recieved: " + messageJSON + " from " + senderID);
    if (channel === CHANNEL_NAME) {
        var message = parseJSON(messageJSON);
        if (message === undefined) {
            print("towerDefense.js | Received non-json message");
            return;
        }
        switch (message.type) {
            case 'start-game':
                if (currentState != GAME_STATES.IDLE) {
                    print("Got start message, but not in idle state. Current state: " + currentState);
                    return;
                }

                Entities.deleteEntity(buttonID);

                for (var i in BASES) {
                    var position = Vec3.sum(BASE_POSITION, BASES[i].position);

                    // Create target entity
                    var targetID = Entities.addEntity({
                        name: 'TD.TargetEntity',
                        type: 'Sphere',
                        position: position,
                        dimensions: { x: TARGET_SIZE, y: TARGET_SIZE, z: TARGET_SIZE },
                        color: BASES[i].color,
                        script: Script.resolvePath("targetEntity.js"),
                        userData: JSON.stringify({
                            gameChannel: CHANNEL_NAME,
                            teamNumber: i
                        }),
                    });
                    entityIDs.push(targetID);
                    teamEntities[i].targetID = targetID;

                    // Create box spawner
                    var spawnerID = Entities.addEntity({
                        name: 'TD.BoxSpawner',
                        type: 'Box',
                        position: Vec3.sum(BASE_POSITION, BASES[i].spawnerPosition),
                        dimensions: { x: TARGET_SIZE, y: TARGET_SIZE, z: TARGET_SIZE },
                        color: BASES[i].color,
                        script: Script.resolvePath("launch.js"),
                        userData: JSON.stringify({
                            grabbableKey: { 
                                wantsTrigger: true
                            },
                            gameChannel: CHANNEL_NAME,
                        })
                    });
                    teamEntities[i].spawnerID = targetID;
                    spawnerIDs.push(spawnerID);
                    Audio.playSound(BEGIN_BUILDING_SOUND, {
                        volume: 1.0,
                        position: BASE_POSITION
                    });
                }

                currentState = GAME_STATES.BUILD;

                Script.setTimeout(function() {
                    Audio.playSound(TEN_SECONDS_REMAINING_SOUND, {
                        volume: 1.0,
                        position: BASE_POSITION
                    });
                }, (BUILD_TIME_SECONDS - 10) * 1000);
                Script.setTimeout(function() {
                    print("============ Done building, FIGHT =============");

                    Audio.playSound(BEGIN_FIGHTING_SOUND, {
                        volume: 1.0,
                        position: BASE_POSITION
                    });

                    Messages.sendMessage(CHANNEL_NAME, "FIGHT");


                    // Cleanup spawner cubes
                    currentState = GAME_STATES.FIGHT;
                    for (var i = 0; i < spawnerIDs.length; ++i) {
                        Entities.deleteEntity(spawnerIDs[i]);
                    }

                    // Spawn bows
                    for (var i = 0; i < BASES.length; ++i) {
                        var position = Vec3.sum(BASE_POSITION, BASES[i].position);
                        position.y += BASES_HEIGHT + 1;
                        teamEntities[i].bowIDs = [];

                        for (var j = 0; j < 4; ++j) {
                            teamEntities[i].bowIDs.push(Entities.addEntity({
                                position: position,
                                "collisionsWillMove": 1,
                                "compoundShapeURL": Script.resolvePath("bow/bow_collision_hull.obj"),
                                "created": "2016-09-01T23:57:55Z",
                                "dimensions": {
                                    "x": 0.039999999105930328,
                                    "y": 1.2999999523162842,
                                    "z": 0.20000000298023224
                                },
                                "dynamic": 1,
                                "gravity": {
                                    "x": 0,
                                    "y": -1,
                                    "z": 0
                                },
                                "modelURL": Script.resolvePath("bow/bow-deadly.fbx"),
                                "name": "TD.Hifi-Bow",
                                "rotation": {
                                    "w": 0.9718012809753418,
                                    "x": 0.15440607070922852,
                                    "y": -0.10469216108322144,
                                    "z": -0.14418250322341919
                                },
                                "script": Script.resolvePath("bow/bow.js"),
                                "shapeType": "compound",
                                "type": "Model",
                                "userData": "{\"grabbableKey\":{\"grabbable\":true},\"wearable\":{\"joints\":{\"RightHand\":[{\"x\":0.0813,\"y\":0.0452,\"z\":0.0095},{\"x\":-0.3946,\"y\":-0.6604,\"z\":0.4748,\"w\":-0.4275}],\"LeftHand\":[{\"x\":-0.0881,\"y\":0.0259,\"z\":0.0159},{\"x\":0.4427,\"y\":-0.6519,\"z\":0.4592,\"w\":0.4099}]}}}"
                            }));
                        }
                    }

                }, BUILD_TIME_SECONDS * 1000);
                break;
            case 'target-hit':
                print("Got target-hit for: ", message.teamNumber);
                if (currentState !== GAME_STATES.FIGHT) {
                    return;
                }
                print("game is over");
                Audio.playSound(GAME_OVER_SOUND, {
                    volume: 1.0,
                    position: BASE_POSITION
                });
                currentState = GAME_STATES.GAME_OVER;

                // Delete the entities for the team that lost
                var t = teamEntities[message.teamNumber];
                Entities.deleteEntity(t.targetID);
                Entities.deleteEntity(t.spawnerID);
                Entities.deleteEntity(t.roofID);
                Entities.deleteEntity(t.arenaID);

                // TODO If more than 1 team is still alive, don't cleanup
                // Do a full cleanup after 10 seconds
                Script.setTimeout(function() {
                    cleanup();
                    currentState = GAME_STATES.IDLE;
                }, 10 * 1000);
                break;
            default:
                print("towerDefense.js | Ignoring unknown message type: ", message.type);
                break;
        }

    }  
});

// Script.update.connect(function() {
//     EntityViewer.queryOctree();
// });

Script.scriptEnding.connect(cleanup);

