//
//  GeometryUtil.h
//  interface
//
//  Created by Andrzej Kapolka on 5/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__GeometryUtil__
#define __interface__GeometryUtil__

#include <glm/glm.hpp>

glm::vec3 computeVectorFromPointToSegment(const glm::vec3& point, const glm::vec3& start, const glm::vec3& end);

bool findSpherePenetration(const glm::vec3& penetratorToPenetratee, const glm::vec3& direction,
                           float combinedRadius, glm::vec3& penetration);

bool findSpherePointPenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
                                const glm::vec3& penetrateeLocation, glm::vec3& penetration);

bool findSphereSpherePenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
                                 const glm::vec3& penetrateeCenter, float penetrateeRadius, glm::vec3& penetration);
                     
bool findSphereSegmentPenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
                                  const glm::vec3& penetrateeStart, const glm::vec3& penetrateeEnd, glm::vec3& penetration);

bool findSphereCapsulePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, const glm::vec3& penetrateeStart,
                                  const glm::vec3& penetrateeEnd, float penetrateeRadius, glm::vec3& penetration);
                                  
bool findSpherePlanePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, 
                                const glm::vec4& penetrateePlane, glm::vec3& penetration);
                                  
bool findCapsuleSpherePenetration(const glm::vec3& penetratorStart, const glm::vec3& penetratorEnd, float penetratorRadius,
                                  const glm::vec3& penetrateeCenter, float penetrateeRadius, glm::vec3& penetration);

bool findCapsulePlanePenetration(const glm::vec3& penetratorStart, const glm::vec3& penetratorEnd, float penetratorRadius,
                                 const glm::vec4& penetrateePlane, glm::vec3& penetration);

glm::vec3 addPenetrations(const glm::vec3& currentPenetration, const glm::vec3& newPenetration);

bool doLineSegmentsIntersect(glm::vec2 r1p1, glm::vec2 r1p2, glm::vec2 r2p1, glm::vec2 r2p2);
bool isOnSegment(float xi, float yi, float xj, float yj, float xk, float yk);
int computeDirection(float xi, float yi, float xj, float yj, float xk, float yk);


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


#endif /* defined(__interface__GeometryUtil__) */
