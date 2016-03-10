//
//  Created by Bradley Austin Davis on 2015/07/01
//  Copyright 2015 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var SIDE_SIZE = 10; 

var center = { x: 0, y: 0, z: 0 };

var DEGREES_TO_RADIANS = Math.PI / 180.0;
var PARTICLE_MIN_SIZE = 2.50; 
var PARTICLE_MAX_SIZE = 2.50;
var LIFETIME = 600;
var boxes = [];

var ids = Entities.findEntities(MyAvatar.position, 50);
for (var i = 0; i < ids.length; i++) {
    var id = ids[i];
    var properties = Entities.getEntityProperties(id);
    if (properties.name == "PerfTest") {
        Entities.deleteEntity(id);
    }
}


//  Create initial test particles that will move according to gravity from the planets
for (var x = 0; x < SIDE_SIZE; x++) {
    for (var y = 0; y < SIDE_SIZE; y++) {
        for (var z = 0; z < SIDE_SIZE; z++) {
            var gray = Math.random() * 155;
            var cube = Math.random() > 0.5;
            var color = { red: 100 + gray, green: 100 + gray, blue: 100 + gray };
            var position = Vec3.sum(MyAvatar.position, { x: x * 0.2, y: y * 0.2, z: z * 0.2});
            var radius = Math.random() * 0.1;
            boxes.push(Entities.addEntity({ 
                type: cube ? "Box" : "Sphere",
                name: "PerfTest",
                position: position,  
                dimensions: { x: radius, y: radius, z: radius }, 
                color: color,
                ignoreCollisions: true,
                dynamic: false, 
                lifetime: LIFETIME
            }));
        }
    }
}


function scriptEnding() {
    for (var i = 0; i < boxes.length; i++) {
        Entities.deleteEntity(boxes[i]);
    }
}
Script.scriptEnding.connect(scriptEnding);
