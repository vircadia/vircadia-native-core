//
//  Created by Anthony J. Thibault on 2016/06/21
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// ctor
Xform = function(rot, pos) {
    this.rot = rot;
    this.pos = pos;
}

Xform.ident = function() {
    return new Xform({x: 0, y: 0, z: 0, w: 1}, {x: 0, y: 0, z: 0});
};

Xform.mul = function(lhs, rhs) {
    var rot = Quat.multiply(lhs.rot, rhs.rot);
    var pos = Vec3.sum(lhs.pos, Vec3.multiplyQbyV(lhs.rot, rhs.pos));
    return new Xform(rot, pos);
};

Xform.prototype.inv = function() {
    var invRot = Quat.inverse(this.rot);
    var invPos = Vec3.multiply(-1, this.pos);
    return new Xform(invRot, Vec3.multiplyQbyV(invRot, invPos));
};

Xform.prototype.mirrorX = function() {
    return new Xform({x: this.rot.x, y: -this.rot.y, z: -this.rot.z, w: this.rot.w},
                     {x: -this.pos.x, y: this.pos.y, z: this.pos.z});
};

Xform.prototype.xformVector = function (vector) {
    return Vec3.multiplyQbyV(this.rot, vector);
}

Xform.prototype.xformPoint = function (point) {
    return Vec3.sum(Vec3.multiplyQbyV(this.rot, point), this.pos);
}

Xform.prototype.toString = function() {
    var rot = this.rot;
    var pos = this.pos;
    return "Xform rot = (" + rot.x + ", " + rot.y + ", " + rot.z + ", " + rot.w + "), pos = (" + pos.x + ", " + pos.y + ", " + pos.z + ")";
};
