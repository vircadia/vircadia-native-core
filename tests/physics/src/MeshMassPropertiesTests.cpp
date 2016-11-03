//
//  MeshMassProperties.cpp
//  tests/physics/src
//
//  Created by Virendra Singh on 2015.03.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MeshMassPropertiesTests.h"

#include <iostream>
#include <MeshMassProperties.h>

// Add additional qtest functionality (the include order is important!)
#include "BulletTestUtils.h"
#include "../QTestExtensions.h"

const btScalar acceptableRelativeError(1.0e-5f);
const btScalar acceptableAbsoluteError(1.0e-4f);

QTEST_MAIN(MeshMassPropertiesTests)

void pushTriangle(VectorOfIndices& indices, uint32_t a, uint32_t b, uint32_t c) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);
}

void MeshMassPropertiesTests::testParallelAxisTheorem() {
    // verity we can compute the inertia tensor of a box in two different ways:
    // (a) as one box
    // (b) as a combination of two partial boxes.

    btScalar bigBoxX = 7.0f;
    btScalar bigBoxY = 9.0f;
    btScalar bigBoxZ = 11.0f;
    btScalar bigBoxMass = bigBoxX * bigBoxY * bigBoxZ;
    btMatrix3x3 bitBoxInertia;
    computeBoxInertia(bigBoxMass, btVector3(bigBoxX, bigBoxY, bigBoxZ), bitBoxInertia);

    btScalar smallBoxX = bigBoxX / 2.0f;
    btScalar smallBoxY = bigBoxY;
    btScalar smallBoxZ = bigBoxZ;
    btScalar smallBoxMass = smallBoxX * smallBoxY * smallBoxZ;
    btMatrix3x3 smallBoxI;
    computeBoxInertia(smallBoxMass, btVector3(smallBoxX, smallBoxY, smallBoxZ), smallBoxI);

    btVector3 smallBoxOffset(smallBoxX / 2.0f, 0.0f, 0.0f);

    btMatrix3x3 smallBoxShiftedRight = smallBoxI;
    applyParallelAxisTheorem(smallBoxShiftedRight, smallBoxOffset, smallBoxMass);

    btMatrix3x3 smallBoxShiftedLeft = smallBoxI;
    applyParallelAxisTheorem(smallBoxShiftedLeft, -smallBoxOffset, smallBoxMass);

    btMatrix3x3 twoSmallBoxesInertia = smallBoxShiftedRight + smallBoxShiftedLeft;

    QCOMPARE_WITH_ABS_ERROR(bitBoxInertia, twoSmallBoxesInertia, acceptableAbsoluteError);
}

void MeshMassPropertiesTests::testTetrahedron(){
    // given the four vertices of a tetrahedron verify the analytic formula for inertia
    // agrees with expected results

    // these numbers from the Tonon paper:
    btVector3 points[4];
    points[0] = btVector3(8.33220f, -11.86875f, 0.93355f);
    points[1] = btVector3(0.75523f, 5.00000f, 16.37072f);
    points[2] = btVector3(52.61236f, 5.00000f, -5.38580f);
    points[3] = btVector3(2.00000f, 5.00000f, 3.00000f);

    btScalar expectedVolume = 1873.233236f;

    btMatrix3x3 expectedInertia;
    expectedInertia[0][0] = 43520.33257f;
    expectedInertia[1][1] = 194711.28938f;
    expectedInertia[2][2] = 191168.76173f;
    expectedInertia[1][2] = -4417.66150f;
    expectedInertia[2][1] = -4417.66150f;
    expectedInertia[0][2] = 46343.16662f;
    expectedInertia[2][0] = 46343.16662f;
    expectedInertia[0][1] = -11996.20119f;
    expectedInertia[1][0] = -11996.20119f;

    // compute volume
    btScalar volume = computeTetrahedronVolume(points);
    btScalar error = (volume - expectedVolume) / expectedVolume;
    if (fabsf(error) > acceptableRelativeError) {
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : volume of tetrahedron off by = "
            << error << std::endl;
    }

    btVector3 centerOfMass = 0.25f * (points[0] + points[1] + points[2] + points[3]);

    // compute inertia tensor
    // (shift the points so that tetrahedron's local centerOfMass is at origin)
    for (int i = 0; i < 4; ++i) {
        points[i] -= centerOfMass;
    }
    btMatrix3x3 inertia;
    computeTetrahedronInertia(volume, points, inertia);

    QCOMPARE_WITH_ABS_ERROR(volume, expectedVolume, acceptableRelativeError * volume);

    QCOMPARE_WITH_RELATIVE_ERROR(inertia, expectedInertia, acceptableRelativeError);
}

void MeshMassPropertiesTests::testOpenTetrahedonMesh() {
    // given the simplest possible mesh (open, with one triangle)
    // verify MeshMassProperties computes the right nubers

    // these numbers from the Tonon paper:
    VectorOfPoints points;
    points.push_back(btVector3(8.33220f, -11.86875f, 0.93355f));
    points.push_back(btVector3(0.75523f, 5.00000f, 16.37072f));
    points.push_back(btVector3(52.61236f, 5.00000f, -5.38580f));
    points.push_back(btVector3(2.00000f, 5.00000f, 3.00000f));

    btScalar expectedVolume = 1873.233236f;

    btMatrix3x3 expectedInertia;
    expectedInertia[0][0] = 43520.33257f;
    expectedInertia[1][1] = 194711.28938f;
    expectedInertia[2][2] = 191168.76173f;
    expectedInertia[1][2] = -4417.66150f;
    expectedInertia[2][1] = -4417.66150f;
    expectedInertia[0][2] = 46343.16662f;
    expectedInertia[2][0] = 46343.16662f;
    expectedInertia[0][1] = -11996.20119f;
    expectedInertia[1][0] = -11996.20119f;

    // test as an open mesh with one triangle
    VectorOfPoints shiftedPoints;
    shiftedPoints.push_back(points[0] - points[0]);
    shiftedPoints.push_back(points[1] - points[0]);
    shiftedPoints.push_back(points[2] - points[0]);
    shiftedPoints.push_back(points[3] - points[0]);
    VectorOfIndices triangles;
    pushTriangle(triangles, 1, 2, 3);
    btVector3 expectedCenterOfMass = 0.25f * (shiftedPoints[0] + shiftedPoints[1] + shiftedPoints[2] + shiftedPoints[3]);

    // compute mass properties
    MeshMassProperties mesh(shiftedPoints, triangles);

    // verify
    // (expected - actual) / expected > e   ==>  expected - actual  >  e * expected
    QCOMPARE_WITH_ABS_ERROR(mesh._volume, expectedVolume, acceptableRelativeError * expectedVolume);
    QCOMPARE_WITH_ABS_ERROR(mesh._centerOfMass, expectedCenterOfMass, acceptableAbsoluteError);
    QCOMPARE_WITH_RELATIVE_ERROR(mesh._inertia, expectedInertia, acceptableRelativeError);
}

void MeshMassPropertiesTests::testClosedTetrahedronMesh() {
    // given a tetrahedron as a closed mesh of four tiangles
    // verify MeshMassProperties computes the right nubers

    // these numbers from the Tonon paper:
    VectorOfPoints points;
    points.push_back(btVector3(8.33220f, -11.86875f, 0.93355f));
    points.push_back(btVector3(0.75523f, 5.00000f, 16.37072f));
    points.push_back(btVector3(52.61236f, 5.00000f, -5.38580f));
    points.push_back(btVector3(2.00000f, 5.00000f, 3.00000f));

    btScalar expectedVolume = 1873.233236f;

    btMatrix3x3 expectedInertia;
    expectedInertia[0][0] = 43520.33257f;
    expectedInertia[1][1] = 194711.28938f;
    expectedInertia[2][2] = 191168.76173f;
    expectedInertia[1][2] = -4417.66150f;
    expectedInertia[2][1] = -4417.66150f;
    expectedInertia[0][2] = 46343.16662f;
    expectedInertia[2][0] = 46343.16662f;
    expectedInertia[0][1] = -11996.20119f;
    expectedInertia[1][0] = -11996.20119f;

    btVector3 expectedCenterOfMass = 0.25f * (points[0] + points[1] + points[2] + points[3]);

    VectorOfIndices triangles;
    pushTriangle(triangles, 0, 2, 1);
    pushTriangle(triangles, 0, 3, 2);
    pushTriangle(triangles, 0, 1, 3);
    pushTriangle(triangles, 1, 2, 3);

    // compute mass properties
    MeshMassProperties mesh(points, triangles);

    // verify
    QCOMPARE_WITH_ABS_ERROR(mesh._volume, expectedVolume, acceptableRelativeError * expectedVolume);
    QCOMPARE_WITH_ABS_ERROR(mesh._centerOfMass, expectedCenterOfMass, acceptableAbsoluteError);
    QCOMPARE_WITH_RELATIVE_ERROR(mesh._inertia, expectedInertia, acceptableRelativeError);

    // test again, but this time shift the points so that the origin is definitely OUTSIDE the mesh
    btVector3 shift = points[0] + expectedCenterOfMass;
    for (int i = 0; i < (int)points.size(); ++i) {
        points[i] += shift;
    }
    expectedCenterOfMass = 0.25f * (points[0] + points[1] + points[2] + points[3]);

    // compute mass properties
    mesh.computeMassProperties(points, triangles);

    // verify
//    QCOMPARE_WITH_ABS_ERROR(mesh._volume, expectedVolume, acceptableRelativeError * expectedVolume);
//    QCOMPARE_WITH_ABS_ERROR(mesh._centerOfMass, expectedCenterOfMass, acceptableAbsoluteError);
//    QCOMPARE_WITH_RELATIVE_ERROR(mesh._inertia, expectedInertia, acceptableRelativeError);
}

void MeshMassPropertiesTests::testBoxAsMesh() {
    // verify that a mesh box produces the same mass properties as the analytic box.

    // build a box:
    //                            /
    //                           y
    //                          /
    //            6-------------------------7
    //           /|                        /|
    //          / |                       / |
    //         /  2----------------------/--3
    //        /  /                      /  /
    //   |   4-------------------------5  /  --x--
    //   z   | /                       | /
    //   |   |/                        |/
    //       0 ------------------------1

    btScalar x(5.0f);
    btScalar y(3.0f);
    btScalar z(2.0f);

    VectorOfPoints points;
    points.reserve(8);

    points.push_back(btVector3(0.0f, 0.0f, 0.0f));
    points.push_back(btVector3(x, 0.0f, 0.0f));
    points.push_back(btVector3(0.0f, y, 0.0f));
    points.push_back(btVector3(x, y, 0.0f));
    points.push_back(btVector3(0.0f, 0.0f, z));
    points.push_back(btVector3(x, 0.0f, z));
    points.push_back(btVector3(0.0f, y, z));
    points.push_back(btVector3(x, y, z));

    VectorOfIndices triangles;
    pushTriangle(triangles, 0, 1, 4);
    pushTriangle(triangles, 1, 5, 4);
    pushTriangle(triangles, 1, 3, 5);
    pushTriangle(triangles, 3, 7, 5);
    pushTriangle(triangles, 2, 0, 6);
    pushTriangle(triangles, 0, 4, 6);
    pushTriangle(triangles, 3, 2, 7);
    pushTriangle(triangles, 2, 6, 7);
    pushTriangle(triangles, 4, 5, 6);
    pushTriangle(triangles, 5, 7, 6);
    pushTriangle(triangles, 0, 2, 1);
    pushTriangle(triangles, 2, 3, 1);

    // compute expected mass properties analytically
    btVector3 expectedCenterOfMass = 0.5f * btVector3(x, y, z);
    btScalar expectedVolume = x * y * z;
    btMatrix3x3 expectedInertia;
    computeBoxInertia(expectedVolume, btVector3(x, y, z), expectedInertia);

    // compute the mass properties using the mesh
    MeshMassProperties mesh(points, triangles);

    // verify

    QCOMPARE_WITH_ABS_ERROR(mesh._volume, expectedVolume, acceptableRelativeError * expectedVolume);
    QCOMPARE_WITH_ABS_ERROR(mesh._centerOfMass, expectedCenterOfMass, acceptableAbsoluteError);

    // test this twice: _RELATIVE_ERROR doesn't test zero cases (to avoid divide-by-zero); _ABS_ERROR does.
    QCOMPARE_WITH_ABS_ERROR(mesh._inertia, expectedInertia, acceptableAbsoluteError);
    QCOMPARE_WITH_RELATIVE_ERROR(mesh._inertia, expectedInertia, acceptableRelativeError);

    // These two macros impl this:
//    for (int i = 0; i < 3; ++i) {
//        for (int j = 0; j < 3; ++j) {
//            if (expectedInertia [i][j] == btScalar(0.0f)) {
//                error = mesh._inertia[i][j] - expectedInertia[i][j];                                  // COMPARE_WITH_ABS_ERROR
//                if (fabsf(error) > acceptableAbsoluteError) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR : inertia[" << i << "][" << j << "] off by "
//                        << error << " absolute"<< std::endl;
//                }
//            } else {
//                error = (mesh._inertia[i][j] - expectedInertia[i][j]) / expectedInertia[i][j];        // COMPARE_WITH_RELATIVE_ERROR
//                if (fabsf(error) > acceptableRelativeError) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR : inertia[" << i << "][" << j << "] off by "
//                        << error << std::endl;
//                }
//            }
//        }
//    }
}
