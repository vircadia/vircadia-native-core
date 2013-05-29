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

#endif /* defined(__interface__GeometryUtil__) */
