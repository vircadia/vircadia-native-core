//
//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var SCRIPT_URL = "http://hifi-production.s3.amazonaws.com/tutorials/entity_scripts/soundMaker.js";
var MODEL_URL = "http://hifi-production.s3.amazonaws.com/tutorials/soundMaker/Front-Desk-Bell.fbx";

var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));

function makeBell() {
    var soundMakerProperties = {
        position: center,
        type: 'Model',
        modelURL: MODEL_URL,
        script: SCRIPT_URL,
        gravity: {
            x: 0,
            y: -5.0,
            z: 0
        },
        lifetime: 3600
    }

    var soundMaker = Entities.addEntity(soundMakerProperties);
    Script.stop();
}

makeBell();