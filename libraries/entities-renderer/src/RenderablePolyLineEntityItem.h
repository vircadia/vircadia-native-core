//
//  RenderablePolyLineEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Eric Levin on 6/22/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderablePolyLineEntityItem_h
#define hifi_RenderablePolyLineEntityItem_h


#include <gpu/Batch.h>
#include <GeometryCache.h>
#include <PolyLineEntityItem.h>
#include "RenderableEntityItem.h"
#include <TextureCache.h>
#include <Interpolate.h>

#include <QReadWriteLock>


class RenderablePolyLineEntityItem : public PolyLineEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static void createPipeline();
    RenderablePolyLineEntityItem(const EntityItemID& entityItemID);

    virtual void render(RenderArgs* args) override;
    virtual void update(const quint64& now) override;
    virtual bool needsToCallUpdate() const override { return true; }

    bool isTransparent() override { return Interpolate::calculateFadeRatio(_fadeStartTime) < 1.0f; }

    SIMPLE_RENDERABLE();

    NetworkTexturePointer _texture;

    static gpu::PipelinePointer _pipeline;
    static gpu::Stream::FormatPointer _format;
    static int32_t PAINTSTROKE_GPU_SLOT;

protected:
    void updateGeometry();
    void updateVertices();
    gpu::BufferPointer _verticesBuffer;
    gpu::BufferView _uniformBuffer;
    unsigned int _numVertices;
    QVector<glm::vec3> _vertices;
    quint64 _fadeStartTime { usecTimestampNow() };
};


#endif // hifi_RenderablePolyLineEntityItem_h
