//
//  AnimContext.cpp
//
//  Created by Anthony J. Thibault on 9/19/16.
//  Copyright (c) 2016 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimContext.h"

AnimContext::AnimContext(bool enableDebugDrawIKTargets, bool enableDebugDrawIKConstraints, bool enableDebugDrawIKChains,
                         const glm::mat4& geometryToRigMatrix, const glm::mat4& rigToWorldMatrix) :
    _enableDebugDrawIKTargets(enableDebugDrawIKTargets),
    _enableDebugDrawIKConstraints(enableDebugDrawIKConstraints),
    _enableDebugDrawIKChains(enableDebugDrawIKChains),
    _geometryToRigMatrix(geometryToRigMatrix),
    _rigToWorldMatrix(rigToWorldMatrix)
{
}
