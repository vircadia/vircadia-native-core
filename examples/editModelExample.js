//
//  editModelExample.js
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
var moveUntil = 2000;
var stopAfter = moveUntil + 100;

var pitch = 90.0;
var yaw = 0.0;
var roll = 180.0;
var rotation = Quat.fromPitchYawRollDegrees(pitch, yaw, roll)

var originalProperties = {
    position: { x: 10,
                y: 0,
                z: 0 },

    radius : 0.1,

    color: { red: 0,
             green: 255,
             blue: 0 },

    modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Feisar_Ship.FBX",
    //modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/birarda/birarda_head.fbx",
    //modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/pug.fbx",
    //modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/newInvader16x16-large-purple.svo",
    //modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/minotaur/mino_full.fbx",
    //modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Combat_tank_V01.FBX",
    
    modelRotation: rotation
};

var positionDelta = { x: 0, y: 0, z: 0 };


var modelID = Models.addModel(originalProperties);

function moveModel(deltaTime) {
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

    //print("count =" + count);
    count++;

    //print("modelID.creatorTokenID = " + modelID.creatorTokenID);

    var newProperties = {
        position: {
                x: originalProperties.position.x + (count * positionDelta.x),
                y: originalProperties.position.y + (count * positionDelta.y),
                z: originalProperties.position.z + (count * positionDelta.z)
        },
        radius : 0.25,

    };


    //print("modelID = " + modelID);
    //print("newProperties.position = " + newProperties.position.x + "," + newProperties.position.y+ "," + newProperties.position.z);

    Models.editModel(modelID, newProperties);
}


// register the call back so it fires before each data send
Script.update.connect(moveModel);

