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

class GameSpaceToRenderConfig : public workload::Job::Config {
    Q_OBJECT
    Q_PROPERTY(bool freezeViews MEMBER freezeViews NOTIFY dirty)
    Q_PROPERTY(bool showProxies MEMBER showProxies NOTIFY dirty)
    Q_PROPERTY(bool showViews MEMBER showViews NOTIFY dirty)
public:

    bool freezeViews{ false };
    bool showProxies{ false };
    bool showViews{ false };
signals:
    void dirty();

protected:
};

class GameSpaceToRender {
public:
    using Config = GameSpaceToRenderConfig;
    using Outputs = render::Transaction;
    using JobModel = workload::Job::ModelO<GameSpaceToRender, Outputs, Config>;

    GameSpaceToRender() {}

    void configure(const Config& config);
    void run(const workload::WorkloadContextPointer& renderContext, Outputs& outputs);

protected:
    render::ItemID _spaceRenderItemID{ render::Item::INVALID_ITEM_ID };
    bool _freezeViews{ false };
    bool _showAllProxies{ false };
    bool _showAllViews{ false };
};


class GameWorkloadRenderItem {
public:
    using Payload = render::Payload<GameWorkloadRenderItem>;
    using Pointer = Payload::DataPointer;

    GameWorkloadRenderItem();
    ~GameWorkloadRenderItem() {}
    void render(RenderArgs* args);

    render::Item::Bound& editBound() { return _bound; }
    const render::Item::Bound& getBound(RenderArgs* args) { return _bound; }

    void setVisible(bool visible);
    void showProxies(bool show);
    void showViews(bool show);

    void setAllProxies(const workload::Proxy::Vector& proxies);
    void setAllViews(const workload::Views& views);

    render::ItemKey getKey() const;

protected:
    render::Item::Bound _bound;

    workload::Proxy::Vector _myOwnProxies;
    gpu::BufferPointer _allProxiesBuffer;
    uint32_t _numAllProxies{ 0 };

    workload::Views _myOwnViews;
    gpu::BufferPointer _allViewsBuffer;
    uint32_t _numAllViews{ 0 };

    gpu::PipelinePointer _drawAllProxiesPipeline;
    const gpu::PipelinePointer getProxiesPipeline();

    gpu::PipelinePointer _drawAllViewsPipeline;
    const gpu::PipelinePointer getViewsPipeline();

    uint32_t _numDrawViewVerts{ 0 };
    gpu::BufferPointer _drawViewBuffer;
    const gpu::BufferPointer getDrawViewBuffer();

    render::ItemKey _key;
    bool _showProxies{ true };
    bool _showViews{ true };
};

namespace render {
    template <> const ItemKey payloadGetKey(const GameWorkloadRenderItem::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const GameWorkloadRenderItem::Pointer& payload, RenderArgs* args);
    template <> void payloadRender(const GameWorkloadRenderItem::Pointer& payload, RenderArgs* args);
    template <> const ShapeKey shapeGetShapeKey(const GameWorkloadRenderItem::Pointer& payload);
}

#endif
