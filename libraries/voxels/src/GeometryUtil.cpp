//
//  GeometryUtil.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "GeometryUtil.h"

glm::vec3 computeVectorFromPointToSegment(const glm::vec3& point, const glm::vec3& start, const glm::vec3& end) {
    // compute the projection of the point vector onto the segment vector
    glm::vec3 segmentVector = end - start;
    float proj = glm::dot(point - start, segmentVector) / glm::dot(segmentVector, segmentVector);
    if (proj <= 0.0f) { // closest to the start
        return start - point;
        
    } else if (proj >= 1.0f) { // closest to the end
        return end - point;
    
    } else { // closest to the middle
        return start + segmentVector*proj - point;
    }
}
