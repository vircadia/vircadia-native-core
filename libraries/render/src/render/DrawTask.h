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

void renderItems(const RenderContextPointer& renderContext, const ItemBounds& inItems, int maxDrawnItems = -1);
void renderShapes(const RenderContextPointer& renderContext, const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems = -1, const ShapeKey& globalKey = ShapeKey());
void renderStateSortShapes(const RenderContextPointer& renderContext, const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems = -1, const ShapeKey& globalKey = ShapeKey());

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
    void run(const RenderContextPointer& renderContext, const ItemBounds& inLights);
protected:
    int _maxDrawn; // initialized by Config
};

class DrawBounds {
public:
    class Config : public render::JobConfig {
    public:
        Config(bool enabled = false) : JobConfig(enabled) {}
    };

    using Inputs = render::ItemBounds;
    using JobModel = render::Job::ModelI<DrawBounds, Inputs, Config>;

    void configure(const Config& configuration) {}
    void run(const render::RenderContextPointer& renderContext,
        const Inputs& items);

private:
    const gpu::PipelinePointer getPipeline();
    gpu::PipelinePointer _boundsPipeline;
    gpu::BufferPointer _drawBuffer;

    int _colorLocation { -1 };
};

}

#endif // hifi_render_DrawTask_h
