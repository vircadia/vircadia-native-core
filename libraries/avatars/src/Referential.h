//
//  Referential.h
//
//
//  Created by Clement on 7/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Referential_h
#define hifi_Referential_h

#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

class AvatarData;

class Referential {
public:
    virtual ~Referential();
    
    virtual bool isValid() { return _isValid; }
    virtual void update() = 0;
    
protected:
    Referential(AvatarData* avatar);
    
    bool _isValid;
    AvatarData* _avatar;
    
    glm::vec3 _refPosition;
    glm::quat _refRotation;
    float _refScale;
    
    glm::vec3 _translation;
    glm::quat _rotation;
    float _scale;
};


#endif // hifi_Referential_h