//
//  GeometryUtil.h
//  libraries/shared/src
//
//  Created by Andrzej Kapolka on 5/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GeometryUtil_h
#define hifi_GeometryUtil_h

#include <glm/glm.hpp>

glm::vec3 computeVectorFromPointToSegment(const glm::vec3& point, const glm::vec3& start, const glm::vec3& end);

/// Computes the penetration between a point and a sphere (centered at the origin)
/// \param point the point location relative to sphere center (origin)
/// \param defaultDirection the direction of the pentration when the point is near the origin
/// \param sphereRadius the radius of the sphere
/// \param penetration[out] the displacement that would move the point out of penetration with the sphere
/// \return true if point is inside sphere, otherwise false
bool findSpherePenetration(const glm::vec3& point, const glm::vec3& defaultDirection,
                           float sphereRadius, glm::vec3& penetration);

bool findSpherePointPenetration(const glm::vec3& sphereCenter, float sphereRadius,
                                const glm::vec3& point, glm::vec3& penetration);

bool findPointSpherePenetration(const glm::vec3& point, const glm::vec3& sphereCenter,
    float sphereRadius, glm::vec3& penetration);

bool findSphereSpherePenetration(const glm::vec3& firstCenter, float firstRadius,
                                 const glm::vec3& secondCenter, float secondRadius, glm::vec3& penetration);

bool findSphereSegmentPenetration(const glm::vec3& sphereCenter, float sphereRadius,
                                  const glm::vec3& segmentStart, const glm::vec3& segmentEnd, glm::vec3& penetration);

bool findSphereCapsulePenetration(const glm::vec3& sphereCenter, float sphereRadius, const glm::vec3& capsuleStart,
                                  const glm::vec3& capsuleEnd, float capsuleRadius, glm::vec3& penetration);

bool findPointCapsuleConePenetration(const glm::vec3& point, const glm::vec3& capsuleStart,
    const glm::vec3& capsuleEnd, float startRadius, float endRadius, glm::vec3& penetration);

bool findSphereCapsuleConePenetration(const glm::vec3& sphereCenter, float sphereRadius,
    const glm::vec3& capsuleStart, const glm::vec3& capsuleEnd,
    float startRadius, float endRadius, glm::vec3& penetration);

bool findSpherePlanePenetration(const glm::vec3& sphereCenter, float sphereRadius,
                                const glm::vec4& plane, glm::vec3& penetration);

/// Computes the penetration between a sphere and a disk.
/// \param sphereCenter center of sphere
/// \param sphereRadius radius of sphere
/// \param diskCenter center of disk
/// \param diskRadius radius of disk
/// \param diskNormal normal of disk plan
/// \param penetration[out] the depth that the sphere penetrates the disk
/// \return true if sphere touches disk (does not handle collisions with disk edge)
bool findSphereDiskPenetration(const glm::vec3& sphereCenter, float sphereRadius,
                               const glm::vec3& diskCenter, float diskRadius, float diskThickness, const glm::vec3& diskNormal,
                               glm::vec3& penetration);

bool findCapsuleSpherePenetration(const glm::vec3& capsuleStart, const glm::vec3& capsuleEnd, float capsuleRadius,
                                  const glm::vec3& sphereCenter, float sphereRadius, glm::vec3& penetration);

bool findCapsulePlanePenetration(const glm::vec3& capsuleStart, const glm::vec3& capsuleEnd, float capsuleRadius,
                                 const glm::vec4& plane, glm::vec3& penetration);

glm::vec3 addPenetrations(const glm::vec3& currentPenetration, const glm::vec3& newPenetration);

bool findRaySphereIntersection(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& center, float radius, float& distance);

bool findRayCapsuleIntersection(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& start, const glm::vec3& end, float radius, float& distance);

bool findRayRectangleIntersection(const glm::vec3& origin, const glm::vec3& direction, const glm::quat& rotation,
        const glm::vec3& position, const glm::vec2& dimensions, float& distance);

bool findRayTriangleIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& distance);

/// \brief decomposes rotation into its components such that: rotation = swing * twist
/// \param rotation[in] rotation to decompose
/// \param direction[in] normalized axis about which the twist happens (typically original direction before rotation applied)
/// \param swing[out] the swing part of rotation
/// \param twist[out] the twist part of rotation
void swingTwistDecomposition(const glm::quat& rotation,
        const glm::vec3& direction, // must be normalized
        glm::quat& swing,
        glm::quat& twist);

class Triangle {
public:
    glm::vec3 v0;
    glm::vec3 v1;
    glm::vec3 v2;
    glm::vec3 getNormal() const;
};

inline bool findRayTriangleIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                    const Triangle& triangle, float& distance) {
    return findRayTriangleIntersection(origin, direction, triangle.v0, triangle.v1, triangle.v2, distance);
}


bool doLineSegmentsIntersect(glm::vec2 r1p1, glm::vec2 r1p2, glm::vec2 r2p1, glm::vec2 r2p2);
bool isOnSegment(float xi, float yi, float xj, float yj, float xk, float yk);
int computeDirection(float xi, float yi, float xj, float yj, float xk, float yk);

// calculate the angle between a point on a sphere that is closest to the cone.
float coneSphereAngle(const glm::vec3& coneCenter, const glm::vec3& coneDirection, const glm::vec3& sphereCenter, float sphereRadius);

inline bool rayPlaneIntersection(const glm::vec3& planePosition, const glm::vec3& planeNormal,
                                 const glm::vec3& rayStart, const glm::vec3& rayDirection, float& distanceOut) {
    float rayDirectionDotPlaneNormal = glm::dot(rayDirection, planeNormal);
    const float PARALLEL_THRESHOLD = 0.0001f;
    if (fabsf(rayDirectionDotPlaneNormal) > PARALLEL_THRESHOLD) {
        float rayStartDotPlaneNormal = glm::dot(planePosition - rayStart, planeNormal);
        distanceOut = rayStartDotPlaneNormal / rayDirectionDotPlaneNormal;
        return true;
    } else {
        // ray is parallel to the plane
        return false;
    }
}

typedef glm::vec2 LineSegment2[2];

// Polygon Clipping routines inspired by, pseudo code found here: http://www.cs.rit.edu/~icss571/clipTrans/PolyClipBack.html
class PolygonClip {

public:
    static void clipToScreen(const glm::vec2* inputVertexArray, int length, glm::vec2*& outputVertexArray, int& outLength);

    static const float TOP_OF_CLIPPING_WINDOW;
    static const float BOTTOM_OF_CLIPPING_WINDOW;
    static const float LEFT_OF_CLIPPING_WINDOW;
    static const float RIGHT_OF_CLIPPING_WINDOW;

    static const glm::vec2 TOP_LEFT_CLIPPING_WINDOW;
    static const glm::vec2 TOP_RIGHT_CLIPPING_WINDOW;
    static const glm::vec2 BOTTOM_LEFT_CLIPPING_WINDOW;
    static const glm::vec2 BOTTOM_RIGHT_CLIPPING_WINDOW;

private:

    static void sutherlandHodgmanPolygonClip(glm::vec2* inVertexArray, glm::vec2* outVertexArray,
                                             int inLength, int& outLength,  const LineSegment2& clipBoundary);

    static bool pointInsideBoundary(const glm::vec2& testVertex, const LineSegment2& clipBoundary);

    static void segmentIntersectsBoundary(const glm::vec2& first, const glm::vec2&  second,
                                          const LineSegment2& clipBoundary, glm::vec2& intersection);

    static void appendPoint(glm::vec2 newVertex, int& outLength, glm::vec2* outVertexArray);

    static void copyCleanArray(int& lengthA, glm::vec2* vertexArrayA, int& lengthB, glm::vec2* vertexArrayB);
};

// vertical capsule
bool pointIsInsideCapsule(const glm::vec3& point, const glm::vec3& capsulePosition, float capsuleLength, float capsuleRadius);
glm::vec3 projectPointOntoCapsule(const glm::vec3& point, const glm::vec3& capsulePosition, float capsuleLength, float capsuleRadius);

#endif // hifi_GeometryUtil_h
