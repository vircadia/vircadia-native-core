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
var BUILD_TIME_SECONDS = 5;
var BUTTON_POSITION = { x: 0, y: 0, z: 0 };
var BASES = [
    {
        position: { x: -20, y: 0, z: 0 },
        color: { red: 255, green: 0, blue: 0 },
        spawnerPosition: { x: -20, y: 0, z: -1 },
        buttonPosition: { x: -20, y: 0, z: -1 },
    },
    {
        position: { x: 20, y: 0, z: 0 },
        color: { red: 0, green: 255, blue: 0 },
        spawnerPosition: { x: 20, y: 0, z: 1 },
        buttonPosition: { x: 20, y: 0, z: 1 },
    },
    {
        position: { x: 0, y: 0, z: -20 },
        color: { red: 0, green: 0, blue: 255 },
        spawnerPosition: { x: 1, y: 0, z: -20 },
        buttonPosition: { x: 1, y: 0, z: -20 },
    },
    {
        position: { x: 0, y: 0, z: 20 },
        color: { red: 255, green: 0, blue: 255 },
        spawnerPosition: { x: -1, y: 0, z: 20 },
        buttonPosition: { x: -1, y: 0, z: 20 },
    },
];
var BASES_SIZE = 20;
var TARGET_SIZE = 2;
var BASES_HEIGHT = 6;
var BASES_TRANSPARENCY = 0.2;


var CHANNEL_NAME = "tower-defense-" //+ Math.floor(Math.random() * 9999);
print(CHANNEL_NAME);

var QUERY_RADIUS = 200;

var buttonID;
var bases = [];
var entityIDs = [];
var spawnerIDs = [];

var currentState = GAME_STATES.IDLE;

Messages.subscribe(CHANNEL_NAME);

if (this.EntityViewer) {
    EntityViewer.setPosition(Vec3.sum(BASE_POSITION, BUTTON_POSITION));
    EntityViewer.setCenterRadius(QUERY_RADIUS);
}

var BEGIN_BUILDING_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/letTheGamesBegin.wav"));
var BEGIN_FIGHTING_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/fight.wav"));
var GAME_OVER_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/gameOver.wav"));

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
        bases.push(Entities.addEntity(arenaProperties));


        const ROOF_HEIGHT = 0.2;
        position.y += BASES_HEIGHT - ROOF_HEIGHT;
        var roofProperties = {
            name: 'TD.Roof',
            type: 'Box',
            position: position,
            registrationPoint: { x: 0.5, y: 0, z: 0.5 },
            dimensions: { x: BASES_SIZE, y: ROOF_HEIGHT, z: BASES_SIZE },
            color: BASES[i].color,
            userData: JSON.stringify({
                gameChannel: CHANNEL_NAME,
            })
        }
        // Player area
        bases.push(Entities.addEntity(roofProperties));
    }
}

function cleanup() {
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

    print("============= Script Stopping =============");
    Script.stop();
}

Messages.messageReceived.connect(function(channel, message, senderID) {
    print("Recieved: " + message + " from " + senderID);
    if (channel === CHANNEL_NAME) {
        switch (message) {
            case "START":
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
                        }),
                    });
                    entityIDs.push(targetID);

                    // Create box spawner
                    var spawnerID = Entities.addEntity({
                        name: 'TD.BoxSpawner',
                        type: 'Box',
                        position: Vec3.sum(BASE_POSITION, BASES[i].spawnerPosition),
                        dimensions: { x: TARGET_SIZE, y: TARGET_SIZE, z: TARGET_SIZE },
                        color: BASES[i].color,
                        script: Script.resolvePath("launch.js"),
                        userData: JSON.stringify({
                            gameChannel: CHANNEL_NAME,
                        })
                    });
                    spawnerIDs.push(spawnerID);
                    Audio.playSound(BEGIN_BUILDING_SOUND, {
                        volume: 1.0,
                        position: BASE_POSITION
                    });
                }

                currentState = GAME_STATES.BUILD;

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

                }, BUILD_TIME_SECONDS * 1000);
                break;
            case "STOP":
                Audio.playSound(GAME_OVER_SOUND, {
                    volume: 1.0,
                    position: BASE_POSITION
                });
                currentState = GAME_STATES.GAME_OVER;
                cleanup();
                currentState = GAME_STATES.IDLE;
                break;
        }

    }  
});

// Script.update.connect(function() {
//     EntityViewer.queryOctree();
// });

Script.scriptEnding.connect(cleanup);

