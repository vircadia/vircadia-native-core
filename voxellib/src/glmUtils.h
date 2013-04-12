//
//  glmUtils.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 4/12/13.
//
//

#ifndef __hifi__glmUtils__
#define __hifi__glmUtils__

#include <glm/glm.hpp>

glm::vec3 operator* (float lhs, const glm::vec3& rhs);
glm::vec3 operator* (const glm::vec3& lhs, float rhs);


#endif // __hifi__glmUtils__