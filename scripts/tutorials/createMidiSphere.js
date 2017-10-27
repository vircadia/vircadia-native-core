//
//  Created by James B. Pollack @imgntn on April 18, 2016.
//  Adapted by Burt
//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//var SCRIPT_URL =  "file:///e:/hifi/scripts/tutorials/entity_scripts/midiSphere.js";
var SCRIPT_URL =  "http://hifi-files.s3-website-us-west-2.amazonaws.com/midiSphere.js";
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(0.5, Quat.getForward(Camera.getOrientation())));

var BALL_GRAVITY = {
    x: 0,
    y: 0,
    z: 0
};
    
var BALL_DIMENSIONS = {
    x: 0.4,
    y: 0.4,
    z: 0.4
};


var BALL_COLOR = {
    red: 255,
    green: 0,
    blue: 0
};

var midiSphereProperties = {
    name: 'MIDI Sphere',
    shapeType: 'sphere',
    type: 'Sphere',
    script: SCRIPT_URL,
    color: BALL_COLOR,
    dimensions: BALL_DIMENSIONS,
    gravity: BALL_GRAVITY,
    dynamic: false,
    position: center,
    collisionless: false,
    ignoreForCollisions: true
};

var midiSphere = Entities.addEntity(midiSphereProperties);

Script.stop();
