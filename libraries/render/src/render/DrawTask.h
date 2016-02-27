//
//  DrawTask.h
//  render/src/render
//
//  Created by Sam Gateau on 5/21/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_DrawTask_h
#define hifi_render_DrawTask_h

#include "Engine.h"

namespace render {

void renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems);
void renderShapes(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems = -1);

class DrawLight {
public:
    using JobModel = Job::ModelI<DrawLight, ItemBounds>;

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inLights);
protected:
};

}

#endif // hifi_render_DrawTask_h
