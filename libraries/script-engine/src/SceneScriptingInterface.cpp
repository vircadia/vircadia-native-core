//
//  SceneScriptingInterface.cpp
//  libraries/script-engine
//
//  Created by Sam Gateau on 2/24/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SceneScriptingInterface.h"

void SceneScriptingInterface::setShouldRenderAvatars(bool shouldRenderAvatars) {
    if (shouldRenderAvatars != _shouldRenderAvatars) {
        _shouldRenderAvatars = shouldRenderAvatars;
        emit shouldRenderAvatarsChanged(_shouldRenderAvatars);
    }
}

void SceneScriptingInterface::setShouldRenderEntities(bool shouldRenderEntities) {
    if (shouldRenderEntities != _shouldRenderEntities) {
        _shouldRenderEntities = shouldRenderEntities;
        emit shouldRenderEntitiesChanged(_shouldRenderEntities);
    }
}