//
//  GameWorkload.cpp
//
//  Created by Sam Gateau on 2/16/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GameWorkload.h"

GameWorkloadContext::GameWorkloadContext(const render::ScenePointer& scene) : WorkloadContext(), _scene(scene) {
}

GameWorkloadContext::~GameWorkloadContext() {
}


GameWorkload::GameWorkload() {
}

GameWorkload::~GameWorkload() {
    shutdown();
}

void GameWorkload::startup(const render::ScenePointer& scene) {
    _engine.reset(new workload::Engine(std::make_shared<GameWorkloadContext>(scene)));

    _engine->addJob<GameSpaceToRender>("SpaceToRender");
}

void GameWorkload::shutdown() {
    _engine.reset();
}





class GameWorkloadRenderItem {
public:
    using Payload = render::Payload<GameWorkloadRenderItem>;
    using Pointer = Payload::DataPointer;

    GameWorkloadRenderItem() {}
    ~GameWorkloadRenderItem() {}
    void render(RenderArgs* args) {}

    render::Item::Bound& editBound() { _needUpdate = true; return _bound; }
    const render::Item::Bound& getBound() { return _bound; }

    void setVisible(bool visible) { _isVisible = visible; }
    bool isVisible() const { return _isVisible; }

protected:
    render::Item::Bound _bound;

    bool _needUpdate{ true };
    bool _isVisible{ true };
};

namespace render {
    template <> const ItemKey payloadGetKey(const GameWorkloadRenderItem::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const GameWorkloadRenderItem::Pointer& payload);
    template <> void payloadRender(const GameWorkloadRenderItem::Pointer& payload, RenderArgs* args);
}

namespace render {
    template <> const ItemKey payloadGetKey(const GameWorkloadRenderItem::Pointer& payload) {
        auto builder = ItemKey::Builder().withTypeShape();
        return builder.build();
    }
    template <> const Item::Bound payloadGetBound(const GameWorkloadRenderItem::Pointer& payload) {
        if (payload) {
            return payload->getBound();
        }
        return Item::Bound();
    }
    template <> void payloadRender(const GameWorkloadRenderItem::Pointer& payload, RenderArgs* args) {
        if (payload) {
            payload->render(args);
        }
    }
}


void GameSpaceToRender::run(const workload::WorkloadContextPointer& runContext, Outputs& outputs) {
    auto gameWorkloadContext = std::dynamic_pointer_cast<GameWorkloadContext>(runContext);
    if (!gameWorkloadContext) {
        return;
    }

    auto scene = gameWorkloadContext->_scene;

    // Valid space, let's display its content
    render::Transaction transaction;
    if (!render::Item::isValidID(_spaceRenderItemID)) {
        _spaceRenderItemID = scene->allocateID();
        auto renderItem = std::make_shared<GameWorkloadRenderItem>();
        renderItem->editBound().expandedContains(glm::vec3(0.0), 32000.0);
        transaction.resetItem(_spaceRenderItemID, std::make_shared<GameWorkloadRenderItem::Payload>(std::make_shared<GameWorkloadRenderItem>()));
    }

    scene->enqueueTransaction(transaction);

    auto space = gameWorkloadContext->_space;
    if (!space) {
        return;
    }


}