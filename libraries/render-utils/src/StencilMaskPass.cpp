//
//  StencilMaskPass.cpp
//  render-utils/src/
//
//  Created by Sam Gateau on 5/31/17.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "StencilMaskPass.h"

#include <ViewFrustum.h>
#include <gpu/Context.h>


#include <gpu/StandardShaderLib.h>

#include "stencil_drawMask_frag.h"

using namespace render;

void PrepareStencil::configure(const Config& config) {
    _maskMode = config.maskMode;
    _forceDraw = config.forceDraw;
}

model::MeshPointer PrepareStencil::getMesh() {
    if (!_mesh) {

        std::vector<glm::vec3> vertices {
            { -1.0f, -1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },
            { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
            { 1.0f, -1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } };

        std::vector<uint32_t> indices { 0, 7, 1, 1, 3, 2, 3, 5, 4, 5, 7, 6 };
        _mesh = model::Mesh::createIndexedTriangles_P3F((uint32_t) vertices.size(), (uint32_t) indices.size(), vertices.data(), indices.data());
    }
    return _mesh;
}

gpu::PipelinePointer PrepareStencil::getMeshStencilPipeline() {
    if (!_meshStencilPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawVertexPositionVS();
        auto ps = gpu::StandardShaderLib::getDrawNadaPS();
        auto program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram((*program));

        auto state = std::make_shared<gpu::State>();
        drawMask(*state);
        state->setColorWriteMask(0);

        _meshStencilPipeline = gpu::Pipeline::create(program, state);
    }
    return _meshStencilPipeline;
}

gpu::PipelinePointer PrepareStencil::getPaintStencilPipeline() {
    if (!_paintStencilPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(stencil_drawMask_frag));
        auto program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram((*program));

        auto state = std::make_shared<gpu::State>();
        drawMask(*state);
        state->setColorWriteMask(0);

        _paintStencilPipeline = gpu::Pipeline::create(program, state);
    }
    return _paintStencilPipeline;
}

void PrepareStencil::run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& srcFramebuffer) {
    RenderArgs* args = renderContext->args;

    // Only draw the stencil mask if in HMD mode or not forced.
    if (!_forceDraw && (args->_displayMode != RenderArgs::STEREO_HMD)) {
        return;
    }

    doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setViewportTransform(args->_viewport);

        if (_maskMode < 0) {
            batch.setPipeline(getMeshStencilPipeline());

            auto mesh = getMesh();
            batch.setIndexBuffer(mesh->getIndexBuffer());
            batch.setInputFormat((mesh->getVertexFormat()));
            batch.setInputStream(0, mesh->getVertexStream());

            // Draw
            auto part = mesh->getPartBuffer().get<model::Mesh::Part>(0);
            batch.drawIndexed(gpu::TRIANGLES, part._numIndices, part._startIndex);
        } else {
            batch.setPipeline(getPaintStencilPipeline());
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }
    });
}

void PrepareStencil::drawMask(gpu::State& state) {
    state.setStencilTest(true, 0xFF, gpu::State::StencilTest(PrepareStencil::STENCIL_MASK, 0xFF, gpu::ALWAYS, gpu::State::STENCIL_OP_REPLACE, gpu::State::STENCIL_OP_REPLACE, gpu::State::STENCIL_OP_REPLACE));
}

void PrepareStencil::testMask(gpu::State& state) {
    state.setStencilTest(true, 0x00, gpu::State::StencilTest(PrepareStencil::STENCIL_MASK, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
}

void PrepareStencil::testMaskNoAA(gpu::State& state) {
    state.setStencilTest(true, 0x00, gpu::State::StencilTest(PrepareStencil::STENCIL_MASK | PrepareStencil::STENCIL_NO_AA, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
}

void PrepareStencil::testBackground(gpu::State& state) {
    state.setStencilTest(true, 0x00, gpu::State::StencilTest(PrepareStencil::STENCIL_BACKGROUND, 0xFF, gpu::EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
}

void PrepareStencil::testMaskDrawShape(gpu::State& state) {
    state.setStencilTest(true, 0xFF, gpu::State::StencilTest(PrepareStencil::STENCIL_MASK, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_ZERO));
}

void PrepareStencil::testMaskDrawShapeNoAA(gpu::State& state) {
    state.setStencilTest(true, 0xFF, gpu::State::StencilTest(PrepareStencil::STENCIL_MASK | PrepareStencil::STENCIL_NO_AA, 0xFF, gpu::ALWAYS, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_REPLACE));
}

void PrepareStencil::testShape(gpu::State& state) {
    state.setStencilTest(true, 0x00, gpu::State::StencilTest(PrepareStencil::STENCIL_SHAPE, 0xFF, gpu::EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
}