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

AnimContext::AnimContext(bool enableDebugDrawIKTargets, const glm::mat4& geometryToRigMatrix) :
    _enableDebugDrawIKTargets(enableDebugDrawIKTargets),
    _geometryToRigMatrix(geometryToRigMatrix) {
}
