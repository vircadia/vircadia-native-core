//
//  zoneEntityExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 4/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example script that demonstrates creating and editing a entity
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var VIRCADIA_PUBLIC_CDN = networkingConstants.PUBLIC_BUCKET_CDN_URL;

var count = 0;
var stopAfter = 1000;

var zoneEntityA = Entities.addEntity({
    type: "Zone",
    position: { x: 5, y: 5, z: 5 },
    dimensions: { x: 10, y: 10, z: 10 },
    keyLightColor: { red: 255, green: 0, blue: 0 },
    stageSunModelEnabled: false,
    keyLightDirection: { x: 0, y: -1.0, z: 0 },
    shapeType: "sphere"
});

print("zoneEntityA:" + zoneEntityA);

var zoneEntityB = Entities.addEntity({
    type: "Zone",
    position: { x: 5, y: 5, z: 5 },
    dimensions: { x: 2, y: 2, z: 2 },
    keyLightColor: { red: 0, green: 255, blue: 0 },
    keyLightIntensity: 0.9,
    stage: {
        latitude: 37.777,
        longitude: 122.407,
        altitude: 0.03,
        day: 60,
        hour: 0,
        sunModelEnabled: true
    }
});

print("zoneEntityB:" + zoneEntityB);

var zoneEntityC = Entities.addEntity({
    type: "Zone",
    position: { x: 5, y: 10, z: 5 },
    dimensions: { x: 10, y: 10, z: 10 },
    keyLightColor: { red: 0, green: 0, blue: 255 },
    keyLightIntensity: 0.75,
    keyLightDirection: { x: 0, y: 0, z: -1 },
    stageSunModelEnabled: false,
    shapeType: "compound",
    compoundShapeURL: "http://headache.hungry.com/~seth/hifi/cube.fbx"
});

print("zoneEntityC:" + zoneEntityC);

// register the call back so it fires before each data send
Script.update.connect(function(deltaTime) {
    // stop it...
    if (count >= stopAfter) {
        print("calling Script.stop()");
        Script.stop();
    }
    count++;
});

