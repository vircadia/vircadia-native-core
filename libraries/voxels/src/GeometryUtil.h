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

#endif /* defined(__interface__GeometryUtil__) */
