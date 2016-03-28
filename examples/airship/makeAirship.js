//  makeAirship.js
//  
// //  Created by Philip Rosedale on March 7, 2016
//  Copyright 2016 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var SIZE = 0.2; 
var TYPE = "Model";            //   Right now this can be "Box" or "Model" or "Sphere"

var MODEL_URL = "https://s3.amazonaws.com/hifi-public/philip/airship_compact.fbx"

var MODEL_DIMENSION = { x: 19.257, y: 24.094, z: 40.3122 };
var ENTITY_URL = "https://s3.amazonaws.com/hifi-public/scripts/airship/airship.js";


var LIFETIME = 3600 * 48;            

var GRAVITY = { x: 0, y: 0, z: 0 };
var DAMPING = 0.05;
var ANGULAR_DAMPING = 0.01;

var collidable = true; 
var gravity = true; 

var HOW_FAR_IN_FRONT_OF_ME = 30;
var HOW_FAR_ABOVE_ME = 15;

var leaveBehind = true;

var shipLocation = Vec3.sum(MyAvatar.position, Vec3.multiply(HOW_FAR_IN_FRONT_OF_ME, Quat.getFront(Camera.orientation)));
shipLocation.y += HOW_FAR_ABOVE_ME;


var airship = Entities.addEntity({ 
            type: TYPE,
            modelURL: MODEL_URL,
            name: "airship",
            position: shipLocation,
            dimensions: (TYPE == "Model") ? MODEL_DIMENSION : { x: SIZE, y: SIZE, z: SIZE },       
            damping: DAMPING,
            angularDamping: ANGULAR_DAMPING,
            gravity: (gravity ? GRAVITY : { x: 0, y: 0, z: 0}),
            dynamic: collidable,
            lifetime: LIFETIME, 
            animation: {url: MODEL_URL, running: true, currentFrame: 0, loop: true},
            script: ENTITY_URL
        });

function scriptEnding() {
    if (!leaveBehind) {
        Entities.deleteEntity(airship);
    }
}

Script.scriptEnding.connect(scriptEnding);