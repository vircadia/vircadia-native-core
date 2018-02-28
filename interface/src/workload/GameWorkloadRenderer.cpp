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
#include "render-utils/drawWorkloadProxy_frag.h"

void GameSpaceToRender::run(const workload::WorkloadContextPointer& runContext, Outputs& outputs) {
    auto gameWorkloadContext = std::dynamic_pointer_cast<GameWorkloadContext>(runContext);
    if (!gameWorkloadContext) {
        return;
    }
    auto space = gameWorkloadContext->_space;
    if (!space) {
        return;
    }

    std::vector<workload::Space::Proxy> proxies(space->getNumAllocatedProxies());
    space->copyProxyValues(proxies.data(), (uint32_t)proxies.size());


    auto scene = gameWorkloadContext->_scene;

    // Valid space, let's display its content
    render::Transaction transaction;
    if (!render::Item::isValidID(_spaceRenderItemID)) {
        _spaceRenderItemID = scene->allocateID();
        auto renderItem = std::make_shared<GameWorkloadRenderItem>();
        renderItem->editBound().setBox(glm::vec3(-100.0f), 200.0f);
        renderItem->setAllProxies(proxies);
        transaction.resetItem(_spaceRenderItemID, std::make_shared<GameWorkloadRenderItem::Payload>(renderItem));
    } else {
        transaction.updateItem<GameWorkloadRenderItem>(_spaceRenderItemID, [proxies](GameWorkloadRenderItem& item) {
            item.setAllProxies(proxies);
        });
    }
    scene->enqueueTransaction(transaction);
}

namespace render {
    template <> const ItemKey payloadGetKey(const GameWorkloadRenderItem::Pointer& payload) {
        return ItemKey::Builder::opaqueShape().withTagBits(render::ItemKey::TAG_BITS_0 | render::ItemKey::TAG_BITS_1);
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



void GameWorkloadRenderItem::setAllProxies(const std::vector<workload::Space::Proxy>& proxies) {
    _myOwnProxies = proxies;
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
        slotBindings.insert(gpu::Shader::Binding("ssbo0Buffer", 11));
        gpu::Shader::makeProgram(*program, slotBindings);

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
      /*  state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);*/

        PrepareStencil::testMaskDrawShape(*state);
        state->setCullMode(gpu::State::CULL_NONE);
        _drawAllProxiesPipeline = gpu::Pipeline::create(program, state);
    }
    return _drawAllProxiesPipeline;
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

    // Bind program
    batch.setPipeline(getPipeline());

   // batch.setResourceBuffer(11, _allProxiesBuffer);
    batch.setUniformBuffer(11, _allProxiesBuffer);

    static const int NUM_VERTICES_PER_QUAD = 3;
    batch.draw(gpu::TRIANGLES, NUM_VERTICES_PER_QUAD * _numAllProxies, 0);

 //   batch.setResourceBuffer(11, nullptr);
    batch.setUniformBuffer(11, nullptr);

/*
    auto geometryCache = DependencyManager::get<GeometryCache>();
    GeometryCache::Shape geometryShape;
    glm::vec4 outColor;
    geometryShape = GeometryCache::Sphere;
//   geometryCache->renderShape(batch, geometryShape, outColor);

    auto pipeline = geometryCache->getOpaqueShapePipeline();
  
    for (int i = 0; i < _numAllProxies; i++) {
        auto sphere = _myOwnProxies[i].sphere;
        batch.setModelTransform(Transform(glm::quat(), glm::vec3(1.0), glm::vec3(sphere.x, sphere.y, sphere.z)));
        geometryCache->renderSolidShapeInstance(args, batch, geometryShape, outColor, pipeline);
    }*/

}



