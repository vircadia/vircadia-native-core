//
//  MeshMassProperties.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2015.05.25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>
#include <stdint.h>

#include "MeshMassProperties.h"

// this method is included for unit test verification
void computeBoxInertia(btScalar mass, const btVector3& diagonal, btMatrix3x3& inertia) {
    // formula for box inertia tensor:
    //
    //                  | y^2 + z^2    0        0     |
    //                  |                             |
    // inertia = M/12 * |     0    z^2 + x^2    0     |
    //                  |                             |
    //                  |     0        0    x^2 + y^2 |
    //
    
    mass = mass / btScalar(12.0f);
    btScalar x = diagonal[0];
    x = mass * x * x;
    btScalar y = diagonal[1];
    y = mass * y * y;
    btScalar z = diagonal[2];
    z = mass * z * z;
    inertia.setIdentity();
    inertia[0][0] = y + z;
    inertia[1][1] = z + x;
    inertia[2][2] = x + y;
}

void computeTetrahedronInertia(btScalar mass, btVector3* points, btMatrix3x3& inertia) {
    // Computes the inertia tensor of a tetrahedron about its center of mass.
    // The tetrahedron is defined by array of four points in its center of mass frame.
    //
    // The analytic formulas were obtained from Tonon's paper:
    //     http://docsdrive.com/pdfs/sciencepublications/jmssp/2005/8-11.pdf
    //     http://thescipub.com/PDF/jmssp.2005.8.11.pdf
    //
    // The inertia tensor has the following form:
    //
    //           | a    f    e |
    //           |             |
    // inertia = | f    b    d |
    //           |             |
    //           | e    d    c |

    const btVector3& p0 = points[0];
    const btVector3& p1 = points[1];
    const btVector3& p2 = points[2];
    const btVector3& p3 = points[3];

    for (uint32_t i = 0; i < 3; ++i ) {
        uint32_t j = (i + 1) % 3;
        uint32_t k = (j + 1) % 3;

        // compute diagonal
        inertia[i][i] = mass * btScalar(0.1f) * 
            ( p0[j] * (p0[j] + p1[j] + p2[j] + p3[j])
            + p1[j] * (p1[j] + p2[j] + p3[j])
            + p2[j] * (p2[j] + p3[j])
            + p3[j] * p3[j]
            + p0[k] * (p0[k] + p1[k] + p2[k] + p3[k])
            + p1[k] * (p1[k] + p2[k] + p3[k])
            + p2[k] * (p2[k] + p3[k])
            + p3[k] * p3[k] );

        // compute off-diagonals
        inertia[j][k] = inertia[k][j] = - mass * btScalar(0.05f) *
            ( btScalar(2.0f) * ( p0[j] * p0[k] +  p1[j] * p1[k] +  p2[j] * p2[k] +  p3[j] * p3[k] )
            + p0[j] * (p1[k] + p2[k] + p3[k])
            + p1[j] * (p0[k] + p2[k] + p3[k])
            + p2[j] * (p0[k] + p1[k] + p3[k])
            + p3[j] * (p0[k] + p1[k] + p2[k]) );
    }
}

// helper function
void computePointInertia(const btVector3& point, btScalar mass, btMatrix3x3& inertia) {
    btScalar distanceSquared = point.length2();
    if (distanceSquared > 0.0f) {
        for (uint32_t i = 0; i < 3; ++i) {
            btScalar pointi = point[i];
            inertia[i][i] = mass * (distanceSquared - (pointi * pointi));
            for (uint32_t j = i + 1; j < 3; ++j) {
                btScalar offDiagonal = - mass * pointi * point[j];
                inertia[i][j] = offDiagonal;
                inertia[j][i] = offDiagonal;
            }
        }
    }
}

// this method is included for unit test verification
void computeTetrahedronInertiaByBruteForce(btVector3* points, btMatrix3x3& inertia) {
    // Computes the approximate inertia tensor of a tetrahedron (about frame's origin) 
    // by integration over the "point" masses.  This is numerically expensive so it may 
    // take a while to complete.

    VectorOfIndices triangles = {
        0, 2, 1, 
        0, 3, 2, 
        0, 1, 3, 
        1, 2, 3 };

    for (int i = 0; i < 3; ++i) {
        inertia[i].setZero();
    }

    // compute normals
    btVector3 center = btScalar(0.25f) * (points[0] + points[1] + points[2] + points[3]);
    btVector3 normals[4];
    btVector3 pointsOnPlane[4];
    for (int i = 0; i < 4; ++i) {
        int t = 3 * i;
        btVector3& p0 = points[triangles[t]];
        btVector3& p1 = points[triangles[t + 1]];
        btVector3& p2 = points[triangles[t + 2]];
        normals[i] = ((p1 - p0).cross(p2 - p1)).normalized();
        // make sure normal points away from center 
        if (normals[i].dot(p0 - center) < btScalar(0.0f)) {
            normals[i] *= btScalar(-1.0f);
        }
        pointsOnPlane[i] = p0;
    }

    // compute bounds of integration
    btVector3 boxMax = points[0];
    btVector3 boxMin = points[0];
    for (int i = 1; i < 4; ++i) {
        for(int j = 0; j < 3; ++j) {
            if (points[i][j] > boxMax[j]) {
                boxMax[j] = points[i][j];
            }
            if (points[i][j] < boxMin[j]) {
                boxMin[j] = points[i][j];
            }
        }
    }

    // compute step size
    btVector3 diagonal = boxMax - boxMin;
    btScalar maxDimension = diagonal[0];
    if (diagonal[1] > maxDimension) {
        maxDimension = diagonal[1];
    }
    if (diagonal[2] > maxDimension) {
        maxDimension = diagonal[2];
    }
    btScalar resolutionOfIntegration = btScalar(400.0f);
    btScalar delta = maxDimension / resolutionOfIntegration;
    btScalar deltaVolume = delta * delta * delta;

    // integrate over three dimensions
    btMatrix3x3 deltaInertia;
    btScalar XX = boxMax[0];
    btScalar YY = boxMax[1];
    btScalar ZZ = boxMax[2];
    btScalar x = boxMin[0];
    while(x < XX) {
        btScalar y = boxMin[1];
        while (y < YY) {
            btScalar z = boxMin[2];
            while (z < ZZ) {
                btVector3 p(x, y, z);
                // the point is inside the shape if it is behind all face planes
                bool pointInside = true;
                for (int i = 0; i < 4; ++i) {
                    if ((p - pointsOnPlane[i]).dot(normals[i]) > btScalar(0.0f)) {
                        pointInside = false;
                        break;
                    }
                }
                if (pointInside) {
                    // this point contributes to the total
                    computePointInertia(p, deltaVolume, deltaInertia);
                    inertia += deltaInertia;
                }
                z += delta;
            }
            y += delta;
        }
        x += delta;
    }
}

btScalar computeTetrahedronVolume(btVector3* points) {
    // Assumes triangle {1, 2, 3} is wound according to the right-hand-rule.
    // NOTE: volume may be negative, in which case the tetrahedron contributes negatively to totals
    // volume = (face_area * face_normal).dot(face_to_far_point) / 3.0
    // (face_area * face_normal) = side0.cross(side1) / 2.0
    return ((points[2] - points[1]).cross(points[3] - points[2])).dot(points[3] - points[0]) / btScalar(6.0f);
}

void applyParallelAxisTheorem(btMatrix3x3& inertia, const btVector3& shift, btScalar mass) {
    // Parallel Axis Theorem says:
    //
    // Ishifted = Icm + M * [ (R*R)E - R(x)R ]
    // 
    // where R*R   = inside product
    //       R(x)R = outside product
    //       E     = identity matrix

    btScalar distanceSquared = shift.length2();
    if (distanceSquared > btScalar(0.0f)) {
        for (uint32_t i = 0; i < 3; ++i) {
            btScalar shifti = shift[i];
            inertia[i][i] += mass * (distanceSquared - (shifti * shifti));
            for (uint32_t j = i + 1; j < 3; ++j) {
                btScalar offDiagonal = mass * shifti * shift[j];
                inertia[i][j] -= offDiagonal;
                inertia[j][i] -= offDiagonal;
            }
        }
    }
}

// helper function
void applyInverseParallelAxisTheorem(btMatrix3x3& inertia, const btVector3& shift, btScalar mass) {
    // Parallel Axis Theorem says:
    //
    // Ishifted = Icm + M * [ (R*R)E - R(x)R ]
    // 
    // So the inverse would be:
    //
    // Icm = Ishifted - M * [ (R*R)E - R(x)R ]

    btScalar distanceSquared = shift.length2();
    if (distanceSquared > btScalar(0.0f)) {
        for (uint32_t i = 0; i < 3; ++i) {
            btScalar shifti = shift[i];
            inertia[i][i] -= mass * (distanceSquared - (shifti * shifti));
            for (uint32_t j = i + 1; j < 3; ++j) {
                btScalar offDiagonal = mass * shifti * shift[j];
                inertia[i][j] += offDiagonal;
                inertia[j][i] += offDiagonal;
            }
        }
    }
}

MeshMassProperties::MeshMassProperties(const VectorOfPoints& points, const VectorOfIndices& triangleIndices) {
    computeMassProperties(points, triangleIndices);
}

void MeshMassProperties::computeMassProperties(const VectorOfPoints& points, const VectorOfIndices& triangleIndices) {
    // We process the mesh one triangle at a time.  Each triangle defines a tetrahedron 
    // relative to some local point p0 (which we chose to be the local origin for convenience). 
    // Each tetrahedron contributes to the three totals: volume, centerOfMass, and inertiaTensor.
    //
    // We assume the mesh triangles are wound using the right-hand-rule, such that the 
    // triangle's points circle counter-clockwise about its face normal.
    // 

    // initialize the totals
    _volume = btScalar(0.0f);
    btVector3 weightedCenter;
    weightedCenter.setZero();
    for (uint32_t i = 0; i < 3; ++i) {
        _inertia[i].setZero();
    }

    // create some variables to hold temporary results
    #ifndef NDEBUG
    uint32_t numPoints = (uint32_t)points.size();
    #endif
    const btVector3 p0(0.0f, 0.0f, 0.0f);
    btMatrix3x3 tetraInertia;
    btMatrix3x3 doubleDebugInertia;
    btVector3 tetraPoints[4];
    btVector3 center;
    
    // loop over triangles
    uint32_t numTriangles = (uint32_t)triangleIndices.size() / 3;
    for (uint32_t i = 0; i < numTriangles; ++i) {
        uint32_t t = 3 * i;
        #ifndef NDEBUG
        assert(triangleIndices[t] < numPoints);
        assert(triangleIndices[t + 1] < numPoints);
        assert(triangleIndices[t + 2] < numPoints);
        #endif

        // extract raw vertices
        tetraPoints[0] = p0;
        tetraPoints[1] = points[triangleIndices[t]];
        tetraPoints[2] = points[triangleIndices[t + 1]];
        tetraPoints[3] = points[triangleIndices[t + 2]];

        // compute volume
        btScalar volume = computeTetrahedronVolume(tetraPoints);

        // compute center
        // NOTE: since tetraPoints[0] is the origin, we don't include it in the sum
        center = btScalar(0.25f) * (tetraPoints[1] + tetraPoints[2] + tetraPoints[3]);

        // shift vertices so that center of mass is at origin
        tetraPoints[0] -= center;
        tetraPoints[1] -= center;
        tetraPoints[2] -= center;
        tetraPoints[3] -= center;

        // compute inertia tensor then shift it to origin-frame
        computeTetrahedronInertia(volume, tetraPoints, tetraInertia);
        applyParallelAxisTheorem(tetraInertia, center, volume);

        // tally results
        weightedCenter += volume * center;
        _volume += volume;
        _inertia += tetraInertia;
    }

    _centerOfMass = weightedCenter / _volume;
    
    applyInverseParallelAxisTheorem(_inertia, _centerOfMass, _volume);
}

