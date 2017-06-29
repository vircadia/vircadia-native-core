#include "FadeEffect.h"
#include "TextureCache.h"

#include <PathUtils.h>
#include <NumericalConstants.h>
#include <Interpolate.h>
#include <gpu/Context.h>

void FadeSwitchJob::configure(const Config& config) {
    _parameters->_isEditEnabled = config.editFade;
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

    // Find the nearest item that intersects the view direction
    const render::Item* editedItem = nullptr;
    if (_parameters->_isEditEnabled) {
        float nearestOpaqueDistance = std::numeric_limits<float>::max();
        float nearestTransparentDistance = std::numeric_limits<float>::max();
        const render::Item* nearestItem;

        editedItem = findNearestItem(renderContext, input[RenderFetchCullSortTask::OPAQUE_SHAPE], nearestOpaqueDistance);
        nearestItem = findNearestItem(renderContext, input[RenderFetchCullSortTask::TRANSPARENT_SHAPE], nearestTransparentDistance);
        if (nearestTransparentDistance < nearestOpaqueDistance) {
            editedItem = nearestItem;
        }
    }

    // Now, distribute items that need to be faded accross both outputs
    distribute(renderContext, input[RenderFetchCullSortTask::OPAQUE_SHAPE], normalOutputs[RenderFetchCullSortTask::OPAQUE_SHAPE], fadeOutputs[OPAQUE_SHAPE], editedItem);
    distribute(renderContext, input[RenderFetchCullSortTask::TRANSPARENT_SHAPE], normalOutputs[RenderFetchCullSortTask::TRANSPARENT_SHAPE], fadeOutputs[TRANSPARENT_SHAPE], editedItem);
}

const render::Item* FadeSwitchJob::findNearestItem(const render::RenderContextPointer& renderContext, const render::Varying& input, float& minIsectDistance) const {
    const glm::vec3 rayOrigin = renderContext->args->getViewFrustum().getPosition();
    const glm::vec3 rayDirection = renderContext->args->getViewFrustum().getDirection();
    const auto& inputItems = input.get<render::ItemBounds>();
    auto& scene = renderContext->_scene;
    BoxFace face;
    glm::vec3 normal;
    float isectDistance;
    const render::Item* nearestItem = nullptr;
    const float minDistance = 5.f;

    for (const auto& itemBound : inputItems) {
        if (itemBound.bound.findRayIntersection(rayOrigin, rayDirection, isectDistance, face, normal)) {
            if (isectDistance>minDistance && isectDistance < minIsectDistance) {
                auto& item = scene->getItem(itemBound.id);
                nearestItem = &item;
                minIsectDistance = isectDistance;
            }
        }
    }
    return nearestItem;
}

void FadeSwitchJob::distribute(const render::RenderContextPointer& renderContext, const render::Varying& input, 
    render::Varying& normalOutput, render::Varying& fadeOutput, const render::Item* editedItem) const {
    auto& scene = renderContext->_scene;
    assert(_parameters);
    const double fadeDuration = double(_parameters->_durations[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN]) * USECS_PER_SECOND;
    const auto& inputItems = input.get<render::ItemBounds>();

    // Clear previous values
    normalOutput.template edit<render::ItemBounds>().clear();
    fadeOutput.template edit<render::ItemBounds>().clear();

    for (const auto&  itemBound : inputItems) {
        auto& item = scene->getItem(itemBound.id);

        if (!item.mustFade() && &item!=editedItem) {
            // No need to fade
            normalOutput.template edit<render::ItemBounds>().emplace_back(itemBound);
        }
        else {
            fadeOutput.template edit<render::ItemBounds>().emplace_back(itemBound);
        }
    }
}

void FadeJobConfig::setEditedCategory(int value) {
    assert(value < EVENT_CATEGORY_COUNT);
    editedCategory = std::min<int>(EVENT_CATEGORY_COUNT, value);
    emit dirty();
}

void FadeJobConfig::setDuration(float value) {
    duration[editedCategory] = value;
    emit dirty();
}

void FadeJobConfig::setBaseSizeX(float value) {
    baseSize[editedCategory].x = value;
    emit dirty();
}

void FadeJobConfig::setBaseSizeY(float value) {
    baseSize[editedCategory].y = value;
    emit dirty();
}

void FadeJobConfig::setBaseSizeZ(float value) {
    baseSize[editedCategory].z = value;
    emit dirty();
}

void FadeJobConfig::setBaseLevel(float value) {
    baseLevel[editedCategory] = value;
    emit dirty();
}

void FadeJobConfig::setBaseInverted(bool value) {
    baseInverted[editedCategory] = value;
    emit dirty();
}

void FadeJobConfig::setNoiseSizeX(float value) {
    noiseSize[editedCategory].x = value;
    emit dirty();
}

void FadeJobConfig::setNoiseSizeY(float value) {
    noiseSize[editedCategory].y = value;
    emit dirty();
}

void FadeJobConfig::setNoiseSizeZ(float value) {
    noiseSize[editedCategory].z = value;
    emit dirty();
}

void FadeJobConfig::setNoiseLevel(float value) {
    noiseLevel[editedCategory] = value;
    emit dirty();
}

void FadeJobConfig::setEdgeWidth(float value) {
    edgeWidth[editedCategory] = value;
    emit dirty();
}

void FadeJobConfig::setEdgeInnerColorR(float value) {
    edgeInnerColor[editedCategory].r = value;
    emit dirty();
}

void FadeJobConfig::setEdgeInnerColorG(float value) {
    edgeInnerColor[editedCategory].g = value;
    emit dirty();
}

void FadeJobConfig::setEdgeInnerColorB(float value) {
    edgeInnerColor[editedCategory].b = value;
    emit dirty();
}

void FadeJobConfig::setEdgeInnerIntensity(float value) {
    edgeInnerColor[editedCategory].a = value;
    emit dirty();
}

void FadeJobConfig::setEdgeOuterColorR(float value) {
    edgeOuterColor[editedCategory].r = value;
    emit dirty();
}

void FadeJobConfig::setEdgeOuterColorG(float value) {
    edgeOuterColor[editedCategory].g = value;
    emit dirty();
}

void FadeJobConfig::setEdgeOuterColorB(float value) {
    edgeOuterColor[editedCategory].b = value;
    emit dirty();
}

void FadeJobConfig::setEdgeOuterIntensity(float value) {
    edgeOuterColor[editedCategory].a = value;
    emit dirty();
}

FadeConfigureJob::FadeConfigureJob(FadeCommonParameters::Pointer commonParams) : 
    _parameters{ commonParams }
{
	auto texturePath = PathUtils::resourcesPath() + "images/fadeMask.png";
	_fadeMaskMap = DependencyManager::get<TextureCache>()->getImageTexture(texturePath, image::TextureUsage::STRICT_TEXTURE);
}

void FadeConfigureJob::configure(const Config& config) {
    assert(_parameters);
    _parameters->_editedCategory = config.editedCategory;
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

void FadeRenderJob::run(const render::RenderContextPointer& renderContext, const Input& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    const auto& inItems = inputs.get0();

    if (!inItems.empty()) {
        const auto& lightingModel = inputs.get1();
        const auto& configuration = inputs.get2();
        const auto& fadeMaskMap = configuration.get0();
        const auto& fadeParamBuffer = configuration.get1();

        RenderArgs* args = renderContext->args;
        render::ShapeKey::Builder   defaultKeyBuilder;

        defaultKeyBuilder.withFade();

        // Update interactive edit effect
        if (_parameters->_isEditEnabled) {
            updateFadeEdit();
        }
        else {
            _editStartTime = 0;
        }

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            args->_batch = &batch;

            // Very, very ugly hack to keep track of the current fade render job
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

float FadeRenderJob::computeElementEnterThreshold(double time) const {
    float fadeAlpha = 1.0f;
    const double INV_FADE_PERIOD = 1.0 / (double)(_parameters->_durations[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN]);
    double fraction = time * INV_FADE_PERIOD;
    fraction = std::max(fraction, 0.0);
    if (fraction < 1.0) {
        fadeAlpha = Interpolate::easeInOutQuad(fraction);
    }
    return fadeAlpha;
}

float FadeRenderJob::computeFadePercent(quint64 startTime) {
    const double time = (double)(int64_t(usecTimestampNow()) - int64_t(startTime)) / (double)(USECS_PER_SECOND);
    assert(_currentInstance);
    return _currentInstance->computeElementEnterThreshold(time);
}

void FadeRenderJob::updateFadeEdit() {
    if (_editStartTime == 0) {
        _editStartTime = usecTimestampNow();
    }

    const double time = (int64_t(usecTimestampNow()) - int64_t(_editStartTime)) / double(USECS_PER_SECOND);

    switch (_parameters->_editedCategory) {
    case FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN:
    {
        const double waitTime = 0.5;    // Wait between fade in and out
        const float eventDuration = _parameters->_durations[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN];
        double  cycleTime = fmod(time, (eventDuration+waitTime) * 2.0);

        if (cycleTime < eventDuration) {
            _editThreshold = 1.f-computeElementEnterThreshold(cycleTime);
        }
        else if (cycleTime < (eventDuration + waitTime)) {
            _editThreshold = 0.f;
        }
        else if (cycleTime < (2 * eventDuration + waitTime)) {
            _editThreshold = computeElementEnterThreshold(cycleTime- (eventDuration + waitTime));
        }
        else {
            _editThreshold = 1.f;
        }
    }
    break;

    case FadeJobConfig::BUBBLE_ISECT_OWNER:
        break;

    case FadeJobConfig::BUBBLE_ISECT_TRESPASSER:
        break;

    case FadeJobConfig::USER_ENTER_LEAVE_DOMAIN:
        break;

    case FadeJobConfig::AVATAR_CHANGE:
        break;

    default:
        assert(false);
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
        float threshold;
        int eventCategory = FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN;

        threshold = 1.f-computeFadePercent(startTime);

        // Manage interactive edition override
        assert(_currentInstance);
        if (_currentInstance->_parameters->_isEditEnabled) {
            eventCategory = _currentInstance->_parameters->_editedCategory;
            threshold = _currentInstance->_editThreshold;
        }

        batch._glUniform1i(fadeCategoryLocation, eventCategory);
        batch._glUniform1f(fadeThresholdLocation, threshold);
        // This is really temporary
        batch._glUniform3f(fadeNoiseOffsetLocation, offset.x, offset.y, offset.z);
        // This is really temporary
        batch._glUniform3f(fadeBaseOffsetLocation, offset.x, offset.y, offset.z);

        return threshold > 0.f;
    }
    return false;
}
