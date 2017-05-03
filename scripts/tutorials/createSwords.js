//  createSwords.js
//
//  Created by Philip Rosedale on April 9, 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Makes two grabbable 'swords' in front of the user, that can be held and used 
//  to hit the other.  Demonstration of an action that would allow two people to hold 
//  entities that are also colliding. 
//  

var COLOR = { red: 255, green: 0, blue: 0 };
var SIZE = { x: 0.10, y: 1.5, z: 0.10 };

var SCRIPT_URL = Script.resolvePath("entity_scripts/springHold.js");

function inFrontOfMe(distance) {
    return Vec3.sum(Camera.getPosition(), Vec3.multiply(distance, Quat.getForward(Camera.getOrientation())));
}

var sword1 = Entities.addEntity({
        type: "Box",
        name: "Sword1",
        position: inFrontOfMe(2 * SIZE.y),
        dimensions: SIZE,
        color: COLOR,
        angularDamping: 0,
        damping: 0,
        script: SCRIPT_URL,
        userData:JSON.stringify({
                grabbableKey:{
                    grabbable:true
                }
            })
    });

var sword2 = Entities.addEntity({
        type: "Box",
        name: "Sword2",
        position: inFrontOfMe(3 * SIZE.y),
        dimensions: SIZE,
        color: COLOR,
        angularDamping: 0,
        damping: 0,
        script: SCRIPT_URL,
        userData:JSON.stringify({
                grabbableKey:{
                    grabbable:true
                }
            })
    });

Script.scriptEnding.connect(function scriptEnding() {
    Entities.deleteEntity(sword1);
    Entities.deleteEntity(sword2);
});

