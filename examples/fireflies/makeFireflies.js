//
//  Created by Philip Rosedale on March 7, 2016
//  Copyright 2016 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Make some fireflies 
// 

var SIZE = 0.05; 
var TYPE = "Box";            //   Right now this can be "Box" or "Model" or "Sphere"
var MODEL_URL = "http://s3.amazonaws.com/hifi-public/models/content/basketball2.fbx";
var MODEL_DIMENSION = { x: 0.3, y: 0.3, z: 0.3 };
//var ENTITY_URL = "file:///c:/users/dev/philip/examples/fireflies/firefly.js?"+Math.random()
var ENTITY_URL = "https://s3.amazonaws.com/hifi-public/philip/firefly.js"

var RATE_PER_SECOND = 50;    //    The entity server will drop data if we create things too fast.
var SCRIPT_INTERVAL = 100;
var LIFETIME = 3600;            

var NUMBER_TO_CREATE = 200;

var GRAVITY = { x: 0, y: -1.0, z: 0 };
var VELOCITY = { x: 0.0, y: 0, z: 0 };
var ANGULAR_VELOCITY = { x: 1, y: 1, z: 1 };

var DAMPING = 0.5;
var ANGULAR_DAMPING = 0.5;

var collidable = true; 
var gravity = true; 


var x = 0;
var z = 0;
var totalCreated = 0;

var RANGE = 50;
var HEIGHT = 3;
var HOW_FAR_IN_FRONT_OF_ME = 1.0;
 

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(HOW_FAR_IN_FRONT_OF_ME, Quat.getFront(Camera.orientation)));


function randomVector(range) {
    return {
        x: (Math.random() -  0.5) * range.x,
        y: (Math.random() -  0.5) * range.y,
        z: (Math.random() -  0.5) * range.z
    }
}

Vec3.print("Center: ", center);

Script.setInterval(function () {
    if (!Entities.serversExist() || !Entities.canRez() || (totalCreated > NUMBER_TO_CREATE)) {
        return;
    } 
    
    var numToCreate = RATE_PER_SECOND * (SCRIPT_INTERVAL / 1000.0);
    for (var i = 0; (i < numToCreate) && (totalCreated < NUMBER_TO_CREATE); i++) {
        
        var position = Vec3.sum(center, randomVector({ x: RANGE, y: HEIGHT, z: RANGE }));
        position.y += HEIGHT / 2.0;

        Entities.addEntity({ 
            type: TYPE,
            modelURL: MODEL_URL,
            name: "firefly",
            position: position,
            dimensions: (TYPE == "Model") ? MODEL_DIMENSION : { x: SIZE, y: SIZE, z: SIZE },       
            color: { red: 150 + Math.random() * 100, green: 100 + Math.random() * 50, blue: 0 },
            velocity: VELOCITY,
            angularVelocity: Vec3.multiply(Math.random(), ANGULAR_VELOCITY),
            damping: DAMPING,
            angularDamping: ANGULAR_DAMPING,
            gravity: (gravity ? GRAVITY : { x: 0, y: 0, z: 0}),
            dynamic: collidable,
            script: ENTITY_URL,
            lifetime: LIFETIME
        });

        totalCreated++;
        print("Firefly #" + totalCreated);
    } 
}, SCRIPT_INTERVAL);

