//
//  spotlightExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 10/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates creating and editing a particle
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var DEGREES_TO_RADIANS = Math.PI / 180.0;

var lightProperties = {
    type: "Light",
    position: { x: 0, y: 0, z: 0 },
    dimensions: { x: 1000, y: 1000, z: 1000 },
    angularVelocity: { x: 0, y: 10 * DEGREES_TO_RADIANS, z: 0 },
    angularDamping: 0,

    isSpotlight: true,
    diffuseColor: { red: 255, green: 255, blue: 255 },
    ambientColor: { red: 255, green: 255, blue: 255 },
    specularColor: { red: 255, green: 255, blue: 255 },

    constantAttenuation: 1,
    linearAttenuation: 0,
    quadraticAttenuation: 0,
    exponent: 0,
    cutoff: 180, // in degrees
};

var spotlightID = Entities.addEntity(lightProperties);

Script.scriptEnding.connect(function() {
    Entities.deleteEntity(spotlightID);
});
