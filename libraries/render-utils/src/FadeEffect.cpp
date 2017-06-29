#include "FadeEffect.h"
#include "TextureCache.h"

#include <PathUtils.h>
#include <NumericalConstants.h>
#include <Interpolate.h>
#include <gpu/Context.h>

void FadeSwitchJob::configure(const Config& config) {
    _isEditEnabled = config.editFade;
}

void FadeSwitchJob::run(const render::RenderContextPointer& renderContext, const Input& input, Output& output) {
    auto& normalOutputs = output.edit0();
    auto& fadeOutputs = output.edit1();

    // Only shapes are affected by fade at this time.
    normalOutputs[RenderFetchCullSortTask::LIGHT] = input[RenderFetchCullSortTask::LIGHT];
    normalOutputs[RenderFetchCullSortTask::META] = input[RenderFetchCullSortTask::META];
    normalOutputs[RenderFetchCullSortTask::OVERLAY_OPAQUE_SHAPE] = input[RenderFetchCullSortTask::OVERLAY_OPAQUE_SHAPE];
    normalOutputs[RenderFetchCullSortTask::OVERLAY_TRANSPARENT_SHAPE] = input[RenderFetchCullSortTask::OVERLAY_TRANSPARENT_SHAPE];
    normalOutputs[RenderFetchCullSortTask::BACKGROUND] = input[RenderFetchCullSortTask::BACKGROUND];
    normalOutputs[RenderFetchCullSortTask::SPATIAL_SELECTION] = input[RenderFetchCullSortTask::SPATIAL_SELECTION];

    // Now, distribute items that need to be faded accross both outputs
    distribute(renderContext, input[RenderFetchCullSortTask::OPAQUE_SHAPE], normalOutputs[RenderFetchCullSortTask::OPAQUE_SHAPE], fadeOutputs[OPAQUE_SHAPE]);
    distribute(renderContext, input[RenderFetchCullSortTask::TRANSPARENT_SHAPE], normalOutputs[RenderFetchCullSortTask::TRANSPARENT_SHAPE], fadeOutputs[TRANSPARENT_SHAPE]);
}

void FadeSwitchJob::distribute(const render::RenderContextPointer& renderContext, const render::Varying& input, 
    render::Varying& normalOutput, render::Varying& fadeOutput) const {
    auto& scene = renderContext->_scene;
    assert(_parameters);
    const double fadeDuration = double(_parameters->_durations[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN]) * USECS_PER_SECOND;
    const auto& inputItems = input.get<render::ItemBounds>();

    // Clear previous values
    normalOutput.template edit<render::ItemBounds>().clear();
    fadeOutput.template edit<render::ItemBounds>().clear();

    for (const auto&  itemBound : inputItems) {
        auto& item = scene->getItem(itemBound.id);

        if (!item.mustFade()) {
            // No need to fade
            normalOutput.template edit<render::ItemBounds>().emplace_back(itemBound);
        }
        else {
            fadeOutput.template edit<render::ItemBounds>().emplace_back(itemBound);
        }
    }
/*    if (!_isEditEnabled) {

    }*/
}

FadeConfigureJob::FadeConfigureJob(FadeCommonParameters::Pointer commonParams) : 
    _parameters{ commonParams }
{
	auto texturePath = PathUtils::resourcesPath() + "images/fadeMask.png";
	_fadeMaskMap = DependencyManager::get<TextureCache>()->getImageTexture(texturePath, image::TextureUsage::STRICT_TEXTURE);
}

void FadeConfigureJob::configure(const Config& config) {
    assert(_parameters);
    for (auto i = 0; i < FadeJobConfig::EVENT_CATEGORY_COUNT; i++) {
        auto& configuration = _configurations[i];

        _parameters->_durations[i] = config.duration[i];
        configuration._baseInvSizeAndLevel.x = 1.f / config.baseSize[i].x;
        configuration._baseInvSizeAndLevel.y = 1.f / config.baseSize[i].y;
        configuration._baseInvSizeAndLevel.z = 1.f / config.baseSize[i].z;
        configuration._baseInvSizeAndLevel.w = config.baseLevel[i];
        configuration._noiseInvSizeAndLevel.x = 1.f / config.noiseSize[i].x;
        configuration._noiseInvSizeAndLevel.y = 1.f / config.noiseSize[i].y;
        configuration._noiseInvSizeAndLevel.z = 1.f / config.noiseSize[i].z;
        configuration._noiseInvSizeAndLevel.w = config.noiseLevel[i];
        configuration._invertBase = config.baseInverted[i];
        configuration._edgeWidthInvWidth.x = config.edgeWidth[i];
        configuration._edgeWidthInvWidth.y = 1.f / configuration._edgeWidthInvWidth.x;
        configuration._innerEdgeColor.r = config.edgeInnerColor[i].r * config.edgeInnerColor[i].a;
        configuration._innerEdgeColor.g = config.edgeInnerColor[i].g * config.edgeInnerColor[i].a;
        configuration._innerEdgeColor.b = config.edgeInnerColor[i].b * config.edgeInnerColor[i].a;
        configuration._outerEdgeColor.r = config.edgeOuterColor[i].r * config.edgeOuterColor[i].a;
        configuration._outerEdgeColor.g = config.edgeOuterColor[i].g * config.edgeOuterColor[i].a;
        configuration._outerEdgeColor.b = config.edgeOuterColor[i].b * config.edgeOuterColor[i].a;
    }
    _isBufferDirty = true;
}

void FadeConfigureJob::run(const render::RenderContextPointer& renderContext, Output& output) {
    if (_isBufferDirty) {
        auto& configurations = output.edit1().edit();
        std::copy(_configurations, _configurations + FadeJobConfig::EVENT_CATEGORY_COUNT, configurations.parameters);
        _isBufferDirty = false;
    }
    output.edit0() = _fadeMaskMap;
}

const FadeRenderJob* FadeRenderJob::_currentInstance{ nullptr };
gpu::TexturePointer FadeRenderJob::_currentFadeMaskMap;
const gpu::BufferView* FadeRenderJob::_currentFadeBuffer{ nullptr };

float FadeRenderJob::computeFadePercent(quint64 startTime) {
    assert(_currentInstance);
    float fadeAlpha = 1.0f;
    const double INV_FADE_PERIOD = 1.0 / (double)(_currentInstance->_parameters->_durations[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] * USECS_PER_SECOND);
    double fraction = (double)(int64_t(usecTimestampNow()) - int64_t(startTime)) * INV_FADE_PERIOD;
    fraction = std::max(fraction, 0.0);
    if (fraction < 1.0) {
        fadeAlpha = Interpolate::easeInOutQuad(fraction);
    }
    return fadeAlpha;
}

void FadeRenderJob::run(const render::RenderContextPointer& renderContext, const Input& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    const auto& inItems = inputs.get0();

    if (!inItems.empty()) {
        const auto& lightingModel = inputs.get1();
        const auto& configuration = inputs.get2();
        const auto& fadeMaskMap = configuration.get0();
        const auto& fadeParamBuffer = configuration.get1();

        // Very, very ugly hack to keep track of the current fade render job
        RenderArgs* args = renderContext->args;
        render::ShapeKey::Builder   defaultKeyBuilder;

        defaultKeyBuilder.withFade();

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            args->_batch = &batch;

            _currentInstance = this;
            _currentFadeMaskMap = fadeMaskMap;
            _currentFadeBuffer = &fadeParamBuffer;

            // Setup camera, projection and viewport for all items
            batch.setViewportTransform(args->_viewport);
            batch.setStateScissorRect(args->_viewport);

            glm::mat4 projMat;
            Transform viewMat;
            args->getViewFrustum().evalProjectionMatrix(projMat);
            args->getViewFrustum().evalViewTransform(viewMat);

            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat);

            // Setup lighting model for all items;
            batch.setUniformBuffer(render::ShapePipeline::Slot::LIGHTING_MODEL, lightingModel->getParametersBuffer());

            // From the lighting model define a global shapKey ORED with individiual keys
            render::ShapeKey::Builder keyBuilder = defaultKeyBuilder;
            if (lightingModel->isWireframeEnabled()) {
                keyBuilder.withWireframe();
            }

            // Prepare fade effect
            bindPerBatch(batch, fadeMaskMap, render::ShapePipeline::Slot::MAP::FADE_MASK, &fadeParamBuffer, render::ShapePipeline::Slot::BUFFER::FADE_PARAMETERS);

            render::ShapeKey globalKey = keyBuilder.build();
            args->_globalShapeKey = globalKey._flags.to_ulong();
            args->_enableFade = true;

            renderShapes(renderContext, _shapePlumber, inItems, -1, globalKey);

            args->_enableFade = false;
            args->_batch = nullptr;
            args->_globalShapeKey = 0;

            // Very, very ugly hack to keep track of the current fade render job
            _currentInstance = nullptr;
            _currentFadeMaskMap.reset();
            _currentFadeBuffer = nullptr;
        });
    }
}

void FadeRenderJob::bindPerBatch(gpu::Batch& batch, int fadeMaskMapLocation, int fadeBufferLocation) {
    assert(_currentFadeMaskMap);
    assert(_currentFadeBuffer!=nullptr);
    bindPerBatch(batch, _currentFadeMaskMap, fadeMaskMapLocation, _currentFadeBuffer, fadeBufferLocation);
}

void FadeRenderJob::bindPerBatch(gpu::Batch& batch, gpu::TexturePointer texture, int fadeMaskMapLocation, const gpu::BufferView* buffer, int fadeBufferLocation) {
    batch.setResourceTexture(fadeMaskMapLocation, texture);
    batch.setUniformBuffer(fadeBufferLocation, *buffer);
}

void FadeRenderJob::bindPerBatch(gpu::Batch& batch, gpu::TexturePointer texture, const gpu::BufferView* buffer, const gpu::PipelinePointer& pipeline)  {
    auto program = pipeline->getProgram();
    auto maskMapLocation = program->getTextures().findLocation("fadeMaskMap");
    auto bufferLocation = program->getUniformBuffers().findLocation("fadeParametersBuffer");
    bindPerBatch(batch, texture, maskMapLocation, buffer, bufferLocation);
}

bool FadeRenderJob::bindPerItem(gpu::Batch& batch, RenderArgs* args, glm::vec3 offset, quint64 startTime) {
    return bindPerItem(batch, args->_pipeline->pipeline.get(), offset, startTime);
}

bool FadeRenderJob::bindPerItem(gpu::Batch& batch, const gpu::Pipeline* pipeline, glm::vec3 offset, quint64 startTime) {
    auto& uniforms = pipeline->getProgram()->getUniforms();
    auto fadeNoiseOffsetLocation = uniforms.findLocation("fadeNoiseOffset");
    auto fadeBaseOffsetLocation = uniforms.findLocation("fadeBaseOffset");
    auto fadeThresholdLocation = uniforms.findLocation("fadeThreshold");
    auto fadeCategoryLocation = uniforms.findLocation("fadeCategory");

    if (fadeNoiseOffsetLocation >= 0 || fadeBaseOffsetLocation>=0 || fadeThresholdLocation >= 0 || fadeCategoryLocation>=0) {
        float percent;

        percent = computeFadePercent(startTime);
        batch._glUniform1i(fadeCategoryLocation, FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN);
        batch._glUniform1f(fadeThresholdLocation, 1.f-percent);
        // This is really temporary
        batch._glUniform3f(fadeNoiseOffsetLocation, offset.x, offset.y, offset.z);
        // This is really temporary
        batch._glUniform3f(fadeBaseOffsetLocation, offset.x, offset.y, offset.z);

        return percent < 1.f;
    }
    return false;
}
