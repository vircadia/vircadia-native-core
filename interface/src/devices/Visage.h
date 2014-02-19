//
//  Visage.h
//  interface
//
//  Created by Andrzej Kapolka on 2/11/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Visage__
#define __interface__Visage__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace VisageSDK {
    class VisageTracker2;
    struct FaceData;
}

/// Handles input from the Visage webcam feature tracking software.
class Visage {
public:
    
    Visage();
    ~Visage();
    
    bool isActive() const { return _active; }
    
    const glm::quat& getHeadRotation() const { return _headRotation; }
    const glm::vec3& getHeadTranslation() const { return _headTranslation; }
    
    void update();
    void reset();
    
private:
    
    VisageSDK::VisageTracker2* _tracker;
    VisageSDK::FaceData* _data;
    
    bool _active;
    glm::quat _headRotation;
    glm::vec3 _headTranslation;
    
    glm::vec3 _headOrigin;
};

#endif /* defined(__interface__Visage__) */
