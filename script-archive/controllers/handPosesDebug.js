//
//  handPosesDebug.js
//  examples
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



function makeSphere(color) {
    var SPHERE_SIZE = 0.05;
    var sphere = Overlays.addOverlay("sphere", {
        position: { x: 0, y: 0, z: 0 },
        size: SPHERE_SIZE,
        color: color,
        alpha: 1.0,
        solid: true,
        visible: true,
    });

    return sphere;
}


var NUM_HANDS = 2;
var NUM_SPHERES_PER_HAND = 2;
var LEFT_HAND = 0;
var RIGHT_HAND = 1;

var COLORS = [ { red: 255, green: 0, blue: 0 }, { red: 0, green: 0, blue: 255 } ];


function index(handNum, indexNum) {
    return handNum * NUM_HANDS + indexNum;
}

var app = {};


function setup() {
    app.spheres = new Array();

    for (var h = 0; h < NUM_HANDS; h++) {
        for (var s = 0; s < NUM_SPHERES_PER_HAND; s++) {
            var i = index(h, s);
            app.spheres[i] = makeSphere(COLORS[h]);
            print("Added Sphere num " + i + " = " + JSON.stringify(app.spheres[i]));
        }
    }
}

function updateHand(handNum, deltaTime) {
    var pose;
    var handName = "right";
    if (handNum == LEFT_HAND) {
        pose = MyAvatar.getLeftHandPose();
        handName = "left";
    } else {
        pose = MyAvatar.getRightHandPose();
        handName = "right";
    }

    if (pose.valid) {
        //print(handName + " hand moving" +  JSON.stringify(pose));
        Overlays.editOverlay(app.spheres[index(handNum, 0)], {
            position: pose.translation,
            visible: true,
        });
        var vpos = Vec3.sum(Vec3.multiply(10 * deltaTime, pose.velocity), pose.translation);
        Overlays.editOverlay(app.spheres[index(handNum, 1)], {
            position: vpos,
            visible: true,
        });
    } else {
        Overlays.editOverlay(app.spheres[index(handNum, 0)], {
            visible: false
        });

        Overlays.editOverlay(app.spheres[index(handNum, 1)], {
            visible: false
        });
    }
}

function update(deltaTime) {
    updateHand(LEFT_HAND, deltaTime);
    updateHand(RIGHT_HAND, deltaTime);
}

function scriptEnding() {
    print("Removing spheres = " + JSON.stringify(app.spheres));
    for (var i = 0; i < app.spheres.length; i++) {
        Overlays.deleteOverlay(app.spheres[i]);
    }
}

setup();
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
