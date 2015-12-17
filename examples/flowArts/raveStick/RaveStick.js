//
//  RaveStick.js
//  examples/flowArats/raveStick
//
//  Created by Eric Levin on 12/17/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script creates a rave stick which makes pretty light trails as you paint
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/utils.js");
var modelURL = "https://s3.amazonaws.com/hifi-public/eric/models/rave/raveStick.fbx";
var scriptURL = Script.resolvePath("raveStickEntityScript.js");
RaveStick = function(spawnPosition) {
    var colorPalette = [{
        red: 0,
        green: 200,
        blue: 40
    }, {
        red: 200,
        green: 10,
        blue: 40
    }];
    var stick = Entities.addEntity({
        type: "Model",
        name: "raveStick",
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

    var light = Entities.addEntity({
        type: 'Light',
        name: "raveLight",
        parentID: stick,
        dimensions: {
            x: 30,
            y: 30,
            z: 30
        },
        color: colorPalette[randInt(0, colorPalette.length)],
        intensity: 5
    });



    function cleanup() {
        Entities.deleteEntity(stick);
        Entities.deleteEntity(light);
    }

    this.cleanup = cleanup;
}