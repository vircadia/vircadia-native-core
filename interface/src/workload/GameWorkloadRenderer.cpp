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
#include <shaders/Shaders.h>


void GameSpaceToRender::configure(const Config& config) {
    _freezeViews = config.freezeViews;
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

    workload::Proxy::Vector proxies(space->getNumAllocatedProxies());
    space->copyProxyValues(proxies.data(), (uint32_t)proxies.size());

    workload::Views views(space->getNumViews());
    space->copyViews(views);

    // Valid space, let's display its content
    if (!render::Item::isValidID(_spaceRenderItemID)) {
        _spaceRenderItemID = scene->allocateID();
        auto renderItem = std::make_shared<GameWorkloadRenderItem>();
        renderItem->editBound().setBox(glm::vec3(-16000.0f), 32000.0f);
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
}


GameWorkloadRenderItem::GameWorkloadRenderItem() : _key(render::ItemKey::Builder::opaqueShape().withShadowCaster().withTagBits(render::ItemKey::TAG_BITS_0 | render::ItemKey::TAG_BITS_1)) {
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


void GameWorkloadRenderItem::setAllProxies(const workload::Proxy::Vector& proxies) {
    _myOwnProxies = proxies;
    static const uint32_t sizeOfProxy = sizeof(workload::Proxy);
    if (!_allProxiesBuffer) {
        _allProxiesBuffer = std::make_shared<gpu::Buffer>(sizeOfProxy);
    }

    _allProxiesBuffer->setData(proxies.size() * sizeOfProxy, (const gpu::Byte*) proxies.data());
    _numAllProxies = (uint32_t) proxies.size();
}

void GameWorkloadRenderItem::setAllViews(const workload::Views& views) {
    _myOwnViews = views;
    static const uint32_t sizeOfView = sizeof(workload::View);
    if (!_allViewsBuffer) {
        _allViewsBuffer = std::make_shared<gpu::Buffer>(sizeOfView);
    }

    _allViewsBuffer->setData(views.size() * sizeOfView, (const gpu::Byte*) views.data());
    _numAllViews = (uint32_t)views.size();
}

const gpu::PipelinePointer GameWorkloadRenderItem::getProxiesPipeline() {
    if (!_drawAllProxiesPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::drawWorkloadProxy);
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
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::drawWorkloadView);
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

const gpu::BufferPointer GameWorkloadRenderItem::getDrawViewBuffer() {
    if (!_drawViewBuffer) {
        int numSegments = 64;
        float angleStep = (float)M_PI * 2.0f / (float)numSegments;

        struct Vert {
            glm::vec4 p;
        };
        std::vector<Vert> verts(numSegments + 1);
        for (int i = 0; i < numSegments; i++) {
            float angle = (float)i * angleStep;
            verts[i].p.x = cos(angle);
            verts[i].p.y = sin(angle);
            verts[i].p.z = angle;
            verts[i].p.w = 1.0f;
        }
        verts[numSegments] = verts[0];
        verts[numSegments].p.w = 0.0f;

        _drawViewBuffer = std::make_shared<gpu::Buffer>(verts.size() * sizeof(Vert), (const gpu::Byte*) verts.data());
        _numDrawViewVerts = numSegments + 1;
    }
    return _drawViewBuffer;
}

void GameWorkloadRenderItem::render(RenderArgs* args) {
    gpu::Batch& batch = *(args->_batch);

    batch.setModelTransform(Transform());

    batch.setResourceBuffer(0, _allProxiesBuffer);
    batch.setResourceBuffer(1, _allViewsBuffer);

    // Show Proxies
    if (_showProxies) {
        batch.setPipeline(getProxiesPipeline());

        static const int NUM_VERTICES_PER_PROXY = 3;
        batch.draw(gpu::TRIANGLES, NUM_VERTICES_PER_PROXY * _numAllProxies, 0);
    }

    // Show Views
    if (_showViews) {
        batch.setPipeline(getViewsPipeline());

        batch.setUniformBuffer(0, getDrawViewBuffer());
        static const int NUM_VERTICES_PER_DRAWVIEWVERT = 2;
        static const int NUM_REGIONS = 3;
        batch.draw(gpu::TRIANGLE_STRIP, NUM_REGIONS * NUM_VERTICES_PER_DRAWVIEWVERT * _numDrawViewVerts * _numAllViews, 0);

    }

    batch.setResourceBuffer(0, nullptr);
    batch.setResourceBuffer(1, nullptr);

}



