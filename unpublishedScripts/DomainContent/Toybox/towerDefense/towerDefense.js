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

var BUILD_TIME = 1; // in minutes
var BUTTON_POSITION = { x: 100, y: 100, z: 100 };
var BASES = [
    {
        position: { x: 80, y: 100, z: 100 },
        color: { red: 255, green: 0, blue: 0 },
    },
    {
        position: { x: 120, y: 100, z: 100 },
        color: { red: 0, green: 255, blue: 0 },
    },
    {
        position: { x: 100, y: 100, z: 80 },
        color: { red: 0, green: 0, blue: 255 },
    },
    {
        position: { x: 100, y: 100, z: 120 },
        color: { red: 255, green: 0, blue: 255 },
    },
];
var BASES_SIZE = 10;
var BASES_HEIGHT = 2;
var BASES_TRANSPARENCY = 0.2;


var CHANNEL_NAME = "tower-defense-" //+ Math.floor(Math.random() * 9999);
print(CHANNEL_NAME);

var QUERY_RADIUS = 200;

var buttonID;
var bases = [];


Messages.subscribe(CHANNEL_NAME);
EntityViewer.setPosition(BUTTON_POSITION);
EntityViewer.setCenterRadius(QUERY_RADIUS);

setup();


function setup() {
    var buttonProperties = {
        type: 'Box',
        name: 'TestBox',
        position: BUTTON_POSITION,
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
        var baseProperties = {
            type: 'Box',
            name: 'TestBox',
            position: BASES[i].position,
            dimensions: { x: BASES_SIZE, y: 0.2, z: BASES_SIZE },
            color: BASES[i].color,
            alpha: 1.0,
        }
        // Base block
        bases.push(Entities.addEntity(baseProperties));
        baseProperties.alpha = BASES_TRANSPARENCY;
        baseProperties.position.y += BASES_HEIGHT;
        // Player area
        bases.push(Entities.addEntity(baseProperties));
    }
}

function cleanup() {
    while (bases.length > 0) {
        Entities.deleteEntity(bases.pop());
    }
    Entities.deleteEntity(buttonID);

    print("============= Script Stopping =============");
    Script.stop();
}

Messages.messageReceived.connect(function(channel, message, senderID) {
    print("Recieved: " + message + " from " + senderID);
    if (channel === CHANNEL_NAME) {
        switch (message) {
            case "BUILD":

                // Spawn spawner

                Script.setTimer(function() {
                    Messages.sendMessage(CHANNEL_NAME, "FIGHT");
                }, BUILD_TIME);
                break;
            case "STOP":
                cleanup();
                break;
        }

    }  
});

// Script.update.connect(function() {
//     EntityViewer.queryOctree();
// });

Script.scriptEnding.connect(cleanup);

