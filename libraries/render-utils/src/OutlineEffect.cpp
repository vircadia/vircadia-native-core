//
//  OutlineEffect.cpp
//  render-utils/src/
//
//  Created by Olivier Prat on 08/08/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OutlineEffect.h"

#include "GeometryCache.h"

#include "gpu/Context.h"
#include "gpu/StandardShaderLib.h"

#include "surfaceGeometry_copyDepth_frag.h"
#include "debug_deferred_buffer_vert.h"
#include "debug_deferred_buffer_frag.h"
#include "Outline_frag.h"

OutlineFramebuffer::OutlineFramebuffer() {
}

void OutlineFramebuffer::update(const gpu::TexturePointer& linearDepthBuffer) {
    //If the depth buffer or size changed, we need to delete our FBOs
    bool reset = false;

    if (_depthTexture) {
        auto newFrameSize = glm::ivec2(linearDepthBuffer->getDimensions());
        if (_frameSize != newFrameSize) {
            _frameSize = newFrameSize;
            reset = true;
        }
    }

    if (reset) {
        clear();
    }
}

void OutlineFramebuffer::clear() {
    _depthFramebuffer.reset();
    _depthTexture.reset();
}

void OutlineFramebuffer::allocate() {

    auto width = _frameSize.x;
    auto height = _frameSize.y;
    auto format = gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::RED);

    _depthTexture = gpu::TexturePointer(gpu::Texture::createRenderBuffer(format, width, height));
    _depthFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("outlineDepth"));
    _depthFramebuffer->setRenderBuffer(0, _depthTexture);
}

gpu::FramebufferPointer OutlineFramebuffer::getDepthFramebuffer() {
    if (!_depthFramebuffer) {
        allocate();
    }
    return _depthFramebuffer;
}

gpu::TexturePointer OutlineFramebuffer::getDepthTexture() {
    if (!_depthTexture) {
        allocate();
    }
    return _depthTexture;
}

void PrepareOutline::run(const render::RenderContextPointer& renderContext, const PrepareOutline::Inputs& input, PrepareOutline::Output& output) {
    auto outlinedItems = input.get1();

    if (!outlinedItems.empty()) {
        auto args = renderContext->args;
        auto deferredFrameBuffer = input.get0();
        auto frameSize = deferredFrameBuffer->getFrameSize();

        if (!_outlineFramebuffer) {
            _outlineFramebuffer = std::make_shared<OutlineFramebuffer>();
        }
        _outlineFramebuffer->update(deferredFrameBuffer->getPrimaryDepthTexture());

        if (!_copyDepthPipeline) {
            auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
            auto ps = gpu::Shader::createPixel(std::string(surfaceGeometry_copyDepth_frag));
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

            gpu::Shader::BindingSet slotBindings;
            slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), 0));
            gpu::Shader::makeProgram(*program, slotBindings);

            gpu::StatePointer state = gpu::StatePointer(new gpu::State());

            state->setColorWriteMask(true, false, false, false);

            // Good to go add the brand new pipeline
            _copyDepthPipeline = gpu::Pipeline::create(program, state);
        }

        // TODO : Instead of copying entire buffer, we should only copy the sub rect containing the outlined object
        // grown to take into account blur width
        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            batch.enableStereo(false);

            batch.setViewportTransform(args->_viewport);
            batch.setProjectionTransform(glm::mat4());
            batch.resetViewTransform();
            batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(frameSize, args->_viewport));

            batch.setFramebuffer(_outlineFramebuffer->getDepthFramebuffer());
            batch.setPipeline(_copyDepthPipeline);
            batch.setResourceTexture(0, deferredFrameBuffer->getPrimaryDepthTexture());
            batch.draw(gpu::TRIANGLE_STRIP, 4);

            // Restore previous frame buffer
            batch.setFramebuffer(deferredFrameBuffer->getDeferredFramebuffer());
        });

        output = _outlineFramebuffer;
    } else {
        output = nullptr;
    }
}

DrawOutline::DrawOutline() {
}

void DrawOutline::configure(const Config& config) {
    _color = config.color;
    _size = config.width / 1024.f;
    _fillOpacityUnoccluded = config.fillOpacityUnoccluded;
    _fillOpacityOccluded = config.fillOpacityOccluded;
    _threshold = config.glow ? 1.f : 0.f;
    _intensity = config.intensity * (config.glow ? 2.f : 1.f);
}

void DrawOutline::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    auto mainFrameBuffer = inputs.get0();

    if (mainFrameBuffer) {
        auto sceneDepthBuffer = mainFrameBuffer->getPrimaryDepthTexture();
        auto outlinedDepthBuffer = inputs.get1();
        auto pipeline = getPipeline();

        if (outlinedDepthBuffer) {
            auto framebufferSize = glm::ivec2(sceneDepthBuffer->getDimensions());
            auto args = renderContext->args;
            {
                auto& configuration = _configuration.edit();
                configuration._color = _color;
                configuration._size.x = _size * framebufferSize.y / framebufferSize.x;
                configuration._size.y = _size;
                configuration._intensity = _intensity;
                configuration._fillOpacityUnoccluded = _fillOpacityUnoccluded;
                configuration._fillOpacityOccluded = _fillOpacityOccluded;
                configuration._threshold = _threshold;
            }

            gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
                batch.enableStereo(false);

                batch.setViewportTransform(args->_viewport);
                batch.setProjectionTransform(glm::mat4());
                batch.resetViewTransform();
                batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, args->_viewport));
                batch.setPipeline(pipeline);

                batch.setUniformBuffer(0, _configuration);
                batch.setResourceTexture(SCENE_DEPTH_SLOT, sceneDepthBuffer);
                batch.setResourceTexture(OUTLINED_DEPTH_SLOT, outlinedDepthBuffer->getDepthTexture());
                batch.draw(gpu::TRIANGLE_STRIP, 4);
            });
        }
    }
}

const gpu::PipelinePointer& DrawOutline::getPipeline() {
    if (!_pipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(Outline_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("outlineParamsBuffer", 0));
        slotBindings.insert(gpu::Shader::Binding("sceneDepthMap", SCENE_DEPTH_SLOT));
        slotBindings.insert(gpu::Shader::Binding("outlinedDepthMap", OUTLINED_DEPTH_SLOT));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(false);
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        _pipeline = gpu::Pipeline::create(program, state);
    }
    return _pipeline;
}

DebugOutline::DebugOutline() {
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

DebugOutline::~DebugOutline() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

void DebugOutline::configure(const Config& config) {
    _isDisplayDepthEnabled = config.viewOutlinedDepth;
}

void DebugOutline::run(const render::RenderContextPointer& renderContext, const Inputs& input) {
    const auto outlineFramebuffer = input;

    if (_isDisplayDepthEnabled && outlineFramebuffer) {
        assert(renderContext->args);
        assert(renderContext->args->hasViewFrustum());
        RenderArgs* args = renderContext->args;

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            batch.enableStereo(false);
            batch.setViewportTransform(args->_viewport);

            const auto geometryBuffer = DependencyManager::get<GeometryCache>();

            glm::mat4 projMat;
            Transform viewMat;
            args->getViewFrustum().evalProjectionMatrix(projMat);
            args->getViewFrustum().evalViewTransform(viewMat);
            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat, true);
            batch.setModelTransform(Transform());

            batch.setPipeline(getDebugPipeline());
            batch.setResourceTexture(0, outlineFramebuffer->getDepthTexture());

            const glm::vec4 color(1.0f, 0.5f, 0.2f, 1.0f);
            const glm::vec2 bottomLeft(-1.0f, -1.0f);
            const glm::vec2 topRight(1.0f, 1.0f);
            geometryBuffer->renderQuad(batch, bottomLeft, topRight, color, _geometryId);

            batch.setResourceTexture(0, nullptr);
        });
    }
}

const gpu::PipelinePointer& DebugOutline::getDebugPipeline() {
    if (!_debugPipeline) {
        static const std::string VERTEX_SHADER{ debug_deferred_buffer_vert };
        static const std::string FRAGMENT_SHADER{ debug_deferred_buffer_frag };
        static const std::string SOURCE_PLACEHOLDER{ "//SOURCE_PLACEHOLDER" };
        static const auto SOURCE_PLACEHOLDER_INDEX = FRAGMENT_SHADER.find(SOURCE_PLACEHOLDER);
        Q_ASSERT_X(SOURCE_PLACEHOLDER_INDEX != std::string::npos, Q_FUNC_INFO,
            "Could not find source placeholder");
        static const std::string DEFAULT_DEPTH_SHADER{
            "vec4 getFragmentColor() {"
            "   float Zdb = texelFetch(depthMap, ivec2(gl_FragCoord.xy), 0).x;"
            "   Zdb = 1.0-(1.0-Zdb)*100;"
            "   return vec4(Zdb, Zdb, Zdb, 1.0);"
            " }"
        };

        auto bakedFragmentShader = FRAGMENT_SHADER;
        bakedFragmentShader.replace(SOURCE_PLACEHOLDER_INDEX, SOURCE_PLACEHOLDER.size(), DEFAULT_DEPTH_SHADER);

        static const auto vs = gpu::Shader::createVertex(VERTEX_SHADER);
        const auto ps = gpu::Shader::createPixel(bakedFragmentShader);
        const auto program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("depthMap", 0));
        gpu::Shader::makeProgram(*program, slotBindings);

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(gpu::State::DepthTest(false));
        _debugPipeline = gpu::Pipeline::create(program, state);
    }

    return _debugPipeline;
}
