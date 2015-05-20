//
//  fullDomainZoneEntityExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 4/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example script that demonstrates creating a Zone which is as large as the entire domain
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Entities.addEntity({
    type: "Zone",
    position: { x: -10000, y: -10000, z: -10000 },
    dimensions: { x: 30000, y: 30000, z: 30000 },
    keyLightColor: { red: 255, green: 255, blue: 0 },
    keyLightDirection: { x: 0, y: -1.0, z: 0 },
    keyLightIntensity: 1.0,
    keyLightAmbientIntensity: 0.5
});

