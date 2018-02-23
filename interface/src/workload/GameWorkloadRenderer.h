//
//  GameWorkloadRender.h
//
//  Created by Sam Gateau on 2/20/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_GameWorkloadRenderer_h
#define hifi_GameWorkloadRenderer_h

#include "GameWorkload.h"

class GameSpaceToRender {
public:
    using Outputs = render::Transaction;
    using JobModel = workload::Job::ModelO<GameSpaceToRender, Outputs>;

    GameSpaceToRender() {}
    void run(const workload::WorkloadContextPointer& renderContext, Outputs& outputs);

protected:
    render::ItemID _spaceRenderItemID{ render::Item::INVALID_ITEM_ID };
};


class GameWorkloadRenderItem {
public:
    using Payload = render::Payload<GameWorkloadRenderItem>;
    using Pointer = Payload::DataPointer;

    GameWorkloadRenderItem() {}
    ~GameWorkloadRenderItem() {}
    void render(RenderArgs* args);

    render::Item::Bound& editBound() { _needUpdate = true; return _bound; }
    const render::Item::Bound& getBound() { return _bound; }

    void setVisible(bool visible) { _isVisible = visible; }
    bool isVisible() const { return _isVisible; }

    void setAllProxies(const std::vector<workload::Space::Proxy>& proxies);

protected:
    render::Item::Bound _bound;

    std::vector<workload::Space::Proxy> _myOwnProxies;
    gpu::BufferPointer _allProxiesBuffer;
    uint32_t _numAllProxies{ 0 };

    gpu::PipelinePointer _drawAllProxiesPipeline;
    const gpu::PipelinePointer getPipeline();

    bool _needUpdate{ true };
    bool _isVisible{ true };
};

namespace render {
    template <> const ItemKey payloadGetKey(const GameWorkloadRenderItem::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const GameWorkloadRenderItem::Pointer& payload);
    template <> void payloadRender(const GameWorkloadRenderItem::Pointer& payload, RenderArgs* args);
    template <> const ShapeKey shapeGetShapeKey(const GameWorkloadRenderItem::Pointer& payload);
    template <> int payloadGetLayer(const GameWorkloadRenderItem::Pointer& payloadData);


}

#endif