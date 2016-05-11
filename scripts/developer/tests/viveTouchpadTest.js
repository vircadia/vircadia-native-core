//
//  viveTouchpadTest.js
//
//  Anthony J. Thibault
//  Copyright 2016 High Fidelity, Inc.
//
//  An example of reading touch and move events from the vive controller touch pad.
//
//  It will spawn a gray cube in front of you, then as you use the right touch pad,
//  the cube should turn green and respond to the motion of your thumb on the pad.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var GRAY = {red: 57, green: 57, blue: 57};
var GREEN = {red: 0, green: 255, blue: 0};
var ZERO = {x: 0, y: 0, z: 0};
var Y_AXIS = {x: 0, y: 1, x: 0};
var ROT_Y_90 = Quat.angleAxis(Y_AXIS, 90.0);

var boxEntity;
var boxPosition;
var boxZAxis, boxYAxis;
var prevThumbDown = false;

function init() {
    boxPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
    var front = Quat.getFront(Camera.getOrientation());
    boxZAxis = Vec3.normalize(Vec3.cross(front, Y_AXIS));
    boxYAxis = Vec3.normalize(Vec3.cross(boxZAxis, front));

    boxEntity = Entities.addEntity({
        type: "Box",
        position: boxPosition,
        dimentions: {x: 0.25, y: 0.25, z: 0.25},
        color: GRAY,
        gravity: ZERO,
        visible: true,
        locked: false,
        lifetime: 60000
    });
}

function shutdown() {
    Entities.deleteEntity(boxEntity);
}
Script.scriptEnding.connect(shutdown);

function viveIsConnected() {
    return Controller.Hardware.Vive;
}

function update(dt) {
    if (viveIsConnected()) {
        var thumbDown = Controller.getValue(Controller.Hardware.Vive.RSTouch);
        if (thumbDown) {
            var x = Controller.getValue(Controller.Hardware.Vive.RX);
            var y = Controller.getValue(Controller.Hardware.Vive.RY);
            var xOffset = Vec3.multiply(boxZAxis, x);
            var yOffset = Vec3.multiply(boxYAxis, y);
            var offset = Vec3.sum(xOffset, yOffset);
            Entities.editEntity(boxEntity, {position: Vec3.sum(boxPosition, offset)});
        }
        if (thumbDown && !prevThumbDown) {
            Entities.editEntity(boxEntity, {color: GREEN});
        }
        if (!thumbDown && prevThumbDown) {
            Entities.editEntity(boxEntity, {color: GRAY});
        }
        prevThumbDown = thumbDown;
    }
}

Script.update.connect(update);

init();



