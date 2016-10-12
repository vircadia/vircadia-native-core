//
//  WebTablet.js
//
//  Created by Anthony J. Thibault on 8/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var RAD_TO_DEG = 180 / Math.PI;
var X_AXIS = {x: 1, y: 0, z: 0};
var Y_AXIS = {x: 0, y: 1, z: 0};
var DEFAULT_DPI = 32;
var DEFAULT_WIDTH = 0.5;

var TABLET_URL = "https://s3.amazonaws.com/hifi-public/tony/tablet.fbx";

// returns object with two fields:
//    * position - position in front of the user
//    * rotation - rotation of entity so it faces the user.
function calcSpawnInfo() {
    var front;
    var pitchBackRotation = Quat.angleAxis(20.0, X_AXIS);
    if (HMD.active) {
        front = Quat.getFront(HMD.orientation);
        var yawOnlyRotation = Quat.angleAxis(Math.atan2(front.x, front.z) * RAD_TO_DEG, Y_AXIS);
        return {
            position: Vec3.sum(Vec3.sum(HMD.position, Vec3.multiply(0.6, front)), Vec3.multiply(-0.5, Y_AXIS)),
            rotation: Quat.multiply(yawOnlyRotation, pitchBackRotation)
        };
    } else {
        front = Quat.getFront(MyAvatar.orientation);
        return {
            position: Vec3.sum(Vec3.sum(MyAvatar.position, Vec3.multiply(0.6, front)), {x: 0, y: 0.6, z: 0}),
            rotation: Quat.multiply(MyAvatar.orientation, pitchBackRotation)
        };
    }
}

// ctor
WebTablet = function (url, width, dpi, clientOnly) {

    var ASPECT = 4.0 / 3.0;
    var WIDTH = width || DEFAULT_WIDTH;
    var HEIGHT = WIDTH * ASPECT;
    var DEPTH = 0.025;
    var DPI = dpi || DEFAULT_DPI;

    var spawnInfo = calcSpawnInfo();

    var tabletEntityPosition = spawnInfo.position;
    var tabletEntityRotation = spawnInfo.rotation;
    this.tabletEntityID = Entities.addEntity({
        name: "tablet",
        type: "Model",
        modelURL: TABLET_URL,
        position: tabletEntityPosition,
        rotation: tabletEntityRotation,
        userData: JSON.stringify({
            "grabbableKey": {"grabbable": true}
        }),
        dimensions: {x: WIDTH, y: HEIGHT, z: DEPTH},
        parentID: MyAvatar.sessionUUID,
        parentJointIndex: -2
    }, clientOnly);

    var WEB_ENTITY_REDUCTION_FACTOR = {x: 0.78, y: 0.85};
    var WEB_ENTITY_Z_OFFSET = -0.01;

    var webEntityRotation = Quat.multiply(spawnInfo.rotation, Quat.angleAxis(180, Y_AXIS));
    var webEntityPosition = Vec3.sum(spawnInfo.position, Vec3.multiply(WEB_ENTITY_Z_OFFSET, Quat.getFront(webEntityRotation)));

    this.webEntityID = Entities.addEntity({
        name: "web",
        type: "Web",
        sourceUrl: url,
        dimensions: {x: WIDTH * WEB_ENTITY_REDUCTION_FACTOR.x,
                     y: HEIGHT * WEB_ENTITY_REDUCTION_FACTOR.y,
                     z: 0.1},
        position: webEntityPosition,
        rotation: webEntityRotation,
        shapeType: "box",
        dpi: DPI,
        parentID: this.tabletEntityID,
        parentJointIndex: -1
    }, clientOnly);

    this.state = "idle";
};

WebTablet.prototype.destroy = function () {
    Entities.deleteEntity(this.webEntityID);
    Entities.deleteEntity(this.tabletEntityID);
};

