//
//  Created by Bradley Austin Davis on 2015/07/01
//  Copyright 2015 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var NUM_MOONS = 20; 
// 1 = 60Hz, 2 = 30Hz, 3 = 20Hz, etc
var UPDATE_FREQUENCY_DIVISOR = 2;
var MAX_RANGE = 75.0; 
var SCALE = 0.1;

var center = Vec3.sum(MyAvatar.position, 
                 Vec3.multiply(MAX_RANGE * SCALE, Quat.getFront(Camera.getOrientation())));

var DEGREES_TO_RADIANS = Math.PI / 180.0;
var PARTICLE_MIN_SIZE = 2.50; 
var PARTICLE_MAX_SIZE = 2.50;


function deleteAnimationTestEntitites() {
    var ids = Entities.findEntities(MyAvatar.position, 50);
    for (var i = 0; i < ids.length; i++) {
        var id = ids[i];
        var properties = Entities.getEntityProperties(id);
        if (properties.name == "AnimationTest") {
            Entities.deleteEntity(id);
        }
    }
}

deleteAnimationTestEntitites();

var moons = [];

//  Create initial test particles that will move according to gravity from the planets
for (var i = 0; i < NUM_MOONS; i++) {
    var radius = PARTICLE_MIN_SIZE + Math.random() * PARTICLE_MAX_SIZE;
    radius *= SCALE;
    var gray = Math.random() * 155;
    var position = { x: 10 , y: i * 3, z: 0 };
    var color = { red: 100 + gray, green: 100 + gray, blue: 100 + gray };
    if (i == 0) {
        color = { red: 255, green: 0, blue: 0 };
        radius = 6 * SCALE
    }
    moons.push(Entities.addEntity({ 
        type: "Sphere",
        name: "AnimationTest",
        position: Vec3.sum(center, position),  
        dimensions: { x: radius, y: radius, z: radius }, 
        color: color,
        ignoreCollisions: true,
        dynamic: false

    })); 
}

Script.update.connect(update);

function scriptEnding() {
    for (var i = 0; i < moons.length; i++) {
        Entities.deleteEntity(moons[i]);
    }
}

var totalTime = 0.0;
var updateCount = 0;
function update(deltaTime) {
    //  Apply gravitational force from planets
    totalTime += deltaTime;
    updateCount++;
    if (0 != updateCount % UPDATE_FREQUENCY_DIVISOR) {
        return;
    }
    
    var particlePos = Entities.getEntityProperties(moons[0]).position;
    var relativePos = Vec3.subtract(particlePos.position, center);
    for (var t = 0; t < moons.length; t++) {
        var thetaDelta = (Math.PI * 2.0 / NUM_MOONS) * t;
        var y = Math.sin(totalTime + thetaDelta) * 10.0 * SCALE;
        var x = Math.cos(totalTime + thetaDelta) * 10.0 * SCALE;
        var newBasePos = Vec3.sum({ x: 0, y: y, z: x }, center);
        Entities.editEntity(moons[t], { position: newBasePos}); 
    }
}

Script.scriptEnding.connect(deleteAnimationTestEntitites);
