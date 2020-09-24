//
//  GeometryUtil.cpp
//  libraries/shared/src
//
//  Created by Andrzej Kapolka on 5/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GeometryUtil.h"

#include <assert.h>
#include <cstring>
#include <cmath>
#include <bitset>
#include <complex>
#include <qmath.h>
#include <glm/gtx/quaternion.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include "NumericalConstants.h"
#include "GLMHelpers.h"
#include "Plane.h"

glm::vec3 computeVectorFromPointToSegment(const glm::vec3& point, const glm::vec3& start, const glm::vec3& end) {
    // compute the projection of the point vector onto the segment vector
    glm::vec3 segmentVector = end - start;
    float lengthSquared = glm::dot(segmentVector, segmentVector);
    if (lengthSquared < EPSILON) {
        return start - point; // start and end the same
    }
    float proj = glm::dot(point - start, segmentVector) / lengthSquared;
    if (proj <= 0.0f) { // closest to the start
        return start - point;

    } else if (proj >= 1.0f) { // closest to the end
        return end - point;

    } else { // closest to the middle
        return start + segmentVector*proj - point;
    }
}

// Computes the penetration between a point and a sphere (centered at the origin)
// if point is inside sphere: returns true and stores the result in 'penetration'
// (the vector that would move the point outside the sphere)
// otherwise returns false
bool findSpherePenetration(const glm::vec3& point, const glm::vec3& defaultDirection, float sphereRadius,
                           glm::vec3& penetration) {
    float vectorLength = glm::length(point);
    if (vectorLength < EPSILON) {
        penetration = defaultDirection * sphereRadius;
        return true;
    }
    float distance = vectorLength - sphereRadius;
    if (distance < 0.0f) {
        penetration = point * (-distance / vectorLength);
        return true;
    }
    return false;
}

bool findSpherePointPenetration(const glm::vec3& sphereCenter, float sphereRadius,
                                const glm::vec3& point, glm::vec3& penetration) {
    return findSpherePenetration(point - sphereCenter, glm::vec3(0.0f, -1.0f, 0.0f), sphereRadius, penetration);
}

bool findPointSpherePenetration(const glm::vec3& point, const glm::vec3& sphereCenter,
       float sphereRadius, glm::vec3& penetration) {
    return findSpherePenetration(sphereCenter - point, glm::vec3(0.0f, -1.0f, 0.0f), sphereRadius, penetration);
}

bool findSphereSpherePenetration(const glm::vec3& firstCenter, float firstRadius,
                                 const glm::vec3& secondCenter, float secondRadius, glm::vec3& penetration) {
    return findSpherePointPenetration(firstCenter, firstRadius + secondRadius, secondCenter, penetration);
}

bool findSphereSegmentPenetration(const glm::vec3& sphereCenter, float sphereRadius,
                                  const glm::vec3& segmentStart, const glm::vec3& segmentEnd, glm::vec3& penetration) {
    return findSpherePenetration(computeVectorFromPointToSegment(sphereCenter, segmentStart, segmentEnd),
                                 glm::vec3(0.0f, -1.0f, 0.0f), sphereRadius, penetration);
}

bool findSphereCapsulePenetration(const glm::vec3& sphereCenter, float sphereRadius, const glm::vec3& capsuleStart,
                                  const glm::vec3& capsuleEnd, float capsuleRadius, glm::vec3& penetration) {
    return findSphereSegmentPenetration(sphereCenter, sphereRadius + capsuleRadius,
        capsuleStart, capsuleEnd, penetration);
}

bool findPointCapsuleConePenetration(const glm::vec3& point, const glm::vec3& capsuleStart,
        const glm::vec3& capsuleEnd, float startRadius, float endRadius, glm::vec3& penetration) {
    // compute the projection of the point vector onto the segment vector
    glm::vec3 segmentVector = capsuleEnd - capsuleStart;
    float lengthSquared = glm::dot(segmentVector, segmentVector);
    if (lengthSquared < EPSILON) { // start and end the same
        return findPointSpherePenetration(point, capsuleStart,
            glm::max(startRadius, endRadius), penetration);
    }
    float proj = glm::dot(point - capsuleStart, segmentVector) / lengthSquared;
    if (proj <= 0.0f) { // closest to the start
        return findPointSpherePenetration(point, capsuleStart, startRadius, penetration);

    } else if (proj >= 1.0f) { // closest to the end
        return findPointSpherePenetration(point, capsuleEnd, endRadius, penetration);

    } else { // closest to the middle
        return findPointSpherePenetration(point, capsuleStart + segmentVector * proj,
            glm::mix(startRadius, endRadius, proj), penetration);
    }
}

bool findSphereCapsuleConePenetration(const glm::vec3& sphereCenter,
        float sphereRadius, const glm::vec3& capsuleStart, const glm::vec3& capsuleEnd,
        float startRadius, float endRadius, glm::vec3& penetration) {
    return findPointCapsuleConePenetration(sphereCenter, capsuleStart, capsuleEnd,
        startRadius + sphereRadius, endRadius + sphereRadius, penetration);
}

bool findSpherePlanePenetration(const glm::vec3& sphereCenter, float sphereRadius,
                                const glm::vec4& plane, glm::vec3& penetration) {
    float distance = glm::dot(plane, glm::vec4(sphereCenter, 1.0f)) - sphereRadius;
    if (distance < 0.0f) {
        penetration = glm::vec3(plane) * distance;
        return true;
    }
    return false;
}

bool findSphereDiskPenetration(const glm::vec3& sphereCenter, float sphereRadius,
                               const glm::vec3& diskCenter, float diskRadius, float diskThickness, const glm::vec3& diskNormal,
                               glm::vec3& penetration) {
    glm::vec3 localCenter = sphereCenter - diskCenter;
    float axialDistance = glm::dot(localCenter, diskNormal);
    if (std::fabs(axialDistance) < (sphereRadius + 0.5f * diskThickness)) {
        // sphere hit the plane, but does it hit the disk?
        // Note: this algorithm ignores edge hits.
        glm::vec3 axialOffset = axialDistance * diskNormal;
        if (glm::length(localCenter - axialOffset) < diskRadius) {
            // yes, hit the disk
            penetration = (std::fabs(axialDistance) - (sphereRadius + 0.5f * diskThickness) ) * diskNormal;
            if (axialDistance < 0.0f) {
                // hit the backside of the disk, so negate penetration vector
                penetration *= -1.0f;
            }
            return true;
        }
    }
    return false;
}

bool findCapsuleSpherePenetration(const glm::vec3& capsuleStart, const glm::vec3& capsuleEnd, float capsuleRadius,
                                  const glm::vec3& sphereCenter, float sphereRadius, glm::vec3& penetration) {
    if (findSphereCapsulePenetration(sphereCenter, sphereRadius,
            capsuleStart, capsuleEnd, capsuleRadius, penetration)) {
        penetration = -penetration;
        return true;
    }
    return false;
}

bool findCapsulePlanePenetration(const glm::vec3& capsuleStart, const glm::vec3& capsuleEnd, float capsuleRadius,
                                 const glm::vec4& plane, glm::vec3& penetration) {
    float distance = glm::min(glm::dot(plane, glm::vec4(capsuleStart, 1.0f)),
        glm::dot(plane, glm::vec4(capsuleEnd, 1.0f))) - capsuleRadius;
    if (distance < 0.0f) {
        penetration = glm::vec3(plane) * distance;
        return true;
    }
    return false;
}

glm::vec3 addPenetrations(const glm::vec3& currentPenetration, const glm::vec3& newPenetration) {
    // find the component of the new penetration in the direction of the current
    float currentLength = glm::length(currentPenetration);
    if (currentLength == 0.0f) {
        return newPenetration;
    }
    glm::vec3 currentDirection = currentPenetration / currentLength;
    float directionalComponent = glm::dot(newPenetration, currentDirection);

    // if orthogonal or in the opposite direction, we can simply add
    if (directionalComponent <= 0.0f) {
        return currentPenetration + newPenetration;
    }

    // otherwise, we need to take the maximum component of current and new
    return currentDirection * glm::max(directionalComponent, currentLength) +
        newPenetration - (currentDirection * directionalComponent);
}

// finds the intersection between a ray and the facing plane on one axis
bool findIntersection(float origin, float direction, float corner, float size, float& distance) {
    if (direction > EPSILON) {
        distance = (corner - origin) / direction;
        return true;
    } else if (direction < -EPSILON) {
        distance = (corner + size - origin) / direction;
        return true;
    }
    return false;
}

// finds the intersection between a ray and the inside facing plane on one axis
bool findInsideOutIntersection(float origin, float direction, float corner, float size, float& distance) {
    if (direction > EPSILON) {
        distance = -1.0f * (origin - (corner + size)) / direction;
        return true;
    } else if (direction < -EPSILON) {
        distance = -1.0f * (origin - corner) / direction;
        return true;
    }
    return false;
}

// https://tavianator.com/fast-branchless-raybounding-box-intersections/
bool findRayAABoxIntersection(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& invDirection,
                              const glm::vec3& corner, const glm::vec3& scale, float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    float t1, t2, newTmin, newTmax, tmin = -INFINITY, tmax = INFINITY;
    int minAxis = -1, maxAxis = -1;

    for (int i = 0; i < 3; ++i) {
        t1 = (corner[i] - origin[i]) * invDirection[i];
        t2 = (corner[i] + scale[i] - origin[i]) * invDirection[i];

        newTmin = glm::min(t1, t2);
        newTmax = glm::max(t1, t2);

        minAxis = newTmin > tmin ? i : minAxis;
        tmin = glm::max(tmin, newTmin);
        maxAxis = newTmax < tmax ? i : maxAxis;
        tmax = glm::min(tmax, newTmax);
    }

    if (tmax >= glm::max(tmin, 0.0f)) {
        if (tmin < 0.0f) {
            distance = tmax;
            bool positiveDirection = direction[maxAxis] > 0.0f;
            surfaceNormal = glm::vec3(0.0f);
            surfaceNormal[maxAxis] = positiveDirection ? -1.0f : 1.0f;
            face = positiveDirection ? BoxFace(2 * maxAxis + 1) : BoxFace(2 * maxAxis);
        } else {
            distance = tmin;
            bool positiveDirection = direction[minAxis] > 0.0f;
            surfaceNormal = glm::vec3(0.0f);
            surfaceNormal[minAxis] = positiveDirection ? -1.0f : 1.0f;
            face = positiveDirection ? BoxFace(2 * minAxis) : BoxFace(2 * minAxis + 1);
        }
        return true;
    }
    return false;
}

bool findRaySphereIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& center, float radius, float& distance) {
    glm::vec3 relativeOrigin = origin - center;
    float c = glm::dot(relativeOrigin, relativeOrigin) - radius * radius;
    if (c < 0.0f) {
        distance = 0.0f;
        return true; // starts inside the sphere
    }
    float b = 2.0f * glm::dot(direction, relativeOrigin);
    float a = glm::dot(direction, direction);
    float radicand = b * b - 4.0f * a * c;
    if (radicand < 0.0f) {
        return false; // doesn't hit the sphere
    }
    float t = 0.5f * (-b - sqrtf(radicand)) / a;
    if (t < 0.0f) {
        return false; // doesn't hit the sphere
    }
    distance = t;
    return true;
}

bool pointInSphere(const glm::vec3& origin, const glm::vec3& center, float radius) {
    glm::vec3 relativeOrigin = origin - center;
    float c = glm::dot(relativeOrigin, relativeOrigin) - radius * radius;
    return c <= 0.0f;
}


bool pointInCapsule(const glm::vec3& origin, const glm::vec3& start, const glm::vec3& end, float radius) {
    glm::vec3 relativeOrigin = origin - start;
    glm::vec3 relativeEnd = end - start;
    float capsuleLength = glm::length(relativeEnd);
    relativeEnd /= capsuleLength;
    float originProjection = glm::dot(relativeEnd, relativeOrigin);
    glm::vec3 constant = relativeOrigin - relativeEnd * originProjection;
    float c = glm::dot(constant, constant) - radius * radius;
    if (c < 0.0f) { // starts inside cylinder
        if (originProjection < 0.0f) { // below start
            return pointInSphere(origin, start, radius);
        } else if (originProjection > capsuleLength) { // above end
            return pointInSphere(origin, end, radius);
        } else { // between start and end
            return true;
        }
    }
    return false;
}

bool findRayCapsuleIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& start, const glm::vec3& end, float radius, float& distance) {
    if (start == end) {
        return findRaySphereIntersection(origin, direction, start, radius, distance); // handle degenerate case
    }
    glm::vec3 relativeOrigin = origin - start;
    glm::vec3 relativeEnd = end - start;
    float capsuleLength = glm::length(relativeEnd);
    relativeEnd /= capsuleLength;
    float originProjection = glm::dot(relativeEnd, relativeOrigin);
    glm::vec3 constant = relativeOrigin - relativeEnd * originProjection;
    float c = glm::dot(constant, constant) - radius * radius;
    if (c < 0.0f) { // starts inside cylinder
        if (originProjection < 0.0f) { // below start
            return findRaySphereIntersection(origin, direction, start, radius, distance);

        } else if (originProjection > capsuleLength) { // above end
            return findRaySphereIntersection(origin, direction, end, radius, distance);

        } else { // between start and end
            distance = 0.0f;
            return true;
        }
    }
    glm::vec3 coefficient = direction - relativeEnd * glm::dot(relativeEnd, direction);
    float a = glm::dot(coefficient, coefficient);
    if (a == 0.0f) {
        return false; // parallel to enclosing cylinder
    }
    float b = 2.0f * glm::dot(constant, coefficient);
    float radicand = b * b - 4.0f * a * c;
    if (radicand < 0.0f) {
        return false; // doesn't hit the enclosing cylinder
    }
    float t = (-b - sqrtf(radicand)) / (2.0f * a);
    if (t < 0.0f) {
        return false; // doesn't hit the enclosing cylinder
    }
    glm::vec3 intersection = relativeOrigin + direction * t;
    float intersectionProjection = glm::dot(relativeEnd, intersection);
    if (intersectionProjection < 0.0f) { // below start
        return findRaySphereIntersection(origin, direction, start, radius, distance);

    } else if (intersectionProjection > capsuleLength) { // above end
        return findRaySphereIntersection(origin, direction, end, radius, distance);
    }
    distance = t; // between start and end
    return true;
}

// reference https://www.opengl.org/wiki/Calculating_a_Surface_Normal
glm::vec3 Triangle::getNormal() const {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    return glm::normalize(glm::cross(edge1, edge2));
}

float Triangle::getArea() const {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    return 0.5f * glm::length(glm::cross(edge1, edge2));
}

Triangle Triangle::operator*(const glm::mat4& transform) const {
    return {
        glm::vec3(transform * glm::vec4(v0, 1.0f)),
        glm::vec3(transform * glm::vec4(v1, 1.0f)),
        glm::vec3(transform * glm::vec4(v2, 1.0f))
    };
}

// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
bool findRayTriangleIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& distance, bool allowBackface) {
    glm::vec3 firstSide = v1 - v0;
    glm::vec3 secondSide = v2 - v0;
    glm::vec3 P = glm::cross(direction, secondSide);
    float det = glm::dot(firstSide, P);
    if (!allowBackface && det < EPSILON) {
        return false;
    } else if (fabsf(det) < EPSILON) {
        return false;
    }

    float invDet = 1.0f / det;
    glm::vec3 T = origin - v0;
    float u = glm::dot(T, P) * invDet;
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    glm::vec3 Q = glm::cross(T, firstSide);
    float v = glm::dot(direction, Q) * invDet;
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    float t = glm::dot(secondSide, Q) * invDet;
    if (t > EPSILON) {
        distance = t;
        return true;
    }
    return false;
}

static void getTrianglePlaneIntersectionPoints(const glm::vec3 trianglePoints[3], const float pointPlaneDistances[3],
                                               const int clippedPointIndex, const int keptPointIndices[2],
                                               glm::vec3 points[2]) {
    assert(clippedPointIndex >= 0 && clippedPointIndex < 3);
    const auto& clippedPoint = trianglePoints[clippedPointIndex];
    const float clippedPointPlaneDistance = pointPlaneDistances[clippedPointIndex];
    for (auto i = 0; i < 2; i++) {
        assert(keptPointIndices[i] >= 0 && keptPointIndices[i] < 3);
        const auto& keptPoint = trianglePoints[keptPointIndices[i]];
        const float keptPointPlaneDistance = pointPlaneDistances[keptPointIndices[i]];
        auto intersectionEdgeRatio = clippedPointPlaneDistance / (clippedPointPlaneDistance - keptPointPlaneDistance);
        points[i] = clippedPoint + (keptPoint - clippedPoint) * intersectionEdgeRatio;
    }
}

int clipTriangleWithPlane(const Triangle& triangle, const Plane& plane, Triangle* clippedTriangles, int maxClippedTriangleCount) {
    float pointDistanceToPlane[3];
    std::bitset<3> arePointsClipped;
    glm::vec3 triangleVertices[3] = { triangle.v0, triangle.v1, triangle.v2 };
    int clippedTriangleCount = 0;
    int i;

    for (i = 0; i < 3; i++) {
        pointDistanceToPlane[i] = plane.distance(triangleVertices[i]);
        arePointsClipped.set(i, pointDistanceToPlane[i] < 0.0f);
    }

    switch (arePointsClipped.count()) {
        case 0:
            // Easy, the entire triangle is kept as is.
            *clippedTriangles = triangle;
            clippedTriangleCount = 1;
            break;

        case 1:
        {
            int clippedPointIndex = 2;
            int keptPointIndices[2] = { 0, 1 };
            glm::vec3 newVertices[2];

            // Determine which point was clipped.
            if (arePointsClipped.test(0)) {
                clippedPointIndex = 0;
                keptPointIndices[0] = 2;
            } else if (arePointsClipped.test(1)) {
                clippedPointIndex = 1;
                keptPointIndices[1] = 2;
            }
            // We have a quad now, so we need to create two triangles.
            getTrianglePlaneIntersectionPoints(triangleVertices, pointDistanceToPlane, clippedPointIndex, keptPointIndices, newVertices);
            clippedTriangles->v0 = triangleVertices[keptPointIndices[0]];
            clippedTriangles->v1 = triangleVertices[keptPointIndices[1]];
            clippedTriangles->v2 = newVertices[1];
            clippedTriangles++;
            clippedTriangleCount++;

            if (clippedTriangleCount < maxClippedTriangleCount) {
                clippedTriangles->v0 = triangleVertices[keptPointIndices[0]];
                clippedTriangles->v1 = newVertices[0];
                clippedTriangles->v2 = newVertices[1];
                clippedTriangles++;
                clippedTriangleCount++;
            }
        }
        break;

        case 2:
        {
            int keptPointIndex = 2;
            int clippedPointIndices[2] = { 0, 1 };
            glm::vec3 newVertices[2];

            // Determine which point was NOT clipped.
            if (!arePointsClipped.test(0)) {
                keptPointIndex = 0;
                clippedPointIndices[0] = 2;
            } else if (!arePointsClipped.test(1)) {
                keptPointIndex = 1;
                clippedPointIndices[1] = 2;
            }
            // We have a single triangle
            getTrianglePlaneIntersectionPoints(triangleVertices, pointDistanceToPlane, keptPointIndex, clippedPointIndices, newVertices);
            clippedTriangles->v0 = triangleVertices[keptPointIndex];
            clippedTriangles->v1 = newVertices[0];
            clippedTriangles->v2 = newVertices[1];
            clippedTriangleCount = 1;
        }
        break;

        default:
            // Entire triangle is clipped.
           break;
    }

    return clippedTriangleCount;
}

int clipTriangleWithPlanes(const Triangle& triangle, const Plane* planes, int planeCount, Triangle* clippedTriangles, int maxClippedTriangleCount) {
    auto planesEnd = planes + planeCount;
    int triangleCount = 1;
    std::vector<Triangle> trianglesToTest;

    assert(maxClippedTriangleCount > 0);

    *clippedTriangles = triangle;

    while (planes < planesEnd && triangleCount) {
        int clippedSubTriangleCount;

        trianglesToTest.clear();
        trianglesToTest.insert(trianglesToTest.begin(), clippedTriangles, clippedTriangles + triangleCount);
        triangleCount = 0;

        for (const auto& triangleToTest : trianglesToTest) {
            clippedSubTriangleCount = clipTriangleWithPlane(triangleToTest, *planes,
                                                            clippedTriangles + triangleCount, maxClippedTriangleCount - triangleCount);
            triangleCount += clippedSubTriangleCount;
            if (triangleCount >= maxClippedTriangleCount) {
                return triangleCount;
            }
        }
        ++planes;
    }
    return triangleCount;
}

// Do line segments (r1p1.x, r1p1.y)--(r1p2.x, r1p2.y) and (r2p1.x, r2p1.y)--(r2p2.x, r2p2.y) intersect?
// from: http://ptspts.blogspot.com/2010/06/how-to-determine-if-two-line-segments.html
bool doLineSegmentsIntersect(glm::vec2 r1p1, glm::vec2 r1p2, glm::vec2 r2p1, glm::vec2 r2p2) {
  int d1 = computeDirection(r2p1.x, r2p1.y, r2p2.x, r2p2.y, r1p1.x, r1p1.y);
  int d2 = computeDirection(r2p1.x, r2p1.y, r2p2.x, r2p2.y, r1p2.x, r1p2.y);
  int d3 = computeDirection(r1p1.x, r1p1.y, r1p2.x, r1p2.y, r2p1.x, r2p1.y);
  int d4 = computeDirection(r1p1.x, r1p1.y, r1p2.x, r1p2.y, r2p2.x, r2p2.y);
  return (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
          ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) ||
         (d1 == 0 && isOnSegment(r2p1.x, r2p1.y, r2p2.x, r2p2.y, r1p1.x, r1p1.y)) ||
         (d2 == 0 && isOnSegment(r2p1.x, r2p1.y, r2p2.x, r2p2.y, r1p2.x, r1p2.y)) ||
         (d3 == 0 && isOnSegment(r1p1.x, r1p1.y, r1p2.x, r1p2.y, r2p1.x, r2p1.y)) ||
         (d4 == 0 && isOnSegment(r1p1.x, r1p1.y, r1p2.x, r1p2.y, r2p2.x, r2p2.y));
}

bool findClosestApproachOfLines(glm::vec3 p1, glm::vec3 d1, glm::vec3 p2, glm::vec3 d2,
                                // return values...
                                float& t1, float& t2) {
    // https://math.stackexchange.com/questions/1993953/closest-points-between-two-lines/1993990#1993990
    // https://en.wikipedia.org/wiki/Skew_lines#Nearest_Points
    glm::vec3 n1 = glm::cross(d1, glm::cross(d2, d1));
    glm::vec3 n2 = glm::cross(d2, glm::cross(d1, d2));

    float denom1 = glm::dot(d1, n2);
    float denom2 = glm::dot(d2, n1);

    if (denom1 != 0.0f && denom2 != 0.0f) {
        t1 = glm::dot((p2 - p1), n2) / denom1;
        t2 = glm::dot((p1 - p2), n1) / denom2;
        return true;
    } else {
        t1 = 0.0f;
        t2 = 0.0f;
        return false;
    }
}

bool isOnSegment(float xi, float yi, float xj, float yj, float xk, float yk)  {
  return (xi <= xk || xj <= xk) && (xk <= xi || xk <= xj) &&
         (yi <= yk || yj <= yk) && (yk <= yi || yk <= yj);
}

int computeDirection(float xi, float yi, float xj, float yj, float xk, float yk) {
  float a = (xk - xi) * (yj - yi);
  float b = (xj - xi) * (yk - yi);
  return a < b ? -1 : a > b ? 1 : 0;
}


//
// Polygon Clipping routines inspired by, pseudo code found here: http://www.cs.rit.edu/~icss571/clipTrans/PolyClipBack.html
//
// Coverage Map's polygon coordinates are from -1 to 1 in the following mapping to screen space.
//
//         (0,0)                   (windowWidth, 0)
//         -1,1                    1,1
//           +-----------------------+
//           |           |           |
//           |           |           |
//           | -1,0      |           |
//           |-----------+-----------|
//           |          0,0          |
//           |           |           |
//           |           |           |
//           |           |           |
//           +-----------------------+
//           -1,-1                  1,-1
// (0,windowHeight)                (windowWidth,windowHeight)
//

const float PolygonClip::TOP_OF_CLIPPING_WINDOW    =  1.0f;
const float PolygonClip::BOTTOM_OF_CLIPPING_WINDOW = -1.0f;
const float PolygonClip::LEFT_OF_CLIPPING_WINDOW   = -1.0f;
const float PolygonClip::RIGHT_OF_CLIPPING_WINDOW  =  1.0f;

const glm::vec2 PolygonClip::TOP_LEFT_CLIPPING_WINDOW       ( LEFT_OF_CLIPPING_WINDOW , TOP_OF_CLIPPING_WINDOW    );
const glm::vec2 PolygonClip::TOP_RIGHT_CLIPPING_WINDOW      ( RIGHT_OF_CLIPPING_WINDOW, TOP_OF_CLIPPING_WINDOW    );
const glm::vec2 PolygonClip::BOTTOM_LEFT_CLIPPING_WINDOW    ( LEFT_OF_CLIPPING_WINDOW , BOTTOM_OF_CLIPPING_WINDOW );
const glm::vec2 PolygonClip::BOTTOM_RIGHT_CLIPPING_WINDOW   ( RIGHT_OF_CLIPPING_WINDOW, BOTTOM_OF_CLIPPING_WINDOW );

void PolygonClip::clipToScreen(const glm::vec2* inputVertexArray, int inLength, glm::vec2*& outputVertexArray, int& outLength) {
    int tempLengthA = inLength;
    int tempLengthB;
    int maxLength = inLength * 2;
    glm::vec2* tempVertexArrayA = new glm::vec2[maxLength];
    glm::vec2* tempVertexArrayB = new glm::vec2[maxLength];

    // set up our temporary arrays
    for (int i=0; i<inLength; i++) {
        tempVertexArrayA[i] = inputVertexArray[i];
    }

    // Left edge
    LineSegment2 edge;
    edge[0] = TOP_LEFT_CLIPPING_WINDOW;
    edge[1] = BOTTOM_LEFT_CLIPPING_WINDOW;
    // clip the array from tempVertexArrayA and copy end result to tempVertexArrayB
    sutherlandHodgmanPolygonClip(tempVertexArrayA, tempVertexArrayB, tempLengthA, tempLengthB, edge);
    // clean the array from tempVertexArrayA and copy cleaned result to tempVertexArrayA
    copyCleanArray(tempLengthA, tempVertexArrayA, tempLengthB, tempVertexArrayB);

    // Bottom Edge
    edge[0] = BOTTOM_LEFT_CLIPPING_WINDOW;
    edge[1] = BOTTOM_RIGHT_CLIPPING_WINDOW;
    // clip the array from tempVertexArrayA and copy end result to tempVertexArrayB
    sutherlandHodgmanPolygonClip(tempVertexArrayA, tempVertexArrayB, tempLengthA, tempLengthB, edge);
    // clean the array from tempVertexArrayA and copy cleaned result to tempVertexArrayA
    copyCleanArray(tempLengthA, tempVertexArrayA, tempLengthB, tempVertexArrayB);

    // Right Edge
    edge[0] = BOTTOM_RIGHT_CLIPPING_WINDOW;
    edge[1] = TOP_RIGHT_CLIPPING_WINDOW;
    // clip the array from tempVertexArrayA and copy end result to tempVertexArrayB
    sutherlandHodgmanPolygonClip(tempVertexArrayA, tempVertexArrayB, tempLengthA, tempLengthB, edge);
    // clean the array from tempVertexArrayA and copy cleaned result to tempVertexArrayA
    copyCleanArray(tempLengthA, tempVertexArrayA, tempLengthB, tempVertexArrayB);

    // Top Edge
    edge[0] = TOP_RIGHT_CLIPPING_WINDOW;
    edge[1] = TOP_LEFT_CLIPPING_WINDOW;
    // clip the array from tempVertexArrayA and copy end result to tempVertexArrayB
    sutherlandHodgmanPolygonClip(tempVertexArrayA, tempVertexArrayB, tempLengthA, tempLengthB, edge);
    // clean the array from tempVertexArrayA and copy cleaned result to tempVertexArrayA
    copyCleanArray(tempLengthA, tempVertexArrayA, tempLengthB, tempVertexArrayB);

    // copy final output to outputVertexArray
    outputVertexArray = tempVertexArrayA;
    outLength         = tempLengthA;

    // cleanup our unused temporary buffer...
    delete[] tempVertexArrayB;

    // Note: we don't delete tempVertexArrayA, because that's the caller's responsibility
}

void PolygonClip::sutherlandHodgmanPolygonClip(glm::vec2* inVertexArray, glm::vec2* outVertexArray,
                                               int inLength, int& outLength,  const LineSegment2& clipBoundary) {
    glm::vec2 start, end; // Start, end point of current polygon edge
    glm::vec2 intersection;   // Intersection point with a clip boundary

    outLength = 0;
    start = inVertexArray[inLength - 1]; // Start with the last vertex in inVertexArray
    for (int j = 0; j < inLength; j++) {
        end = inVertexArray[j]; // Now start and end correspond to the vertices

        // Cases 1 and 4 - the endpoint is inside the boundary
        if (pointInsideBoundary(end,clipBoundary)) {
            // Case 1 - Both inside
            if (pointInsideBoundary(start, clipBoundary)) {
                appendPoint(end, outLength, outVertexArray);
            } else { // Case 4 - end is inside, but start is outside
                segmentIntersectsBoundary(start, end, clipBoundary, intersection);
                appendPoint(intersection, outLength, outVertexArray);
                appendPoint(end, outLength, outVertexArray);
            }
        } else { // Cases 2 and 3 - end is outside
            if (pointInsideBoundary(start, clipBoundary))  {
                // Cases 2 - start is inside, end is outside
                segmentIntersectsBoundary(start, end, clipBoundary, intersection);
                appendPoint(intersection, outLength, outVertexArray);
            } else {
                // Case 3 - both are outside, No action
            }
       }
       start = end;  // Advance to next pair of vertices
    }
}

bool PolygonClip::pointInsideBoundary(const glm::vec2& testVertex, const LineSegment2& clipBoundary) {
    // bottom edge
    if (clipBoundary[1].x > clipBoundary[0].x) {
        if (testVertex.y >= clipBoundary[0].y) {
            return true;
        }
    }
    // top edge
    if (clipBoundary[1].x < clipBoundary[0].x) {
        if (testVertex.y <= clipBoundary[0].y) {
            return true;
        }
    }
    // right edge
    if (clipBoundary[1].y > clipBoundary[0].y) {
        if (testVertex.x <= clipBoundary[1].x) {
            return true;
        }
    }
    // left edge
    if (clipBoundary[1].y < clipBoundary[0].y) {
        if (testVertex.x >= clipBoundary[1].x) {
            return true;
        }
    }
    return false;
}

void PolygonClip::segmentIntersectsBoundary(const glm::vec2& first, const glm::vec2&  second,
                                           const LineSegment2& clipBoundary, glm::vec2& intersection) {
    // horizontal
    if (clipBoundary[0].y==clipBoundary[1].y) {
        intersection.y = clipBoundary[0].y;
        intersection.x = first.x + (clipBoundary[0].y - first.y) * (second.x - first.x) / (second.y - first.y);
    } else { // Vertical
        intersection.x = clipBoundary[0].x;
        intersection.y = first.y + (clipBoundary[0].x - first.x) * (second.y - first.y) / (second.x - first.x);
    }
}

void PolygonClip::appendPoint(glm::vec2 newVertex, int& outLength, glm::vec2* outVertexArray) {
    outVertexArray[outLength].x = newVertex.x;
    outVertexArray[outLength].y = newVertex.y;
    outLength++;
}

// The copyCleanArray() function sets the resulting polygon of the previous step up to be the input polygon for next step of the
// clipping algorithm. As the Sutherland-Hodgman algorithm is a polygon clipping algorithm, it does not handle line
// clipping very well. The modification so that lines may be clipped as well as polygons is included in this function.
// when completed vertexArrayA will be ready for output and/or next step of clipping
void PolygonClip::copyCleanArray(int& lengthA, glm::vec2* vertexArrayA, int& lengthB, glm::vec2* vertexArrayB) {
    // Fix lines: they will come back with a length of 3, from an original of length of 2
    if ((lengthA == 2) && (lengthB == 3)) {
        // The first vertex should be copied as is.
        vertexArrayA[0] = vertexArrayB[0];
        // If the first two vertices of the "B" array are same, then collapse them down to be the 2nd vertex
        if (vertexArrayB[0].x == vertexArrayB[1].x)  {
            vertexArrayA[1] = vertexArrayB[2];
        } else {
            // Otherwise the first vertex should be the same as third vertex
            vertexArrayA[1] = vertexArrayB[1];
        }
        lengthA=2;
    } else {
        // for all other polygons, then just copy the vertexArrayB to vertextArrayA for next step
        lengthA = lengthB;
        for (int i = 0; i < lengthB; i++) {
            vertexArrayA[i] = vertexArrayB[i];
        }
    }
}

bool findRayRectangleIntersection(const glm::vec3& origin, const glm::vec3& direction, const glm::quat& rotation,
        const glm::vec3& position, const glm::vec2& dimensions, float& distance) {
    const glm::vec3 UNROTATED_NORMAL(0.0f, 0.0f, -1.0f);
    glm::vec3 normal = rotation * UNROTATED_NORMAL;

    bool maybeIntersects = false;
    float denominator = glm::dot(normal, direction);
    glm::vec3 offset = origin - position;
    float normDotOffset = glm::dot(offset, normal);
    float d = 0.0f;
    if (fabsf(denominator) < EPSILON) {
        // line is perpendicular to plane
        if (fabsf(normDotOffset) < EPSILON) {
            // ray starts on the plane
            maybeIntersects = true;

            // compute distance to closest approach
            d = - glm::dot(offset, direction);  // distance to closest approach of center of rectangle
            if (d < 0.0f) {
                // ray points away from center of rectangle, so ray's start is the closest approach
                d = 0.0f;
            }
        }
    } else {
        d = - normDotOffset / denominator;
        if (d > 0.0f) {
            // ray points toward plane
            maybeIntersects = true;
        }
    }

    if (maybeIntersects) {
        glm::vec3 hitPosition = origin + (d * direction);
        glm::vec3 localHitPosition = glm::inverse(rotation) * (hitPosition - position);
        glm::vec2 halfDimensions = 0.5f * dimensions;
        if (fabsf(localHitPosition.x) < halfDimensions.x && fabsf(localHitPosition.y) < halfDimensions.y) {
            // only update distance on intersection
            distance = d;
            return true;
        }
    }

    return false;
}

// determines whether a value is within the extents
bool isWithin(float value, float corner, float size) {
    return value >= corner && value <= corner + size;
}

bool aaBoxContains(const glm::vec3& point, const glm::vec3& corner, const glm::vec3& scale) {
    return isWithin(point.x, corner.x, scale.x) &&
        isWithin(point.y, corner.y, scale.y) &&
        isWithin(point.z, corner.z, scale.z);
}

void checkPossibleParabolicIntersectionWithZPlane(float t, float& minDistance,
    const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration, const glm::vec2& corner, const glm::vec2& scale) {
    if (t < minDistance && t > 0.0f &&
        isWithin(origin.x + velocity.x * t + 0.5f * acceleration.x * t * t, corner.x, scale.x) &&
        isWithin(origin.y + velocity.y * t + 0.5f * acceleration.y * t * t, corner.y, scale.y)) {
        minDistance = t;
    }
}

// Intersect with the plane z = 0 and make sure the intersection is within dimensions
bool findParabolaRectangleIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
    const glm::vec2& dimensions, float& parabolicDistance) {
    glm::vec2 localCorner = -0.5f * dimensions;

    float minDistance = FLT_MAX;
    if (fabsf(acceleration.z) < EPSILON) {
        if (fabsf(velocity.z) > EPSILON) {
            // Handle the degenerate case where we only have a line in the z-axis
            float possibleDistance = -origin.z / velocity.z;
            checkPossibleParabolicIntersectionWithZPlane(possibleDistance, minDistance, origin, velocity, acceleration, localCorner, dimensions);
        }
    } else {
        float a = 0.5f * acceleration.z;
        float b = velocity.z;
        float c = origin.z;
        glm::vec2 possibleDistances = { FLT_MAX, FLT_MAX };
        if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
            for (int i = 0; i < 2; i++) {
                checkPossibleParabolicIntersectionWithZPlane(possibleDistances[i], minDistance, origin, velocity, acceleration, localCorner, dimensions);
            }
        }
    }
    if (minDistance < FLT_MAX) {
        parabolicDistance = minDistance;
        return true;
    }
    return false;
}

bool findParabolaSphereIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                    const glm::vec3& center, float radius, float& parabolicDistance) {
    glm::vec3 localCenter = center - origin;
    float radiusSquared = radius * radius;

    float accelerationLength = glm::length(acceleration);
    float minDistance = FLT_MAX;

    if (accelerationLength < EPSILON) {
        // Handle the degenerate case where acceleration == (0, 0, 0)
        glm::vec3 offset = origin - center;
        float a = glm::dot(velocity, velocity);
        float b = 2.0f * glm::dot(velocity, offset);
        float c = glm::dot(offset, offset) - radius * radius;
        glm::vec2 possibleDistances(FLT_MAX);
        if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
            for (int i = 0; i < 2; i++) {
                if (possibleDistances[i] < minDistance && possibleDistances[i] > 0.0f) {
                    minDistance = possibleDistances[i];
                }
            }
        }
    } else {
        glm::vec3 vectorOnPlane = velocity;
        if (fabsf(glm::dot(glm::normalize(velocity), glm::normalize(acceleration))) > 1.0f - EPSILON) {
            // Handle the degenerate case where velocity is parallel to acceleration
            // We pick t = 1 and calculate a second point on the plane
            vectorOnPlane = velocity + 0.5f * acceleration;
        }
        // Get the normal of the plane, the cross product of two vectors on the plane
        glm::vec3 normal = glm::normalize(glm::cross(vectorOnPlane, acceleration));

        // Project vector from plane to sphere center onto the normal
        float distance = glm::dot(localCenter, normal);
        // Exit early if the sphere doesn't intersect the plane defined by the parabola
        if (fabsf(distance) > radius) {
            return false;
        }

        glm::vec3 circleCenter = center - distance * normal;
        float circleRadius = sqrtf(radiusSquared - distance * distance);
        glm::vec3 q = glm::normalize(acceleration);
        glm::vec3 p = glm::cross(normal, q);

        float a1 = accelerationLength * 0.5f;
        float b1 = glm::dot(velocity, q);
        float c1 = glm::dot(origin - circleCenter, q);
        float a2 = glm::dot(velocity, p);
        float b2 = glm::dot(origin - circleCenter, p);

        float a = a1 * a1;
        float b = 2.0f * a1 * b1;
        float c = 2.0f * a1 * c1 + b1 * b1 + a2 * a2;
        float d = 2.0f * b1 * c1 + 2.0f * a2 * b2;
        float e = c1 * c1 + b2 * b2 - circleRadius * circleRadius;

        glm::vec4 possibleDistances(FLT_MAX);
        if (computeRealQuarticRoots(a, b, c, d, e, possibleDistances)) {
            for (int i = 0; i < 4; i++) {
                if (possibleDistances[i] < minDistance && possibleDistances[i] > 0.0f) {
                    minDistance = possibleDistances[i];
                }
            }
        }
    }

    if (minDistance < FLT_MAX) {
        parabolicDistance = minDistance;
        return true;
    }
    return false;
}

void checkPossibleParabolicIntersectionWithTriangle(float t, float& minDistance,
    const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
    const glm::vec3& localVelocity, const glm::vec3& localAcceleration, const glm::vec3& normal,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, bool allowBackface) {
    // Check if we're hitting the backface in the rotated coordinate space
    float localIntersectionVelocityZ = localVelocity.z + localAcceleration.z * t;
    if (!allowBackface && localIntersectionVelocityZ < 0.0f) {
        return;
    }

    // Check that the point is within all three sides
    glm::vec3 point = origin + velocity * t + 0.5f * acceleration * t * t;
    if (t < minDistance && t > 0.0f &&
        glm::dot(normal, glm::cross(point - v1, v0 - v1)) > 0.0f &&
        glm::dot(normal, glm::cross(v2 - v1, point - v1)) > 0.0f &&
        glm::dot(normal, glm::cross(point - v0, v2 - v0)) > 0.0f) {
        minDistance = t;
    }
}

bool findParabolaTriangleIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& parabolicDistance, bool allowBackface) {
    glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v0 - v1));

    // We transform the parabola and triangle so that the triangle is in the plane z = 0, with v0 at the origin
    glm::quat inverseRot;
    // Note: OpenGL view matrix is already the inverse of our camera matrix
    // if the direction is nearly aligned with the Y axis, then use the X axis for 'up'
    const float MAX_ABS_Y_COMPONENT = 0.9999991f;
    if (fabsf(normal.y) > MAX_ABS_Y_COMPONENT) {
        inverseRot = glm::quat_cast(glm::lookAt(glm::vec3(0.0f), normal, Vectors::UNIT_X));
    } else {
        inverseRot = glm::quat_cast(glm::lookAt(glm::vec3(0.0f), normal, Vectors::UNIT_Y));
    }

    glm::vec3 localOrigin = inverseRot * (origin - v0);
    glm::vec3 localVelocity = inverseRot * velocity;
    glm::vec3 localAcceleration = inverseRot * acceleration;

    float minDistance = FLT_MAX;
    if (fabsf(localAcceleration.z) < EPSILON) {
        if (fabsf(localVelocity.z) > EPSILON) {
            float possibleDistance = -localOrigin.z / localVelocity.z;
            checkPossibleParabolicIntersectionWithTriangle(possibleDistance, minDistance, origin, velocity, acceleration,
                localVelocity, localAcceleration, normal, v0, v1, v2, allowBackface);
        }
    } else {
        float a = 0.5f * localAcceleration.z;
        float b = localVelocity.z;
        float c = localOrigin.z;
        glm::vec2 possibleDistances = { FLT_MAX, FLT_MAX };
        if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
            for (int i = 0; i < 2; i++) {
                checkPossibleParabolicIntersectionWithTriangle(possibleDistances[i], minDistance, origin, velocity, acceleration,
                    localVelocity, localAcceleration, normal, v0, v1, v2, allowBackface);
            }
        }
    }
    if (minDistance < FLT_MAX) {
        parabolicDistance = minDistance;
        return true;
    }
    return false;
}

bool findParabolaCapsuleIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
    const glm::vec3& start, const glm::vec3& end, float radius, const glm::quat& rotation, float& parabolicDistance) {
    if (start == end) {
        return findParabolaSphereIntersection(origin, velocity, acceleration, start, radius, parabolicDistance); // handle degenerate case
    }
    if (glm::distance2(origin, start) < radius * radius) { // inside start sphere
        float startDistance;
        bool intersectsStart = findParabolaSphereIntersection(origin, velocity, acceleration, start, radius, startDistance);
        if (glm::distance2(origin, end) < radius * radius) { // also inside end sphere
            float endDistance;
            bool intersectsEnd = findParabolaSphereIntersection(origin, velocity, acceleration, end, radius, endDistance);
            if (endDistance < startDistance) {
                parabolicDistance = endDistance;
                return intersectsEnd;
            }
        }
        parabolicDistance = startDistance;
        return intersectsStart;
    } else if (glm::distance2(origin, end) < radius * radius) { // inside end sphere (and not start sphere)
        return findParabolaSphereIntersection(origin, velocity, acceleration, end, radius, parabolicDistance);
    }

    // We are either inside the middle of the capsule or outside it completely
    // Either way, we need to check all three parts of the capsule and find the closest intersection
    glm::vec3 results(FLT_MAX);
    findParabolaSphereIntersection(origin, velocity, acceleration, start, radius, results[0]);
    findParabolaSphereIntersection(origin, velocity, acceleration, end, radius, results[1]);

    // We rotate the infinite cylinder to be aligned with the y-axis and then cap the values at the end
    glm::quat inverseRot = glm::inverse(rotation);
    glm::vec3 localOrigin = inverseRot * (origin - start);
    glm::vec3 localVelocity = inverseRot * velocity;
    glm::vec3 localAcceleration = inverseRot * acceleration;
    float capsuleLength = glm::length(end - start);

    const float MIN_ACCELERATION_PRODUCT = 0.00001f;
    if (fabsf(localAcceleration.x * localAcceleration.z) < MIN_ACCELERATION_PRODUCT) {
        // Handle the degenerate case where we only have a line in the XZ plane
        float a = localVelocity.x * localVelocity.x + localVelocity.z * localVelocity.z;
        float b = 2.0f * (localVelocity.x * localOrigin.x + localVelocity.z * localOrigin.z);
        float c = localOrigin.x * localOrigin.x + localOrigin.z * localOrigin.z - radius * radius;
        glm::vec2 possibleDistances = { FLT_MAX, FLT_MAX };
        if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
            for (int i = 0; i < 2; i++) {
                if (possibleDistances[i] < results[2] && possibleDistances[i] > 0.0f) {
                    float y = localOrigin.y + localVelocity.y * possibleDistances[i] + 0.5f * localAcceleration.y * possibleDistances[i] * possibleDistances[i];
                    if (y > 0.0f && y < capsuleLength) {
                        results[2] = possibleDistances[i];
                    }
                }
            }
        }
    } else {
        float a = 0.25f * (localAcceleration.x * localAcceleration.x + localAcceleration.z * localAcceleration.z);
        float b = localVelocity.x * localAcceleration.x + localVelocity.z * localAcceleration.z;
        float c = localOrigin.x * localAcceleration.x + localOrigin.z * localAcceleration.z + localVelocity.x * localVelocity.x + localVelocity.z * localVelocity.z;
        float d = 2.0f * (localOrigin.x * localVelocity.x + localOrigin.z * localVelocity.z);
        float e = localOrigin.x * localOrigin.x + localOrigin.z * localOrigin.z - radius * radius;
        glm::vec4 possibleDistances(FLT_MAX);
        if (computeRealQuarticRoots(a, b, c, d, e, possibleDistances)) {
            for (int i = 0; i < 4; i++) {
                if (possibleDistances[i] < results[2] && possibleDistances[i] > 0.0f) {
                    float y = localOrigin.y + localVelocity.y * possibleDistances[i] + 0.5f * localAcceleration.y * possibleDistances[i] * possibleDistances[i];
                    if (y > 0.0f && y < capsuleLength) {
                        results[2] = possibleDistances[i];
                    }
                }
            }
        }
    }

    float minDistance = FLT_MAX;
    for (int i = 0; i < 3; i++) {
        minDistance = glm::min(minDistance, results[i]);
    }
    parabolicDistance = minDistance;
    return minDistance != FLT_MAX;
}

void checkPossibleParabolicIntersection(float t, int i, float& minDistance, const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                        const glm::vec3& corner, const glm::vec3& scale, bool& hit) {
    if (t < minDistance && t > 0.0f &&
        isWithin(origin[(i + 1) % 3] + velocity[(i + 1) % 3] * t + 0.5f * acceleration[(i + 1) % 3] * t * t, corner[(i + 1) % 3], scale[(i + 1) % 3]) &&
        isWithin(origin[(i + 2) % 3] + velocity[(i + 2) % 3] * t + 0.5f * acceleration[(i + 2) % 3] * t * t, corner[(i + 2) % 3], scale[(i + 2) % 3])) {
        minDistance = t;
        hit = true;
    }
}

inline float parabolaVelocityAtT(float velocity, float acceleration, float t) {
    return velocity + acceleration * t;
}

bool findParabolaAABoxIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                   const glm::vec3& corner, const glm::vec3& scale, float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal) {
    float minDistance = FLT_MAX;
    BoxFace minFace = UNKNOWN_FACE;
    glm::vec3 minNormal;
    glm::vec2 possibleDistances;
    float a, b, c;

    // Solve the intersection for each face of the cube.  As we go, keep track of the smallest, positive, real distance
    // that is within the bounds of the other two dimensions
    for (int i = 0; i < 3; i++) {
        if (fabsf(acceleration[i]) < EPSILON) {
            // Handle the degenerate case where we only have a line in this axis
            if (origin[i] < corner[i]) {
                { // min
                    if (velocity[i] > 0.0f) {
                        float possibleDistance = (corner[i] - origin[i]) / velocity[i];
                        bool hit = false;
                        checkPossibleParabolicIntersection(possibleDistance, i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                        if (hit) {
                            minFace = BoxFace(2 * i);
                            minNormal = glm::vec3(0.0f);
                            minNormal[i] = -1.0f;
                        }
                    }
                }
            } else if (origin[i] > corner[i] + scale[i]) {
                { // max
                    if (velocity[i] < 0.0f) {
                        float possibleDistance = (corner[i] + scale[i] - origin[i]) / velocity[i];
                        bool hit = false;
                        checkPossibleParabolicIntersection(possibleDistance, i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                        if (hit) {
                            minFace = BoxFace(2 * i + 1);
                            minNormal = glm::vec3(0.0f);
                            minNormal[i] = 1.0f;
                        }
                    }
                }
            } else {
                { // min
                    if (velocity[i] < 0.0f) {
                        float possibleDistance = (corner[i] - origin[i]) / velocity[i];
                        bool hit = false;
                        checkPossibleParabolicIntersection(possibleDistance, i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                        if (hit) {
                            minFace = BoxFace(2 * i + 1);
                            minNormal = glm::vec3(0.0f);
                            minNormal[i] = 1.0f;
                        }
                    }
                }
                { // max
                    if (velocity[i] > 0.0f) {
                        float possibleDistance = (corner[i] + scale[i] - origin[i]) / velocity[i];
                        bool hit = false;
                        checkPossibleParabolicIntersection(possibleDistance, i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                        if (hit) {
                            minFace = BoxFace(2 * i);
                            minNormal = glm::vec3(0.0f);
                            minNormal[i] = -1.0f;
                        }
                    }
                }
            }
        } else {
            a = 0.5f * acceleration[i];
            b = velocity[i];
            if (origin[i] < corner[i]) {
                // If we're below corner, we have the following cases:
                // - within bounds on other axes
                //     - if +velocity or +acceleration
                //         - can only hit MIN_FACE with -normal
                // - else
                //     - if +acceleration
                //         - can only hit MIN_FACE with -normal
                //     - else if +velocity
                //         - can hit MIN_FACE with -normal iff velocity at intersection is +
                //         - else can hit MAX_FACE with +normal iff velocity at intersection is -
                if (origin[(i + 1) % 3] > corner[(i + 1) % 3] && origin[(i + 1) % 3] < corner[(i + 1) % 3] + scale[(i + 1) % 3] &&
                    origin[(i + 2) % 3] > corner[(i + 2) % 3] && origin[(i + 2) % 3] < corner[(i + 2) % 3] + scale[(i + 2) % 3]) {
                    if (velocity[i] > 0.0f || acceleration[i] > 0.0f) {
                        { // min
                            c = origin[i] - corner[i];
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                bool hit = false;
                                for (int j = 0; j < 2; j++) {
                                    checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = -1.0f;
                                }
                            }
                        }
                    }
                } else {
                    if (acceleration[i] > 0.0f) {
                        { // min
                            c = origin[i] - corner[i];
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                bool hit = false;
                                for (int j = 0; j < 2; j++) {
                                    checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = -1.0f;
                                }
                            }
                        }
                    } else if (velocity[i] > 0.0f) {
                        bool hit = false;
                        { // min
                            c = origin[i] - corner[i];
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                for (int j = 0; j < 2; j++) {
                                    if (parabolaVelocityAtT(velocity[i], acceleration[i], possibleDistances[j]) > 0.0f) {
                                        checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                    }
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = -1.0f;
                                }
                            }
                        }
                        if (!hit) { // max
                            c = origin[i] - (corner[i] + scale[i]);
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                for (int j = 0; j < 2; j++) {
                                    if (parabolaVelocityAtT(velocity[i], acceleration[i], possibleDistances[j]) < 0.0f) {
                                        checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                    }
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i + 1);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = 1.0f;
                                }
                            }
                        }
                    }
                }
            } else if (origin[i] > corner[i] + scale[i]) {
                // If we're above corner + scale, we have the following cases:
                // - within bounds on other axes
                //     - if -velocity or -acceleration
                //         - can only hit MAX_FACE with +normal
                // - else
                //     - if -acceleration
                //         - can only hit MAX_FACE with +normal
                //     - else if -velocity
                //         - can hit MAX_FACE with +normal iff velocity at intersection is -
                //         - else can hit MIN_FACE with -normal iff velocity at intersection is +
                if (origin[(i + 1) % 3] > corner[(i + 1) % 3] && origin[(i + 1) % 3] < corner[(i + 1) % 3] + scale[(i + 1) % 3] &&
                    origin[(i + 2) % 3] > corner[(i + 2) % 3] && origin[(i + 2) % 3] < corner[(i + 2) % 3] + scale[(i + 2) % 3]) {
                    if (velocity[i] < 0.0f || acceleration[i] < 0.0f) {
                        { // max
                            c = origin[i] - (corner[i] + scale[i]);
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                bool hit = false;
                                for (int j = 0; j < 2; j++) {
                                    checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i + 1);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = 1.0f;
                                }
                            }
                        }
                    }
                } else {
                    if (acceleration[i] < 0.0f) {
                        { // max
                            c = origin[i] - (corner[i] + scale[i]);
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                bool hit = false;
                                for (int j = 0; j < 2; j++) {
                                    checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i + 1);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = 1.0f;
                                }
                            }
                        }
                    } else if (velocity[i] < 0.0f) {
                        bool hit = false;
                        { // max
                            c = origin[i] - (corner[i] + scale[i]);
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                for (int j = 0; j < 2; j++) {
                                    if (parabolaVelocityAtT(velocity[i], acceleration[i], possibleDistances[j]) < 0.0f) {
                                        checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                    }
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i + 1);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = 1.0f;
                                }
                            }
                        }
                        if (!hit) { // min
                            c = origin[i] - corner[i];
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                for (int j = 0; j < 2; j++) {
                                    if (parabolaVelocityAtT(velocity[i], acceleration[i], possibleDistances[j]) > 0.0f) {
                                        checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                    }
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = -1.0f;
                                }
                            }
                        }
                    }
                }
            } else {
                // If we're between corner  and corner + scale, we have the following cases:
                // - within bounds on other axes
                //     - if -velocity and -acceleration
                //         - can only hit MIN_FACE with +normal
                //     - else if +velocity and +acceleration
                //         - can only hit MAX_FACE with -normal
                //     - else
                //         - can hit MIN_FACE with +normal iff velocity at intersection is -
                //         - can hit MAX_FACE with -normal iff velocity at intersection is +
                // - else
                //     - if -velocity and +acceleration
                //         - can hit MIN_FACE with -normal iff velocity at intersection is +
                //     - else if +velocity and -acceleration
                //         - can hit MAX_FACE with +normal iff velocity at intersection is -
                if (origin[(i + 1) % 3] > corner[(i + 1) % 3] && origin[(i + 1) % 3] < corner[(i + 1) % 3] + scale[(i + 1) % 3] &&
                    origin[(i + 2) % 3] > corner[(i + 2) % 3] && origin[(i + 2) % 3] < corner[(i + 2) % 3] + scale[(i + 2) % 3]) {
                    if (velocity[i] < 0.0f && acceleration[i] < 0.0f) {
                        { // min
                            c = origin[i] - corner[i];
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                bool hit = false;
                                for (int j = 0; j < 2; j++) {
                                    checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = 1.0f;
                                }
                            }
                        }
                    } else if (velocity[i] > 0.0f && acceleration[i] > 0.0f) {
                        { // max
                            c = origin[i] - (corner[i] + scale[i]);
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                bool hit = false;
                                for (int j = 0; j < 2; j++) {
                                    checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i + 1);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = -1.0f;
                                }
                            }
                        }
                    } else {
                        { // min
                            c = origin[i] - corner[i];
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                bool hit = false;
                                for (int j = 0; j < 2; j++) {
                                    if (parabolaVelocityAtT(velocity[i], acceleration[i], possibleDistances[j]) < 0.0f) {
                                        checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                    }
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = 1.0f;
                                }
                            }
                        }
                        { // max
                            c = origin[i] - (corner[i] + scale[i]);
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                bool hit = false;
                                for (int j = 0; j < 2; j++) {
                                    if (parabolaVelocityAtT(velocity[i], acceleration[i], possibleDistances[j]) > 0.0f) {
                                        checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                    }
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i + 1);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = -1.0f;
                                }
                            }
                        }
                    }
                } else {
                    if (velocity[i] < 0.0f && acceleration[i] > 0.0f) {
                        { // min
                            c = origin[i] - corner[i];
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                bool hit = false;
                                for (int j = 0; j < 2; j++) {
                                    if (parabolaVelocityAtT(velocity[i], acceleration[i], possibleDistances[j]) > 0.0f) {
                                        checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                    }
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = -1.0f;
                                }
                            }
                        }
                    } else if (velocity[i] > 0.0f && acceleration[i] < 0.0f) {
                        { // max
                            c = origin[i] - (corner[i] + scale[i]);
                            possibleDistances = { FLT_MAX, FLT_MAX };
                            if (computeRealQuadraticRoots(a, b, c, possibleDistances)) {
                                bool hit = false;
                                for (int j = 0; j < 2; j++) {
                                    if (parabolaVelocityAtT(velocity[i], acceleration[i], possibleDistances[j]) < 0.0f) {
                                        checkPossibleParabolicIntersection(possibleDistances[j], i, minDistance, origin, velocity, acceleration, corner, scale, hit);
                                    }
                                }
                                if (hit) {
                                    minFace = BoxFace(2 * i + 1);
                                    minNormal = glm::vec3(0.0f);
                                    minNormal[i] = 1.0f;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (minDistance < FLT_MAX) {
        parabolicDistance = minDistance;
        face = minFace;
        surfaceNormal = minNormal;
        return true;
    }
    return false;
}

void swingTwistDecomposition(const glm::quat& rotation,
        const glm::vec3& direction,
        glm::quat& swing,
        glm::quat& twist) {
    // direction MUST be normalized else the decomposition will be inaccurate
    assert(fabsf(glm::length2(direction) - 1.0f) < 1.0e-4f); 

    // the twist part has an axis (imaginary component) that is parallel to direction argument
    glm::vec3 axisOfRotation(rotation.x, rotation.y, rotation.z);
    glm::vec3 twistImaginaryPart = glm::dot(direction, axisOfRotation) * direction;
    // and a real component that is relatively proportional to rotation's real component
    twist = glm::normalize(glm::quat(rotation.w, twistImaginaryPart.x, twistImaginaryPart.y, twistImaginaryPart.z));

    // once twist is known we can solve for swing:
    // rotation = swing * twist  -->  swing = rotation * invTwist
    swing = rotation * glm::inverse(twist);
}

// calculate the minimum angle between a point and a sphere.
float coneSphereAngle(const glm::vec3& coneCenter, const glm::vec3& coneDirection, const glm::vec3& sphereCenter, float sphereRadius) {
    glm::vec3 d = sphereCenter - coneCenter;
    float dLen = glm::length(d);

    // theta is the angle between the coneDirection normal and the center of the sphere.
    float theta = acosf(glm::dot(d, coneDirection) / dLen);

    // phi is the deflection angle from the center of the sphere to a point tangent to the sphere.
    float phi = atanf(sphereRadius / dLen);

    return glm::max(0.0f, theta - phi);
}

// given a set of points, compute a best fit plane that passes as close as possible through all the points.
// http://www.ilikebigbits.com/blog/2015/3/2/plane-from-points
bool findPlaneFromPoints(const glm::vec3* points, size_t numPoints, glm::vec3& planeNormalOut, glm::vec3& pointOnPlaneOut) {
    if (numPoints < 3) {
        return false;
    }
    glm::vec3 sum;
    for (size_t i = 0; i < numPoints; i++) {
        sum += points[i];
    }
    glm::vec3 centroid = sum * (1.0f / (float)numPoints);
    float xx = 0.0f, xy = 0.0f, xz = 0.0f;
    float yy = 0.0f, yz = 0.0f, zz = 0.0f;

    for (size_t i = 0; i < numPoints; i++) {
        glm::vec3 r = points[i] - centroid;
        xx += r.x * r.x;
        xy += r.x * r.y;
        xz += r.x * r.z;
        yy += r.y * r.y;
        yz += r.y * r.z;
        zz += r.z * r.z;
    }

    float det_x = yy * zz - yz * yz;
    float det_y = xx * zz - xz * xz;
    float det_z = xx * yy - xy * xy;
    float det_max = std::max(std::max(det_x, det_y), det_z);

    if (det_max == 0.0f) {
        return false; // The points don't span a plane
    }

    glm::vec3 dir;
    if (det_max == det_x) {
        float a = (xz * yz - xy * zz) / det_x;
        float b = (xy * yz - xz * yy) / det_x;
        dir = glm::vec3(1.0f, a, b);
    } else if (det_max == det_y) {
        float a = (yz * xz - xy * zz) / det_y;
        float b = (xy * xz - yz * xx) / det_y;
        dir = glm::vec3(a, 1.0f, b);
    } else {
        float a = (yz * xy - xz * yy) / det_z;
        float b = (xz * xy - yz * xx) / det_z;
        dir = glm::vec3(a, b, 1.0f);
    }
    pointOnPlaneOut = centroid;
    planeNormalOut = glm::normalize(dir);
    return true;
}

bool findIntersectionOfThreePlanes(const glm::vec4& planeA, const glm::vec4& planeB, const glm::vec4& planeC, glm::vec3& intersectionPointOut) {
    glm::vec3 normalA(planeA);
    glm::vec3 normalB(planeB);
    glm::vec3 normalC(planeC);
    glm::vec3 u = glm::cross(normalB, normalC);
    float denom = glm::dot(normalA, u);
    if (fabsf(denom) < EPSILON) {
        return false;  // planes do not intersect in a point.
    } else {
        intersectionPointOut = (planeA.w * u + glm::cross(normalA, planeC.w * normalB - planeB.w * normalC)) / denom;
        return true;
    }
}

const float INV_SQRT_3 = 1.0f / sqrtf(3.0f);
const int DOP14_COUNT = 14;
const glm::vec3 DOP14_NORMALS[DOP14_COUNT] = {
    Vectors::UNIT_X,
    -Vectors::UNIT_X,
    Vectors::UNIT_Y,
    -Vectors::UNIT_Y,
    Vectors::UNIT_Z,
    -Vectors::UNIT_Z,
    glm::vec3(INV_SQRT_3, INV_SQRT_3, INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, INV_SQRT_3, INV_SQRT_3),
    glm::vec3(INV_SQRT_3, -INV_SQRT_3, INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, -INV_SQRT_3, INV_SQRT_3),
    glm::vec3(INV_SQRT_3, INV_SQRT_3, -INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, INV_SQRT_3, -INV_SQRT_3),
    glm::vec3(INV_SQRT_3, -INV_SQRT_3, -INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, -INV_SQRT_3, -INV_SQRT_3)
};

typedef std::tuple<int, int, int> Int3Tuple;
const std::tuple<int, int, int> DOP14_PLANE_COMBINATIONS[] = {
    Int3Tuple(0, 2, 4),  Int3Tuple(0, 2, 5),  Int3Tuple(0, 2, 6),  Int3Tuple(0, 2, 7),  Int3Tuple(0, 2, 8),  Int3Tuple(0, 2, 9),  Int3Tuple(0, 2, 10),  Int3Tuple(0, 2, 11),  Int3Tuple(0, 2, 12),  Int3Tuple(0, 2, 13),
    Int3Tuple(0, 3, 4),  Int3Tuple(0, 3, 5),  Int3Tuple(0, 3, 6),  Int3Tuple(0, 3, 7),  Int3Tuple(0, 3, 8),  Int3Tuple(0, 3, 9),  Int3Tuple(0, 3, 10),  Int3Tuple(0, 3, 11),  Int3Tuple(0, 3, 12),  Int3Tuple(0, 3, 13),
    Int3Tuple(0, 4, 6),  Int3Tuple(0, 4, 7),  Int3Tuple(0, 4, 8),  Int3Tuple(0, 4, 9),  Int3Tuple(0, 4, 10),  Int3Tuple(0, 4, 11),  Int3Tuple(0, 4, 12),  Int3Tuple(0, 4, 13),
    Int3Tuple(0, 5, 6),  Int3Tuple(0, 5, 7),  Int3Tuple(0, 5, 8),  Int3Tuple(0, 5, 9),  Int3Tuple(0, 5, 10),  Int3Tuple(0, 5, 11),  Int3Tuple(0, 5, 12),  Int3Tuple(0, 5, 13),
    Int3Tuple(0, 6, 8),  Int3Tuple(0, 6, 9),  Int3Tuple(0, 6, 10),  Int3Tuple(0, 6, 11),  Int3Tuple(0, 6, 12),  Int3Tuple(0, 6, 13),
    Int3Tuple(0, 7, 8),  Int3Tuple(0, 7, 9),  Int3Tuple(0, 7, 10),  Int3Tuple(0, 7, 11),  Int3Tuple(0, 7, 12),  Int3Tuple(0, 7, 13),
    Int3Tuple(0, 8, 10),  Int3Tuple(0, 8, 11),  Int3Tuple(0, 8, 12),  Int3Tuple(0, 8, 13),  Int3Tuple(0, 9, 10),
    Int3Tuple(0, 9, 11),  Int3Tuple(0, 9, 12),  Int3Tuple(0, 9, 13),
    Int3Tuple(0, 10, 12),  Int3Tuple(0, 10, 13),
    Int3Tuple(0, 11, 12),  Int3Tuple(0, 11, 13),
    Int3Tuple(1, 2, 4),  Int3Tuple(1, 2, 5),  Int3Tuple(1, 2, 6),  Int3Tuple(1, 2, 7),  Int3Tuple(1, 2, 8),  Int3Tuple(1, 2, 9),  Int3Tuple(1, 2, 10),  Int3Tuple(1, 2, 11),  Int3Tuple(1, 2, 12),  Int3Tuple(1, 2, 13),
    Int3Tuple(1, 3, 4),  Int3Tuple(1, 3, 5),  Int3Tuple(1, 3, 6),  Int3Tuple(1, 3, 7),  Int3Tuple(1, 3, 8),  Int3Tuple(1, 3, 9),  Int3Tuple(1, 3, 10),  Int3Tuple(1, 3, 11),  Int3Tuple(1, 3, 12),  Int3Tuple(1, 3, 13),
    Int3Tuple(1, 4, 6),  Int3Tuple(1, 4, 7),  Int3Tuple(1, 4, 8),  Int3Tuple(1, 4, 9),  Int3Tuple(1, 4, 10),  Int3Tuple(1, 4, 11),  Int3Tuple(1, 4, 12),  Int3Tuple(1, 4, 13),
    Int3Tuple(1, 5, 6),  Int3Tuple(1, 5, 7),  Int3Tuple(1, 5, 8),  Int3Tuple(1, 5, 9),  Int3Tuple(1, 5, 10),  Int3Tuple(1, 5, 11),  Int3Tuple(1, 5, 12),  Int3Tuple(1, 5, 13),
    Int3Tuple(1, 6, 8),  Int3Tuple(1, 6, 9),  Int3Tuple(1, 6, 10),  Int3Tuple(1, 6, 11),  Int3Tuple(1, 6, 12),  Int3Tuple(1, 6, 13),
    Int3Tuple(1, 7, 8),  Int3Tuple(1, 7, 9),  Int3Tuple(1, 7, 10),  Int3Tuple(1, 7, 11),  Int3Tuple(1, 7, 12),  Int3Tuple(1, 7, 13),
    Int3Tuple(1, 8, 10),  Int3Tuple(1, 8, 11),  Int3Tuple(1, 8, 12),  Int3Tuple(1, 8, 13),
    Int3Tuple(1, 9, 10),  Int3Tuple(1, 9, 11),  Int3Tuple(1, 9, 12),  Int3Tuple(1, 9, 13),
    Int3Tuple(1, 10, 12),  Int3Tuple(1, 10, 13),
    Int3Tuple(1, 11, 12),  Int3Tuple(1, 11, 13),
    Int3Tuple(2, 4, 6),  Int3Tuple(2, 4, 7),  Int3Tuple(2, 4, 8),  Int3Tuple(2, 4, 9),  Int3Tuple(2, 4, 10),  Int3Tuple(2, 4, 11),  Int3Tuple(2, 4, 12),  Int3Tuple(2, 4, 13),
    Int3Tuple(2, 5, 6),  Int3Tuple(2, 5, 7),  Int3Tuple(2, 5, 8),  Int3Tuple(2, 5, 9),  Int3Tuple(2, 5, 10),  Int3Tuple(2, 5, 11),  Int3Tuple(2, 5, 12),  Int3Tuple(2, 5, 13),
    Int3Tuple(2, 6, 8),  Int3Tuple(2, 6, 9),  Int3Tuple(2, 6, 10),  Int3Tuple(2, 6, 11),  Int3Tuple(2, 6, 12),  Int3Tuple(2, 6, 13),
    Int3Tuple(2, 7, 8),  Int3Tuple(2, 7, 9),  Int3Tuple(2, 7, 10),  Int3Tuple(2, 7, 11),  Int3Tuple(2, 7, 12),  Int3Tuple(2, 7, 13),
    Int3Tuple(2, 8, 10),  Int3Tuple(2, 8, 11),  Int3Tuple(2, 8, 12),  Int3Tuple(2, 8, 13),
    Int3Tuple(2, 9, 10),  Int3Tuple(2, 9, 11),  Int3Tuple(2, 9, 12),  Int3Tuple(2, 9, 13),
    Int3Tuple(2, 10, 12),  Int3Tuple(2, 10, 13),
    Int3Tuple(2, 11, 12),  Int3Tuple(2, 11, 13),
    Int3Tuple(3, 4, 6),  Int3Tuple(3, 4, 7),  Int3Tuple(3, 4, 8),  Int3Tuple(3, 4, 9),  Int3Tuple(3, 4, 10),  Int3Tuple(3, 4, 11),  Int3Tuple(3, 4, 12),  Int3Tuple(3, 4, 13),
    Int3Tuple(3, 5, 6),  Int3Tuple(3, 5, 7),  Int3Tuple(3, 5, 8),  Int3Tuple(3, 5, 9),  Int3Tuple(3, 5, 10),  Int3Tuple(3, 5, 11),  Int3Tuple(3, 5, 12),  Int3Tuple(3, 5, 13),
    Int3Tuple(3, 6, 8),  Int3Tuple(3, 6, 9),  Int3Tuple(3, 6, 10),  Int3Tuple(3, 6, 11),  Int3Tuple(3, 6, 12),  Int3Tuple(3, 6, 13),
    Int3Tuple(3, 7, 8),  Int3Tuple(3, 7, 9),  Int3Tuple(3, 7, 10),  Int3Tuple(3, 7, 11),  Int3Tuple(3, 7, 12),  Int3Tuple(3, 7, 13),
    Int3Tuple(3, 8, 10),  Int3Tuple(3, 8, 11),  Int3Tuple(3, 8, 12),  Int3Tuple(3, 8, 13),
    Int3Tuple(3, 9, 10),  Int3Tuple(3, 9, 11),  Int3Tuple(3, 9, 12),  Int3Tuple(3, 9, 13),
    Int3Tuple(3, 10, 12),  Int3Tuple(3, 10, 13),
    Int3Tuple(3, 11, 12),  Int3Tuple(3, 11, 13),
    Int3Tuple(4, 6, 8),  Int3Tuple(4, 6, 9),  Int3Tuple(4, 6, 10),  Int3Tuple(4, 6, 11),  Int3Tuple(4, 6, 12),  Int3Tuple(4, 6, 13),
    Int3Tuple(4, 7, 8),  Int3Tuple(4, 7, 9),  Int3Tuple(4, 7, 10),  Int3Tuple(4, 7, 11),  Int3Tuple(4, 7, 12),  Int3Tuple(4, 7, 13),
    Int3Tuple(4, 8, 10),  Int3Tuple(4, 8, 11),  Int3Tuple(4, 8, 12),  Int3Tuple(4, 8, 13),
    Int3Tuple(4, 9, 10),  Int3Tuple(4, 9, 11),  Int3Tuple(4, 9, 12),  Int3Tuple(4, 9, 13),
    Int3Tuple(4, 10, 12),  Int3Tuple(4, 10, 13),
    Int3Tuple(4, 11, 12),  Int3Tuple(4, 11, 13),
    Int3Tuple(5, 6, 8),  Int3Tuple(5, 6, 9),  Int3Tuple(5, 6, 10),  Int3Tuple(5, 6, 11),  Int3Tuple(5, 6, 12),  Int3Tuple(5, 6, 13),
    Int3Tuple(5, 7, 8),  Int3Tuple(5, 7, 9),  Int3Tuple(5, 7, 10),  Int3Tuple(5, 7, 11),  Int3Tuple(5, 7, 12),  Int3Tuple(5, 7, 13),
    Int3Tuple(5, 8, 10),  Int3Tuple(5, 8, 11),  Int3Tuple(5, 8, 12),  Int3Tuple(5, 8, 13),
    Int3Tuple(5, 9, 10),  Int3Tuple(5, 9, 11),  Int3Tuple(5, 9, 12),  Int3Tuple(5, 9, 13),
    Int3Tuple(5, 10, 12),  Int3Tuple(5, 10, 13),
    Int3Tuple(5, 11, 12),  Int3Tuple(5, 11, 13),
    Int3Tuple(6, 8, 10),  Int3Tuple(6, 8, 11),  Int3Tuple(6, 8, 12),  Int3Tuple(6, 8, 13),
    Int3Tuple(6, 9, 10),  Int3Tuple(6, 9, 11),  Int3Tuple(6, 9, 12),  Int3Tuple(6, 9, 13),
    Int3Tuple(6, 10, 12),  Int3Tuple(6, 10, 13),
    Int3Tuple(6, 11, 12),  Int3Tuple(6, 11, 13),
    Int3Tuple(7, 8, 10),  Int3Tuple(7, 8, 11),  Int3Tuple(7, 8, 12),  Int3Tuple(7, 8, 13),
    Int3Tuple(7, 9, 10),  Int3Tuple(7, 9, 11),  Int3Tuple(7, 9, 12),  Int3Tuple(7, 9, 13),
    Int3Tuple(7, 10, 12),  Int3Tuple(7, 10, 13),
    Int3Tuple(7, 11, 12),  Int3Tuple(7, 11, 13),
    Int3Tuple(8, 10, 12),  Int3Tuple(8, 10, 13),
    Int3Tuple(8, 11, 12),  Int3Tuple(8, 11, 13),
    Int3Tuple(9, 10, 12),  Int3Tuple(9, 10, 13),
    Int3Tuple(9, 11, 12),  Int3Tuple(9, 11, 13)
};

void generateBoundryLinesForDop14(const std::vector<float>& dots, const glm::vec3& center, std::vector<glm::vec3>& linesOut) {
    if (dots.size() != DOP14_COUNT) {
        return;
    }

    // iterate over all purmutations of non-parallel planes.
    // find all the vertices that lie on the surface of the k-dop
    std::vector<glm::vec3> vertices;
    for (auto& tuple : DOP14_PLANE_COMBINATIONS) {
        int i = std::get<0>(tuple);
        int j = std::get<1>(tuple);
        int k = std::get<2>(tuple);
        glm::vec4 planeA(DOP14_NORMALS[i], dots[i]);
        glm::vec4 planeB(DOP14_NORMALS[j], dots[j]);
        glm::vec4 planeC(DOP14_NORMALS[k], dots[k]);
        glm::vec3 intersectionPoint;
        const float IN_FRONT_MARGIN = 0.01f;
        if (findIntersectionOfThreePlanes(planeA, planeB, planeC, intersectionPoint)) {
            bool inFront = false;
            for (int p = 0; p < DOP14_COUNT; p++) {
                if (glm::dot(DOP14_NORMALS[p], intersectionPoint) > dots[p] + IN_FRONT_MARGIN) {
                    inFront = true;
                }
            }
            if (!inFront) {
                vertices.push_back(intersectionPoint);
            }
        }
    }

    // build a set of lines between these vertices, that also lie on the surface of the k-dop.
    for (size_t i = 0; i < vertices.size(); i++) {
        for (size_t j = i; j < vertices.size(); j++) {
            glm::vec3 midPoint = (vertices[i] + vertices[j]) * 0.5f;
            int onSurfaceCount = 0;
            const float SURFACE_MARGIN = 0.01f;
            for (int p = 0; p < DOP14_COUNT; p++) {
                float d = glm::dot(DOP14_NORMALS[p], midPoint);
                if (d > dots[p] - SURFACE_MARGIN && d < dots[p] + SURFACE_MARGIN) {
                    onSurfaceCount++;
                }
            }
            if (onSurfaceCount > 1) {
                linesOut.push_back(vertices[i] + center);
                linesOut.push_back(vertices[j] + center);
            }
        }
    }
}

bool computeRealQuadraticRoots(float a, float b, float c, glm::vec2& roots) {
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return false;
    } else if (discriminant == 0.0f) {
        roots.x = (-b + sqrtf(discriminant)) / (2.0f * a);
    } else {
        float discriminantRoot = sqrtf(discriminant);
        roots.x = (-b + discriminantRoot) / (2.0f * a);
        roots.y = (-b - discriminantRoot) / (2.0f * a);
    }
    return true;
}

// The following functions provide an analytical solution to a quartic equation, adapted from the solution here: https://github.com/sasamil/Quartic
unsigned int solveP3(float* x, float a, float b, float c) {
    float a2 = a * a;
    float q = (a2 - 3.0f * b) / 9.0f;
    float r = (a * (2.0f * a2 - 9.0f * b) + 27.0f * c) / 54.0f;
    float r2 = r * r;
    float q3 = q * q * q;
    float A, B;
    if (r2 < q3) {
        float t = r / sqrtf(q3);
        t = glm::clamp(t, -1.0f, 1.0f);
        t = acosf(t);
        a /= 3.0f;
        q = -2.0f * sqrtf(q);
        x[0] = q * cosf(t / 3.0f) - a;
        x[1] = q * cosf((t + 2.0f * (float)M_PI) / 3.0f) - a;
        x[2] = q * cosf((t - 2.0f * (float)M_PI) / 3.0f) - a;
        return 3;
    } else {
        A = -powf(fabsf(r) + sqrtf(r2 - q3), 1.0f / 3.0f);
        if (r < 0) {
            A = -A;
        }
        B = (A == 0.0f ? 0.0f : q / A);

        a /= 3.0f;
        x[0] = (A + B) - a;
        x[1] = -0.5f * (A + B) - a;
        x[2] = 0.5f * sqrtf(3.0f) * (A - B);
        if (fabsf(x[2]) < EPSILON) {
            x[2] = x[1];
            return 2;
        }

        return 1;
    }
}

bool solve_quartic(float a, float b, float c, float d, glm::vec4& roots) {
    float a3 = -b;
    float b3 = a * c - 4.0f *d;
    float c3 = -a * a * d - c * c + 4.0f * b * d;

    float px3[3];
    unsigned int iZeroes = solveP3(px3, a3, b3, c3);

    float q1, q2, p1, p2, D, sqD, y;

    y = px3[0];
    if (iZeroes != 1) {
        if (fabsf(px3[1]) > fabsf(y)) {
            y = px3[1];
        }
        if (fabsf(px3[2]) > fabsf(y)) {
            y = px3[2];
        }
    }

    D = y * y - 4.0f * d;
    if (fabsf(D) < EPSILON) {
        q1 = q2 = 0.5f * y;
        D = a * a - 4.0f * (b - y);
        if (fabsf(D) < EPSILON) {
            p1 = p2 = 0.5f * a;
        } else {
            sqD = sqrtf(D);
            p1 = 0.5f * (a + sqD);
            p2 = 0.5f * (a - sqD);
        }
    } else {
        sqD = sqrtf(D);
        q1 = 0.5f * (y + sqD);
        q2 = 0.5f * (y - sqD);
        p1 = (a * q1 - c) / (q1 - q2);
        p2 = (c - a * q2) / (q1 - q2);
    }

    std::complex<float> x1, x2, x3, x4;
    D = p1 * p1 - 4.0f * q1;
    if (D < 0.0f) {
        x1.real(-0.5f * p1);
        x1.imag(0.5f * sqrtf(-D));
        x2 = std::conj(x1);
    } else {
        sqD = sqrtf(D);
        x1.real(0.5f * (-p1 + sqD));
        x2.real(0.5f * (-p1 - sqD));
    }

    D = p2 * p2 - 4.0f * q2;
    if (D < 0.0f) {
        x3.real(-0.5f * p2);
        x3.imag(0.5f * sqrtf(-D));
        x4 = std::conj(x3);
    } else {
        sqD = sqrtf(D);
        x3.real(0.5f * (-p2 + sqD));
        x4.real(0.5f * (-p2 - sqD));
    }

    bool hasRealRoot = false;
    if (fabsf(x1.imag()) < EPSILON) {
        roots.x = x1.real();
        hasRealRoot = true;
    }
    if (fabsf(x2.imag()) < EPSILON) {
        roots.y = x2.real();
        hasRealRoot = true;
    }
    if (fabsf(x3.imag()) < EPSILON) {
        roots.z = x3.real();
        hasRealRoot = true;
    }
    if (fabsf(x4.imag()) < EPSILON) {
        roots.w = x4.real();
        hasRealRoot = true;
    }

    return hasRealRoot;
}

bool computeRealQuarticRoots(float a, float b, float c, float d, float e, glm::vec4& roots) {
    return solve_quartic(b / a, c / a, d / a, e / a, roots);
}
