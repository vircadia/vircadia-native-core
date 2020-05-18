
#ifndef hifi_JointData_h
#define hifi_JointData_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class EntityJointData {
public:
    glm::quat rotation;
    glm::vec3 translation;
    bool rotationSet = false;
    bool translationSet = false;
};

// Used by the avatar mixer to describe a single joint
// Translations relative to their parent joint
// Rotations are absolute (i.e. not relative to parent) and are in rig space.
// No JSDoc because it's not provided as a type to the script engine.
class JointData {
public:
    glm::quat rotation;
    glm::vec3 translation;

    // This indicates that the rotation or translation is the same as the defaultPose for the avatar.
    // if true, it also means that the rotation or translation value in this structure is not valid and
    // should be replaced by the avatar's actual default pose value.
    bool rotationIsDefaultPose = true;
    bool translationIsDefaultPose = true;
};

#endif
