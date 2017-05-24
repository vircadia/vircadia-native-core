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
    AnimContext(bool enableDebugDrawIKTargets, bool enableDebugDrawIKConstraints, bool enableDebugDrawIKChains,
                const glm::mat4& geometryToRigMatrix, const glm::mat4& rigToWorldMatrix);

    bool getEnableDebugDrawIKTargets() const { return _enableDebugDrawIKTargets; }
    bool getEnableDebugDrawIKConstraints() const { return _enableDebugDrawIKConstraints; }
    bool getEnableDebugDrawIKChains() const { return _enableDebugDrawIKChains; }
    const glm::mat4& getGeometryToRigMatrix() const { return _geometryToRigMatrix; }
    const glm::mat4& getRigToWorldMatrix() const { return _rigToWorldMatrix; }

protected:

    bool _enableDebugDrawIKTargets { false };
    bool _enableDebugDrawIKConstraints { false };
    bool _enableDebugDrawIKChains { false };
    glm::mat4 _geometryToRigMatrix;
    glm::mat4 _rigToWorldMatrix;
};

#endif  // hifi_AnimContext_h
