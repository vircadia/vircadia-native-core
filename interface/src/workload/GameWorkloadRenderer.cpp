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

#include <gpu/Context.h>


#include "render-utils/drawWorkloadProxy_vert.h"
#include "render-utils/drawWorkloadProxy_frag.h"

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

    space->getNumObjects();

}

namespace render {
    template <> const ItemKey payloadGetKey(const GameWorkloadRenderItem::Pointer& payload) {
        auto builder = ItemKey::Builder().opaqueShape().withTagBits(ItemKey::TAG_BITS_0 | ItemKey::TAG_BITS_1);
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



void GameWorkloadRenderItem::setAllProxies(const std::vector<workload::Space::Proxy>& proxies) {
    static const uint32_t sizeOfProxy = sizeof(workload::Space::Proxy);
    if (!_allProxiesBuffer) {
        _allProxiesBuffer = std::make_shared<gpu::Buffer>(sizeOfProxy);
    }

    _allProxiesBuffer->setData(proxies.size() * sizeOfProxy, (const gpu::Byte*) proxies.data());
    _numAllProxies = (uint32_t) proxies.size();
}

const gpu::PipelinePointer GameWorkloadRenderItem::getPipeline() {
    if (!_drawAllProxiesPipeline) {
        auto vs = drawWorkloadProxy_vert::getShader();
        auto ps = drawWorkloadProxy_frag::getShader();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        _drawAllProxiesPipeline = gpu::Pipeline::create(program, state);
    }
    return _drawAllProxiesPipeline;
}

void GameWorkloadRenderItem::render(RenderArgs* args) {
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        // Setup projection
        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // Bind program
        batch.setPipeline(getPipeline());

        batch.setResourceBuffer(0, _allProxiesBuffer);

        static const int NUM_VERTICES_PER_QUAD = 4;
        batch.draw(gpu::LINES, NUM_VERTICES_PER_QUAD * _numAllProxies, 0);
    });

}



