//
//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var SCRIPT_URL = "https://cdn-1.vircadia.com/us-e-1/Developer/Tutorials/entity_scripts/soundMaker.js";
var MODEL_URL = "https://cdn-1.vircadia.com/us-e-1/Developer/Tutorials/soundMaker/Front-Desk-Bell.fbx";

var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(1, Quat.getForward(Camera.getOrientation())));

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