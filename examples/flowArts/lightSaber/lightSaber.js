//
//  LightSaber.js
//  examples
//
//  Created by Eric Levin on 12/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates a lightsaber which activates on grab
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/utils.js");
var modelURL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/lightSaber.fbx";
var scriptURL = Script.resolvePath("lightSaberEntityScript.js");
LightSaber = function(spawnPosition) {

    var saberHandle = Entities.addEntity({
        type: "Model",
        name: "LightSaber Handle",
        modelURL: modelURL,
        position: spawnPosition,
        shapeType: 'box',
        dynamic: true,
        script: scriptURL,
        dimensions: {
            x: 0.06,
            y: 0.06,
            z: 0.31
        },
        userData: JSON.stringify({
            grabbableKey: {
                spatialKey: {
                    relativePosition: {
                        x: 0,
                        y: 0,
                        z: -0.1
                    },
                    relativeRotation: Quat.fromPitchYawRollDegrees(180, 90, 0)
                },
                invertSolidWhileHeld: true
            }
        })
    });

    var light = Entities.addEntity({
        type: 'Light',
        name: "raveLight",
        parentID: saberHandle,
        dimensions: {
            x: 30,
            y: 30,
            z: 30
        },
        color: {red: 200, green: 10, blue: 200},
        intensity: 5
    });


    function cleanup() {
        Entities.deleteEntity(saberHandle);
    }

    this.cleanup = cleanup;
}
