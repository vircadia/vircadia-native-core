//  createBoxes.js
//  part of bubblewand
//
//  Created by James B. Pollack @imgntn -- 09/07/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Loads a wand model and attaches the bubble wand behavior.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html



Script.include("https://raw.githubusercontent.com/highfidelity/hifi/master/examples/utilities.js");
Script.include("https://raw.githubusercontent.com/highfidelity/hifi/master/examples/libraries/utils.js");

var bubbleModel = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/models/bubble/bubble.fbx?' + randInt(0, 10000);;
//var scriptURL'http://hifi-public.s3.amazonaws.com/james/bubblewand/scripts/wand.js?'+randInt(0,10000);

//for local testing 
//var scriptURL = "http://localhost:8080/scripts/setRecurringTimeout.js?" + randInt(0, 10000);


var scriptURL='http://hifi-public.s3.amazonaws.com/james/debug/timeouts/setRecurringTimeout.js?'+ randInt(0, 10000);
//create the wand in front of the avatar

var boxes=[];
var TEST_ENTITY_NAME = "TimerScript";


var TOTAL_ENTITIES = 100;
for (var i = 0; i < TOTAL_ENTITIES; i++) {
    var box = Entities.addEntity({
        type: "Box",
        name: TEST_ENTITY_NAME,
        position: {
            x: randInt(0, 100) - 50 +  MyAvatar.position.x,
            y: randInt(0, 100) - 50 +  MyAvatar.position.x,
            z: randInt(0, 100) - 50 +  MyAvatar.position.x,
        },
        dimensions: {
            x: 1,
            y: 1,
            z: 1,
        },
        color: {
            red: 255,
            green: 0,
            blue: 0,
        },
        //must be enabled to be grabbable in the physics engine
        collisionsWillMove: true,
        shapeType: 'box',
        lifetime:60,
        script: scriptURL
    });
    boxes.push(box)
}


function cleanup() {
    while (boxes.length > 0) {
        Entities.deleteEntity(boxes.pop());

    }
}


Script.scriptEnding.connect(cleanup);