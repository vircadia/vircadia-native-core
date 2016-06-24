//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var spriteURL = "https://hifi-production.s3.amazonaws.com/DomainContent/CellScience/Sprites/nucleosomes_sprite.fbx";
var spriteDimensions = {
    x: 10,
    y: 10,
    z: 10
};
var sprite;
var isMouseDown = false;
var RAD_TO_DEG = 180.0 / Math.PI;
var Y_AXIS = {
    x: 0,
    y: 1,
    z: 0
};
var X_AXIS = {
    x: 1,
    y: 0,
    z: 0
};

function MakeSprite() {
    sprite = Entities.addEntity({
        type: "Model",
        name: "sprite",
        modelURL: spriteURL,
        dimensions: spriteDimensions,
        position: Vec3.sum(MyAvatar.position, Quat.getFront(MyAvatar.orientation)),
        rotation: Quat.inverse(Quat.getFront(MyAvatar.orientation))
    });
}

function UpdateOrientation(event) {
    if (isMouseDown && event.isRightButton) {

        var direction,
            yaw,
            pitch,
            rot;

        direction = Vec3.normalize(Vec3.subtract(MyAvatar.position, Entities.getEntityProperties(sprite).position));
        yaw = Quat.angleAxis(Math.atan2(direction.x, direction.z) * RAD_TO_DEG, Y_AXIS);
        pitch = Quat.angleAxis(Math.asin(-direction.y) * RAD_TO_DEG, X_AXIS);
        rot = Quat.multiply(yaw, pitch);

        var avatar = Quat.safeEulerAngles(MyAvatar.orientation);
        var printRot = Quat.safeEulerAngles(rot);
        print("avatar = (" + avatar.x + ", " + avatar.y + ", " + avatar.z + ")");
        print("sprite = (" + printRot.x + ", " + printRot.y + ", " + printRot.z + ")");
        Entities.editEntity(sprite, {
            rotation: rot
        });
    }
}

function OnMouseDown(event) {
    isMouseDown = true;
}

function OnMouseUp(event) {
    isMouseDown = false;
}

MakeSprite();
Controller.mouseMoveEvent.connect(UpdateOrientation);
Controller.mousePressEvent.connect(OnMouseDown);
Controller.mouseReleaseEvent.connect(OnMouseUp);