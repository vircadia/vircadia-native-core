//
//  GameWorkloadRender.cpp
//
//  Created by Sam Gateau on 2/20/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GameWorkloadRenderer.h"

#include <cstring>
#include <gpu/Context.h>

#include <StencilMaskPass.h>
#include <GeometryCache.h>

#include "render-utils/drawWorkloadProxy_vert.h"
#include "render-utils/drawWorkloadView_vert.h"
#include "render-utils/drawWorkloadProxy_frag.h"


void GameSpaceToRender::configure(const Config& config) {
    _showAllProxies = config.showProxies;
    _showAllViews = config.showViews;
}

void GameSpaceToRender::run(const workload::WorkloadContextPointer& runContext, Outputs& outputs) {
    auto gameWorkloadContext = std::dynamic_pointer_cast<GameWorkloadContext>(runContext);
    if (!gameWorkloadContext) {
        return;
    }
    auto space = gameWorkloadContext->_space;
    if (!space) {
        return;
    }

    auto visible = _showAllProxies || _showAllViews;
    auto showProxies = _showAllProxies;
    auto showViews = _showAllViews;

    render::Transaction transaction;
    auto scene = gameWorkloadContext->_scene;

    // Nothing really needed, early exit
    if (!visible) {
        if (render::Item::isValidID(_spaceRenderItemID)) {
            transaction.updateItem<GameWorkloadRenderItem>(_spaceRenderItemID, [](GameWorkloadRenderItem& item) {
                item.setVisible(false);
            });
            scene->enqueueTransaction(transaction);
        }
        return;
    }

    std::vector<workload::Space::Proxy> proxies(space->getNumAllocatedProxies());
    space->copyProxyValues(proxies.data(), (uint32_t)proxies.size());

    std::vector<workload::Space::View> views(space->getNumViews());
    space->copyViews(views);

    // Valid space, let's display its content
    if (!render::Item::isValidID(_spaceRenderItemID)) {
        _spaceRenderItemID = scene->allocateID();
        auto renderItem = std::make_shared<GameWorkloadRenderItem>();
        renderItem->editBound().setBox(glm::vec3(-100.0f), 200.0f);
        renderItem->setAllProxies(proxies);
        transaction.resetItem(_spaceRenderItemID, std::make_shared<GameWorkloadRenderItem::Payload>(renderItem));
    }
    
    transaction.updateItem<GameWorkloadRenderItem>(_spaceRenderItemID, [visible, showProxies, proxies, showViews, views](GameWorkloadRenderItem& item) {
        item.setVisible(visible);
        item.showProxies(showProxies);
        item.setAllProxies(proxies);
        item.showViews(showViews);
        item.setAllViews(views);
    });
    
    scene->enqueueTransaction(transaction);
}

namespace render {
    template <> const ItemKey payloadGetKey(const GameWorkloadRenderItem::Pointer& payload) {
        return payload->getKey();
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
    template <> const ShapeKey shapeGetShapeKey(const GameWorkloadRenderItem::Pointer& payload) {
        return ShapeKey::Builder::ownPipeline();
    }
    template <> int payloadGetLayer(const GameWorkloadRenderItem::Pointer& payloadData) {
        return render::Item::LAYER_3D_FRONT;
    }

}

GameWorkloadRenderItem::GameWorkloadRenderItem() : _key(render::ItemKey::Builder::opaqueShape().withTagBits(render::ItemKey::TAG_BITS_0 | render::ItemKey::TAG_BITS_1)) {
}

render::ItemKey GameWorkloadRenderItem::getKey() const {
    return _key;
}

void GameWorkloadRenderItem::setVisible(bool isVisible) {
    if (isVisible) {
        _key = render::ItemKey::Builder(_key).withVisible();
    } else {
        _key = render::ItemKey::Builder(_key).withInvisible();
    }
}

void GameWorkloadRenderItem::showProxies(bool show) {
    _showProxies = show;
}

void GameWorkloadRenderItem::showViews(bool show) {
    _showViews = show;
}


void GameWorkloadRenderItem::setAllProxies(const std::vector<workload::Space::Proxy>& proxies) {
    _myOwnProxies = proxies;
    static const uint32_t sizeOfProxy = sizeof(workload::Space::Proxy);
    if (!_allProxiesBuffer) {
        _allProxiesBuffer = std::make_shared<gpu::Buffer>(sizeOfProxy);
    }

    _allProxiesBuffer->setData(proxies.size() * sizeOfProxy, (const gpu::Byte*) proxies.data());
    _numAllProxies = (uint32_t) proxies.size();
}

void GameWorkloadRenderItem::setAllViews(const std::vector<workload::Space::View>& views) {
    _myOwnViews = views;
    static const uint32_t sizeOfView = sizeof(workload::Space::View);
    if (!_allViewsBuffer) {
        _allViewsBuffer = std::make_shared<gpu::Buffer>(sizeOfView);
    }

    _allViewsBuffer->setData(views.size() * sizeOfView, (const gpu::Byte*) views.data());
    _numAllViews = (uint32_t)views.size();
}

const gpu::PipelinePointer GameWorkloadRenderItem::getProxiesPipeline() {
    if (!_drawAllProxiesPipeline) {
        auto vs = drawWorkloadProxy_vert::getShader();
        auto ps = drawWorkloadProxy_frag::getShader();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("workloadProxiesBuffer", 0));
        gpu::Shader::makeProgram(*program, slotBindings);

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
      /*  state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);*/

        PrepareStencil::testMaskDrawShape(*state);
        state->setCullMode(gpu::State::CULL_NONE);
        _drawAllProxiesPipeline = gpu::Pipeline::create(program, state);
    }
    return _drawAllProxiesPipeline;
}


const gpu::PipelinePointer GameWorkloadRenderItem::getViewsPipeline() {
    if (!_drawAllViewsPipeline) {
        auto vs = drawWorkloadView_vert::getShader();
        auto ps = drawWorkloadProxy_frag::getShader();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("workloadViewsBuffer", 1));
        gpu::Shader::makeProgram(*program, slotBindings);

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        /*  state->setBlendFunction(true,
        gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
        gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);*/

        PrepareStencil::testMaskDrawShape(*state);
        state->setCullMode(gpu::State::CULL_NONE);
        _drawAllViewsPipeline = gpu::Pipeline::create(program, state);
    }
    return _drawAllViewsPipeline;
}
void GameWorkloadRenderItem::render(RenderArgs* args) {
    gpu::Batch& batch = *(args->_batch);

    // Setup projection
    glm::mat4 projMat;
    Transform viewMat;
    args->getViewFrustum().evalProjectionMatrix(projMat);
    args->getViewFrustum().evalViewTransform(viewMat);
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewMat);
    batch.setModelTransform(Transform());

    batch.setResourceBuffer(0, _allProxiesBuffer);
    batch.setResourceBuffer(1, _allViewsBuffer);

    // Show Proxies
    if (_showProxies) {
        batch.setPipeline(getProxiesPipeline());

        static const int NUM_VERTICES_PER_QUAD = 3;
        batch.draw(gpu::TRIANGLES, NUM_VERTICES_PER_QUAD * _numAllProxies, 0);
    }

    // Show Views
    if (_showViews) {
        batch.setPipeline(getViewsPipeline());

        static const int NUM_VERTICES_PER_QUAD = 3;
        batch.draw(gpu::TRIANGLES, NUM_VERTICES_PER_QUAD * 3 * _numAllViews, 0);
    }

    batch.setResourceBuffer(0, nullptr);
    batch.setResourceBuffer(1, nullptr);

}



