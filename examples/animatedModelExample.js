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
var moveUntil = 6000;
var stopAfter = moveUntil + 100;

var pitch = 0.0;
var yaw = 0.0;
var roll = 0.0;
var rotation = Quat.fromPitchYawRollDegrees(pitch, yaw, roll)

var originalProperties = {
    position: { x: 10,
                y: 0,
                z: 0 },

    radius : 1,

    color: { red: 0,
             green: 255,
             blue: 0 },

    modelURL: "http://www.fungibleinsight.com/faces/beta.fst",
    modelRotation: rotation,
    animationURL: "http://www.fungibleinsight.com/faces/gangnam_style_2.fbx",
    animationIsPlaying: true,
};

var positionDelta = { x: 0, y: 0, z: 0 };


var modelID = Models.addModel(originalProperties);
print("Models.addModel()... modelID.creatorTokenID = " + modelID.creatorTokenID);

var isPlaying = true;
var playPauseEveryWhile = 360;
var animationFPS = 30;
var adjustFPSEveryWhile = 120;

function moveModel(deltaTime) {

//print("count =" + count);

print("(count % playPauseEveryWhile)=" + (count % playPauseEveryWhile));

    if (count % playPauseEveryWhile == 0) {
        isPlaying = !isPlaying;
        print("isPlaying=" + isPlaying);
    }

print("(count % adjustFPSEveryWhile)=" + (count % adjustFPSEveryWhile));

    if (count % adjustFPSEveryWhile == 0) {

print("considering adjusting animationFPS=" + animationFPS);

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
    }

    if (count >= moveUntil) {

        // delete it...
        if (count == moveUntil) {
            print("calling Models.deleteModel()");
            Models.deleteModel(modelID);
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

    //print("modelID.creatorTokenID = " + modelID.creatorTokenID);

    if (true) {
        var newProperties = {
            //position: {
            //        x: originalProperties.position.x + (count * positionDelta.x),
            //        y: originalProperties.position.y + (count * positionDelta.y),
            //        z: originalProperties.position.z + (count * positionDelta.z)
            //},
            animationIsPlaying: isPlaying,
            animationFPS: animationFPS,
        };


        //print("modelID = " + modelID);
        //print("newProperties.position = " + newProperties.position.x + "," + newProperties.position.y+ "," + newProperties.position.z);

        Models.editModel(modelID, newProperties);
    }
}


// register the call back so it fires before each data send
Script.update.connect(moveModel);


Script.scriptEnding.connect(function () {
    print("cleaning up...");
    print("modelID="+ modelID.creatorTokenID + ", id:" + modelID.id);
    Models.deleteModel(modelID);
});

