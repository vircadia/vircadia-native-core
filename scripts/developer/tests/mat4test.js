//
//  mat4test.js
//  examples/tests
//
//  Created by Anthony Thibault on 2016/3/7
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var IDENTITY = {r0c0: 1, r0c1: 0, r0c2: 0, r0c3: 0,
                r1c0: 0, r1c1: 1, r1c2: 0, r1c3: 0,
                r2c0: 0, r2c1: 0, r2c2: 1, r2c3: 0,
                r3c0: 0, r3c1: 0, r3c2: 0, r3c3: 1};

var ROT_ZERO = {x: 0, y: 0, z: 0, w: 1};
var ROT_Y_180 = {x: 0, y: 1, z: 0, w: 0};

var ONE = {x: 1, y: 1, z: 1};
var ZERO = {x: 0, y: 0, z: 0};
var ONE_TWO_THREE = {x: 1, y: 2, z: 3};
var ONE_HALF = {x: 0.5, y: 0.5, z: 0.5};

var EPSILON = 0.000001;

function mat4FuzzyEqual(a, b) {
    var r, c;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            if (Math.abs(a["r" + r + "c" + c] - b["r" + r + "c" + c]) > EPSILON) {
                return false;
            }
        }
    }
    return true;
}

function vec3FuzzyEqual(a, b) {
    if (Math.abs(a.x - b.x) > EPSILON ||
        Math.abs(a.y - b.y) > EPSILON ||
        Math.abs(a.z - b.z) > EPSILON) {
        return false;
    }
    return true;
}

function quatFuzzyEqual(a, b) {
    if (Math.abs(a.x - b.x) > EPSILON ||
        Math.abs(a.y - b.y) > EPSILON ||
        Math.abs(a.z - b.z) > EPSILON ||
        Math.abs(a.w - b.w) > EPSILON) {
        return false;
    }
    return true;
}

var failureCount = 0;
var testCount = 0;
function assert(test) {
    testCount++;
    if (!test) {
        print("MAT4 TEST " + testCount + " failed!");
        failureCount++;
    }
}

function testCreate() {
    var test0 = Mat4.createFromScaleRotAndTrans(ONE, {x: 0, y: 0, z: 0, w: 1}, ZERO);
    assert(mat4FuzzyEqual(test0, IDENTITY));

    var test1 = Mat4.createFromRotAndTrans({x: 0, y: 0, z: 0, w: 1}, ZERO);
    assert(mat4FuzzyEqual(test1, IDENTITY));

    var test2 = Mat4.createFromRotAndTrans(ROT_Y_180, ONE_TWO_THREE);
    assert(mat4FuzzyEqual(test2, {r0c0: -1, r0c1: 0, r0c2: 0, r0c3: 1,
                                  r1c0: 0, r1c1: 1, r1c2: 0, r1c3: 2,
                                  r2c0: 0, r2c1: 0, r2c2: -1, r2c3: 3,
                                  r3c0: 0, r3c1: 0, r3c2: 0, r3c3: 1}));

    var test3 = Mat4.createFromScaleRotAndTrans(ONE_HALF, ROT_Y_180, ONE_TWO_THREE);
    assert(mat4FuzzyEqual(test3, {r0c0: -0.5, r0c1: 0, r0c2: 0, r0c3: 1,
                                  r1c0: 0, r1c1: 0.5, r1c2: 0, r1c3: 2,
                                  r2c0: 0, r2c1: 0, r2c2: -0.5, r2c3: 3,
                                  r3c0: 0, r3c1: 0, r3c2: 0, r3c3: 1}));
}

function testExtractTranslation() {
    var test0 = Mat4.extractTranslation(IDENTITY);
    assert(vec3FuzzyEqual(ZERO, test0));

    var test1 = Mat4.extractTranslation(Mat4.createFromRotAndTrans(ROT_Y_180, ONE_TWO_THREE));
    assert(vec3FuzzyEqual(ONE_TWO_THREE, test1));
}

function testExtractRotation() {
    var test0 = Mat4.extractRotation(IDENTITY);
    assert(quatFuzzyEqual(ROT_ZERO, test0));

    var test1 = Mat4.extractRotation(Mat4.createFromRotAndTrans(ROT_Y_180, ONE_TWO_THREE));
    assert(quatFuzzyEqual(ROT_Y_180, test1));
}

function testExtractScale() {
    var test0 = Mat4.extractScale(IDENTITY);
    assert(vec3FuzzyEqual(ONE, test0));

    var test1 = Mat4.extractScale(Mat4.createFromScaleRotAndTrans(ONE_HALF, ROT_Y_180, ONE_TWO_THREE));
    assert(vec3FuzzyEqual(ONE_HALF, test1));

    var test2 = Mat4.extractScale(Mat4.createFromScaleRotAndTrans(ONE_TWO_THREE, ROT_ZERO, ONE_TWO_THREE));
    assert(vec3FuzzyEqual(ONE_TWO_THREE, test2));
}

function testTransformPoint() {
    var test0 = Mat4.transformPoint(IDENTITY, ONE);
    assert(vec3FuzzyEqual(ONE, test0));

    var m = Mat4.createFromScaleRotAndTrans(ONE_HALF, ROT_Y_180, ONE_TWO_THREE);
    var test1 = Mat4.transformPoint(m, ONE);
    assert(vec3FuzzyEqual({x: 0.5, y: 2.5, z: 2.5}, test1));
}

function testTransformVector() {
    var test0 = Mat4.transformVector(IDENTITY, ONE);
    assert(vec3FuzzyEqual(ONE, test0));

    var m = Mat4.createFromScaleRotAndTrans(ONE_HALF, ROT_Y_180, ONE_TWO_THREE);
    var test1 = Mat4.transformVector(m, ONE);
    assert(vec3FuzzyEqual({x: -0.5, y: 0.5, z: -0.5}, test1));
}

function testInverse() {
    var test0 = IDENTITY;
    assert(mat4FuzzyEqual(IDENTITY, Mat4.multiply(test0, Mat4.inverse(test0))));

    var test1 = Mat4.createFromScaleRotAndTrans(ONE_HALF, ROT_Y_180, ONE_TWO_THREE);
    assert(mat4FuzzyEqual(IDENTITY, Mat4.multiply(test1, Mat4.inverse(test1))));

    var test2 = Mat4.extractScale(Mat4.createFromScaleRotAndTrans(ONE_TWO_THREE, ROT_ZERO, ONE_TWO_THREE));
    assert(mat4FuzzyEqual(IDENTITY, Mat4.multiply(test2, Mat4.inverse(test2))));
}

function testFront() {
    var test0 = IDENTITY;
    assert(mat4FuzzyEqual({x: 0, y: 0, z: -1}, Mat4.getFront(test0)));

    var test1 = Mat4.createFromScaleRotAndTrans(ONE_HALF, ROT_Y_180, ONE_TWO_THREE);
    assert(mat4FuzzyEqual({x: 0, y: 0, z: 1}, Mat4.getFront(test1)));
}

function testMat4() {
    testCreate();
    testExtractTranslation();
    testExtractRotation();
    testExtractScale();
    testTransformPoint();
    testTransformVector();
    testInverse();
    testFront();

    print("MAT4 TEST complete! (" + (testCount - failureCount) + "/" + testCount + ") tests passed!");
}

testMat4();
