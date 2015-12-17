//
//  LightSaber.js
//  examples
//
//  Created by Eric Levin on 12/17/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script creates a lightsaber which activates on grab
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/utils.js");
var modelURL = "https://s3.amazonaws.com/hifi-public/eric/models/rave/lightSaber.fbx";
var scriptURL = Script.resolvePath("lightSaberEntityScript.js");
LightSaber = function(spawnPosition) {

    var saberHandle = Entities.addEntity({
        type: "Model",
        name: "LightSaber Handle",
        modelURL: modelURL,
        position: spawnPosition,
        shapeType: 'box',
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
                    relativeRotation: Quat.fromPitchYawRollDegrees(90, 90, 0)
                },
                invertSolidWhileHeld: true
            }
        })
    });


    function cleanup() {
        Entities.deleteEntity(saberHandle);
    }

    this.cleanup = cleanup;
}