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
//var ENTITY_URL = "file:///c:/users/dev/philip/examples/fireflies/firefly.js?"+Math.random()
var ENTITY_URL = "https://s3.amazonaws.com/hifi-public/scripts/fireflies/firefly.js"

var RATE_PER_SECOND = 50;    //    The entity server will drop data if we create things too fast.
var SCRIPT_INTERVAL = 100;
var LIFETIME = 120;            

var NUMBER_TO_CREATE = 100;

var GRAVITY = { x: 0, y: -1.0, z: 0 };

var DAMPING = 0.5;
var ANGULAR_DAMPING = 0.5;

var collidable = true; 
var gravity = true; 

var RANGE = 10;
var HEIGHT = 3;
var HOW_FAR_IN_FRONT_OF_ME = 1.0;
 
 var totalCreated = 0;

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
            type: "Box",
            name: "firefly",
            position: position,
            dimensions: { x: SIZE, y: SIZE, z: SIZE },       
            color: { red: 150 + Math.random() * 100, green: 100 + Math.random() * 50, blue: 0 },
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

