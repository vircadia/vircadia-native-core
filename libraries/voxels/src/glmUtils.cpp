//
//  glmUtils.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 4/12/13.
//
//

#include <glm/glm.hpp>


// XXXBHG - These handy operators should probably go somewhere else, I'm surprised they don't
// already exist somewhere in OpenGL. Maybe someone can point me to them if they do exist!
glm::vec3 operator* (float lhs, const glm::vec3& rhs)
{
    glm::vec3 result = rhs;
    result.x *= lhs;
    result.y *= lhs;
    result.z *= lhs;
    return result;
}

// XXXBHG - These handy operators should probably go somewhere else, I'm surprised they don't
// already exist somewhere in OpenGL. Maybe someone can point me to them if they do exist!
glm::vec3 operator* (const glm::vec3& lhs, float rhs)
{
    glm::vec3 result = lhs;
    result.x *= rhs;
    result.y *= rhs;
    result.z *= rhs;
    return result;
}

