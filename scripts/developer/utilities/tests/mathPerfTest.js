//
//  Created by Seiji Emery on 9/4/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("perfTest.js");

// Native vec3 (for comparison)
var Native = {};
(function () {
    Native.Vec3 = function (x, y, z) {
        this.x = x || 0.0;
        this.y = y || 0.0;
        this.z = z || 0.0;
    }
    Native.Vec3.prototype.add = function (other) {
        this.x += other.x;
        this.y += other.y;
        this.z += other.z;
    }
    Native.Vec3.prototype.distance = function (other) {
        var rx = this.x - other.x;
        var ry = this.y - other.y;
        var rz = this.z - other.z;
        return Math.sqrt(rx * rx + ry * ry + rz * rz);
    }
    Native.Vec3.prototype.dist2 = function (other) {
        var rx = this.x - other.x;
        var ry = this.y - other.y;
        var rz = this.z - other.z;
        return rx * rx + ry * ry + rz * rz;
    }

    Native.Quaternion = function (x, y, z, w) {
        this.x = x || 0.0;
        this.y = y || 0.0;
        this.z = z || 0.0;
        this.w = w || 0.0;
    }
    // Pulled from THREE.js
    Native.Quaternion.prototype.multiply = function(quat2) {
        var qax = this.x,  qay = this.y,  qaz = this.z,  qaw = this.w,
        qbx = quat2.x, qby = quat2.y, qbz = quat2.z, qbw = quat2.w;
        this.x = qax * qbw + qaw * qbx + qay * qbz - qaz * qby;
        this.y = qay * qbw + qaw * qby + qaz * qbx - qax * qbz;
        this.z = qaz * qbw + qaw * qbz + qax * qby - qay * qbx;
        this.w = qaw * qbw - qax * qbx - qay * qby - qaz * qbz;
        return this;
    }
})();

// Create + run tests
(function () {
    var mathTests = new TestRunner();
    var foo;

    print("math tests:");
    var iterations = [ 1000, 10000, 100000, 1000000 ];

    print("builtin Vec3 + Quat (wrapped glm::vec3 + glm::quat)");
    var builtinTests = new TestRunner();
    builtinTests.addTestCase('Vec3.sum')
        .run(function () {
            foo = Vec3.sum({x: 10, y: 12, z: 3}, {x: 1, y: 2, z: 4});
        })
    builtinTests.addTestCase('Vec3.distance')
        .run(function () {
            foo = Vec3.distance({x: 1209, y: 298, z: 238}, {x: 239, y: 20, z: 23})
        })
    builtinTests.addTestCase('Quat.multiply')
        .run(function () {
            foo = Quat.multiply({x: 2190.0, y: 2109.0, z: 1209.0, w: 829.0}, {x: -1293.0, y: 1029.1, z: 2190.1, w: 129.0})
        });
    builtinTests.runAllTestsWithIterations([ 1e3, 1e4, 1e5 ], 1e3, 10);

    print("");
    print("native JS Vec3 + Quaternion");
    var nativeTests = new TestRunner();
    nativeTests.addTestCase('Native Vec3.sum')
        .run(function () {
            foo = new Native.Vec3(10, 12, 3).add(new Native.Vec3(1, 2, 4));
        })
    nativeTests.addTestCase('Native Vec3.distance')
        .run(function () {
            foo = new Native.Vec3(1209, 298, 238).distance(new Native.Vec3(239, 20, 23));
        })
    nativeTests.addTestCase('Native Vec3.dist2')
        .run(function () {
            foo = new Native.Vec3(1209, 298, 238).dist2(new Native.Vec3(239, 20, 23));
        })
    nativeTests.addTestCase('Native Quat.multiply')
        .run(function () {
            foo = new Native.Quaternion(2190.0, 2109.0, 1209.0, 829.0).multiply(new Native.Quaternion(-1293.0, 1029.1, 2190.1, 129.0));
        });
    nativeTests.runAllTestsWithIterations([ 1e3, 1e4, 1e5, 1e6 ], 1e6, 10);

    print("");
    print("no-ops (for test assignment / construction overhead)")
    mathTests.addTestCase('no-op')
        .run(function(){});
    mathTests.addTestCase('assign null')
        .run(function () {
            foo = null;
        });
    mathTests.addTestCase('assign object')
        .run(function () {
            foo = { x: 1902, y: 129, z: 21 };
        });
    mathTests.addTestCase('Native Vec3.construct -- 3 args')
        .run(function () {
            foo = new Native.Vec3(1209, 298, 238);
        });
    mathTests.addTestCase('Native Vec3.construct -- 2 args')
        .run(function () {
            foo = new Native.Vec3(1209, 298);
        });
    mathTests.addTestCase('Native Vec3.construct -- no args')
        .run(function() {
            foo = new Native.Vec3();
        });

    mathTests.runAllTestsWithIterations([1e3, 1e4, 1e5, 1e6], 1e6, 10);
    mathTests.writeResultsToLog();
})();