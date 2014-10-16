//
//  ViewFrustum.cpp
//  libraries/octree/src
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

#include <QtCore/QDebug>

#include "GeometryUtil.h"
#include "SharedUtil.h"
#include "ViewFrustum.h"
#include "OctreeConstants.h"

using namespace std;

ViewFrustum::ViewFrustum() :
    _position(0,0,0),
    _positionVoxelScale(0,0,0),
    _orientation(),
    _direction(IDENTITY_FRONT),
    _up(IDENTITY_UP),
    _right(IDENTITY_RIGHT),
    _orthographic(false),
    _width(1.0f),
    _height(1.0f),
    _fieldOfView(0.0),
    _aspectRatio(1.0f),
    _nearClip(DEFAULT_NEAR_CLIP),
    _farClip(DEFAULT_FAR_CLIP),
    _focalLength(0.25f),
    _keyholeRadius(DEFAULT_KEYHOLE_RADIUS),
    _farTopLeft(0,0,0),
    _farTopRight(0,0,0),
    _farBottomLeft(0,0,0),
    _farBottomRight(0,0,0),
    _nearTopLeft(0,0,0),
    _nearTopRight(0,0,0),
    _nearBottomLeft(0,0,0),
    _nearBottomRight(0,0,0)
{
}

void ViewFrustum::setOrientation(const glm::quat& orientationAsQuaternion) {
    _orientation = orientationAsQuaternion;
    _right       = glm::vec3(orientationAsQuaternion * glm::vec4(IDENTITY_RIGHT, 0.0f));
    _up          = glm::vec3(orientationAsQuaternion * glm::vec4(IDENTITY_UP,    0.0f));
    _direction   = glm::vec3(orientationAsQuaternion * glm::vec4(IDENTITY_FRONT, 0.0f));
}

// ViewFrustum::calculateViewFrustum()
//
// Description: this will calculate the view frustum bounds for a given position and direction
//
// Notes on how/why this works:
//     http://www.lighthouse3d.com/tutorials/view-frustum-culling/view-frustums-shape/
//
void ViewFrustum::calculate() {
    if (_orthographic) {
        calculateOrthographic();
        return;
    }

    // compute the off-axis frustum parameters as we would for glFrustum
    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;
    computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);

    // start with the corners of the near frustum window
    glm::vec3 topLeft(left, top, -nearVal);
    glm::vec3 topRight(right, top, -nearVal);
    glm::vec3 bottomLeft(left, bottom, -nearVal);
    glm::vec3 bottomRight(right, bottom, -nearVal);

    // find the intersections of the rays through the corners with the clip planes in view space,
    // then transform them to world space
    glm::mat4 worldMatrix = glm::translate(_position) * glm::mat4(glm::mat3(_right, _up, -_direction)) *
        glm::translate(_eyeOffsetPosition) * glm::mat4_cast(_eyeOffsetOrientation);
    _farTopLeft = glm::vec3(worldMatrix * glm::vec4(topLeft *
        (-farClipPlane.w / glm::dot(topLeft, glm::vec3(farClipPlane))), 1.0f));
    _farTopRight = glm::vec3(worldMatrix * glm::vec4(topRight *
        (-farClipPlane.w / glm::dot(topRight, glm::vec3(farClipPlane))), 1.0f));
    _farBottomLeft = glm::vec3(worldMatrix * glm::vec4(bottomLeft *
        (-farClipPlane.w / glm::dot(bottomLeft, glm::vec3(farClipPlane))), 1.0f));
    _farBottomRight = glm::vec3(worldMatrix * glm::vec4(bottomRight *
        (-farClipPlane.w / glm::dot(bottomRight, glm::vec3(farClipPlane))), 1.0f));
    _nearTopLeft = glm::vec3(worldMatrix * glm::vec4(topLeft *
        (-nearClipPlane.w / glm::dot(topLeft, glm::vec3(nearClipPlane))), 1.0f));
    _nearTopRight = glm::vec3(worldMatrix * glm::vec4(topRight *
        (-nearClipPlane.w / glm::dot(topRight, glm::vec3(nearClipPlane))), 1.0f));
    _nearBottomLeft = glm::vec3(worldMatrix * glm::vec4(bottomLeft *
        (-nearClipPlane.w / glm::dot(bottomLeft, glm::vec3(nearClipPlane))), 1.0f));
    _nearBottomRight = glm::vec3(worldMatrix * glm::vec4(bottomRight *
        (-nearClipPlane.w / glm::dot(bottomRight, glm::vec3(nearClipPlane))), 1.0f));

    // compute the offset position and axes in world space
    _offsetPosition = glm::vec3(worldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    _offsetDirection = glm::vec3(worldMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    _offsetUp = glm::vec3(worldMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
    _offsetRight = glm::vec3(worldMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));

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

    _planes[TOP_PLANE   ].set3Points(_nearTopRight,_nearTopLeft,_farTopLeft);
    _planes[BOTTOM_PLANE].set3Points(_nearBottomLeft,_nearBottomRight,_farBottomRight);
    _planes[LEFT_PLANE  ].set3Points(_nearBottomLeft,_farBottomLeft,_farTopLeft);
    _planes[RIGHT_PLANE ].set3Points(_farBottomRight,_nearBottomRight,_nearTopRight);
    _planes[NEAR_PLANE  ].set3Points(_nearBottomRight,_nearBottomLeft,_nearTopLeft);
    _planes[FAR_PLANE   ].set3Points(_farBottomLeft,_farBottomRight,_farTopRight);

    // Also calculate our projection matrix in case people want to project points...
    // Projection matrix : Field of View, ratio, display range : near to far
    const float CLIP_NUDGE = 1.0f;
    float farClip = (_farClip != _nearClip) ? _farClip : _nearClip + CLIP_NUDGE; // don't allow near and far to be equal
    glm::mat4 projection = glm::perspective(_fieldOfView, _aspectRatio, _nearClip, farClip);
    glm::vec3 lookAt = _position + _direction;
    glm::mat4 view = glm::lookAt(_position, lookAt, _up);

    // Our ModelViewProjection : multiplication of our 3 matrices (note: model is identity, so we can drop it)
    _ourModelViewProjectionMatrix = projection * view; // Remember, matrix multiplication is the other way around

    // Set up our keyhole bounding box...
    glm::vec3 corner = _position - _keyholeRadius;
    _keyholeBoundingCube = AACube(corner,(_keyholeRadius * 2.0f));
}

void ViewFrustum::calculateOrthographic() {
    float halfWidth = _width * 0.5f;
    float halfHeight = _height * 0.5f;

    // find the corners of the view box in world space
    glm::mat4 worldMatrix = glm::translate(_position) * glm::mat4(glm::mat3(_right, _up, -_direction)) *
        glm::translate(_eyeOffsetPosition) * glm::mat4_cast(_eyeOffsetOrientation);
    _farTopLeft = glm::vec3(worldMatrix * glm::vec4(-halfWidth, halfHeight, -_farClip, 1.0f));
    _farTopRight = glm::vec3(worldMatrix * glm::vec4(halfWidth, halfHeight, -_farClip, 1.0f));
    _farBottomLeft = glm::vec3(worldMatrix * glm::vec4(-halfWidth, -halfHeight, -_farClip, 1.0f));
    _farBottomRight = glm::vec3(worldMatrix * glm::vec4(halfWidth, -halfHeight, -_farClip, 1.0f));
    _nearTopLeft = glm::vec3(worldMatrix * glm::vec4(-halfWidth, halfHeight, -_nearClip, 1.0f));
    _nearTopRight = glm::vec3(worldMatrix * glm::vec4(halfWidth, halfHeight, -_nearClip, 1.0f));
    _nearBottomLeft = glm::vec3(worldMatrix * glm::vec4(-halfWidth, -halfHeight, -_nearClip, 1.0f));
    _nearBottomRight = glm::vec3(worldMatrix * glm::vec4(halfWidth, -halfHeight, -_nearClip, 1.0f));
    
    // compute the offset position and axes in world space
    _offsetPosition = glm::vec3(worldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    _offsetDirection = glm::vec3(worldMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    _offsetUp = glm::vec3(worldMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
    _offsetRight = glm::vec3(worldMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    
    _planes[TOP_PLANE].set3Points(_nearTopRight, _nearTopLeft, _farTopLeft);
    _planes[BOTTOM_PLANE].set3Points(_nearBottomLeft, _nearBottomRight, _farBottomRight);
    _planes[LEFT_PLANE].set3Points(_nearBottomLeft, _farBottomLeft, _farTopLeft);
    _planes[RIGHT_PLANE].set3Points(_farBottomRight, _nearBottomRight, _nearTopRight);
    _planes[NEAR_PLANE].set3Points(_nearBottomRight, _nearBottomLeft, _nearTopLeft);
    _planes[FAR_PLANE].set3Points(_farBottomLeft, _farBottomRight, _farTopRight);

    // Also calculate our projection matrix in case people want to project points...
    // Projection matrix : Field of View, ratio, display range : near to far
    glm::mat4 projection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, _nearClip, _farClip);
    glm::vec3 lookAt = _position + _direction;
    glm::mat4 view = glm::lookAt(_position, lookAt, _up);

    // Our ModelViewProjection : multiplication of our 3 matrices (note: model is identity, so we can drop it)
    _ourModelViewProjectionMatrix = projection * view; // Remember, matrix multiplication is the other way around

    // Set up our keyhole bounding box...
    glm::vec3 corner = _position - _keyholeRadius;
    _keyholeBoundingCube = AACube(corner, (_keyholeRadius * 2.0f));
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

ViewFrustum::location ViewFrustum::pointInKeyhole(const glm::vec3& point) const {

    ViewFrustum::location result = INTERSECT;

    float distance = glm::distance(point, _position);
    if (distance > _keyholeRadius) {
        result = OUTSIDE;
    } else if (distance < _keyholeRadius) {
        result = INSIDE;
    }

    return result;
}

// To determine if two spheres intersect, simply calculate the distance between the centers of the two spheres.
// If the distance is greater than the sum of the two sphere radii, they don’t intersect. Otherwise they intersect.
// If the distance plus the radius of sphere A is less than the radius of sphere B then, sphere A is inside of sphere B
ViewFrustum::location ViewFrustum::sphereInKeyhole(const glm::vec3& center, float radius) const {
    ViewFrustum::location result = INTERSECT;

    float distance = glm::distance(center, _position);
    if (distance > (radius + _keyholeRadius)) {
        result = OUTSIDE;
    } else if ((distance + radius) < _keyholeRadius) {
        result = INSIDE;
    }

    return result;
}


// A box is inside a sphere if all of its corners are inside the sphere
// A box intersects a sphere if any of its edges (as rays) interesect the sphere
// A box is outside a sphere if none of its edges (as rays) interesect the sphere
ViewFrustum::location ViewFrustum::cubeInKeyhole(const AACube& cube) const {

    // First check to see if the cube is in the bounding cube for the sphere, if it's not, then we can short circuit
    // this and not check with sphere penetration which is more expensive
    if (!_keyholeBoundingCube.contains(cube)) {
        return OUTSIDE;
    }

    glm::vec3 penetration;
    bool intersects = cube.findSpherePenetration(_position, _keyholeRadius, penetration);

    ViewFrustum::location result = OUTSIDE;

    // if the cube intersects the sphere, then it may also be inside... calculate further
    if (intersects) {
        result = INTERSECT;

        // test all the corners, if they are all inside the sphere, the entire cube is in the sphere
        bool allPointsInside = true; // assume the best
        for (int v = BOTTOM_LEFT_NEAR; v < TOP_LEFT_FAR; v++) {
            glm::vec3 vertex = cube.getVertex((BoxVertex)v);
            if (!pointInKeyhole(vertex)) {
                allPointsInside = false;
                break;
            }
        }

        if (allPointsInside) {
            result = INSIDE;
        }
    }

    return result;
}

// A box is inside a sphere if all of its corners are inside the sphere
// A box intersects a sphere if any of its edges (as rays) interesect the sphere
// A box is outside a sphere if none of its edges (as rays) interesect the sphere
ViewFrustum::location ViewFrustum::boxInKeyhole(const AABox& box) const {

    // First check to see if the box is in the bounding box for the sphere, if it's not, then we can short circuit
    // this and not check with sphere penetration which is more expensive
    if (!_keyholeBoundingCube.contains(box)) {
        return OUTSIDE;
    }

    glm::vec3 penetration;
    bool intersects = box.findSpherePenetration(_position, _keyholeRadius, penetration);

    ViewFrustum::location result = OUTSIDE;

    // if the box intersects the sphere, then it may also be inside... calculate further
    if (intersects) {
        result = INTERSECT;

        // test all the corners, if they are all inside the sphere, the entire box is in the sphere
        bool allPointsInside = true; // assume the best
        for (int v = BOTTOM_LEFT_NEAR; v < TOP_LEFT_FAR; v++) {
            glm::vec3 vertex = box.getVertex((BoxVertex)v);
            if (!pointInKeyhole(vertex)) {
                allPointsInside = false;
                break;
            }
        }

        if (allPointsInside) {
            result = INSIDE;
        }
    }

    return result;
}

ViewFrustum::location ViewFrustum::pointInFrustum(const glm::vec3& point) const {
    ViewFrustum::location regularResult = INSIDE;
    ViewFrustum::location keyholeResult = OUTSIDE;

    // If we have a keyholeRadius, check that first, since it's cheaper
    if (_keyholeRadius >= 0.0f) {
        keyholeResult = pointInKeyhole(point);
    }
    if (keyholeResult == INSIDE) {
        return keyholeResult;
    }

    // If we're not known to be INSIDE the keyhole, then check the regular frustum
    for(int i=0; i < 6; i++) {
        float distance = _planes[i].distance(point);
        if (distance < 0) {
            return keyholeResult; // escape early will be the value from checking the keyhole
        }
    }

    return regularResult;
}

ViewFrustum::location ViewFrustum::sphereInFrustum(const glm::vec3& center, float radius) const {
    ViewFrustum::location regularResult = INSIDE;
    ViewFrustum::location keyholeResult = OUTSIDE;

    // If we have a keyholeRadius, check that first, since it's cheaper
    if (_keyholeRadius >= 0.0f) {
        keyholeResult = sphereInKeyhole(center, radius);
    }
    if (keyholeResult == INSIDE) {
        return keyholeResult;
    }

    float distance;
    for(int i=0; i < 6; i++) {
        distance = _planes[i].distance(center);
        if (distance < -radius) {
            // This is outside the regular frustum, so just return the value from checking the keyhole
            return keyholeResult;
        } else if (distance < radius) {
            regularResult =  INTERSECT;
        }
    }

    return regularResult;
}


ViewFrustum::location ViewFrustum::cubeInFrustum(const AACube& cube) const {

    ViewFrustum::location regularResult = INSIDE;
    ViewFrustum::location keyholeResult = OUTSIDE;

    // If we have a keyholeRadius, check that first, since it's cheaper
    if (_keyholeRadius >= 0.0f) {
        keyholeResult = cubeInKeyhole(cube);
    }
    if (keyholeResult == INSIDE) {
        return keyholeResult;
    }

    // TODO: These calculations are expensive, taking up 80% of our time in this function.
    // This appears to be expensive because we have to test the distance to each plane.
    // One suggested optimization is to first check against the approximated cone. We might
    // also be able to test against the cone to the bounding sphere of the box.
    for(int i=0; i < 6; i++) {
        const glm::vec3& normal = _planes[i].getNormal();
        const glm::vec3& boxVertexP = cube.getVertexP(normal);
        float planeToBoxVertexPDistance = _planes[i].distance(boxVertexP);

        const glm::vec3& boxVertexN = cube.getVertexN(normal);
        float planeToBoxVertexNDistance = _planes[i].distance(boxVertexN);

        if (planeToBoxVertexPDistance < 0) {
            // This is outside the regular frustum, so just return the value from checking the keyhole
            return keyholeResult;
        } else if (planeToBoxVertexNDistance < 0) {
            regularResult =  INTERSECT;
        }
    }
    return regularResult;
}

ViewFrustum::location ViewFrustum::boxInFrustum(const AABox& box) const {

    ViewFrustum::location regularResult = INSIDE;
    ViewFrustum::location keyholeResult = OUTSIDE;

    // If we have a keyholeRadius, check that first, since it's cheaper
    if (_keyholeRadius >= 0.0f) {
        keyholeResult = boxInKeyhole(box);
    }
    if (keyholeResult == INSIDE) {
        return keyholeResult;
    }

    // TODO: These calculations are expensive, taking up 80% of our time in this function.
    // This appears to be expensive because we have to test the distance to each plane.
    // One suggested optimization is to first check against the approximated cone. We might
    // also be able to test against the cone to the bounding sphere of the box.
    for(int i=0; i < 6; i++) {
        const glm::vec3& normal = _planes[i].getNormal();
        const glm::vec3& boxVertexP = box.getVertexP(normal);
        float planeToBoxVertexPDistance = _planes[i].distance(boxVertexP);

        const glm::vec3& boxVertexN = box.getVertexN(normal);
        float planeToBoxVertexNDistance = _planes[i].distance(boxVertexN);

        if (planeToBoxVertexPDistance < 0) {
            // This is outside the regular frustum, so just return the value from checking the keyhole
            return keyholeResult;
        } else if (planeToBoxVertexNDistance < 0) {
            regularResult =  INTERSECT;
        }
    }
    return regularResult;
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
           testMatches(compareTo._focalLength, _focalLength) &&
           testMatches(compareTo._eyeOffsetPosition, _eyeOffsetPosition) &&
           testMatches(compareTo._eyeOffsetOrientation, _eyeOffsetOrientation);

    if (!result && debug) {
        qDebug("ViewFrustum::matches()... result=%s", debug::valueOf(result));
        qDebug("%s -- compareTo._position=%f,%f,%f _position=%f,%f,%f",
            (testMatches(compareTo._position,_position) ? "MATCHES " : "NO MATCH"),
            compareTo._position.x, compareTo._position.y, compareTo._position.z,
            _position.x, _position.y, _position.z );
        qDebug("%s -- compareTo._direction=%f,%f,%f _direction=%f,%f,%f",
            (testMatches(compareTo._direction, _direction) ? "MATCHES " : "NO MATCH"),
            compareTo._direction.x, compareTo._direction.y, compareTo._direction.z,
            _direction.x, _direction.y, _direction.z );
        qDebug("%s -- compareTo._up=%f,%f,%f _up=%f,%f,%f",
            (testMatches(compareTo._up, _up) ? "MATCHES " : "NO MATCH"),
            compareTo._up.x, compareTo._up.y, compareTo._up.z,
            _up.x, _up.y, _up.z );
        qDebug("%s -- compareTo._right=%f,%f,%f _right=%f,%f,%f",
            (testMatches(compareTo._right, _right) ? "MATCHES " : "NO MATCH"),
            compareTo._right.x, compareTo._right.y, compareTo._right.z,
            _right.x, _right.y, _right.z );
        qDebug("%s -- compareTo._fieldOfView=%f _fieldOfView=%f",
            (testMatches(compareTo._fieldOfView, _fieldOfView) ? "MATCHES " : "NO MATCH"),
            compareTo._fieldOfView, _fieldOfView);
        qDebug("%s -- compareTo._aspectRatio=%f _aspectRatio=%f",
            (testMatches(compareTo._aspectRatio, _aspectRatio) ? "MATCHES " : "NO MATCH"),
            compareTo._aspectRatio, _aspectRatio);
        qDebug("%s -- compareTo._nearClip=%f _nearClip=%f",
            (testMatches(compareTo._nearClip, _nearClip) ? "MATCHES " : "NO MATCH"),
            compareTo._nearClip, _nearClip);
        qDebug("%s -- compareTo._farClip=%f _farClip=%f",
            (testMatches(compareTo._farClip, _farClip) ? "MATCHES " : "NO MATCH"),
            compareTo._farClip, _farClip);
        qDebug("%s -- compareTo._focalLength=%f _focalLength=%f",
            (testMatches(compareTo._focalLength, _focalLength) ? "MATCHES " : "NO MATCH"),
            compareTo._focalLength, _focalLength);
        qDebug("%s -- compareTo._eyeOffsetPosition=%f,%f,%f _eyeOffsetPosition=%f,%f,%f",
            (testMatches(compareTo._eyeOffsetPosition, _eyeOffsetPosition) ? "MATCHES " : "NO MATCH"),
            compareTo._eyeOffsetPosition.x, compareTo._eyeOffsetPosition.y, compareTo._eyeOffsetPosition.z,
            _eyeOffsetPosition.x, _eyeOffsetPosition.y, _eyeOffsetPosition.z);
        qDebug("%s -- compareTo._eyeOffsetOrientation=%f,%f,%f,%f _eyeOffsetOrientation=%f,%f,%f,%f",
            (testMatches(compareTo._eyeOffsetOrientation, _eyeOffsetOrientation) ? "MATCHES " : "NO MATCH"),
            compareTo._eyeOffsetOrientation.x, compareTo._eyeOffsetOrientation.y,
                compareTo._eyeOffsetOrientation.z, compareTo._eyeOffsetOrientation.w,
            _eyeOffsetOrientation.x, _eyeOffsetOrientation.y, _eyeOffsetOrientation.z, _eyeOffsetOrientation.w);
    }
    return result;
}

bool ViewFrustum::isVerySimilar(const ViewFrustum& compareTo, bool debug) const {

    //  Compute distance between the two positions
    const float POSITION_SIMILAR_ENOUGH = 5.0f; // 5 meters
    float positionDistance = glm::distance(_position, compareTo._position);

    const float EYEOFFSET_POSITION_SIMILAR_ENOUGH = 0.15f; // 0.15 meters
    float eyeOffsetpositionDistance = glm::distance(_eyeOffsetPosition, compareTo._eyeOffsetPosition);

    // Compute the angular distance between the two orientations
    const float ORIENTATION_SIMILAR_ENOUGH = 10.0f; // 10 degrees in any direction
    glm::quat dQOrientation = _orientation * glm::inverse(compareTo._orientation);
    float angleOrientation = compareTo._orientation == _orientation ? 0.0f : glm::degrees(glm::angle(dQOrientation));
    if (isNaN(angleOrientation)) {
        angleOrientation = 0.0f;
    }

    glm::quat dQEyeOffsetOrientation = _eyeOffsetOrientation * glm::inverse(compareTo._eyeOffsetOrientation);
    float angleEyeOffsetOrientation = compareTo._eyeOffsetOrientation == _eyeOffsetOrientation
                                            ? 0.0f : glm::degrees(glm::angle(dQEyeOffsetOrientation));
    if (isNaN(angleEyeOffsetOrientation)) {
        angleEyeOffsetOrientation = 0.0f;
    }

    bool result =
           testMatches(0, positionDistance, POSITION_SIMILAR_ENOUGH) &&
           testMatches(0, angleOrientation, ORIENTATION_SIMILAR_ENOUGH) &&
           testMatches(compareTo._fieldOfView, _fieldOfView) &&
           testMatches(compareTo._aspectRatio, _aspectRatio) &&
           testMatches(compareTo._nearClip, _nearClip) &&
           testMatches(compareTo._farClip, _farClip) &&
           testMatches(compareTo._focalLength, _focalLength) &&
           testMatches(0, eyeOffsetpositionDistance, EYEOFFSET_POSITION_SIMILAR_ENOUGH) &&
           testMatches(0, angleEyeOffsetOrientation, ORIENTATION_SIMILAR_ENOUGH);


    if (!result && debug) {
        qDebug("ViewFrustum::isVerySimilar()... result=%s\n", debug::valueOf(result));
        qDebug("%s -- compareTo._position=%f,%f,%f _position=%f,%f,%f",
            (testMatches(compareTo._position,_position, POSITION_SIMILAR_ENOUGH) ? "IS SIMILAR ENOUGH " : "IS NOT SIMILAR ENOUGH"),
            compareTo._position.x, compareTo._position.y, compareTo._position.z,
            _position.x, _position.y, _position.z );

        qDebug("%s -- positionDistance=%f",
            (testMatches(0,positionDistance, POSITION_SIMILAR_ENOUGH) ? "IS SIMILAR ENOUGH " : "IS NOT SIMILAR ENOUGH"),
            positionDistance);

        qDebug("%s -- angleOrientation=%f",
            (testMatches(0, angleOrientation, ORIENTATION_SIMILAR_ENOUGH) ? "IS SIMILAR ENOUGH " : "IS NOT SIMILAR ENOUGH"),
            angleOrientation);
        
        qDebug("%s -- compareTo._fieldOfView=%f _fieldOfView=%f",
            (testMatches(compareTo._fieldOfView, _fieldOfView) ? "MATCHES " : "NO MATCH"),
            compareTo._fieldOfView, _fieldOfView);
        qDebug("%s -- compareTo._aspectRatio=%f _aspectRatio=%f",
            (testMatches(compareTo._aspectRatio, _aspectRatio) ? "MATCHES " : "NO MATCH"),
            compareTo._aspectRatio, _aspectRatio);
        qDebug("%s -- compareTo._nearClip=%f _nearClip=%f",
            (testMatches(compareTo._nearClip, _nearClip) ? "MATCHES " : "NO MATCH"),
            compareTo._nearClip, _nearClip);
        qDebug("%s -- compareTo._farClip=%f _farClip=%f",
            (testMatches(compareTo._farClip, _farClip) ? "MATCHES " : "NO MATCH"),
            compareTo._farClip, _farClip);
        qDebug("%s -- compareTo._focalLength=%f _focalLength=%f",
            (testMatches(compareTo._focalLength, _focalLength) ? "MATCHES " : "NO MATCH"),
            compareTo._focalLength, _focalLength);

        qDebug("%s -- compareTo._eyeOffsetPosition=%f,%f,%f _eyeOffsetPosition=%f,%f,%f",
            (testMatches(compareTo._eyeOffsetPosition, _eyeOffsetPosition, POSITION_SIMILAR_ENOUGH) ? "IS SIMILAR ENOUGH " : "IS NOT SIMILAR ENOUGH"),
            compareTo._eyeOffsetPosition.x, compareTo._eyeOffsetPosition.y, compareTo._eyeOffsetPosition.z,
            _eyeOffsetPosition.x, _eyeOffsetPosition.y, _eyeOffsetPosition.z);

        qDebug("%s -- eyeOffsetpositionDistance=%f",
            (testMatches(0,eyeOffsetpositionDistance, EYEOFFSET_POSITION_SIMILAR_ENOUGH) ? "IS SIMILAR ENOUGH " : "IS NOT SIMILAR ENOUGH"),
            eyeOffsetpositionDistance);

        qDebug("%s -- angleEyeOffsetOrientation=%f",
            (testMatches(0, angleEyeOffsetOrientation, ORIENTATION_SIMILAR_ENOUGH) ? "IS SIMILAR ENOUGH " : "IS NOT SIMILAR ENOUGH"),
            angleEyeOffsetOrientation);
    }
    return result;
}

void ViewFrustum::computePickRay(float x, float y, glm::vec3& origin, glm::vec3& direction) const {
    origin = _nearTopLeft + x*(_nearTopRight - _nearTopLeft) + y*(_nearBottomLeft - _nearTopLeft);
    direction = glm::normalize(origin - (_position + _orientation * _eyeOffsetPosition));
}

void ViewFrustum::computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearValue, float& farValue,
                                        glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const {
    // compute our dimensions the usual way
    float hheight = _nearClip * tanf(_fieldOfView * 0.5f * RADIANS_PER_DEGREE);
    float hwidth = _aspectRatio * hheight;

    // get our frustum corners in view space
    glm::mat4 eyeMatrix = glm::mat4_cast(glm::inverse(_eyeOffsetOrientation)) * glm::translate(-_eyeOffsetPosition);
    glm::vec4 corners[8];
    float farScale = _farClip / _nearClip;
    corners[0] = eyeMatrix * glm::vec4(-hwidth, -hheight, -_nearClip, 1.0f);
    corners[1] = eyeMatrix * glm::vec4(hwidth, -hheight, -_nearClip, 1.0f);
    corners[2] = eyeMatrix * glm::vec4(hwidth, hheight, -_nearClip, 1.0f);
    corners[3] = eyeMatrix * glm::vec4(-hwidth, hheight, -_nearClip, 1.0f);
    corners[4] = eyeMatrix * glm::vec4(-hwidth * farScale, -hheight * farScale, -_farClip, 1.0f);
    corners[5] = eyeMatrix * glm::vec4(hwidth * farScale, -hheight * farScale, -_farClip, 1.0f);
    corners[6] = eyeMatrix * glm::vec4(hwidth * farScale, hheight * farScale, -_farClip, 1.0f);
    corners[7] = eyeMatrix * glm::vec4(-hwidth * farScale, hheight * farScale, -_farClip, 1.0f);

    // find the minimum and maximum z values, which will be our near and far clip distances
    nearValue = FLT_MAX;
    farValue = -FLT_MAX;
    for (int i = 0; i < 8; i++) {
        nearValue = min(nearValue, -corners[i].z);
        farValue = max(farValue, -corners[i].z);
    }

    // make sure the near clip isn't too small to be valid
    const float MIN_NEAR = 0.01f;
    nearValue = max(MIN_NEAR, nearValue);

    // get the near/far normal and use it to find the clip planes
    glm::vec4 normal = eyeMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    nearClipPlane = glm::vec4(-normal.x, -normal.y, -normal.z, glm::dot(normal, corners[0]));
    farClipPlane = glm::vec4(normal.x, normal.y, normal.z, -glm::dot(normal, corners[4]));

    // compute the focal proportion (zero is near clip, one is far clip)
    float focalProportion = (_focalLength - _nearClip) / (_farClip - _nearClip);

    // get the extents at Z = -near
    left = FLT_MAX;
    right = -FLT_MAX;
    bottom = FLT_MAX;
    top = -FLT_MAX;
    for (int i = 0; i < 4; i++) {
        glm::vec4 corner = glm::mix(corners[i], corners[i + 4], focalProportion);
        glm::vec4 intersection = corner * (-nearValue / corner.z);
        left = min(left, intersection.x);
        right = max(right, intersection.x);
        bottom = min(bottom, intersection.y);
        top = max(top, intersection.y);
    }
}

void ViewFrustum::printDebugDetails() const {
    qDebug("ViewFrustum::printDebugDetails()...");
    qDebug("_position=%f,%f,%f",  _position.x, _position.y, _position.z );
    qDebug("_direction=%f,%f,%f", _direction.x, _direction.y, _direction.z );
    qDebug("_up=%f,%f,%f", _up.x, _up.y, _up.z );
    qDebug("_right=%f,%f,%f", _right.x, _right.y, _right.z );
    qDebug("_fieldOfView=%f", _fieldOfView);
    qDebug("_aspectRatio=%f", _aspectRatio);
    qDebug("_keyHoleRadius=%f", _keyholeRadius);
    qDebug("_nearClip=%f", _nearClip);
    qDebug("_farClip=%f", _farClip);
    qDebug("_focalLength=%f", _focalLength);
    qDebug("_eyeOffsetPosition=%f,%f,%f",  _eyeOffsetPosition.x, _eyeOffsetPosition.y, _eyeOffsetPosition.z );
    qDebug("_eyeOffsetOrientation=%f,%f,%f,%f",  _eyeOffsetOrientation.x, _eyeOffsetOrientation.y, _eyeOffsetOrientation.z,
        _eyeOffsetOrientation.w );
}

glm::vec2 ViewFrustum::projectPoint(glm::vec3 point, bool& pointInView) const {

    glm::vec4 pointVec4 = glm::vec4(point,1);
    glm::vec4 projectedPointVec4 = _ourModelViewProjectionMatrix * pointVec4;
    pointInView = (projectedPointVec4.w > 0); // math! If the w result is negative then the point is behind the viewer

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

OctreeProjectedPolygon ViewFrustum::getProjectedPolygon(const AACube& box) const {
    const glm::vec3& bottomNearRight = box.getCorner();
    glm::vec3 topFarLeft = box.calcTopFarLeft();

    int lookUp = ((_position.x < bottomNearRight.x)     )   //  1 = right      |   compute 6-bit
               + ((_position.x > topFarLeft.x     ) << 1)   //  2 = left       |         code to
               + ((_position.y < bottomNearRight.y) << 2)   //  4 = bottom     | classify camera
               + ((_position.y > topFarLeft.y     ) << 3)   //  8 = top        | with respect to
               + ((_position.z < bottomNearRight.z) << 4)   // 16 = front/near |  the 6 defining
               + ((_position.z > topFarLeft.z     ) << 5);  // 32 = back/far   |          planes

    int vertexCount = hullVertexLookup[lookUp][0];  //look up number of vertices

    OctreeProjectedPolygon projectedPolygon(vertexCount);

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

void ViewFrustum::getFurthestPointFromCameraVoxelScale(const AACube& box, glm::vec3& furthestPoint) const {
    const glm::vec3& bottomNearRight = box.getCorner();
    float scale = box.getScale();
    float halfScale = scale * 0.5f;

    if (_positionVoxelScale.x < bottomNearRight.x + halfScale) {
        // we are to the right of the center, so the left edge is furthest
        furthestPoint.x = bottomNearRight.x + scale;
    } else {
        furthestPoint.x = bottomNearRight.x;
    }

    if (_positionVoxelScale.y < bottomNearRight.y + halfScale) {
        // we are below of the center, so the top edge is furthest
        furthestPoint.y = bottomNearRight.y + scale;
    } else {
        furthestPoint.y = bottomNearRight.y;
    }

    if (_positionVoxelScale.z < bottomNearRight.z + halfScale) {
        // we are to the near side of the center, so the far side edge is furthest
        furthestPoint.z = bottomNearRight.z + scale;
    } else {
        furthestPoint.z = bottomNearRight.z;
    }
}

float ViewFrustum::distanceToCamera(const glm::vec3& point) const {
    glm::vec3 temp = getPosition() - point;
    float distanceToPoint = sqrtf(glm::dot(temp, temp));
    return distanceToPoint;
}


