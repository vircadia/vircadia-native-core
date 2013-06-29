//
//  GeometryUtil.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <cstring>

#include <Log.h>
#include <SharedUtil.h>

#include "GeometryUtil.h"

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

bool findSpherePenetration(const glm::vec3& penetratorToPenetratee, const glm::vec3& direction,
                           float combinedRadius, glm::vec3& penetration) {
    float vectorLength = glm::length(penetratorToPenetratee);
    if (vectorLength < EPSILON) {
        penetration = direction * combinedRadius;
        return true;
    }
    float distance = vectorLength - combinedRadius;
    if (distance < 0.0f) {
        penetration = penetratorToPenetratee * (-distance / vectorLength);
        return true;
    }
    return false;
}

bool findSpherePointPenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
                                const glm::vec3& penetrateeLocation, glm::vec3& penetration) {
    return findSpherePenetration(penetrateeLocation - penetratorCenter, glm::vec3(0.0f, -1.0f, 0.0f),
        penetratorRadius, penetration);
}

bool findSphereSpherePenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
                                 const glm::vec3& penetrateeCenter, float penetrateeRadius, glm::vec3& penetration) {
    return findSpherePointPenetration(penetratorCenter, penetratorRadius + penetrateeRadius, penetrateeCenter, penetration);
}

bool findSphereSegmentPenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
                                  const glm::vec3& penetrateeStart, const glm::vec3& penetrateeEnd, glm::vec3& penetration) {
    return findSpherePenetration(computeVectorFromPointToSegment(penetratorCenter, penetrateeStart, penetrateeEnd),
        glm::vec3(0.0f, -1.0f, 0.0f), penetratorRadius, penetration);
}

bool findSphereCapsulePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, const glm::vec3& penetrateeStart,
                                  const glm::vec3& penetrateeEnd, float penetrateeRadius, glm::vec3& penetration) {
    return findSphereSegmentPenetration(penetratorCenter, penetratorRadius + penetrateeRadius,
        penetrateeStart, penetrateeEnd, penetration);
}

bool findSpherePlanePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, 
                                const glm::vec4& penetrateePlane, glm::vec3& penetration) {
    float distance = glm::dot(penetrateePlane, glm::vec4(penetratorCenter, 1.0f)) - penetratorRadius;
    if (distance < 0.0f) {
        penetration = glm::vec3(penetrateePlane) * distance;
        return true;
    }
    return false;
}

bool findCapsuleSpherePenetration(const glm::vec3& penetratorStart, const glm::vec3& penetratorEnd, float penetratorRadius,
                                  const glm::vec3& penetrateeCenter, float penetrateeRadius, glm::vec3& penetration) {
    if (findSphereCapsulePenetration(penetrateeCenter, penetrateeRadius,
            penetratorStart, penetratorEnd, penetratorRadius, penetration)) {
        penetration = -penetration;
        return true;
    }
    return false;
}

bool findCapsulePlanePenetration(const glm::vec3& penetratorStart, const glm::vec3& penetratorEnd, float penetratorRadius,
                                 const glm::vec4& penetrateePlane, glm::vec3& penetration) {
    float distance = glm::min(glm::dot(penetrateePlane, glm::vec4(penetratorStart, 1.0f)),
        glm::dot(penetrateePlane, glm::vec4(penetratorEnd, 1.0f))) - penetratorRadius;
    if (distance < 0.0f) {
        penetration = glm::vec3(penetrateePlane) * distance;
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

// Do line segments (r1p1.x, r1p1.y)--(r1p2.x, r1p2.y) and (r2p1.x, r2p1.y)--(r2p2.x, r2p2.y) intersect?
// from: http://ptspts.blogspot.com/2010/06/how-to-determine-if-two-line-segments.html
bool doLineSegmentsIntersect(glm::vec2 r1p1, glm::vec2 r1p2, glm::vec2 r2p1, glm::vec2 r2p2) {
  int d1 = computeDirection(r2p1.x, r2p1.y, r2p2.x, r2p2.y, r1p1.x, r1p1.y);
  int d2 = computeDirection(r2p1.x, r2p1.y, r2p2.x, r2p2.y, r1p2.x, r1p2.y);
  int d3 = computeDirection(r1p1.x, r1p1.y, r1p2.x, r1p2.y, r2p1.x, r2p1.y);
  int d4 = computeDirection(r1p1.x, r1p1.y, r1p2.x, r1p2.y, r2p2.x, r2p2.y);
  return (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
          ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
          
          // I think these following conditions are what handle the case of one end point
          // being exactly on the second line. It seems like we sometimes want this to be
          // considered "inside" and other times, maybe we don't... 
            ||
         (d1 == 0 && isOnSegment(r2p1.x, r2p1.y, r2p2.x, r2p2.y, r1p1.x, r1p1.y)) ||
         (d2 == 0 && isOnSegment(r2p1.x, r2p1.y, r2p2.x, r2p2.y, r1p2.x, r1p2.y)) ||
         (d3 == 0 && isOnSegment(r1p1.x, r1p1.y, r1p2.x, r1p2.y, r2p1.x, r2p1.y)) ||
         (d4 == 0 && isOnSegment(r1p1.x, r1p1.y, r1p2.x, r1p2.y, r2p2.x, r2p2.y))
     ;
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
