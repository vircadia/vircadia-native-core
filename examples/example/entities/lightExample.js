//
//  lightExample.js
//  examples
//
//  Created by Philip Rosedale on November 5, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Makes a light right in front of your avatar, as well as a sphere at that location. 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var position = Vec3.sum(MyAvatar.position, Quat.getFront(Camera.getOrientation()));

var sphereID = Entities.addEntity({
            type: "Sphere",
            position: position,
            dimensions: { x: 0.1, y: 0.1, z: 0.1 },
            color: { red: 255, green: 255, blue: 0 }
            });

var lightID = Entities.addEntity({
    type: "Light",  
    position: position,
    dimensions: { x: 1, y: 1, z: 1 },
    angularVelocity: { x: 0, y: 0, z: 0 },
    angularDamping: 0,

    isSpotlight: true,
    diffuseColor: { red: 255, green: 255, blue: 0 },
    ambientColor: { red: 0, green: 0, blue: 0 },
    specularColor: { red: 255, green: 255, blue: 255 },

    constantAttenuation: 0,
    linearAttenuation: 1,
    quadraticAttenuation: 0,
    exponent: 0,
    cutoff: 180, // in degrees
});

Script.scriptEnding.connect(function() {
    print("Deleted sphere and light");
    Entities.deleteEntity(sphereID);
    Entities.deleteEntity(lightID);
});
