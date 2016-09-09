//
//  ViewFrustum.cpp
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <QtCore/QDebug>


#include "GeometryUtil.h"
#include "GLMHelpers.h"
#include "NumericalConstants.h"
#include "SharedLogging.h"
//#include "OctreeConstants.h"
#include "ViewFrustum.h"

using namespace std;

void ViewFrustum::setOrientation(const glm::quat& orientationAsQuaternion) {
    _orientation = orientationAsQuaternion;
    _right = glm::vec3(orientationAsQuaternion * glm::vec4(IDENTITY_RIGHT, 0.0f));
    _up = glm::vec3(orientationAsQuaternion * glm::vec4(IDENTITY_UP,    0.0f));
    _direction = glm::vec3(orientationAsQuaternion * glm::vec4(IDENTITY_FRONT, 0.0f));
    _view = glm::translate(mat4(), _position) * glm::mat4_cast(_orientation);
}

void ViewFrustum::setPosition(const glm::vec3& position) {
    _position = position;
    _view = glm::translate(mat4(), _position) * glm::mat4_cast(_orientation);
}

// Order cooresponds to the order defined in the BoxVertex enum.
static const glm::vec4 NDC_VALUES[NUM_FRUSTUM_CORNERS] = {
    glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
    glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),
    glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),
    glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),
    glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
    glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
    glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),
};

void ViewFrustum::setProjection(const glm::mat4& projection) {
    _projection = projection;
    glm::mat4 inverseProjection = glm::inverse(projection);

    // compute our dimensions the usual way
    for (int i = 0; i < NUM_FRUSTUM_CORNERS; ++i) {
        _corners[i] = inverseProjection * NDC_VALUES[i];
        _corners[i] /= _corners[i].w;
    }
    _nearClip = -_corners[BOTTOM_LEFT_NEAR].z;
    _farClip = -_corners[BOTTOM_LEFT_FAR].z;
    _aspectRatio = (_corners[TOP_RIGHT_NEAR].x - _corners[BOTTOM_LEFT_NEAR].x) /
        (_corners[TOP_RIGHT_NEAR].y - _corners[BOTTOM_LEFT_NEAR].y);

    glm::vec4 top = inverseProjection * vec4(0.0f, 1.0f, -1.0f, 1.0f);
    top /= top.w;
    _fieldOfView = abs(glm::degrees(2.0f * abs(glm::angle(vec3(0.0f, 0.0f, -1.0f), glm::normalize(vec3(top))))));
}

// ViewFrustum::calculate()
//
// Description: this will calculate the view frustum bounds for a given position and direction
//
// Notes on how/why this works:
//     http://www.lighthouse3d.com/tutorials/view-frustum-culling/view-frustums-shape/
//
void ViewFrustum::calculate() {

    // find the intersections of the rays through the corners with the clip planes in view space,
    // then transform them to world space
    glm::mat4 worldMatrix = glm::translate(_position) * glm::mat4(glm::mat3(_right, _up, -_direction));
    glm::vec4 v;
    for (int i = 0; i < NUM_FRUSTUM_CORNERS; ++i) {
        v = worldMatrix * _corners[i];
        v /= v.w;
        _cornersWorld[i] = glm::vec3(v);
    }

    // compute the six planes
    // The planes are defined such that the normal points towards the inside of the view frustum.
    // Testing if an object is inside the view frustum is performed by computing on which side of
    // the plane the object resides. This can be done computing the signed distance from the point
    // to the plane. If it is on the side that the normal is pointing, i.e. the signed distance
    // is positive, then it is on the right side of the respective plane. If an object is on the
    // right side of all six planes then the object is inside the frustum.

    // the function set3Points assumes that the points are given in counter clockwise order, assume you
    // are inside the frustum, facing the plane. Start with any point, and go counter clockwise for
    // three consecutive points
    _planes[TOP_PLANE].set3Points(_cornersWorld[TOP_RIGHT_NEAR], _cornersWorld[TOP_LEFT_NEAR], _cornersWorld[TOP_LEFT_FAR]);
    _planes[BOTTOM_PLANE].set3Points(_cornersWorld[BOTTOM_LEFT_NEAR], _cornersWorld[BOTTOM_RIGHT_NEAR], _cornersWorld[BOTTOM_RIGHT_FAR]);
    _planes[LEFT_PLANE].set3Points(_cornersWorld[BOTTOM_LEFT_NEAR], _cornersWorld[BOTTOM_LEFT_FAR], _cornersWorld[TOP_LEFT_FAR]);
    _planes[RIGHT_PLANE].set3Points(_cornersWorld[BOTTOM_RIGHT_FAR], _cornersWorld[BOTTOM_RIGHT_NEAR], _cornersWorld[TOP_RIGHT_FAR]);
    _planes[NEAR_PLANE].set3Points(_cornersWorld[BOTTOM_RIGHT_NEAR], _cornersWorld[BOTTOM_LEFT_NEAR], _cornersWorld[TOP_LEFT_NEAR]);
    _planes[FAR_PLANE].set3Points(_cornersWorld[BOTTOM_LEFT_FAR], _cornersWorld[BOTTOM_RIGHT_FAR], _cornersWorld[TOP_RIGHT_FAR]);

    // Also calculate our projection matrix in case people want to project points...
    // Projection matrix : Field of View, ratio, display range : near to far
    glm::vec3 lookAt = _position + _direction;
    glm::mat4 view = glm::lookAt(_position, lookAt, _up);

    // Our ModelViewProjection : multiplication of our 3 matrices (note: model is identity, so we can drop it)
    _ourModelViewProjectionMatrix = _projection * view; // Remember, matrix multiplication is the other way around
}

//enum { TOP_PLANE = 0, BOTTOM_PLANE, LEFT_PLANE, RIGHT_PLANE, NEAR_PLANE, FAR_PLANE };
const char* ViewFrustum::debugPlaneName (int plane) const {
    switch (plane) {
        case TOP_PLANE:    return "Top Plane";
        case BOTTOM_PLANE: return "Bottom Plane";
        case LEFT_PLANE:   return "Left Plane";
        case RIGHT_PLANE:  return "Right Plane";
        case NEAR_PLANE:   return "Near Plane";
        case FAR_PLANE:    return "Far Plane";
    }
    return "Unknown";
}

ViewFrustum::intersection ViewFrustum::calculateCubeFrustumIntersection(const AACube& cube) const {
    // only check against frustum
    ViewFrustum::intersection result = INSIDE;
    for(int i = 0; i < NUM_FRUSTUM_PLANES; i++) {
        const glm::vec3& normal = _planes[i].getNormal();
        // check distance to farthest cube point
        if ( _planes[i].distance(cube.getFarthestVertex(normal)) < 0.0f) {
            return OUTSIDE;
        } else {
            // check distance to nearest cube point
            if (_planes[i].distance(cube.getNearestVertex(normal)) < 0.0f) {
                // cube straddles the plane
                result = INTERSECT;
            }
        }
    }
    return result;
}

const float HALF_SQRT_THREE = 0.8660254f;

ViewFrustum::intersection ViewFrustum::calculateCubeKeyholeIntersection(const AACube& cube) const {
    // check against centeral sphere
    ViewFrustum::intersection sphereResult = INTERSECT;
    glm::vec3 cubeOffset = cube.calcCenter() - _position;
    float distance = glm::length(cubeOffset);
    if (distance > EPSILON) {
        glm::vec3 vertex = cube.getFarthestVertex(cubeOffset) - _position;
        if (glm::dot(vertex, cubeOffset) < _centerSphereRadius * distance) {
            // the most outward cube vertex is inside central sphere
            return INSIDE;
        }
        if (!cube.touchesSphere(_position, _centerSphereRadius)) {
            sphereResult = OUTSIDE;
        }
    } else if (_centerSphereRadius > HALF_SQRT_THREE * cube.getScale()) {
        // the cube is in center of sphere and its bounding radius is inside
        return INSIDE;
    }

    // check against frustum
    ViewFrustum::intersection frustumResult = calculateCubeFrustumIntersection(cube);

    return (frustumResult == OUTSIDE) ? sphereResult : frustumResult;
}

bool ViewFrustum::pointIntersectsFrustum(const glm::vec3& point) const {
    // only check against frustum
    for(int i = 0; i < NUM_FRUSTUM_PLANES; ++i) {
        float distance = _planes[i].distance(point);
        if (distance < 0.0f) {
            return false;
        }
    }
    return true;
}

bool ViewFrustum::sphereIntersectsFrustum(const glm::vec3& center, float radius) const {
    // only check against frustum
    for(int i = 0; i < NUM_FRUSTUM_PLANES; i++) {
        float distance = _planes[i].distance(center);
        if (distance < -radius) {
            // This is outside the regular frustum, so just return the value from checking the keyhole
            return false;
        }
    }
    return true;
}

bool ViewFrustum::boxIntersectsFrustum(const AABox& box) const {
    // only check against frustum
    for(int i = 0; i < NUM_FRUSTUM_PLANES; i++) {
        const glm::vec3& normal = _planes[i].getNormal();
        // check distance to farthest box point
        if ( _planes[i].distance(box.getFarthestVertex(normal)) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool ViewFrustum::sphereIntersectsKeyhole(const glm::vec3& center, float radius) const {
    // check positive touch against central sphere
    if (glm::length(center - _position) <= (radius + _centerSphereRadius)) {
        return true;
    }
    // check negative touches against frustum planes
    for(int i = 0; i < NUM_FRUSTUM_PLANES; i++) {
        if ( _planes[i].distance(center) < -radius) {
            return false;
        }
    }
    return true;
}

bool ViewFrustum::cubeIntersectsKeyhole(const AACube& cube) const {
    // check positive touch against central sphere
    if (cube.touchesSphere(_position, _centerSphereRadius)) {
        return true;
    }
    // check negative touches against frustum planes
    for(int i = 0; i < NUM_FRUSTUM_PLANES; i++) {
        const glm::vec3& normal = _planes[i].getNormal();
        if ( _planes[i].distance(cube.getFarthestVertex(normal)) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool ViewFrustum::boxIntersectsKeyhole(const AABox& box) const {
    // check positive touch against central sphere
    if (box.touchesSphere(_position, _centerSphereRadius)) {
        return true;
    }
    // check negative touches against frustum planes
    for(int i = 0; i < NUM_FRUSTUM_PLANES; i++) {
        const glm::vec3& normal = _planes[i].getNormal();
        if ( _planes[i].distance(box.getFarthestVertex(normal)) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool testMatches(glm::quat lhs, glm::quat rhs, float epsilon = EPSILON) {
    return (fabs(lhs.x - rhs.x) <= epsilon && fabs(lhs.y - rhs.y) <= epsilon && fabs(lhs.z - rhs.z) <= epsilon
            && fabs(lhs.w - rhs.w) <= epsilon);
}

bool testMatches(glm::vec3 lhs, glm::vec3 rhs, float epsilon = EPSILON) {
    return (fabs(lhs.x - rhs.x) <= epsilon && fabs(lhs.y - rhs.y) <= epsilon && fabs(lhs.z - rhs.z) <= epsilon);
}

bool testMatches(float lhs, float rhs, float epsilon = EPSILON) {
    return (fabs(lhs - rhs) <= epsilon);
}

bool ViewFrustum::matches(const ViewFrustum& compareTo, bool debug) const {
    bool result =
           testMatches(compareTo._position, _position) &&
           testMatches(compareTo._direction, _direction) &&
           testMatches(compareTo._up, _up) &&
           testMatches(compareTo._right, _right) &&
           testMatches(compareTo._fieldOfView, _fieldOfView) &&
           testMatches(compareTo._aspectRatio, _aspectRatio) &&
           testMatches(compareTo._nearClip, _nearClip) &&
           testMatches(compareTo._farClip, _farClip) &&
           testMatches(compareTo._focalLength, _focalLength);

    if (!result && debug) {
        qCDebug(shared, "ViewFrustum::matches()... result=%s", debug::valueOf(result));
        qCDebug(shared, "%s -- compareTo._position=%f,%f,%f _position=%f,%f,%f",
                (testMatches(compareTo._position,_position) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._position.x, (double)compareTo._position.y, (double)compareTo._position.z,
                (double)_position.x, (double)_position.y, (double)_position.z);
        qCDebug(shared, "%s -- compareTo._direction=%f,%f,%f _direction=%f,%f,%f",
                (testMatches(compareTo._direction, _direction) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._direction.x, (double)compareTo._direction.y, (double)compareTo._direction.z,
                (double)_direction.x, (double)_direction.y, (double)_direction.z );
        qCDebug(shared, "%s -- compareTo._up=%f,%f,%f _up=%f,%f,%f",
                (testMatches(compareTo._up, _up) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._up.x, (double)compareTo._up.y, (double)compareTo._up.z,
                (double)_up.x, (double)_up.y, (double)_up.z );
        qCDebug(shared, "%s -- compareTo._right=%f,%f,%f _right=%f,%f,%f",
                (testMatches(compareTo._right, _right) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._right.x, (double)compareTo._right.y, (double)compareTo._right.z,
                (double)_right.x, (double)_right.y, (double)_right.z );
        qCDebug(shared, "%s -- compareTo._fieldOfView=%f _fieldOfView=%f",
                (testMatches(compareTo._fieldOfView, _fieldOfView) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._fieldOfView, (double)_fieldOfView);
        qCDebug(shared, "%s -- compareTo._aspectRatio=%f _aspectRatio=%f",
                (testMatches(compareTo._aspectRatio, _aspectRatio) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._aspectRatio, (double)_aspectRatio);
        qCDebug(shared, "%s -- compareTo._nearClip=%f _nearClip=%f",
                (testMatches(compareTo._nearClip, _nearClip) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._nearClip, (double)_nearClip);
        qCDebug(shared, "%s -- compareTo._farClip=%f _farClip=%f",
                (testMatches(compareTo._farClip, _farClip) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._farClip, (double)_farClip);
        qCDebug(shared, "%s -- compareTo._focalLength=%f _focalLength=%f",
                (testMatches(compareTo._focalLength, _focalLength) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._focalLength, (double)_focalLength);
    }
    return result;
}

bool ViewFrustum::isVerySimilar(const ViewFrustum& compareTo, bool debug) const {

    //  Compute distance between the two positions
    const float POSITION_SIMILAR_ENOUGH = 5.0f; // 5 meters
    float positionDistance = glm::distance(_position, compareTo._position);

    // Compute the angular distance between the two orientations
    const float ORIENTATION_SIMILAR_ENOUGH = 10.0f; // 10 degrees in any direction
    glm::quat dQOrientation = _orientation * glm::inverse(compareTo._orientation);
    float angleOrientation = compareTo._orientation == _orientation ? 0.0f : glm::degrees(glm::angle(dQOrientation));
    if (isNaN(angleOrientation)) {
        angleOrientation = 0.0f;
    }

    bool result =
        testMatches(0, positionDistance, POSITION_SIMILAR_ENOUGH) &&
        testMatches(0, angleOrientation, ORIENTATION_SIMILAR_ENOUGH) &&
           testMatches(compareTo._fieldOfView, _fieldOfView) &&
           testMatches(compareTo._aspectRatio, _aspectRatio) &&
           testMatches(compareTo._nearClip, _nearClip) &&
           testMatches(compareTo._farClip, _farClip) &&
           testMatches(compareTo._focalLength, _focalLength);


    if (!result && debug) {
        qCDebug(shared, "ViewFrustum::isVerySimilar()... result=%s\n", debug::valueOf(result));
        qCDebug(shared, "%s -- compareTo._position=%f,%f,%f _position=%f,%f,%f",
                (testMatches(compareTo._position,_position, POSITION_SIMILAR_ENOUGH) ?
                     "IS SIMILAR ENOUGH " : "IS NOT SIMILAR ENOUGH"),
                (double)compareTo._position.x, (double)compareTo._position.y, (double)compareTo._position.z,
                (double)_position.x, (double)_position.y, (double)_position.z );

        qCDebug(shared, "%s -- positionDistance=%f",
                (testMatches(0,positionDistance, POSITION_SIMILAR_ENOUGH) ? "IS SIMILAR ENOUGH " : "IS NOT SIMILAR ENOUGH"),
                (double)positionDistance);

        qCDebug(shared, "%s -- angleOrientation=%f",
                (testMatches(0, angleOrientation, ORIENTATION_SIMILAR_ENOUGH) ? "IS SIMILAR ENOUGH " : "IS NOT SIMILAR ENOUGH"),
                (double)angleOrientation);

        qCDebug(shared, "%s -- compareTo._fieldOfView=%f _fieldOfView=%f",
                (testMatches(compareTo._fieldOfView, _fieldOfView) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._fieldOfView, (double)_fieldOfView);
        qCDebug(shared, "%s -- compareTo._aspectRatio=%f _aspectRatio=%f",
                (testMatches(compareTo._aspectRatio, _aspectRatio) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._aspectRatio, (double)_aspectRatio);
        qCDebug(shared, "%s -- compareTo._nearClip=%f _nearClip=%f",
                (testMatches(compareTo._nearClip, _nearClip) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._nearClip, (double)_nearClip);
        qCDebug(shared, "%s -- compareTo._farClip=%f _farClip=%f",
                (testMatches(compareTo._farClip, _farClip) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._farClip, (double)_farClip);
        qCDebug(shared, "%s -- compareTo._focalLength=%f _focalLength=%f",
                (testMatches(compareTo._focalLength, _focalLength) ? "MATCHES " : "NO MATCH"),
                (double)compareTo._focalLength, (double)_focalLength);
    }
    return result;
}

PickRay ViewFrustum::computePickRay(float x, float y) {
    glm::vec3 pickRayOrigin;
    glm::vec3 pickRayDirection;
    computePickRay(x, y, pickRayOrigin, pickRayDirection);
    return PickRay(pickRayOrigin, pickRayDirection);
}

void ViewFrustum::computePickRay(float x, float y, glm::vec3& origin, glm::vec3& direction) const {
    origin = _cornersWorld[TOP_LEFT_NEAR] + x * (_cornersWorld[TOP_RIGHT_NEAR] - _cornersWorld[TOP_LEFT_NEAR]) +
        y * (_cornersWorld[BOTTOM_LEFT_NEAR] - _cornersWorld[TOP_LEFT_NEAR]);
    direction = glm::normalize(origin - _position);
}

void ViewFrustum::computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearValue, float& farValue,
                                        glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const {
    // find the minimum and maximum z values, which will be our near and far clip distances
    nearValue = FLT_MAX;
    farValue = -FLT_MAX;
    for (int i = 0; i < NUM_FRUSTUM_CORNERS; i++) {
        nearValue = min(nearValue, -_corners[i].z);
        farValue = max(farValue, -_corners[i].z);
    }

    // make sure the near clip isn't too small to be valid
    const float MIN_NEAR = 0.01f;
    nearValue = max(MIN_NEAR, nearValue);

    // get the near/far normal and use it to find the clip planes
    glm::vec4 normal = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    nearClipPlane = glm::vec4(-normal.x, -normal.y, -normal.z, glm::dot(normal, _corners[0]));
    farClipPlane = glm::vec4(normal.x, normal.y, normal.z, -glm::dot(normal, _corners[4]));

    // compute the focal proportion (zero is near clip, one is far clip)
    float focalProportion = (_focalLength - _nearClip) / (_farClip - _nearClip);

    // get the extents at Z = -near
    left = FLT_MAX;
    right = -FLT_MAX;
    bottom = FLT_MAX;
    top = -FLT_MAX;
    for (int i = 0; i < 4; i++) {
        glm::vec4 corner = glm::mix(_corners[i], _corners[i + 4], focalProportion);
        glm::vec4 intersection = corner * (-nearValue / corner.z);
        left = min(left, intersection.x);
        right = max(right, intersection.x);
        bottom = min(bottom, intersection.y);
        top = max(top, intersection.y);
    }
}

void ViewFrustum::printDebugDetails() const {
    qCDebug(shared, "ViewFrustum::printDebugDetails()...");
    qCDebug(shared, "_position=%f,%f,%f",  (double)_position.x, (double)_position.y, (double)_position.z );
    qCDebug(shared, "_direction=%f,%f,%f", (double)_direction.x, (double)_direction.y, (double)_direction.z );
    qCDebug(shared, "_up=%f,%f,%f", (double)_up.x, (double)_up.y, (double)_up.z );
    qCDebug(shared, "_right=%f,%f,%f", (double)_right.x, (double)_right.y, (double)_right.z );
    qCDebug(shared, "_fieldOfView=%f", (double)_fieldOfView);
    qCDebug(shared, "_aspectRatio=%f", (double)_aspectRatio);
    qCDebug(shared, "_centerSphereRadius=%f", (double)_centerSphereRadius);
    qCDebug(shared, "_nearClip=%f", (double)_nearClip);
    qCDebug(shared, "_farClip=%f", (double)_farClip);
    qCDebug(shared, "_focalLength=%f", (double)_focalLength);
}

glm::vec2 ViewFrustum::projectPoint(glm::vec3 point, bool& pointInView) const {

    glm::vec4 pointVec4 = glm::vec4(point, 1.0f);
    glm::vec4 projectedPointVec4 = _ourModelViewProjectionMatrix * pointVec4;
    pointInView = (projectedPointVec4.w > 0.0f); // math! If the w result is negative then the point is behind the viewer

    // what happens with w is 0???
    float x = projectedPointVec4.x / projectedPointVec4.w;
    float y = projectedPointVec4.y / projectedPointVec4.w;
    glm::vec2 projectedPoint(x,y);

    // if the point is out of view we also need to flip the signs of x and y
    if (!pointInView) {
        projectedPoint.x = -x;
        projectedPoint.y = -y;
    }

    return projectedPoint;
}


const int MAX_POSSIBLE_COMBINATIONS = 43;

const int hullVertexLookup[MAX_POSSIBLE_COMBINATIONS][MAX_PROJECTED_POLYGON_VERTEX_COUNT+1] = {
    // Number of vertices in shadow polygon for the visible faces, then a list of the index of each vertice from the AACube

//0
    {0}, // inside
    {4, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_RIGHT_NEAR}, // right
    {4, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR,  TOP_LEFT_NEAR, TOP_LEFT_FAR  },  // left
    {0}, // n/a

//4
    {4, BOTTOM_RIGHT_NEAR, BOTTOM_LEFT_NEAR, BOTTOM_LEFT_FAR, BOTTOM_RIGHT_FAR}, // bottom
//5
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_LEFT_NEAR, BOTTOM_LEFT_FAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_RIGHT_NEAR },//bottom, right
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_LEFT_NEAR, TOP_LEFT_NEAR, TOP_LEFT_FAR, BOTTOM_LEFT_FAR, BOTTOM_RIGHT_FAR, },//bottom, left
    {0}, // n/a
//8
    {4, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR},         // top
    {6, TOP_RIGHT_NEAR, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR},   // top, right
    {6, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR, TOP_LEFT_NEAR},   // top, left
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
//16
    {4, BOTTOM_LEFT_NEAR, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR }, // front or near

    {6, BOTTOM_LEFT_NEAR, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR }, // front, right
    {6, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR, TOP_LEFT_FAR, }, // front, left
    {0}, // n/a
//20
    {6, BOTTOM_LEFT_NEAR, BOTTOM_LEFT_FAR, BOTTOM_RIGHT_FAR, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR }, // front,bottom

//21
    {6, BOTTOM_LEFT_NEAR, BOTTOM_LEFT_FAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR }, //front,bottom,right
//22
    {6, BOTTOM_LEFT_FAR, BOTTOM_RIGHT_FAR, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR, TOP_LEFT_FAR  }, //front,bottom,left
    {0}, // n/a

    {6, BOTTOM_LEFT_NEAR, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR}, // front, top

    {6, BOTTOM_LEFT_NEAR, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR }, // front, top, right

    {6, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, TOP_LEFT_FAR }, // front, top, left
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
//32
    {4, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, TOP_LEFT_FAR, TOP_RIGHT_FAR }, // back
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, TOP_LEFT_FAR, TOP_RIGHT_FAR, TOP_RIGHT_NEAR}, // back, right
//34
    {6, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR, TOP_LEFT_NEAR, TOP_LEFT_FAR, TOP_RIGHT_FAR }, // back, left


    {0}, // n/a
//36
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_LEFT_NEAR, BOTTOM_LEFT_FAR, TOP_LEFT_FAR, TOP_RIGHT_FAR, BOTTOM_RIGHT_FAR}, // back, bottom
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_LEFT_NEAR, BOTTOM_LEFT_FAR, TOP_LEFT_FAR, TOP_RIGHT_FAR, TOP_RIGHT_NEAR},//back, bottom, right

// 38
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_LEFT_NEAR, TOP_LEFT_NEAR, TOP_LEFT_FAR, TOP_RIGHT_FAR, BOTTOM_RIGHT_FAR },//back, bottom, left
    {0}, // n/a

// 40
    {6, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR, TOP_RIGHT_NEAR, TOP_RIGHT_FAR}, // back, top

    {6, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR, TOP_RIGHT_NEAR}, // back, top, right
//42
    {6, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR, TOP_LEFT_NEAR}, // back, top, left
};

CubeProjectedPolygon ViewFrustum::getProjectedPolygon(const AACube& box) const {
    const glm::vec3& bottomNearRight = box.getCorner();
    glm::vec3 topFarLeft = box.calcTopFarLeft();

    int lookUp = ((_position.x < bottomNearRight.x)     )   //  1 = right      |   compute 6-bit
               + ((_position.x > topFarLeft.x     ) << 1)   //  2 = left       |         code to
               + ((_position.y < bottomNearRight.y) << 2)   //  4 = bottom     | classify camera
               + ((_position.y > topFarLeft.y     ) << 3)   //  8 = top        | with respect to
               + ((_position.z < bottomNearRight.z) << 4)   // 16 = front/near |  the 6 defining
               + ((_position.z > topFarLeft.z     ) << 5);  // 32 = back/far   |          planes

    int vertexCount = hullVertexLookup[lookUp][0];  //look up number of vertices

    CubeProjectedPolygon projectedPolygon(vertexCount);

    bool pointInView = true;
    bool allPointsInView = false; // assume the best, but wait till we know we have a vertex
    bool anyPointsInView = false; // assume the worst!
    if (vertexCount) {
        allPointsInView = true; // assume the best!
        for(int i = 0; i < vertexCount; i++) {
            int vertexNum = hullVertexLookup[lookUp][i+1];
            glm::vec3 point = box.getVertex((BoxVertex)vertexNum);
            glm::vec2 projectedPoint = projectPoint(point, pointInView);
            allPointsInView = allPointsInView && pointInView;
            anyPointsInView = anyPointsInView || pointInView;
            projectedPolygon.setVertex(i, projectedPoint);
        }

        /***
        // Now that we've got the polygon, if it extends beyond the clipping window, then let's clip it
        // NOTE: This clipping does not improve our overall performance. It basically causes more polygons to
        // end up in the same quad/half and so the polygon lists get longer, and that's more calls to polygon.occludes()
        if ( (projectedPolygon.getMaxX() > PolygonClip::RIGHT_OF_CLIPPING_WINDOW ) ||
             (projectedPolygon.getMaxY() > PolygonClip::TOP_OF_CLIPPING_WINDOW   ) ||
             (projectedPolygon.getMaxX() < PolygonClip::LEFT_OF_CLIPPING_WINDOW  ) ||
             (projectedPolygon.getMaxY() < PolygonClip::BOTTOM_OF_CLIPPING_WINDOW) ) {

            CoverageRegion::_clippedPolygons++;

            glm::vec2* clippedVertices;
            int        clippedVertexCount;
            PolygonClip::clipToScreen(projectedPolygon.getVertices(), vertexCount, clippedVertices, clippedVertexCount);

            // Now reset the vertices of our projectedPolygon object
            projectedPolygon.setVertexCount(clippedVertexCount);
            for(int i = 0; i < clippedVertexCount; i++) {
                projectedPolygon.setVertex(i, clippedVertices[i]);
            }
            delete[] clippedVertices;

            lookUp += PROJECTION_CLIPPED;
        }
        ***/
    }
    // set the distance from our camera position, to the closest vertex
    float distance = glm::distance(getPosition(), box.calcCenter());
    projectedPolygon.setDistance(distance);
    projectedPolygon.setAnyInView(anyPointsInView);
    projectedPolygon.setAllInView(allPointsInView);
    projectedPolygon.setProjectionType(lookUp); // remember the projection type
    return projectedPolygon;
}

// Similar strategy to getProjectedPolygon() we use the knowledge of camera position relative to the
// axis-aligned voxels to determine which of the voxels vertices must be the furthest. No need for
// squares and square-roots. Just compares.
void ViewFrustum::getFurthestPointFromCamera(const AACube& box, glm::vec3& furthestPoint) const {
    const glm::vec3& bottomNearRight = box.getCorner();
    float scale = box.getScale();
    float halfScale = scale * 0.5f;

    if (_position.x < bottomNearRight.x + halfScale) {
        // we are to the right of the center, so the left edge is furthest
        furthestPoint.x = bottomNearRight.x + scale;
    } else {
        furthestPoint.x = bottomNearRight.x;
    }

    if (_position.y < bottomNearRight.y + halfScale) {
        // we are below of the center, so the top edge is furthest
        furthestPoint.y = bottomNearRight.y + scale;
    } else {
        furthestPoint.y = bottomNearRight.y;
    }

    if (_position.z < bottomNearRight.z + halfScale) {
        // we are to the near side of the center, so the far side edge is furthest
        furthestPoint.z = bottomNearRight.z + scale;
    } else {
        furthestPoint.z = bottomNearRight.z;
    }
}

const ViewFrustum::Corners ViewFrustum::getCorners(const float& depth) const {
    glm::vec3 normal = glm::normalize(_direction);

    auto getCorner = [&](enum::BoxVertex nearCorner, enum::BoxVertex farCorner) {
        auto dir = glm::normalize(_cornersWorld[nearCorner] - _cornersWorld[farCorner]);
        auto factor = depth / glm::dot(dir, normal);
        return _position + factor * dir;
    };

    return Corners{
        getCorner(TOP_LEFT_NEAR, TOP_LEFT_FAR),
        getCorner(TOP_RIGHT_NEAR, TOP_RIGHT_FAR),
        getCorner(BOTTOM_LEFT_NEAR, BOTTOM_LEFT_FAR),
        getCorner(BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR)
    };
}

float ViewFrustum::distanceToCamera(const glm::vec3& point) const {
    glm::vec3 temp = getPosition() - point;
    float distanceToPoint = sqrtf(glm::dot(temp, temp));
    return distanceToPoint;
}

void ViewFrustum::evalProjectionMatrix(glm::mat4& proj) const {
    proj = _projection;
}

glm::mat4 ViewFrustum::evalProjectionMatrixRange(float rangeNear, float rangeFar) const {

    // make sure range near far make sense
    assert(rangeNear > 0.0);
    assert(rangeFar > rangeNear);

    // recreate a projection matrix for only a range of depth of this frustum.
   
    // take the current projection
    glm::mat4 rangeProj = _projection;
    
    float A = -(rangeFar + rangeNear) / (rangeFar - rangeNear);
    float B = -2 * rangeFar*rangeNear / ((rangeFar - rangeNear));

    rangeProj[2][2] = A;
    rangeProj[3][2] = B;
    return rangeProj;
}


void ViewFrustum::evalViewTransform(Transform& view) const {
    view.setTranslation(getPosition());
    view.setRotation(getOrientation());
}

void ViewFrustum::invalidate() {
    // these setting should make nearly all intersection tests fail
    for (int i = 0; i < NUM_FRUSTUM_PLANES; ++i) {
        _planes[i].invalidate();
    }
    _centerSphereRadius = -1.0e6f; // -10^6 should be negative enough
}
