//
//  securityCamera.js
//  examples/example
//
//  Created by Thijs Wenker on November 4, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

const CAMERA_OFFSET = {x: 0, y: 4, z: -14};
const LOOKAT_START_OFFSET = {x: -10, y: 0, z: 14};
const LOOKAT_END_OFFSET = {x: 10, y: 0, z: 14};
const TINY_VALUE = 0.001;

var lookatTargets = [Vec3.sum(MyAvatar.position, LOOKAT_START_OFFSET), Vec3.sum(MyAvatar.position, LOOKAT_END_OFFSET)];
var currentTarget = 0;

var forward = true;

var oldCameraMode = Camera.mode;

var cameraLookAt = function(cameraPos, lookAtPos) {
    var lookAtRaw = Quat.lookAt(cameraPos, lookAtPos, Vec3.UP);
    lookAtRaw.w = -lookAtRaw.w;
    return lookAtRaw;
};

cameraEntity = Entities.addEntity({
    type: "Box",
    visible: false,
    position: Vec3.sum(MyAvatar.position, CAMERA_OFFSET)
});

Camera.mode = "entity";
Camera.cameraEntity = cameraEntity;

Script.update.connect(function(deltaTime) {
    var cameraProperties = Entities.getEntityProperties(cameraEntity, ["position", "rotation"]);
    var targetOrientation = cameraLookAt(cameraProperties.position, lookatTargets[currentTarget]);
    if (Math.abs(targetOrientation.x - cameraProperties.rotation.x) < TINY_VALUE && 
        Math.abs(targetOrientation.y - cameraProperties.rotation.y) < TINY_VALUE && 
        Math.abs(targetOrientation.z - cameraProperties.rotation.z) < TINY_VALUE && 
        Math.abs(targetOrientation.w - cameraProperties.rotation.w) < TINY_VALUE) {

        currentTarget = (currentTarget + 1) % lookatTargets.length;
        return;
    }
    Entities.editEntity(cameraEntity, {rotation: Quat.mix(cameraProperties.rotation, targetOrientation, deltaTime / 3)});
});

Script.scriptEnding.connect(function() {
    Entities.deleteEntity(cameraEntity);
    Camera.mode = oldCameraMode;
    Camera.cameraEntity = null;
});
