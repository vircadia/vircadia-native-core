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

#include <QReadWriteLock>


class PolyLinePayload : public RenderableEntityItemProxy {
public:

    static uint8_t CUSTOM_PIPELINE_NUMBER;
    static render::ShapePipelinePointer shapePipelineFactory(const render::ShapePlumber& plumber, const render::ShapeKey& key);
    static void registerShapePipeline() {
        if (!CUSTOM_PIPELINE_NUMBER) {
            CUSTOM_PIPELINE_NUMBER = render::ShapePipeline::registerCustomShapePipelineFactory(shapePipelineFactory);
        }
    }
    static gpu::PipelinePointer _pipeline;
    static gpu::PipelinePointer _fadePipeline;

    static const int32_t PAINTSTROKE_TEXTURE_SLOT{ 0 };
    static const int32_t PAINTSTROKE_UNIFORM_SLOT{ 0 };

    PolyLinePayload(const EntityItemPointer& entity, render::ItemID metaID)
        : RenderableEntityItemProxy(entity, metaID) {}
    typedef render::Payload<PolyLinePayload> Payload;
    typedef Payload::DataPointer Pointer;

};

namespace render {
    template <> const ItemKey payloadGetKey(const PolyLinePayload::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const PolyLinePayload::Pointer& payload);
    template <> void payloadRender(const PolyLinePayload::Pointer& payload, RenderArgs* args);
    template <> uint32_t metaFetchMetaSubItems(const PolyLinePayload::Pointer& payload, ItemIDs& subItems);
    template <> const ShapeKey shapeGetShapeKey(const PolyLinePayload::Pointer& payload);
}

class RenderablePolyLineEntityItem : public PolyLineEntityItem, public SimplerRenderableEntitySupport {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderablePolyLineEntityItem(const EntityItemID& entityItemID);

    virtual void render(RenderArgs* args) override;
    virtual void update(const quint64& now) override;
    virtual bool needsToCallUpdate() const override { return true; }
    virtual bool addToScene(const EntityItemPointer& self,
        const render::ScenePointer& scene,
        render::Transaction& transaction) override;

    bool isTransparent() override { return true; }

    SIMPLE_RENDERABLE();

    NetworkTexturePointer _texture;

    static gpu::Stream::FormatPointer _format;

protected:
    void updateGeometry();
    void updateVertices();
    void updateMesh();

    static void createStreamFormat();

    gpu::BufferPointer _verticesBuffer;
    gpu::BufferView _uniformBuffer;
    unsigned int _numVertices;
    bool _empty { true };
    QVector<glm::vec3> _vertices;
};


#endif // hifi_RenderablePolyLineEntityItem_h
