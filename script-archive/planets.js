// 
//  planets.js 
//  
//  Created by Philip Rosedale on January 26, 2015
//  Copyright 2015 High Fidelity, Inc.
//  
//  Some planets are created in front of you.  A physical object shot or thrown between them will move 
//  correctly.  
// 
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var MAX_RANGE = 75.0; 
var MAX_TRANSLATION = MAX_RANGE / 20.0;

var LIFETIME = 600;    
var DAMPING = 0.0;
var G = 3.0;

//  In this section, setup where you want your 'Planets' that will exert gravity on the 
//  smaller test particles.  Use the first one for the simplest 'planets around sun' simulation.  
//  Add additional planets to make things a lot more complicated! 

var planetTypes = [];
planetTypes.push({ radius: 15, red: 0, green: 0, blue: 255, x: 0.0, y:0, z: 0.0 });
//planetTypes.push({ radius: 10, red: 0, green: 255, blue: 0, x: 0.60, y:0, z: 0.60 });
//planetTypes.push({ radius: 10, red: 0, green: 0, blue: 255, x: 0.75, y:0, z: 0.75 });
//planetTypes.push({ radius: 5, red: 255, green: 0, blue: 0, x: 0.25, y:0, z: 0.25 });
//planetTypes.push({ radius: 5, red: 0, green: 255, blue: 255, x: 0, y:0, z: 0 });

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(MAX_RANGE, Quat.getFront(Camera.getOrientation())));

var NUM_INITIAL_PARTICLES = 200; 
var PARTICLE_MIN_SIZE = 0.50; 
var PARTICLE_MAX_SIZE = 1.50;
var INITIAL_VELOCITY = 5.0;
var DEGREES_TO_RADIANS = Math.PI / 180.0;

var planets = [];
var particles = [];

//  Create planets that will extert gravity on test particles 
for (var i = 0; i < planetTypes.length; i++) {
    // NOTE: rotationalVelocity is in radians/sec
    var rotationalVelocity = (10 + Math.random() * 60) * DEGREES_TO_RADIANS;
    var position = { x: planetTypes[i].x, y: planetTypes[i].y, z: planetTypes[i].z };
    position = Vec3.multiply(MAX_RANGE / 2, position);
    position = Vec3.sum(center, position);
    
    planets.push(Entities.addEntity(
            { type: "Sphere",
            position: position,  
            dimensions: { x: planetTypes[i].radius, y: planetTypes[i].radius, z: planetTypes[i].radius }, 
              color: { red: planetTypes[i].red, green: planetTypes[i].green, blue: planetTypes[i].blue },
              gravity: {  x: 0, y: 0, z: 0 },
              angularVelocity: {  x: 0, y: rotationalVelocity, z: 0 },
              angularDamping: 0.0,
            ignoreCollisions: false,
            lifetime: LIFETIME,
            dynamic: false })); 
}

Script.setTimeout(createParticles, 1000); 

function createParticles() {
    //  Create initial test particles that will move according to gravity from the planets
    for (var i = 0; i < NUM_INITIAL_PARTICLES; i++) {
        var radius = PARTICLE_MIN_SIZE + Math.random() * PARTICLE_MAX_SIZE;
        var gray = Math.random() * 155;
        var whichPlanet = Math.floor(Math.random() * planets.length); 
        var position = { x: (Math.random() - 0.5) * MAX_RANGE, y: (Math.random() - 0.5) * MAX_TRANSLATION, z: (Math.random() - 0.5) * MAX_RANGE };
        var separation = Vec3.length(position);
        particles.push(Entities.addEntity(
                { type: "Sphere",
                position: Vec3.sum(center, position),  
                dimensions: { x: radius, y: radius, z: radius }, 
                  color: { red: 100 + gray, green: 100 + gray, blue: 100 + gray },
                  gravity: {  x: 0, y: 0, z: 0 },
                  angularVelocity: {  x: 0, y: 0, z: 0 },
                  velocity: Vec3.multiply(INITIAL_VELOCITY * Math.sqrt(separation), Vec3.normalize(Vec3.cross(position, { x: 0, y: 1, z: 0 }))),
                ignoreCollisions: false,
                damping: DAMPING,
                lifetime: LIFETIME,
                dynamic: true })); 
    }
    Script.update.connect(update);
}

function scriptEnding() {
    for (var i = 0; i < planetTypes.length; i++) {
        Entities.deleteEntity(planets[i]);
    }
    for (var i = 0; i < particles.length; i++) {
        Entities.deleteEntity(particles[i]);
    }
}

function update(deltaTime) {
    //  Apply gravitational force from planets  
    for (var t = 0; t < particles.length; t++) {
        var properties1 = Entities.getEntityProperties(particles[t]);
        var velocity = properties1.velocity;
        var vColor = Vec3.length(velocity) / 50 * 255;
        var dV = { x:0, y:0, z:0 };
        var mass1 = Math.pow(properties1.dimensions.x / 2.0, 3.0);
        for (var p = 0; p < planets.length; p++) {
            var properties2 = Entities.getEntityProperties(planets[p]);
            var mass2 = Math.pow(properties2.dimensions.x / 2.0, 3.0);
            var between = Vec3.subtract(properties1.position, properties2.position);
            var separation = Vec3.length(between);
            dV = Vec3.sum(dV, Vec3.multiply(-G * mass1 * mass2 / separation, Vec3.normalize(between))); 
        }
        if (Math.random() < 0.1) {
            Entities.editEntity(particles[t], { color: { red: vColor, green: 100, blue: (255 - vColor) }, velocity: Vec3.sum(velocity, Vec3.multiply(deltaTime, dV))}); 
        } else {
            Entities.editEntity(particles[t], { velocity: Vec3.sum(velocity, Vec3.multiply(deltaTime, dV))});
        }
        
    }
}

Script.scriptEnding.connect(scriptEnding);
