//
//  entityModelExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates creating and editing a entity
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var count = 0;
var stopAfter = 1000;

var modelPropertiesA = {
    type: "Model",
    position: { x: 1, y: 1, z: 1 },
    velocity: { x: 0.5, y: 0, z: 0.5 },
    damping: 0, 
    dimensions: {
                x: 2.16,
                y: 3.34,
                z: 0.54
                },
    modelURL: HIFI_PUBLIC_BUCKET + "meshes/Feisar_Ship.FBX",
    lifetime: 20
};


var ballProperties = {
    type: "Sphere",
    position: { x: 1, y: 0.5, z: 1 },
    velocity: { x: 0.5, y: 0, z: 0.5 },
    damping: 0, 
    dimensions: { x: 0.5, y: 0.5, z: 0.5 },
    color: { red: 255, green: 0, blue: 0 },
    lifetime: 20
};

var modelAEntityID = Entities.addEntity(modelPropertiesA);
var ballEntityID = Entities.addEntity(ballProperties);

function endAfterAWhile(deltaTime) {
    // stop it...
    if (count >= stopAfter) {
        print("calling Script.stop()");
        Script.stop();
    }

    print("count =" + count);
    count++;
}


// register the call back so it fires before each data send
Script.update.connect(endAfterAWhile);

