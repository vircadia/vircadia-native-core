//
//  injectorLoadTest.js
//  audio
//
//  Created by Eric Levin 2/1/2016
//  Copyright 2016 High Fidelity, Inc.

//  This  script tests what happens when many audio injectors are created and played
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


Script.include("../libraries/utils.js");


var numSoundsToPlayPerBatch = 1;
var timeBetweenBatch = 100;
// A green box represents an injector that is playing

var basePosition = {
    x: 0,
    y: 0,
    z: 0
};

var soundBoxes = [];

var testSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/dove.wav");
var totalInjectors = 0;

if(!testSound.downloaded) {

    print("SOUND IS NOT READY YET")
    testSound.ready.connect(function() {
        playSounds();
    });
} else {
    // otherwise play sounds right away
    playSounds();
}

function playSounds() {
    print("PLAY SOUNDS!")
    for (var i = 0; i < numSoundsToPlayPerBatch; i++) {
        playSound();
    } 
    print("EBL Total Number of Injectors: " + totalInjectors);
    
    Script.setTimeout(function() {
        playSounds();
    }, timeBetweenBatch);    
}


function playSound() {
    var position = Vec3.sum(basePosition, {x: randFloat(-.1, .1), y: randFloat(-1, 1), z: randFloat(-3, -.1)});
    var injector = Audio.playSound(testSound, {
        position: position,
        volume: 0.1
    });
    totalInjectors++;

    var soundBox = Entities.addEntity({
        type: "Box",
        color: {red: 200, green: 10, blue: 200},
        dimensions: {x: 0.1, y: 0.1, z: 0.1},
        position: position
    });

    soundBoxes.push(soundBox);
}

function cleanup() {
    soundBoxes.forEach( function(soundBox) {
        Entities.deleteEntity(soundBox);
    });
}

Script.scriptEnding.connect(cleanup);