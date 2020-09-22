//
//  zoneSkyboxExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 4/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example script that demonstrates creating a zone using the atmosphere features
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var VIRCADIA_PUBLIC_CDN = networkingConstants.PUBLIC_BUCKET_CDN_URL;

var count = 0;
var stopAfter = 600;

var zoneEntityA = Entities.addEntity({
    type: "Zone",
    position: { x: 1000, y: 1000, z: 1000}, 
    dimensions: { x: 2000, y: 2000, z: 2000 },
    keyLightColor: { red: 255, green: 0, blue: 0 },
    stageSunModelEnabled: false,
    shapeType: "sphere",
    backgroundMode: "skybox",
    atmosphere: {
        center: { x: 1000, y: 0, z: 1000}, 
        innerRadius: 1000.0,
        outerRadius: 1025.0,
        rayleighScattering: 0.0025,
        mieScattering: 0.0010,
        scatteringWavelengths: { x: 0.650, y: 0.570, z: 0.475 },
        hasStars: false
    },
    skybox: {
        color: { red: 255, green: 0, blue: 255 }, 
        url: ""
    },
    stage: {
        latitude: 37.777,
        longitude: 122.407,
        altitude: 0.03,
        day: 60,
        hour: 0,
        sunModelEnabled: true
    }
});

var props = Entities.getEntityProperties(zoneEntityA);
print(JSON.stringify(props));

// register the call back so it fires before each data send
Script.update.connect(function(deltaTime) {
    // stop it...
    if (count >= stopAfter) {
        print("calling Script.stop()");
        Script.stop();
    }
    count++;
});

