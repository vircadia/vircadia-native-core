//
//  ViewFrustum.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//
//  Simple view frustum class.
//
//

#include <algorithm>

#include <glm/gtx/transform.hpp>

#include "ViewFrustum.h"
#include "VoxelConstants.h"
#include "SharedUtil.h"
#include "Log.h"

using namespace std;

ViewFrustum::ViewFrustum() :
    _position(0,0,0),
    _orientation(),
    _direction(0,0,0),
    _up(0,0,0),
    _right(0,0,0),
    _fieldOfView(0.0),
    _aspectRatio(1.0),
    _nearClip(0.1),
    _farClip(500.0),
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

/////////////////////////////////////////////////////////////////////////////////////
// ViewFrustum::calculateViewFrustum()
//
// Description: this will calculate the view frustum bounds for a given position
//                 and direction
//
// Notes on how/why this works: 
//     http://www.lighthouse3d.com/tutorials/view-frustum-culling/view-frustums-shape/
//
void ViewFrustum::calculate() {
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
    glm::mat4 projection = glm::perspective(_fieldOfView, _aspectRatio, _nearClip, _farClip);
    glm::vec3 lookAt     = _position + _direction;
    glm::mat4 view       = glm::lookAt(_position, lookAt, _up);

    // Our ModelViewProjection : multiplication of our 3 matrices (note: model is identity, so we can drop it)
    _ourModelViewProjectionMatrix = projection * view; // Remember, matrix multiplication is the other way around
    
    // Set up our keyhole bounding box...
    glm::vec3 corner = _position - _keyholeRadius;
    _keyholeBoundingBox = AABox(corner,(_keyholeRadius * 2.0f));
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
ViewFrustum::location ViewFrustum::boxInKeyhole(const AABox& box) const {

    // First check to see if the box is in the bounding box for the sphere, if it's not, then we can short circuit
    // this and not check with sphere penetration which is more expensive
    if (!_keyholeBoundingBox.contains(box)) {
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

    for(int i=0; i < 6; i++) {
        glm::vec3 normal = _planes[i].getNormal();
        glm::vec3 boxVertexP = box.getVertexP(normal);
        float planeToBoxVertexPDistance = _planes[i].distance(boxVertexP);

        glm::vec3 boxVertexN = box.getVertexN(normal);
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

bool testMatches(glm::quat lhs, glm::quat rhs) {
    return (fabs(lhs.x - rhs.x) <= EPSILON && fabs(lhs.y - rhs.y) <= EPSILON && fabs(lhs.z - rhs.z) <= EPSILON
            && fabs(lhs.w - rhs.w) <= EPSILON);
}

bool testMatches(glm::vec3 lhs, glm::vec3 rhs) {
    return (fabs(lhs.x - rhs.x) <= EPSILON && fabs(lhs.y - rhs.y) <= EPSILON && fabs(lhs.z - rhs.z) <= EPSILON);
}

bool testMatches(float lhs, float rhs) {
    return (fabs(lhs - rhs) <= EPSILON);
}

bool ViewFrustum::matches(const ViewFrustum& compareTo, bool debug) const {
    bool result = 
           testMatches(compareTo._position,             _position            ) &&
           testMatches(compareTo._direction,            _direction           ) &&
           testMatches(compareTo._up,                   _up                  ) &&
           testMatches(compareTo._right,                _right               ) &&
           testMatches(compareTo._fieldOfView,          _fieldOfView         ) &&
           testMatches(compareTo._aspectRatio,          _aspectRatio         ) &&
           testMatches(compareTo._nearClip,             _nearClip            ) &&
           testMatches(compareTo._farClip,              _farClip             ) &&
           testMatches(compareTo._eyeOffsetPosition,    _eyeOffsetPosition   ) &&
           testMatches(compareTo._eyeOffsetOrientation, _eyeOffsetOrientation);

    if (!result && debug) {
        printLog("ViewFrustum::matches()... result=%s\n", debug::valueOf(result));
        printLog("%s -- compareTo._position=%f,%f,%f _position=%f,%f,%f\n", 
            (testMatches(compareTo._position,_position) ? "MATCHES " : "NO MATCH"),
            compareTo._position.x, compareTo._position.y, compareTo._position.z,
            _position.x, _position.y, _position.z );
        printLog("%s -- compareTo._direction=%f,%f,%f _direction=%f,%f,%f\n", 
            (testMatches(compareTo._direction, _direction) ? "MATCHES " : "NO MATCH"),
            compareTo._direction.x, compareTo._direction.y, compareTo._direction.z,
            _direction.x, _direction.y, _direction.z );
        printLog("%s -- compareTo._up=%f,%f,%f _up=%f,%f,%f\n", 
            (testMatches(compareTo._up, _up) ? "MATCHES " : "NO MATCH"),
            compareTo._up.x, compareTo._up.y, compareTo._up.z,
            _up.x, _up.y, _up.z );
        printLog("%s -- compareTo._right=%f,%f,%f _right=%f,%f,%f\n", 
            (testMatches(compareTo._right, _right) ? "MATCHES " : "NO MATCH"),
            compareTo._right.x, compareTo._right.y, compareTo._right.z,
            _right.x, _right.y, _right.z );
        printLog("%s -- compareTo._fieldOfView=%f _fieldOfView=%f\n", 
            (testMatches(compareTo._fieldOfView, _fieldOfView) ? "MATCHES " : "NO MATCH"),
            compareTo._fieldOfView, _fieldOfView);
        printLog("%s -- compareTo._aspectRatio=%f _aspectRatio=%f\n", 
            (testMatches(compareTo._aspectRatio, _aspectRatio) ? "MATCHES " : "NO MATCH"),
            compareTo._aspectRatio, _aspectRatio);
        printLog("%s -- compareTo._nearClip=%f _nearClip=%f\n", 
            (testMatches(compareTo._nearClip, _nearClip) ? "MATCHES " : "NO MATCH"),
            compareTo._nearClip, _nearClip);
        printLog("%s -- compareTo._farClip=%f _farClip=%f\n", 
            (testMatches(compareTo._farClip, _farClip) ? "MATCHES " : "NO MATCH"),
            compareTo._farClip, _farClip);
        printLog("%s -- compareTo._eyeOffsetPosition=%f,%f,%f _eyeOffsetPosition=%f,%f,%f\n", 
            (testMatches(compareTo._eyeOffsetPosition, _eyeOffsetPosition) ? "MATCHES " : "NO MATCH"),
            compareTo._eyeOffsetPosition.x, compareTo._eyeOffsetPosition.y, compareTo._eyeOffsetPosition.z,
            _eyeOffsetPosition.x, _eyeOffsetPosition.y, _eyeOffsetPosition.z);
        printLog("%s -- compareTo._eyeOffsetOrientation=%f,%f,%f,%f _eyeOffsetOrientation=%f,%f,%f,%f\n", 
            (testMatches(compareTo._eyeOffsetOrientation, _eyeOffsetOrientation) ? "MATCHES " : "NO MATCH"),
            compareTo._eyeOffsetOrientation.x, compareTo._eyeOffsetOrientation.y,
                compareTo._eyeOffsetOrientation.z, compareTo._eyeOffsetOrientation.w,
            _eyeOffsetOrientation.x, _eyeOffsetOrientation.y, _eyeOffsetOrientation.z, _eyeOffsetOrientation.w);
    }
    return result;
}



void ViewFrustum::computePickRay(float x, float y, glm::vec3& origin, glm::vec3& direction) const {
    origin = _nearTopLeft + x*(_nearTopRight - _nearTopLeft) + y*(_nearBottomLeft - _nearTopLeft);
    direction = glm::normalize(origin - _position);
}

void ViewFrustum::computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& near, float& far,
                                        glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const {
    // compute our dimensions the usual way
    float hheight = _nearClip * tanf(_fieldOfView * 0.5f * PI_OVER_180);
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
    near = FLT_MAX;
    far = -FLT_MAX;
    for (int i = 0; i < 8; i++) {
        near = min(near, -corners[i].z);
        far = max(far, -corners[i].z);
    }
    
    // get the near/far normal and use it to find the clip planes
    glm::vec4 normal = eyeMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    nearClipPlane = glm::vec4(-normal.x, -normal.y, -normal.z, glm::dot(normal, corners[0]));
    farClipPlane = glm::vec4(normal.x, normal.y, normal.z, -glm::dot(normal, corners[4]));
    
    // get the extents at Z = -near
    left = FLT_MAX;
    right = -FLT_MAX;
    bottom = FLT_MAX;
    top = -FLT_MAX;
    for (int i = 0; i < 4; i++) {
        glm::vec4 intersection = corners[i] * (-near / corners[i].z);
        left = min(left, intersection.x);
        right = max(right, intersection.x);
        bottom = min(bottom, intersection.y);
        top = max(top, intersection.y);
    }
}

void ViewFrustum::printDebugDetails() const {
    printLog("ViewFrustum::printDebugDetails()... \n");
    printLog("_position=%f,%f,%f\n",  _position.x, _position.y, _position.z );
    printLog("_direction=%f,%f,%f\n", _direction.x, _direction.y, _direction.z );
    printLog("_up=%f,%f,%f\n", _up.x, _up.y, _up.z );
    printLog("_right=%f,%f,%f\n", _right.x, _right.y, _right.z );
    printLog("_fieldOfView=%f\n", _fieldOfView);
    printLog("_aspectRatio=%f\n", _aspectRatio);
    printLog("_nearClip=%f\n", _nearClip);
    printLog("_farClip=%f\n", _farClip);
    printLog("_eyeOffsetPosition=%f,%f,%f\n",  _eyeOffsetPosition.x, _eyeOffsetPosition.y, _eyeOffsetPosition.z );
    printLog("_eyeOffsetOrientation=%f,%f,%f,%f\n",  _eyeOffsetOrientation.x, _eyeOffsetOrientation.y, _eyeOffsetOrientation.z,
        _eyeOffsetOrientation.w );
}

glm::vec2 ViewFrustum::projectPoint(glm::vec3 point, bool& pointInView) const {

    glm::vec4 pointVec4 = glm::vec4(point,1)    ;
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

const int hullVertexLookup[MAX_POSSIBLE_COMBINATIONS][MAX_SHADOW_VERTEX_COUNT+1] = {
    // Number of vertices in shadow polygon for the visible faces, then a list of the index of each vertice from the AABox
    {0}, // inside
    {4, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_RIGHT_NEAR}, // right
    {4, BOTTOM_LEFT_NEAR,  BOTTOM_LEFT_FAR,  TOP_LEFT_FAR,  TOP_LEFT_NEAR},  // left 
    {0}, // n/a
    {4, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR}, // bottom
    {6, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR},//bottom, right
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR, BOTTOM_LEFT_NEAR},//bottom, left
    {0}, // n/a
    {4, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR},         // top
    {6, TOP_RIGHT_NEAR, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR},   // top, right
    {6, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR, TOP_LEFT_NEAR},   // top, left
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
    {4, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR, BOTTOM_LEFT_NEAR}, // front or near
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR, BOTTOM_LEFT_NEAR}, // front, right
    {6, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR, TOP_LEFT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR}, // front, left
    {0}, // n/a
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR, TOP_LEFT_NEAR, TOP_RIGHT_NEAR}, // front,bottom
    {6, BOTTOM_LEFT_NEAR, TOP_LEFT_NEAR, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR}, //front,bottom,right
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR, TOP_RIGHT_NEAR}, //front,bottom,left
    {0}, // n/a
    {6, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR, BOTTOM_LEFT_NEAR}, // front, top
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR, BOTTOM_LEFT_NEAR}, // front, top, right
    {6, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR}, // front, top, left
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
    {0}, // n/a
    {4, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, BOTTOM_LEFT_FAR}, // back
    {6, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, BOTTOM_LEFT_FAR, BOTTOM_RIGHT_FAR}, // back, right
    {6, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR, BOTTOM_LEFT_NEAR, BOTTOM_LEFT_FAR}, // back, left
    {0}, // n/a
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR}, // back, bottom
    {6, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR},//back, bottom, right
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR, BOTTOM_LEFT_NEAR},//back, bottom, left
    {0}, // n/a
    {6, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR, TOP_LEFT_FAR, BOTTOM_LEFT_FAR}, // back, top
    {6, BOTTOM_RIGHT_NEAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, TOP_LEFT_FAR, TOP_LEFT_NEAR, TOP_RIGHT_NEAR}, // back, top, right
    {6, TOP_RIGHT_NEAR, TOP_RIGHT_FAR, BOTTOM_RIGHT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR, TOP_LEFT_NEAR}, // back, top, left
};

VoxelProjectedPolygon ViewFrustum::getProjectedPolygon(const AABox& box) const {
    glm::vec3 bottomNearRight = box.getCorner();
    glm::vec3 topFarLeft      = box.getCorner() + box.getSize();
    int lookUp = ((_position.x < bottomNearRight.x)     )   //  1 = right      |   compute 6-bit
               + ((_position.x > topFarLeft.x     ) << 1)   //  2 = left       |         code to
               + ((_position.y < bottomNearRight.y) << 2)   //  4 = bottom     | classify camera
               + ((_position.y > topFarLeft.y     ) << 3)   //  8 = top        | with respect to
               + ((_position.z < bottomNearRight.z) << 4)   // 16 = front/near |  the 6 defining
               + ((_position.z > topFarLeft.z     ) << 5);  // 32 = back/far   |          planes

    int vertexCount = hullVertexLookup[lookUp][0];  //look up number of vertices

    VoxelProjectedPolygon shadow(vertexCount);
    
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
            shadow.setVertex(i, projectedPoint);
        }
    }
    // set the distance from our camera position, to the closest vertex
    float distance = glm::distance(getPosition(), box.getCenter());
    shadow.setDistance(distance);
    shadow.setAnyInView(anyPointsInView);
    shadow.setAllInView(allPointsInView);
    return shadow;
}
