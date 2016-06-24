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

void renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, int maxDrawnItems = -1);
void renderShapes(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems = -1);
void renderStateSortShapes(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems = -1);



class DrawLightConfig : public Job::Config {
    Q_OBJECT
    Q_PROPERTY(int numDrawn READ getNumDrawn NOTIFY numDrawnChanged)
    Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
public:
    int getNumDrawn() { return numDrawn; }
    void setNumDrawn(int num) { numDrawn = num; emit numDrawnChanged(); }

    int maxDrawn{ -1 };
signals:
    void numDrawnChanged();
    void dirty();

protected:
    int numDrawn{ 0 };
};

class DrawLight {
public:
    using Config = DrawLightConfig;
    using JobModel = Job::ModelI<DrawLight, ItemBounds, Config>;

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; }
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inLights);
protected:
    int _maxDrawn; // initialized by Config
};

}

#endif // hifi_render_DrawTask_h
