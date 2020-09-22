//
//  nBody.js
//  examples
//
//  Created by Philip Rosedale on March 18, 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Create some spheres that obey gravity, which is great to teach physics.  
//  You can control how many to create by changing the value of 'n' below. 
//  Grab them and watch the others move around.    
//   
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var bodies = [];
var n = 3;  
var radius = 0.1;
var G = 0.25;
var EARTH = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/seth/production/NBody/earth.fbx");
var MOON = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/seth/production/NBody/moon.fbx");

var COLOR1 = { red: 51, green: 51, blue: 255 };
var COLOR2 = { red: 51, green: 51, blue: 255 };
var COLOR3 = { red: 51, green: 51, blue: 255 };

var inFront = Vec3.sum(Camera.getPosition(), Vec3.multiply(radius * 20, Quat.getFront(Camera.getOrientation())));

for (var i = 0; i < n; i++) {
    bodies.push(Entities.addEntity({
        type: "Model",
        modelURL: (i == 0) ? EARTH : MOON,
        shapeType: "sphere",
        dimensions: { x: radius * 2, y: radius * 2, z: radius * 2},
        position: Vec3.sum(inFront, { x: 0, y: i * 2 * radius, z: 0 }),
        gravity: { x: 0, y: 0, z: 0 },
        damping: 0.0,
        angularDamping: 0.0,
        dynamic: true  
    }));
}

Script.update.connect(function(dt) {
    var props = [];
    for (var i = 0; i < n; i++) { 
        props.push(Entities.getEntityProperties(bodies[i]));
    }
    for (var i = 0; i < n; i++) {
        if (props[i].dynamic) {
            var dv = { x: 0, y: 0, z: 0};
            for (var j = 0; j < n; j++) {
                if (i != j) {
                    dv = Vec3.sum(dv, Vec3.multiply(G * dt / Vec3.distance(props[i].position, props[j].position), 
                                    Vec3.normalize(Vec3.subtract(props[j].position, props[i].position))));
                }
            }
            Entities.editEntity(bodies[i], { velocity: Vec3.sum(props[i].velocity, dv)});  
        }
    } 
});

Script.scriptEnding.connect(function scriptEnding() {
    for (var i = 0; i < n; i++) {
        Entities.deleteEntity(bodies[i]);
    }
});