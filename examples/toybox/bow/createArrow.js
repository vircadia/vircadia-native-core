//
//  createArrow.js
//
//  Created byJames Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates a bow you can use to shoot an arrow.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var ARROW_MODEL_URL = "https://hifi-public.s3.amazonaws.com/models/bow/new/arrow.fbx";
var ARROW_COLLISION_HULL_URL = "https://hifi-public.s3.amazonaws.com/models/bow/new/arrow_collision_hull.obj";
var ARROW_SCRIPT_URL = Script.resolvePath('arrow.js');

var ARROW_DIMENSIONS = {
    x: 0.02,
    y: 0.02,
    z: 0.64
};

var ARROW_GRAVITY = {
    x: 0,
    y: 0,
    z: 0
};


function cleanup() {
    Entities.deleteEntity(arrow);
}

var arrow;

function createArrow(i) {
    var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 1,
    z: 0
}), Vec3.multiply(1.25*i, Quat.getFront(Camera.getOrientation())));

    arrow = Entities.addEntity({
        name: 'Hifi-Arrow',
        type: 'Model',
        modelURL: ARROW_MODEL_URL,
        shapeType: 'compound',
        compoundShapeURL: ARROW_COLLISION_HULL_URL,
        dimensions: ARROW_DIMENSIONS,
        position: center,
        script: ARROW_SCRIPT_URL,
        collisionsWillMove: true,
        ignoreForCollisions: false,
        gravity: ARROW_GRAVITY,
        // linearDamping:0.1,
        userData: JSON.stringify({
            grabbableKey: {
                invertSolidWhileHeld: true
            }
        })
    });

}

var i;
for(i=1;i<4;i++){
 createArrow(i);

}



Script.scriptEnding.connect(cleanup);