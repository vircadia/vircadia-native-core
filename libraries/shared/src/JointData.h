
#ifndef hifi_JointData_h
#define hifi_JointData_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Used by the avatar mixer to describe a single joint
// These are relative to their parent and translations are in meters
class JointData {
public:
    glm::quat rotation;
    bool rotationSet = false;
    glm::vec3 translation;  // meters
    bool translationSet = false;
};

#endif
