//
//  RenderableDebugableEntityItem.h
// libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/1/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableDebugableEntityItem_h
#define hifi_RenderableDebugableEntityItem_h

#include <EntityItem.h>

class RenderableDebugableEntityItem {
public:
    static void renderBoundingBox(EntityItem* entity, RenderArgs* args, float puffedOut, glm::vec4& color);
    static void render(EntityItem* entity, RenderArgs* args);
};

#endif // hifi_RenderableDebugableEntityItem_h
