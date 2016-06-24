//
//  animatedModelExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates creating and editing a model
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var count = 0;
var moveUntil = 1000;
var stopAfter = moveUntil + 100;

var pitch = 0.0;
var yaw = 0.0;
var roll = 0.0;
var rotation = Quat.fromPitchYawRollDegrees(pitch, yaw, roll);

var originalProperties = {
    type: "Model",
    position: { x: MyAvatar.position.x,
                y: MyAvatar.position.y,
                z: MyAvatar.position.z },
    dimensions: {
                x: 1.62,
                y: 0.41,
                z: 1.13
                },

    color: { red: 0,
             green: 255,
             blue: 0 },

    modelURL: "http://public.highfidelity.io/cozza13/club/dragon/dragon.fbx",
    rotation: rotation,
    animation: {
        url: "http://public.highfidelity.io/cozza13/club/dragon/flying.fbx",
        running: true,
        fps: 30,
        firstFrame: 10,
        lastFrame: 20,
        loop: true
    }
};

var modelID = Entities.addEntity(originalProperties);

var isPlaying = true;
var playPauseEveryWhile = 360;
var animationFPS = 30;
var adjustFPSEveryWhile = 120;
var resetFrameEveryWhile = 600;

function moveModel(deltaTime) {
    var somethingChanged = false;
    print("count= " + count);
    if (count % playPauseEveryWhile == 0) {
        isPlaying = !isPlaying;
        print("isPlaying=" + isPlaying);
        somethingChanged = true;
    }

    if (count % adjustFPSEveryWhile == 0) {
        if (animationFPS == 30) {
            animationFPS = 10;
        } else if (animationFPS == 10) {
            animationFPS = 60;
        } else if (animationFPS == 60) {
            animationFPS = 30;
        }
        print("animationFPS=" + animationFPS);
        isPlaying = true;
        print("always start playing if we change the FPS -- isPlaying=" + isPlaying);
        somethingChanged = true;
    }

    if (count % resetFrameEveryWhile == 0) {
        resetFrame = true;
        somethingChanged = true;
    }

    if (count >= moveUntil) {

        // delete it...
        if (count == moveUntil) {
            print("calling Models.deleteModel()");
            Entities.deleteEntity(modelID);
        }

        // stop it...
        if (count >= stopAfter) {
            print("calling Script.stop()");
            Script.stop();
        }

        count++;
        return; // break early
    }

    count++;

    if (somethingChanged) {
        var newProperties = {
            animation: {
                running: isPlaying,
                fps: animationFPS
            }
        };
        
        if (resetFrame) {
            print("resetting the frame!");
            newProperties.animation.currentFrame = 0;
            resetFrame = false;
        }

        Entities.editEntity(modelID, newProperties);
    }
}


// register the call back so it fires before each data send
Script.update.connect(moveModel);


Script.scriptEnding.connect(function () {
    print("cleaning up...");
    print("modelID=" + modelID);
    Models.deleteModel(modelID);
});

