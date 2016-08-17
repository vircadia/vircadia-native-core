//
//  WebTablet.js
//
//  Created by Anthony J. Thibault on 8/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var NEGATIVE_ONE = 65535;

var RAD_TO_DEG = 180 / Math.PI;
var X_AXIS = {x: 1, y: 0, z: 0};
var Y_AXIS = {x: 0, y: 1, z: 0};

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
WebTablet = function (url) {

    var ASPECT = 4.0 / 3.0;
    var WIDTH = 0.4;
    var HEIGHT = WIDTH * ASPECT;
    var DEPTH = 0.025;

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
        parentJointIndex: NEGATIVE_ONE
    });

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
        dpi: 45,
        parentID: this.tabletEntityID,
        parentJointIndex: NEGATIVE_ONE
    });

    this.state = "idle";

    // compute the room/sensor matrix of the entity.
    var invRoomMat = Mat4.inverse(MyAvatar.sensorToWorldMatrix);
    var entityWorldMat = Mat4.createFromRotAndTrans(tabletEntityRotation, tabletEntityPosition);
    this.entityRoomMat = Mat4.multiply(invRoomMat, entityWorldMat);

    var _this = this;
    this.updateFunc = function (dt) {
        _this.update(dt);
    };
    Script.update.connect(this.updateFunc);
};

WebTablet.prototype.destroy = function () {
    Entities.deleteEntity(this.webEntityID);
    Entities.deleteEntity(this.tabletEntityID);
    Script.update.disconnect(this.updateFunc);
};

WebTablet.prototype.update = function (dt) {

    var props = Entities.getEntityProperties(this.tabletEntityID, ["position", "rotation", "parentID", "parentJointIndex"]);
    var entityWorldMat;

    if (this.state === "idle") {

        if (props.parentID !== MyAvatar.sessionUUID || props.parentJointIndex !== NEGATIVE_ONE) {
            this.state = "held";
            return;
        }

        // convert the sensor/room matrix of the entity into world space, using the current sensorToWorldMatrix
        var roomMat = MyAvatar.sensorToWorldMatrix;
        entityWorldMat = Mat4.multiply(roomMat, this.entityRoomMat);

        // slam the world space position and orientation
        Entities.editEntity(this.tabletEntityID, {
            position: Mat4.extractTranslation(entityWorldMat),
            rotation: Mat4.extractRotation(entityWorldMat)
        });
    } else if (this.state === "held") {
        if (props.parentID === MyAvatar.sessionUUID && props.parentJointIndex === NEGATIVE_ONE) {

            // re-compute the room/sensor matrix for the avatar now that it has been released.
            var invRoomMat = Mat4.inverse(MyAvatar.sensorToWorldMatrix);
            entityWorldMat = Mat4.createFromRotAndTrans(props.rotation, props.position);
            this.entityRoomMat = Mat4.multiply(invRoomMat, entityWorldMat);

            this.state = "idle";
            return;
        }
    }
};

