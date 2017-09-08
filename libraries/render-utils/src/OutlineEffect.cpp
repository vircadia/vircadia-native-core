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

#include <render/FilterTask.h>
#include <render/SortTask.h>

#include "gpu/Context.h"
#include "gpu/StandardShaderLib.h"


#include "surfaceGeometry_copyDepth_frag.h"
#include "debug_deferred_buffer_vert.h"
#include "debug_deferred_buffer_frag.h"
#include "Outline_frag.h"
#include "Outline_filled_frag.h"

using namespace render;

extern void initZPassPipelines(ShapePlumber& plumber, gpu::StatePointer state);

OutlineFramebuffer::OutlineFramebuffer() {
}

void OutlineFramebuffer::update(const gpu::TexturePointer& colorBuffer) {
    // If the depth buffer or size changed, we need to delete our FBOs and recreate them at the
    // new correct dimensions.
    if (_depthTexture) {
        auto newFrameSize = glm::ivec2(colorBuffer->getDimensions());
        if (_frameSize != newFrameSize) {
            _frameSize = newFrameSize;
            clear();
        }
    }
}

void OutlineFramebuffer::clear() {
    _depthFramebuffer.reset();
    _depthTexture.reset();
}

void OutlineFramebuffer::allocate() {
    
    auto width = _frameSize.x;
    auto height = _frameSize.y;
    auto format = gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::DEPTH);

    _depthTexture = gpu::TexturePointer(gpu::Texture::createRenderBuffer(format, width, height));
    _depthFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("outlineDepth"));
    _depthFramebuffer->setDepthStencilBuffer(_depthTexture, format);
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

void DrawOutlineDepth::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& output) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    auto& inShapes = inputs.get0();
    auto& deferredFrameBuffer = inputs.get1();

    if (!inShapes.empty()) {
        RenderArgs* args = renderContext->args;
        ShapeKey::Builder defaultKeyBuilder;

        if (!_outlineFramebuffer) {
            _outlineFramebuffer = std::make_shared<OutlineFramebuffer>();
        }
        _outlineFramebuffer->update(deferredFrameBuffer->getDeferredColorTexture());

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            args->_batch = &batch;

            batch.setFramebuffer(_outlineFramebuffer->getDepthFramebuffer());
            // Clear it
            batch.clearFramebuffer(
                gpu::Framebuffer::BUFFER_DEPTH,
                vec4(vec3(1.0, 1.0, 1.0), 0.0), 1.0, 0, false);

            // Setup camera, projection and viewport for all items
            batch.setViewportTransform(args->_viewport);
            batch.setStateScissorRect(args->_viewport);

            glm::mat4 projMat;
            Transform viewMat;
            args->getViewFrustum().evalProjectionMatrix(projMat);
            args->getViewFrustum().evalViewTransform(viewMat);

            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat);

            auto shadowPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder);
            auto shadowSkinnedPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder.withSkinned());

            std::vector<ShapeKey> skinnedShapeKeys{};

            // Iterate through all inShapes and render the unskinned
            args->_shapePipeline = shadowPipeline;
            batch.setPipeline(shadowPipeline->pipeline);
            for (auto items : inShapes) {
                if (items.first.isSkinned()) {
                    skinnedShapeKeys.push_back(items.first);
                }
                else {
                    renderItems(renderContext, items.second);
                }
            }

            // Reiterate to render the skinned
            args->_shapePipeline = shadowSkinnedPipeline;
            batch.setPipeline(shadowSkinnedPipeline->pipeline);
            for (const auto& key : skinnedShapeKeys) {
                renderItems(renderContext, inShapes.at(key));
            }

            args->_shapePipeline = nullptr;
            args->_batch = nullptr;
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
    _blurKernelSize = std::min(10, std::max(2, (int)floorf(config.width*2 + 0.5f)));
    // Size is in normalized screen height. We decide that for outline width = 1, this is equal to 1/400.
    _size = config.width / 400.f;
    _fillOpacityUnoccluded = config.fillOpacityUnoccluded;
    _fillOpacityOccluded = config.fillOpacityOccluded;
    _threshold = config.glow ? 1.f : 1e-3f;
    _intensity = config.intensity * (config.glow ? 2.f : 1.f);
}

void DrawOutline::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    auto outlineFrameBuffer = inputs.get1();

    if (outlineFrameBuffer) {
        auto sceneDepthBuffer = inputs.get2();
        const auto frameTransform = inputs.get0();
        auto outlinedDepthTexture = outlineFrameBuffer->getDepthTexture();
        auto destinationFrameBuffer = inputs.get3();
        auto framebufferSize = glm::ivec2(outlinedDepthTexture->getDimensions());

        if (!_primaryWithoutDepthBuffer || framebufferSize!=_frameBufferSize) {
            // Failing to recreate this frame buffer when the screen has been resized creates a bug on Mac
            _primaryWithoutDepthBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("primaryWithoutDepth"));
            _primaryWithoutDepthBuffer->setRenderBuffer(0, destinationFrameBuffer->getRenderBuffer(0));
            _frameBufferSize = framebufferSize;
        }

        if (sceneDepthBuffer) {
            const auto OPACITY_EPSILON = 5e-3f;
            auto pipeline = getPipeline(_fillOpacityUnoccluded>OPACITY_EPSILON || _fillOpacityOccluded>OPACITY_EPSILON);
            auto args = renderContext->args;
            {
                auto& configuration = _configuration.edit();
                configuration._color = _color;
                configuration._intensity = _intensity;
                configuration._fillOpacityUnoccluded = _fillOpacityUnoccluded;
                configuration._fillOpacityOccluded = _fillOpacityOccluded;
                configuration._threshold = _threshold;
                configuration._blurKernelSize = _blurKernelSize;
                configuration._size.x = _size * _frameBufferSize.y / _frameBufferSize.x;
                configuration._size.y = _size;
            }

            gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
                batch.enableStereo(false);
                batch.setFramebuffer(_primaryWithoutDepthBuffer);

                batch.setViewportTransform(args->_viewport);
                batch.setProjectionTransform(glm::mat4());
                batch.resetViewTransform();
                batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(_frameBufferSize, args->_viewport));
                batch.setPipeline(pipeline);

                batch.setUniformBuffer(OUTLINE_PARAMS_SLOT, _configuration);
                batch.setUniformBuffer(FRAME_TRANSFORM_SLOT, frameTransform->getFrameTransformBuffer());
                batch.setResourceTexture(SCENE_DEPTH_SLOT, sceneDepthBuffer->getPrimaryDepthTexture());
                batch.setResourceTexture(OUTLINED_DEPTH_SLOT, outlinedDepthTexture);
                batch.draw(gpu::TRIANGLE_STRIP, 4);

                // Restore previous frame buffer
                batch.setFramebuffer(destinationFrameBuffer);
            });
        }
    }
}

const gpu::PipelinePointer& DrawOutline::getPipeline(bool isFilled) {
    if (!_pipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(Outline_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("outlineParamsBuffer", OUTLINE_PARAMS_SLOT));
        slotBindings.insert(gpu::Shader::Binding("deferredFrameTransformBuffer", FRAME_TRANSFORM_SLOT));
        slotBindings.insert(gpu::Shader::Binding("sceneDepthMap", SCENE_DEPTH_SLOT));
        slotBindings.insert(gpu::Shader::Binding("outlinedDepthMap", OUTLINED_DEPTH_SLOT));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(false, false));
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        _pipeline = gpu::Pipeline::create(program, state);

        ps = gpu::Shader::createPixel(std::string(Outline_filled_frag));
        program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram(*program, slotBindings);
        _pipelineFilled = gpu::Pipeline::create(program, state);
    }
    return isFilled ? _pipelineFilled : _pipeline;
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

DrawOutlineTask::DrawOutlineTask() {

}

void DrawOutlineTask::configure(const Config& config) {

}

void DrawOutlineTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs) {
    const auto input = inputs.get<Inputs>();
    const auto selectedMetas = inputs.getN<Inputs>(0);
    const auto shapePlumber = input.get1();
    const auto sceneFrameBuffer = inputs.getN<Inputs>(2);
    const auto primaryFramebuffer = inputs.getN<Inputs>(3);
    const auto deferredFrameTransform = inputs.getN<Inputs>(4);

    // Prepare the ShapePipeline
    ShapePlumberPointer shapePlumberZPass = std::make_shared<ShapePlumber>();
    {
        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        state->setColorWriteMask(false, false, false, false);

        initZPassPipelines(*shapePlumberZPass, state);
    }

    const auto outlinedItemIDs = task.addJob<render::MetaToSubItems>("OutlineMetaToSubItemIDs", selectedMetas);
    const auto outlinedItems = task.addJob<render::IDsToBounds>("OutlineMetaToSubItems", outlinedItemIDs, true);

    // Sort
    const auto sortedPipelines = task.addJob<render::PipelineSortShapes>("OutlinePipelineSort", outlinedItems);
    const auto sortedShapes = task.addJob<render::DepthSortShapes>("OutlineDepthSort", sortedPipelines);

    // Draw depth of outlined objects in separate buffer
    const auto drawOutlineDepthInputs = DrawOutlineDepth::Inputs(sortedShapes, sceneFrameBuffer).asVarying();
    const auto outlinedFrameBuffer = task.addJob<DrawOutlineDepth>("OutlineDepth", drawOutlineDepthInputs, shapePlumberZPass);

    // Draw outline
    const auto drawOutlineInputs = DrawOutline::Inputs(deferredFrameTransform, outlinedFrameBuffer, sceneFrameBuffer, primaryFramebuffer).asVarying();
    task.addJob<DrawOutline>("OutlineEffect", drawOutlineInputs);

    // Debug outline
    task.addJob<DebugOutline>("OutlineDebug", outlinedFrameBuffer);
}
