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
#include <glm/gtx/quaternion.hpp>

#include "NumericalConstants.h"

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

bool findRaySphereIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& center, float radius, float& distance) {
    glm::vec3 relativeOrigin = origin - center;
    float c = glm::dot(relativeOrigin, relativeOrigin) - radius * radius;
    if (c < 0.0f) {
        distance = 0.0f;
        return true; // starts inside the sphere
    }
    float b = glm::dot(direction, relativeOrigin);
    float radicand = b * b - c;
    if (radicand < 0.0f) {
        return false; // doesn't hit the sphere
    }
    float t = -b - sqrtf(radicand);
    if (t < 0.0f) {
        return false; // doesn't hit the sphere
    }
    distance = t;
    return true;
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

bool findRayTriangleIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& distance) {
    glm::vec3 firstSide = v0 - v1;
    glm::vec3 secondSide = v2 - v1;
    glm::vec3 normal = glm::cross(secondSide, firstSide);
    float dividend = glm::dot(normal, v1) - glm::dot(origin, normal);
    if (dividend > 0.0f) {
        return false; // origin below plane
    }
    float divisor = glm::dot(normal, direction);
    if (divisor >= 0.0f) {
        return false;
    }
    float t = dividend / divisor;
    glm::vec3 point = origin + direction * t;
    if (glm::dot(normal, glm::cross(point - v1, firstSide)) > 0.0f &&
            glm::dot(normal, glm::cross(secondSide, point - v1)) > 0.0f &&
            glm::dot(normal, glm::cross(point - v0, v2 - v0)) > 0.0f) {
        distance = t;
        return true;
    }
    return false;
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
    memcpy(tempVertexArrayA, inputVertexArray, sizeof(glm::vec2) * inLength);

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
