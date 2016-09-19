//
//  AnimContext.h
//
//  Created by Anthony J. Thibault on 9/19/16.
//  Copyright (c) 2016 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimContext_h
#define hifi_AnimContext_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class AnimContext {
public:
    AnimContext(bool enableDebugDrawIKTargets, const glm::mat4& geometryToRigMatrix);

    bool getEnableDebugDrawIKTargets() const { return _enableDebugDrawIKTargets; }
    const glm::mat4& getGeometryToRigMatrix() const { return _geometryToRigMatrix; }

protected:

    bool _enableDebugDrawIKTargets { false };
    glm::mat4 _geometryToRigMatrix;
};

#endif  // hifi_AnimContext_h
