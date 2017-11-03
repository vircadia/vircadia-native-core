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

#include "CubeProjectedPolygon.h"

#include <render/FilterTask.h>
#include <render/SortTask.h>

#include "gpu/Context.h"
#include "gpu/StandardShaderLib.h"

#include <sstream>

#include "surfaceGeometry_copyDepth_frag.h"
#include "debug_deferred_buffer_vert.h"
#include "debug_deferred_buffer_frag.h"
#include "Outline_frag.h"
#include "Outline_filled_frag.h"
#include "Outline_aabox_vert.h"

using namespace render;

#define OUTLINE_STENCIL_MASK    1

OutlineRessources::OutlineRessources() {
}

void OutlineRessources::update(const gpu::FramebufferPointer& primaryFrameBuffer) {
    auto newFrameSize = glm::ivec2(primaryFrameBuffer->getSize());

    // If the buffer size changed, we need to delete our FBOs and recreate them at the
    // new correct dimensions.
    if (_frameSize != newFrameSize) {
        _frameSize = newFrameSize;
        allocateDepthBuffer(primaryFrameBuffer);
        allocateColorBuffer(primaryFrameBuffer);
    } else {
        if (!_depthFrameBuffer) {
            allocateDepthBuffer(primaryFrameBuffer);
        }
        if (!_colorFrameBuffer) {
            allocateColorBuffer(primaryFrameBuffer);
        }
    }
}

void OutlineRessources::allocateColorBuffer(const gpu::FramebufferPointer& primaryFrameBuffer) {
    _colorFrameBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("primaryWithStencil"));
    _colorFrameBuffer->setRenderBuffer(0, primaryFrameBuffer->getRenderBuffer(0));
    _colorFrameBuffer->setDepthStencilBuffer(_depthStencilTexture, _depthStencilTexture->getTexelFormat());
}

void OutlineRessources::allocateDepthBuffer(const gpu::FramebufferPointer& primaryFrameBuffer) {
    auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL);
    _depthStencilTexture = gpu::TexturePointer(gpu::Texture::createRenderBuffer(depthFormat, _frameSize.x, _frameSize.y));
    _depthFrameBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("outlineDepth"));
    _depthFrameBuffer->setDepthStencilBuffer(_depthStencilTexture, depthFormat);
}

gpu::FramebufferPointer OutlineRessources::getDepthFramebuffer() {
    assert(_depthFrameBuffer);
    return _depthFrameBuffer;
}

gpu::TexturePointer OutlineRessources::getDepthTexture() {
    return _depthStencilTexture;
}

gpu::FramebufferPointer OutlineRessources::getColorFramebuffer() {
    assert(_colorFrameBuffer);
    return _colorFrameBuffer;
}

OutlineSharedParameters::OutlineSharedParameters() {
    std::fill(_blurPixelWidths.begin(), _blurPixelWidths.end(), 0);
}

PrepareDrawOutline::PrepareDrawOutline() {
    _ressources = std::make_shared<OutlineRessources>();
}

void PrepareDrawOutline::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    auto destinationFrameBuffer = inputs;

    _ressources->update(destinationFrameBuffer);
    outputs = _ressources;
}

gpu::PipelinePointer DrawOutlineMask::_stencilMaskPipeline;
gpu::PipelinePointer DrawOutlineMask::_stencilMaskFillPipeline;
gpu::BufferPointer DrawOutlineMask::_boundsBuffer;

DrawOutlineMask::DrawOutlineMask(unsigned int outlineIndex, 
                                 render::ShapePlumberPointer shapePlumber, OutlineSharedParametersPointer parameters) :
    _outlineIndex{ outlineIndex },
    _shapePlumber { shapePlumber },
    _sharedParameters{ parameters } {
}

void DrawOutlineMask::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    auto& inShapes = inputs.get0();

    if (!_stencilMaskPipeline || !_stencilMaskFillPipeline) {
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(OUTLINE_STENCIL_MASK, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_ZERO, gpu::State::STENCIL_OP_REPLACE));
        state->setColorWriteMask(false, false, false, false);
        state->setCullMode(gpu::State::CULL_FRONT);

        gpu::StatePointer fillState = gpu::StatePointer(new gpu::State());
        fillState->setDepthTest(false, false, gpu::LESS_EQUAL);
        fillState->setStencilTest(true, 0xFF, gpu::State::StencilTest(OUTLINE_STENCIL_MASK, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_REPLACE));
        fillState->setColorWriteMask(false, false, false, false);
        fillState->setCullMode(gpu::State::CULL_FRONT);

        auto vs = gpu::Shader::createVertex(std::string(Outline_aabox_vert));
        auto ps = gpu::StandardShaderLib::getDrawWhitePS();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        _stencilMaskPipeline = gpu::Pipeline::create(program, state);
        _stencilMaskFillPipeline = gpu::Pipeline::create(program, fillState);
    }

    if (!_boundsBuffer) {
        _boundsBuffer = std::make_shared<gpu::Buffer>(sizeof(render::ItemBound));
    }

    if (!inShapes.empty()) {
        auto ressources = inputs.get1();

        RenderArgs* args = renderContext->args;
        ShapeKey::Builder defaultKeyBuilder;

        // Render full screen
        outputs = args->_viewport;

        // Clear the framebuffer without stereo
        // Needs to be distinct from the other batch because using the clear call 
        // while stereo is enabled triggers a warning
        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            batch.enableStereo(false);
            batch.setFramebuffer(ressources->getDepthFramebuffer());
            batch.clearDepthStencilFramebuffer(1.0f, 0);
        });

        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);

        render::ItemBounds itemBounds;

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            args->_batch = &batch;

            auto maskPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder);
            auto maskSkinnedPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder.withSkinned());

            // Setup camera, projection and viewport for all items
            batch.setViewportTransform(args->_viewport);
            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat);

            std::vector<ShapeKey> skinnedShapeKeys{};

            // Iterate through all inShapes and render the unskinned
            args->_shapePipeline = maskPipeline;
            batch.setPipeline(maskPipeline->pipeline);
            for (const auto& items : inShapes) {
                itemBounds.insert(itemBounds.end(), items.second.begin(), items.second.end());
                if (items.first.isSkinned()) {
                    skinnedShapeKeys.push_back(items.first);
                } else {
                    renderItems(renderContext, items.second);
                }
            }

            // Reiterate to render the skinned
            args->_shapePipeline = maskSkinnedPipeline;
            batch.setPipeline(maskSkinnedPipeline->pipeline);
            for (const auto& key : skinnedShapeKeys) {
                renderItems(renderContext, inShapes.at(key));
            }

            args->_shapePipeline = nullptr;
            args->_batch = nullptr;
        });

        _boundsBuffer->setData(itemBounds.size() * sizeof(render::ItemBound), (const gpu::Byte*) itemBounds.data());

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            // Setup camera, projection and viewport for all items
            batch.setViewportTransform(args->_viewport);
            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat);

            // Draw stencil mask with object bounding boxes
            const auto outlineWidthLoc = _stencilMaskPipeline->getProgram()->getUniforms().findLocation("outlineWidth");
            const auto sqrt3 = 1.74f;
            const float blurPixelWidth = 2.0f * sqrt3 *_sharedParameters->_blurPixelWidths[_outlineIndex];
            const auto framebufferSize = ressources->getSourceFrameSize();

            auto stencilPipeline = _sharedParameters->_isFilled.test(_outlineIndex) ? _stencilMaskFillPipeline : _stencilMaskPipeline;
            batch.setPipeline(stencilPipeline);
            batch.setResourceBuffer(0, _boundsBuffer);
            batch._glUniform2f(outlineWidthLoc, blurPixelWidth / framebufferSize.x, blurPixelWidth / framebufferSize.y);
            static const int NUM_VERTICES_PER_CUBE = 36;
            batch.draw(gpu::TRIANGLES, NUM_VERTICES_PER_CUBE * (gpu::uint32) itemBounds.size(), 0);
        });
    } else {
        // Outline rect should be null as there are no outlined shapes
        outputs = glm::ivec4(0, 0, 0, 0);
    }
}

gpu::PipelinePointer DrawOutline::_pipeline;
gpu::PipelinePointer DrawOutline::_pipelineFilled;

DrawOutline::DrawOutline(unsigned int outlineIndex, OutlineSharedParametersPointer parameters) :
    _outlineIndex{ outlineIndex },
    _sharedParameters{ parameters } {
}

void DrawOutline::configure(const Config& config) {
    const auto OPACITY_EPSILON = 5e-3f;

    _parameters._color = config.color;
    _parameters._intensity = config.intensity * (config.glow ? 2.0f : 1.0f);
    _parameters._unoccludedFillOpacity = config.unoccludedFillOpacity;
    _parameters._occludedFillOpacity = config.occludedFillOpacity;
    _parameters._threshold = config.glow ? 1.0f : 1e-3f;
    _parameters._blurKernelSize = std::min(7, std::max(2, (int)floorf(config.width * 3 + 0.5f)));
    // Size is in normalized screen height. We decide that for outline width = 1, this is equal to 1/400.
    _size = config.width / 400.0f;
    _parameters._size.x = (_size * _framebufferSize.y) / _framebufferSize.x;
    _parameters._size.y = _size;
    _sharedParameters->_blurPixelWidths[_outlineIndex] = (int)ceilf(_size * _framebufferSize.y);
    _sharedParameters->_isFilled.set(_outlineIndex, (config.unoccludedFillOpacity > OPACITY_EPSILON || config.occludedFillOpacity > OPACITY_EPSILON));
    _configuration.edit() = _parameters;
}

void DrawOutline::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    auto outlineFrameBuffer = inputs.get1();
    auto outlineRect = inputs.get3();

    if (outlineFrameBuffer && outlineRect.z>0 && outlineRect.w>0) {
        auto sceneDepthBuffer = inputs.get2();
        const auto frameTransform = inputs.get0();
        auto outlinedDepthTexture = outlineFrameBuffer->getDepthTexture();
        auto destinationFrameBuffer = outlineFrameBuffer->getColorFramebuffer();
        auto framebufferSize = glm::ivec2(outlinedDepthTexture->getDimensions());

        if (sceneDepthBuffer) {
            auto pipeline = getPipeline();
            auto args = renderContext->args;

            if (_framebufferSize != framebufferSize)
            {
                _parameters._size.x = (_size * framebufferSize.y) / framebufferSize.x;
                _parameters._size.y = _size;
                _framebufferSize = framebufferSize;
                _sharedParameters->_blurPixelWidths[_outlineIndex] = (int)ceilf(_size * _framebufferSize.y);
                _configuration.edit() = _parameters;
            }

            gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
                batch.enableStereo(false);
                batch.setFramebuffer(destinationFrameBuffer);

                batch.setViewportTransform(args->_viewport);
                batch.setProjectionTransform(glm::mat4());
                batch.resetViewTransform();
                batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, args->_viewport));
                batch.setPipeline(pipeline);

                batch.setUniformBuffer(OUTLINE_PARAMS_SLOT, _configuration);
                batch.setUniformBuffer(FRAME_TRANSFORM_SLOT, frameTransform->getFrameTransformBuffer());
                batch.setResourceTexture(SCENE_DEPTH_SLOT, sceneDepthBuffer->getPrimaryDepthTexture());
                batch.setResourceTexture(OUTLINED_DEPTH_SLOT, outlinedDepthTexture);
                batch.draw(gpu::TRIANGLE_STRIP, 4);
            });
        }
    }
}

const gpu::PipelinePointer& DrawOutline::getPipeline() {
    if (!_pipeline) {
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(false, false));
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        state->setStencilTest(true, 0, gpu::State::StencilTest(OUTLINE_STENCIL_MASK, 0xFF, gpu::EQUAL));

        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(Outline_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("outlineParamsBuffer", OUTLINE_PARAMS_SLOT));
        slotBindings.insert(gpu::Shader::Binding("deferredFrameTransformBuffer", FRAME_TRANSFORM_SLOT));
        slotBindings.insert(gpu::Shader::Binding("sceneDepthMap", SCENE_DEPTH_SLOT));
        slotBindings.insert(gpu::Shader::Binding("outlinedDepthMap", OUTLINED_DEPTH_SLOT));
        gpu::Shader::makeProgram(*program, slotBindings);

        _pipeline = gpu::Pipeline::create(program, state);

        ps = gpu::Shader::createPixel(std::string(Outline_filled_frag));
        program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram(*program, slotBindings);
        _pipelineFilled = gpu::Pipeline::create(program, state);
    }
    return _sharedParameters->_isFilled.test(_outlineIndex) ? _pipelineFilled : _pipeline;
}

DebugOutline::DebugOutline() {
    _geometryDepthId = DependencyManager::get<GeometryCache>()->allocateID();
}

DebugOutline::~DebugOutline() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryDepthId);
    }
}

void DebugOutline::configure(const Config& config) {
    _isDisplayEnabled = config.viewMask;
}

void DebugOutline::run(const render::RenderContextPointer& renderContext, const Inputs& input) {
    const auto outlineRessources = input.get0();
    const auto outlineRect = input.get1();

    if (_isDisplayEnabled && outlineRessources && outlineRect.z>0 && outlineRect.w>0) {
        assert(renderContext->args);
        assert(renderContext->args->hasViewFrustum());
        RenderArgs* args = renderContext->args;

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            batch.setViewportTransform(args->_viewport);
            batch.setFramebuffer(outlineRessources->getColorFramebuffer());

            const auto geometryBuffer = DependencyManager::get<GeometryCache>();

            glm::mat4 projMat;
            Transform viewMat;
            args->getViewFrustum().evalProjectionMatrix(projMat);
            args->getViewFrustum().evalViewTransform(viewMat);
            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat, true);
            batch.setModelTransform(Transform());

            const glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);

            batch.setPipeline(getDepthPipeline());
            batch.setResourceTexture(0, outlineRessources->getDepthTexture());
            const glm::vec2 bottomLeft(-1.0f, -1.0f);
            const glm::vec2 topRight(1.0f, 1.0f);
            geometryBuffer->renderQuad(batch, bottomLeft, topRight, color, _geometryDepthId);

            batch.setResourceTexture(0, nullptr);
        });
    }
}

void DebugOutline::initializePipelines() {
    static const std::string VERTEX_SHADER{ debug_deferred_buffer_vert };
    static const std::string FRAGMENT_SHADER{ debug_deferred_buffer_frag };
    static const std::string SOURCE_PLACEHOLDER{ "//SOURCE_PLACEHOLDER" };
    static const auto SOURCE_PLACEHOLDER_INDEX = FRAGMENT_SHADER.find(SOURCE_PLACEHOLDER);
    Q_ASSERT_X(SOURCE_PLACEHOLDER_INDEX != std::string::npos, Q_FUNC_INFO,
               "Could not find source placeholder");

    auto state = std::make_shared<gpu::State>();
    state->setDepthTest(gpu::State::DepthTest(false, false));
    state->setStencilTest(true, 0, gpu::State::StencilTest(OUTLINE_STENCIL_MASK, 0xFF, gpu::EQUAL));

    const auto vs = gpu::Shader::createVertex(VERTEX_SHADER);

    // Depth shader
    {
        static const std::string DEPTH_SHADER{
            "vec4 getFragmentColor() {"
            "   float Zdb = texelFetch(depthMap, ivec2(gl_FragCoord.xy), 0).x;"
            "   Zdb = 1.0-(1.0-Zdb)*100;"
            "   return vec4(Zdb, Zdb, Zdb, 1.0); "
            "}"
        };

        auto fragmentShader = FRAGMENT_SHADER;
        fragmentShader.replace(SOURCE_PLACEHOLDER_INDEX, SOURCE_PLACEHOLDER.size(), DEPTH_SHADER);

        const auto ps = gpu::Shader::createPixel(fragmentShader);
        const auto program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("depthMap", 0));
        gpu::Shader::makeProgram(*program, slotBindings);

        _depthPipeline = gpu::Pipeline::create(program, state);
    }
}

const gpu::PipelinePointer& DebugOutline::getDepthPipeline() {
    if (!_depthPipeline) {
        initializePipelines();
    }

    return _depthPipeline;
}

DrawOutlineTask::DrawOutlineTask() {

}

void DrawOutlineTask::configure(const Config& config) {

}

void DrawOutlineTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs) {
    const auto groups = inputs.getN<Inputs>(0).get<Groups>();
    const auto sceneFrameBuffer = inputs.getN<Inputs>(1);
    const auto primaryFramebuffer = inputs.getN<Inputs>(2);
    const auto deferredFrameTransform = inputs.getN<Inputs>(3);

    // Prepare the ShapePipeline
    auto shapePlumber = std::make_shared<ShapePlumber>();
    {
        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        state->setColorWriteMask(false, false, false, false);

        initMaskPipelines(*shapePlumber, state);
    }
    auto sharedParameters = std::make_shared<OutlineSharedParameters>();

    // Prepare for outline group rendering.
    const auto outlineRessources = task.addJob<PrepareDrawOutline>("PrepareOutline", primaryFramebuffer);
    render::Varying outline0Rect;

    for (auto i = 0; i < render::Scene::MAX_OUTLINE_COUNT; i++) {
        const auto groupItems = groups[i];
        const auto outlinedItemIDs = task.addJob<render::MetaToSubItems>("OutlineMetaToSubItemIDs", groupItems);
        const auto outlinedItems = task.addJob<render::IDsToBounds>("OutlineMetaToSubItems", outlinedItemIDs);

        // Sort
        const auto sortedPipelines = task.addJob<render::PipelineSortShapes>("OutlinePipelineSort", outlinedItems);
        const auto sortedBounds = task.addJob<render::DepthSortShapes>("OutlineDepthSort", sortedPipelines);

        // Draw depth of outlined objects in separate buffer
        std::string name;
        {
            std::ostringstream stream;
            stream << "OutlineMask" << i;
            name = stream.str();
        }
        const auto drawMaskInputs = DrawOutlineMask::Inputs(sortedBounds, outlineRessources).asVarying();
        const auto outlinedRect = task.addJob<DrawOutlineMask>(name, drawMaskInputs, i, shapePlumber, sharedParameters);
        if (i == 0) {
            outline0Rect = outlinedRect;
        }

        // Draw outline
        {
            std::ostringstream stream;
            stream << "OutlineEffect" << i;
            name = stream.str();
        }
        const auto drawOutlineInputs = DrawOutline::Inputs(deferredFrameTransform, outlineRessources, sceneFrameBuffer, outlinedRect).asVarying();
        task.addJob<DrawOutline>(name, drawOutlineInputs, i, sharedParameters);
    }

    // Debug outline
    const auto debugInputs = DebugOutline::Inputs(outlineRessources, const_cast<const render::Varying&>(outline0Rect)).asVarying();
    task.addJob<DebugOutline>("OutlineDebug", debugInputs);
}

#include "model_shadow_vert.h"
#include "skin_model_shadow_vert.h"

#include "model_shadow_frag.h"

void DrawOutlineTask::initMaskPipelines(render::ShapePlumber& shapePlumber, gpu::StatePointer state) {
    auto modelVertex = gpu::Shader::createVertex(std::string(model_shadow_vert));
    auto modelPixel = gpu::Shader::createPixel(std::string(model_shadow_frag));
    gpu::ShaderPointer modelProgram = gpu::Shader::createProgram(modelVertex, modelPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withoutSkinned(),
        modelProgram, state);

    auto skinVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_vert));
    gpu::ShaderPointer skinProgram = gpu::Shader::createProgram(skinVertex, modelPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withSkinned(),
        skinProgram, state);
}
