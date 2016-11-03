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

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var count = 0;
var moveUntil = 2000;
var stopAfter = moveUntil + 100;

var pitch = 90.0;
var yaw = 0.0;
var roll = 180.0;
var rotation = Quat.fromPitchYawRollDegrees(pitch, yaw, roll)

var originalProperties = {
type: "Model",
    position: { x: 2.0,
                y: 2.0,
                z: 0.5 },

 dimensions: {
                x: 2.16,
                y: 3.34,
                z: 0.54
            },    
  color: { red: 0,
             green: 255,
             blue: 0 },

    modelURL: HIFI_PUBLIC_BUCKET + "meshes/Feisar_Ship.FBX",

    
    rotation: rotation
};

var positionDelta = { x: 0.002, y: 0.002, z: 0.0 };


var entityID = Entities.addEntity(originalProperties);

function moveEntity(deltaTime) {
    if (count >= moveUntil) {

        // delete it...
        if (count == moveUntil) {
            print("calling Entities.deleteEntity()");
            Entities.deleteEntity(entityID);
        }

        // stop it...
        if (count >= stopAfter) {
            print("calling Script.stop()");
            Script.stop();
        }

        count++;
        return; // break early
    }

    print("count =" + count);
    count++;

    var newProperties = {
        position: {
                x: originalProperties.position.x + (count * positionDelta.x),
                y: originalProperties.position.y + (count * positionDelta.y),
                z: originalProperties.position.z + (count * positionDelta.z)
        },
    };


    //print("entityID = " + entityID);
    //print("newProperties.position = " + newProperties.position.x + "," + newProperties.position.y+ "," + newProperties.position.z);

    Entities.editEntity(entityID, newProperties);
}


// register the call back so it fires before each data send
Script.update.connect(moveEntity);

