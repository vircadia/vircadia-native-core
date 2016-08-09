//
//  WebBuddy.js
//
//  Created by Anthony J. Thibault on 8/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var NEGATIVE_ONE = 65535;

// ctor
WebBuddy = function (url) {

    var ASPECT = 4.0 / 3.0;
    var WIDTH = 0.4;

    var spawnPoint = Vec3.sum(Vec3.sum(MyAvatar.position, Vec3.multiply(1.0, Quat.getFront(MyAvatar.orientation))),
                              {x: 0, y: 0.5, z: 0});

    var webEntityPosition = spawnPoint;
    var webEntityRotation = MyAvatar.orientation;
    this.webEntityID = Entities.addEntity({
        type: "Web",
        sourceUrl: url,
        dimensions: {x: WIDTH, y: WIDTH * ASPECT, z: 0.1},
        position: webEntityPosition,
        rotation: webEntityRotation,
        name: "web",
        dynamic: true,
        angularDamping: 0.9,
        damping: 0.9,
        gravity: {x: 0, y: 0, z: 0},
        shapeType: "box",
        userData: JSON.stringify({
            "grabbableKey": {"grabbable": true}
        }),
        parentID: MyAvatar.sessionUUID,
        parentJointIndex: NEGATIVE_ONE
    });

    this.state = "idle";

    // compute the room/sensor matrix of the entity.
    var invRoomMat = Mat4.inverse(MyAvatar.sensorToWorldMatrix);
    var entityWorldMat = Mat4.createFromRotAndTrans(webEntityRotation, webEntityPosition);
    this.entityRoomMat = Mat4.multiply(invRoomMat, entityWorldMat);

    var _this = this;
    this.updateFunc = function (dt) {
        _this.update(dt);
    };
    Script.update.connect(this.updateFunc);
};

WebBuddy.prototype.destroy = function () {
    Entities.deleteEntity(this.webEntityID);
    Script.update.disconnect(this.updateFunc);
};

WebBuddy.prototype.update = function (dt) {

    var props = Entities.getEntityProperties(this.webEntityID, ["position", "rotation", "parentID", "parentJointIndex"]);
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
        Entities.editEntity(this.webEntityID, {
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

