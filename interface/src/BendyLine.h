//
//  BendyLine.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_bendyLine_h
#define hifi_bendyLine_h

#include <SharedUtil.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class BendyLine {
public:
    BendyLine();
    
    void update(float deltaTime);
    void reset();
    
    void setLength       (float     length       ) { _length        = length;       }
    void setThickness    (float     thickness    ) { _thickness     = thickness;    }
    void setSpringForce  (float     springForce  ) { _springForce   = springForce;  }
    void setTorqueForce  (float     torqueForce  ) { _torqueForce   = torqueForce;  }
    void setDrag         (float     drag         ) { _drag          = drag;         }
    void setBasePosition (glm::vec3 basePosition ) { _basePosition  = basePosition; }
    void setBaseDirection(glm::vec3 baseDirection) { _baseDirection = baseDirection;}
    void setGravityForce (glm::vec3 gravityForce ) { _gravityForce  = gravityForce; }
    
    glm::vec3 getBasePosition() { return _basePosition; }
    glm::vec3 getMidPosition () { return _midPosition;  }
    glm::vec3 getEndPosition () { return _endPosition;  }
    float     getThickness   () { return _thickness;    }
    
private:

    float     _springForce;
    float     _torqueForce;
    float     _drag;
    float     _length;
    float     _thickness;
    glm::vec3 _gravityForce;
    glm::vec3 _basePosition;				
    glm::vec3 _baseDirection;				
    glm::vec3 _midPosition;          
    glm::vec3 _endPosition;          
    glm::vec3 _midVelocity;          
    glm::vec3 _endVelocity;  
};

#endif
