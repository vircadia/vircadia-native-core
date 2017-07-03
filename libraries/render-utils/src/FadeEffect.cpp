#include "FadeEffect.h"
#include "TextureCache.h"

#include <PathUtils.h>
#include <NumericalConstants.h>
#include <Interpolate.h>
#include <gpu/Context.h>

#define FADE_MIN_SCALE  0.001
#define FADE_MAX_SCALE  10000.0

inline float parameterToValuePow(float parameter, const double minValue, const double maxOverMinValue) {
    return (float)(minValue * pow(maxOverMinValue, parameter));
}

inline float valueToParameterPow(float value, const double minValue, const double maxOverMinValue) {
    return (float)(log(value / minValue) / log(maxOverMinValue));
}

void FadeSwitchJob::configure(const Config& config) {
    _parameters->_isEditEnabled = config.editFade;
}

void FadeSwitchJob::run(const render::RenderContextPointer& renderContext, const Input& input, Output& output) {
    auto& normalOutputs = output.edit0().edit0();
    auto& fadeOutputs = output.edit1();

    // Only shapes are affected by fade at this time.
    normalOutputs[RenderFetchCullSortTask::LIGHT].edit<render::ItemBounds>() = input.get0()[RenderFetchCullSortTask::LIGHT].get<render::ItemBounds>();
    normalOutputs[RenderFetchCullSortTask::META].edit<render::ItemBounds>() = input.get0()[RenderFetchCullSortTask::META].get<render::ItemBounds>();
    normalOutputs[RenderFetchCullSortTask::OVERLAY_OPAQUE_SHAPE].edit<render::ItemBounds>() = input.get0()[RenderFetchCullSortTask::OVERLAY_OPAQUE_SHAPE].get<render::ItemBounds>();
    normalOutputs[RenderFetchCullSortTask::OVERLAY_TRANSPARENT_SHAPE].edit<render::ItemBounds>() = input.get0()[RenderFetchCullSortTask::OVERLAY_TRANSPARENT_SHAPE].get<render::ItemBounds>();
    normalOutputs[RenderFetchCullSortTask::BACKGROUND].edit<render::ItemBounds>() = input.get0()[RenderFetchCullSortTask::BACKGROUND].get<render::ItemBounds>();
    output.edit0().edit1() = input.get1();

    // Find the nearest item that intersects the view direction
    const render::Item* editedItem = nullptr;
    if (_parameters->_isEditEnabled) {
        float nearestOpaqueDistance = std::numeric_limits<float>::max();
        float nearestTransparentDistance = std::numeric_limits<float>::max();
        const render::Item* nearestItem;

        editedItem = findNearestItem(renderContext, input.get0()[RenderFetchCullSortTask::OPAQUE_SHAPE], nearestOpaqueDistance);
        nearestItem = findNearestItem(renderContext, input.get0()[RenderFetchCullSortTask::TRANSPARENT_SHAPE], nearestTransparentDistance);
        if (nearestTransparentDistance < nearestOpaqueDistance) {
            editedItem = nearestItem;
        }

        if (editedItem) {
            output.edit2() = editedItem->getBound();
        }
    }

    // Now, distribute items that need to be faded accross both outputs
    distribute(renderContext, input.get0()[RenderFetchCullSortTask::OPAQUE_SHAPE], normalOutputs[RenderFetchCullSortTask::OPAQUE_SHAPE], fadeOutputs[OPAQUE_SHAPE], editedItem);
    distribute(renderContext, input.get0()[RenderFetchCullSortTask::TRANSPARENT_SHAPE], normalOutputs[RenderFetchCullSortTask::TRANSPARENT_SHAPE], fadeOutputs[TRANSPARENT_SHAPE], editedItem);
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
    const float minDistance = 2.f;

    for (const auto& itemBound : inputItems) {
        if (!itemBound.bound.contains(rayOrigin) && itemBound.bound.findRayIntersection(rayOrigin, rayDirection, isectDistance, face, normal)) {
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
    const auto& inputItems = input.get<render::ItemBounds>();

    // Clear previous values
    normalOutput.edit<render::ItemBounds>().clear();
    fadeOutput.edit<render::ItemBounds>().clear();

    for (const auto&  itemBound : inputItems) {
        auto& item = scene->getItem(itemBound.id);

        if (!item.mustFade() && &item!=editedItem) {
            // No need to fade
            normalOutput.edit<render::ItemBounds>().emplace_back(itemBound);
        }
        else {
            fadeOutput.edit<render::ItemBounds>().emplace_back(itemBound);
        }
    }
}

FadeCommonParameters::FadeCommonParameters()
{
    _durations[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = 0.f;
    _durations[FadeJobConfig::BUBBLE_ISECT_OWNER] = 0.f;
    _durations[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = 0.f;
    _durations[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = 0.f;
    _durations[FadeJobConfig::AVATAR_CHANGE] = 0.f;
}

FadeJobConfig::FadeJobConfig() 
{
    noiseSize[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = glm::vec3{ 0.75f, 0.75f, 0.75f };
    noiseSize[FadeJobConfig::BUBBLE_ISECT_OWNER] = glm::vec3{ 0.4f, 0.4f, 0.4f };
    noiseSize[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = glm::vec3{ 0.4f, 0.4f, 0.4f };
    noiseSize[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = glm::vec3{ 10.f, 0.01f, 10.0f };
    noiseSize[FadeJobConfig::AVATAR_CHANGE] = glm::vec3{ 0.4f, 0.4f, 0.4f };

    noiseLevel[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = 1.f;
    noiseLevel[FadeJobConfig::BUBBLE_ISECT_OWNER] = 1.f;
    noiseLevel[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = 1.f;
    noiseLevel[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = 0.7f;
    noiseLevel[FadeJobConfig::AVATAR_CHANGE] = 1.f;

    baseSize[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = glm::vec3{ 1.0f, 1.0f, 1.0f };
    baseSize[FadeJobConfig::BUBBLE_ISECT_OWNER] = glm::vec3{ 2.0f, 2.0f, 2.0f };
    baseSize[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = glm::vec3{ 2.0f, 2.0f, 2.0f };
    baseSize[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = glm::vec3{ 10000.f, 1.0f, 10000.0f };
    baseSize[FadeJobConfig::AVATAR_CHANGE] = glm::vec3{ 0.4f, 0.4f, 0.4f };

    baseLevel[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = 0.f;
    baseLevel[FadeJobConfig::BUBBLE_ISECT_OWNER] = 1.f;
    baseLevel[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = 1.f;
    baseLevel[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = 1.f;
    baseLevel[FadeJobConfig::AVATAR_CHANGE] = 1.f;

    baseInverted[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = false;
    baseInverted[FadeJobConfig::BUBBLE_ISECT_OWNER] = false;
    baseInverted[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = false;
    baseInverted[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = true;
    baseInverted[FadeJobConfig::AVATAR_CHANGE] = false;

    _duration[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = 4.f;
    _duration[FadeJobConfig::BUBBLE_ISECT_OWNER] = 0.f;
    _duration[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = 0.f;
    _duration[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = 3.f;
    _duration[FadeJobConfig::AVATAR_CHANGE] = 3.f;

    edgeWidth[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = 0.1f;
    edgeWidth[FadeJobConfig::BUBBLE_ISECT_OWNER] = 0.08f;
    edgeWidth[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = 0.08f;
    edgeWidth[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = 0.329f;
    edgeWidth[FadeJobConfig::AVATAR_CHANGE] = 0.05f;

    edgeInnerColor[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 0.0f };
    edgeInnerColor[FadeJobConfig::BUBBLE_ISECT_OWNER] = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 1.0f };
    edgeInnerColor[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
    edgeInnerColor[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 0.25f };
    edgeInnerColor[FadeJobConfig::AVATAR_CHANGE] = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };

    edgeOuterColor[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 1.0f };
    edgeOuterColor[FadeJobConfig::BUBBLE_ISECT_OWNER] = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 2.0f };
    edgeOuterColor[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 2.0f };
    edgeOuterColor[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 1.0f };
    edgeOuterColor[FadeJobConfig::AVATAR_CHANGE] = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
}

void FadeJobConfig::setEditedCategory(int value) {
    assert(value < EVENT_CATEGORY_COUNT);
    editedCategory = std::min<int>(EVENT_CATEGORY_COUNT, value);
    emit dirtyCategory();
    emit dirty();
}

void FadeJobConfig::setDuration(float value) {
    _duration[editedCategory] = value;
    emit dirty();
}

float FadeJobConfig::getDuration() const { 
    return _duration[editedCategory]; 
}

void FadeJobConfig::setBaseSizeX(float value) {
    baseSize[editedCategory].x = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE/ FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getBaseSizeX() const { 
    return valueToParameterPow(baseSize[editedCategory].x, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setBaseSizeY(float value) {
    baseSize[editedCategory].y = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getBaseSizeY() const {
    return valueToParameterPow(baseSize[editedCategory].y, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setBaseSizeZ(float value) {
    baseSize[editedCategory].z = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getBaseSizeZ() const {
    return valueToParameterPow(baseSize[editedCategory].z, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setBaseLevel(float value) {
    baseLevel[editedCategory] = value;
    emit dirty();
}

void FadeJobConfig::setBaseInverted(bool value) {
    baseInverted[editedCategory] = value;
    emit dirty();
}

bool FadeJobConfig::isBaseInverted() const { 
    return baseInverted[editedCategory]; 
}

void FadeJobConfig::setNoiseSizeX(float value) {
    noiseSize[editedCategory].x = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getNoiseSizeX() const {
    return valueToParameterPow(noiseSize[editedCategory].x, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setNoiseSizeY(float value) {
    noiseSize[editedCategory].y = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getNoiseSizeY() const {
    return valueToParameterPow(noiseSize[editedCategory].y, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setNoiseSizeZ(float value) {
    noiseSize[editedCategory].z = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getNoiseSizeZ() const {
    return valueToParameterPow(noiseSize[editedCategory].z, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setNoiseLevel(float value) {
    noiseLevel[editedCategory] = value;
    emit dirty();
}

void FadeJobConfig::setEdgeWidth(float value) {
    edgeWidth[editedCategory] = value * value;
    emit dirty();
}

float FadeJobConfig::getEdgeWidth() const { 
    return sqrtf(edgeWidth[editedCategory]); 
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
    _parameters->_isManualThresholdEnabled = config.manualFade;
    _parameters->_manualThreshold = config.manualThreshold;

    for (auto i = 0; i < FadeJobConfig::EVENT_CATEGORY_COUNT; i++) {
        auto& configuration = _configurations[i];

        _parameters->_durations[i] = config._duration[i];
        configuration._baseInvSizeAndLevel.x = 1.f / config.baseSize[i].x;
        configuration._baseInvSizeAndLevel.y = 1.f / config.baseSize[i].y;
        configuration._baseInvSizeAndLevel.z = 1.f / config.baseSize[i].z;
        configuration._baseInvSizeAndLevel.w = config.baseLevel[i];
        configuration._noiseInvSizeAndLevel.x = 1.f / config.noiseSize[i].x;
        configuration._noiseInvSizeAndLevel.y = 1.f / config.noiseSize[i].y;
        configuration._noiseInvSizeAndLevel.z = 1.f / config.noiseSize[i].z;
        configuration._noiseInvSizeAndLevel.w = config.noiseLevel[i];
        configuration._invertBase = config.baseInverted[i] & 1;
        configuration._edgeWidthInvWidth.x = config.edgeWidth[i];
        configuration._edgeWidthInvWidth.y = 1.f / configuration._edgeWidthInvWidth.x;
        configuration._innerEdgeColor = config.edgeInnerColor[i];
        configuration._outerEdgeColor = config.edgeOuterColor[i];
        _parameters->_thresholdScale[i] = 1.f + 2.f*(configuration._edgeWidthInvWidth.x + std::max(0.f, (config.noiseLevel[i] + config.baseLevel[i])*0.5f-0.5f));
    }
    _isBufferDirty = true;
}

void FadeConfigureJob::run(const render::RenderContextPointer& renderContext, const Input& input, Output& output) {
    if (_isBufferDirty || _parameters->_isEditEnabled) {
        auto& configurations = output.edit1().edit();
        std::copy(_configurations, _configurations + FadeJobConfig::EVENT_CATEGORY_COUNT, configurations.parameters);
        if (_parameters->_editedCategory == FadeJobConfig::USER_ENTER_LEAVE_DOMAIN) {
            configurations.parameters[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN]._baseInvSizeAndLevel.y = 1.0f / input.getDimensions().y;
        }
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

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            args->_batch = &batch;

            // Very, very ugly hack to keep track of the current fade render job
            _currentInstance = this;
            _currentFadeMaskMap = fadeMaskMap;
            _currentFadeBuffer = &fadeParamBuffer;

            // Update interactive edit effect
            if (_parameters->_isEditEnabled) {
                updateFadeEdit(renderContext, inItems.front());
            }
            else {
                _editPreviousTime = 0;
            }

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

float FadeRenderJob::computeElementEnterThreshold(double time, const double period) const {
    assert(period > 0.0);
    float fadeAlpha = 1.0f;
    const double INV_FADE_PERIOD = 1.0 / period;
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
    return _currentInstance->computeElementEnterThreshold(time, _currentInstance->_parameters->_durations[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN]);
}

void FadeRenderJob::updateFadeEdit(const render::RenderContextPointer& renderContext, const render::ItemBound& itemBounds) {
    if (_editPreviousTime == 0) {
        _editPreviousTime = usecTimestampNow();
        _editTime = 0.0;
    }

    uint64_t now = usecTimestampNow();
    const double deltaTime = (int64_t(now) - int64_t(_editPreviousTime)) / double(USECS_PER_SECOND);
    const float eventDuration = _parameters->_durations[_parameters->_editedCategory];
    const double waitTime = 0.5;    // Wait between fade in and out
    double  cycleTime = fmod(_editTime, (eventDuration + waitTime) * 2.0);

    _editTime += deltaTime;
    _editPreviousTime = now;

    if (cycleTime < eventDuration) {
        _editThreshold = 1.f - computeElementEnterThreshold(cycleTime, eventDuration);
    }
    else if (cycleTime < (eventDuration + waitTime)) {
        _editThreshold = 0.f;
    }
    else if (cycleTime < (2 * eventDuration + waitTime)) {
        _editThreshold = computeElementEnterThreshold(cycleTime - (eventDuration + waitTime), eventDuration);
    }
    else {
        _editThreshold = 1.f;
    }

    switch (_parameters->_editedCategory) {
    case FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN:
        break;

    case FadeJobConfig::BUBBLE_ISECT_OWNER:
    {
        const glm::vec3 cameraPos = renderContext->args->getViewFrustum().getPosition();
        const glm::vec3 delta = itemBounds.bound.calcCenter() - cameraPos;

        _editNoiseOffset.x = _editTime*0.1f;
        _editNoiseOffset.y = _editTime*2.5f;
        _editNoiseOffset.z = _editTime*0.1f;

        _editBaseOffset = cameraPos + delta*_editThreshold;
        _editThreshold = 0.33f;
    }
    break;

    case FadeJobConfig::BUBBLE_ISECT_TRESPASSER:
        break;

    case FadeJobConfig::USER_ENTER_LEAVE_DOMAIN:
    {
        _editNoiseOffset.x = _editTime*0.5f;
        _editNoiseOffset.y = 0.f;
        _editNoiseOffset.z = _editTime*0.75f;

        _editBaseOffset = itemBounds.bound.calcCenter();
    }
    break;

    case FadeJobConfig::AVATAR_CHANGE:
        break;

    default:
        assert(false);
    }

    if (_parameters->_isManualThresholdEnabled) {
        _editThreshold = _parameters->_manualThreshold;
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
        glm::vec3 noiseOffset = offset;
        glm::vec3 baseOffset = offset;

        threshold = 1.f-computeFadePercent(startTime);

        // Manage interactive edition override
        assert(_currentInstance);
        if (_currentInstance->_parameters->_isEditEnabled) {
            eventCategory = _currentInstance->_parameters->_editedCategory;
            threshold = _currentInstance->_editThreshold;
            noiseOffset += _currentInstance->_editNoiseOffset;
            // This works supposing offset is the world position of the object that is fading.
            baseOffset = _currentInstance->_editBaseOffset - offset;
        }

        threshold = (threshold-0.5f)*_currentInstance->_parameters->_thresholdScale[eventCategory] + 0.5f;

        batch._glUniform1i(fadeCategoryLocation, eventCategory);
        batch._glUniform1f(fadeThresholdLocation, threshold);
        // This is really temporary
        batch._glUniform3f(fadeNoiseOffsetLocation, noiseOffset.x, noiseOffset.y, noiseOffset.z);
        // This is really temporary
        batch._glUniform3f(fadeBaseOffsetLocation, baseOffset.x, baseOffset.y, baseOffset.z);

        return threshold > 0.f;
    }
    return false;
}
