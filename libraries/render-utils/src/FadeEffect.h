//
//  FadeEffect.h
//  libraries/render-utils/src/
//
//  Created by Olivier Prat on 06/06/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_FadeEffect_h
#define hifi_FadeEffect_h

#include <gpu/Pipeline.h>
#include <render/ShapePipeline.h>
#include <render/RenderFetchCullSortTask.h>

#include "LightingModel.h"

class FadeSwitchJobConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(bool editFade MEMBER editFade NOTIFY dirty)

public:

    bool editFade{ false };

signals:
    void dirty();

};

class FadeJobConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(int editedCategory MEMBER editedCategory WRITE setEditedCategory NOTIFY dirtyCategory)
        Q_PROPERTY(float duration READ getDuration WRITE setDuration NOTIFY dirty)
        Q_PROPERTY(float baseSizeX READ getBaseSizeX WRITE setBaseSizeX NOTIFY dirty)
        Q_PROPERTY(float baseSizeY READ getBaseSizeY WRITE setBaseSizeY NOTIFY dirty)
        Q_PROPERTY(float baseSizeZ READ getBaseSizeZ WRITE setBaseSizeZ NOTIFY dirty)
        Q_PROPERTY(float baseLevel READ getBaseLevel WRITE setBaseLevel NOTIFY dirty)
        Q_PROPERTY(bool _isInverted READ isInverted WRITE setInverted NOTIFY dirty)
        Q_PROPERTY(float noiseSizeX READ getNoiseSizeX WRITE setNoiseSizeX NOTIFY dirty)
        Q_PROPERTY(float noiseSizeY READ getNoiseSizeY WRITE setNoiseSizeY NOTIFY dirty)
        Q_PROPERTY(float noiseSizeZ READ getNoiseSizeZ WRITE setNoiseSizeZ NOTIFY dirty)
        Q_PROPERTY(float noiseLevel READ getNoiseLevel WRITE setNoiseLevel NOTIFY dirty)
        Q_PROPERTY(float edgeWidth READ getEdgeWidth WRITE setEdgeWidth NOTIFY dirty)
        Q_PROPERTY(float edgeInnerColorR READ getEdgeInnerColorR WRITE setEdgeInnerColorR NOTIFY dirty)
        Q_PROPERTY(float edgeInnerColorG READ getEdgeInnerColorG WRITE setEdgeInnerColorG NOTIFY dirty)
        Q_PROPERTY(float edgeInnerColorB READ getEdgeInnerColorB WRITE setEdgeInnerColorB NOTIFY dirty)
        Q_PROPERTY(float edgeInnerIntensity READ getEdgeInnerIntensity WRITE setEdgeInnerIntensity NOTIFY dirty)
        Q_PROPERTY(float edgeOuterColorR READ getEdgeOuterColorR WRITE setEdgeOuterColorR NOTIFY dirty)
        Q_PROPERTY(float edgeOuterColorG READ getEdgeOuterColorG WRITE setEdgeOuterColorG NOTIFY dirty)
        Q_PROPERTY(float edgeOuterColorB READ getEdgeOuterColorB WRITE setEdgeOuterColorB NOTIFY dirty)
        Q_PROPERTY(float edgeOuterIntensity READ getEdgeOuterIntensity WRITE setEdgeOuterIntensity NOTIFY dirty)
        Q_PROPERTY(bool manualFade MEMBER manualFade NOTIFY dirty)
        Q_PROPERTY(float manualThreshold MEMBER manualThreshold NOTIFY dirty)
        Q_PROPERTY(int timing READ getTiming WRITE setTiming NOTIFY dirty)
        Q_PROPERTY(float noiseSpeedX READ getNoiseSpeedX WRITE setNoiseSpeedX NOTIFY dirty)
        Q_PROPERTY(float noiseSpeedY READ getNoiseSpeedY WRITE setNoiseSpeedY NOTIFY dirty)
        Q_PROPERTY(float noiseSpeedZ READ getNoiseSpeedZ WRITE setNoiseSpeedZ NOTIFY dirty)

public:

    enum EventCategory {
        ELEMENT_ENTER_LEAVE_DOMAIN = 0,
        BUBBLE_ISECT_OWNER,
        BUBBLE_ISECT_TRESPASSER,
        USER_ENTER_LEAVE_DOMAIN,
        AVATAR_CHANGE,

        // Don't forget to modify Fade.slh to reflect the change in number of categories
        EVENT_CATEGORY_COUNT
    };

    enum Timing {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT,

        TIMING_COUNT
    };

    FadeJobConfig();

    void setEditedCategory(int value);

    void setDuration(float value);
    float getDuration() const;

    void setBaseSizeX(float value);
    float getBaseSizeX() const;

    void setBaseSizeY(float value);
    float getBaseSizeY() const;

    void setBaseSizeZ(float value);
    float getBaseSizeZ() const;

    void setBaseLevel(float value);
    float getBaseLevel() const { return events[editedCategory].baseLevel; }

    void setInverted(bool value);
    bool isInverted() const;

    void setNoiseSizeX(float value);
    float getNoiseSizeX() const;

    void setNoiseSizeY(float value);
    float getNoiseSizeY() const;

    void setNoiseSizeZ(float value);
    float getNoiseSizeZ() const;

    void setNoiseLevel(float value);
    float getNoiseLevel() const { return events[editedCategory].noiseLevel; }

    void setNoiseSpeedX(float value);
    float getNoiseSpeedX() const;

    void setNoiseSpeedY(float value);
    float getNoiseSpeedY() const;

    void setNoiseSpeedZ(float value);
    float getNoiseSpeedZ() const;

    void setEdgeWidth(float value);
    float getEdgeWidth() const;

    void setEdgeInnerColorR(float value);
    float getEdgeInnerColorR() const { return events[editedCategory].edgeInnerColor.r; }

    void setEdgeInnerColorG(float value);
    float getEdgeInnerColorG() const { return events[editedCategory].edgeInnerColor.g; }

    void setEdgeInnerColorB(float value);
    float getEdgeInnerColorB() const { return events[editedCategory].edgeInnerColor.b; }

    void setEdgeInnerIntensity(float value);
    float getEdgeInnerIntensity() const { return events[editedCategory].edgeInnerColor.a; }

    void setEdgeOuterColorR(float value);
    float getEdgeOuterColorR() const { return events[editedCategory].edgeOuterColor.r; }

    void setEdgeOuterColorG(float value);
    float getEdgeOuterColorG() const { return events[editedCategory].edgeOuterColor.g; }

    void setEdgeOuterColorB(float value);
    float getEdgeOuterColorB() const { return events[editedCategory].edgeOuterColor.b; }

    void setEdgeOuterIntensity(float value);
    float getEdgeOuterIntensity() const { return events[editedCategory].edgeOuterColor.a; }

    void setTiming(int value);
    int getTiming() const { return events[editedCategory].timing; }
    
    bool manualFade{ false };
    float manualThreshold{ 0.f };
    int editedCategory{ ELEMENT_ENTER_LEAVE_DOMAIN };

    struct Event {
        glm::vec4 edgeInnerColor;
        glm::vec4 edgeOuterColor;
        glm::vec3 noiseSize;
        glm::vec3 noiseSpeed;
        glm::vec3 baseSize;
        float noiseLevel;
        float baseLevel;
        float _duration;
        float edgeWidth;
        int timing;
        bool _isInverted;
    };

    Event events[EVENT_CATEGORY_COUNT];

    Q_INVOKABLE void save() const;
    Q_INVOKABLE void load();

    static QString eventNames[EVENT_CATEGORY_COUNT];

signals:
    void dirty();
    void dirtyCategory();

};

struct FadeCommonParameters
{
    using Pointer = std::shared_ptr<FadeCommonParameters>;

    FadeCommonParameters();

    bool _isEditEnabled{ false };
    bool _isManualThresholdEnabled{ false };
    float _manualThreshold{ 0.f };
    float _thresholdScale[FadeJobConfig::EVENT_CATEGORY_COUNT];
    int _editedCategory{ FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN };
    float _durations[FadeJobConfig::EVENT_CATEGORY_COUNT];
    glm::vec3 _noiseSpeed[FadeJobConfig::EVENT_CATEGORY_COUNT];
    FadeJobConfig::Timing _timing[FadeJobConfig::EVENT_CATEGORY_COUNT];
};

class FadeSwitchJob {
public:

    enum FadeBuckets {
        OPAQUE_SHAPE = 0,
        TRANSPARENT_SHAPE,

        NUM_BUCKETS
    };

    using FadeOutput = render::VaryingArray<render::ItemBounds, FadeBuckets::NUM_BUCKETS>;

    using Input = RenderFetchCullSortTask::Output;
    using Output = render::VaryingSet3 < RenderFetchCullSortTask::Output, FadeOutput, render::Item::Bound > ;
    using Config = FadeSwitchJobConfig;
    using JobModel = render::Job::ModelIO<FadeSwitchJob, Input, Output, Config>;

    FadeSwitchJob(FadeCommonParameters::Pointer commonParams) : _parameters{ commonParams } {}

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Input& input, Output& output);

private:

    FadeCommonParameters::Pointer _parameters;

    void distribute(const render::RenderContextPointer& renderContext, const render::Varying& input, 
        render::Varying& normalOutput, render::Varying& fadeOutput, const render::Item* editedItem = nullptr) const;
    const render::Item* findNearestItem(const render::RenderContextPointer& renderContext, const render::Varying& input, float& minIsectDistance) const;
};

struct FadeParameters
{
    glm::vec4       _baseInvSizeAndLevel;
    glm::vec4       _noiseInvSizeAndLevel;
    glm::vec4       _innerEdgeColor;
    glm::vec4       _outerEdgeColor;
    glm::vec2       _edgeWidthInvWidth;
    glm::int32      _isInverted;
    glm::float32    _padding;
};

struct FadeConfiguration
{
    FadeParameters  parameters[FadeJobConfig::EVENT_CATEGORY_COUNT];
};

class FadeConfigureJob {

public:

    using UniformBuffer = gpu::StructBuffer<FadeConfiguration>;
    using Input = render::Item::Bound ;
    using Output = render::VaryingSet2<gpu::TexturePointer, UniformBuffer>;
    using Config = FadeJobConfig;
    using JobModel = render::Job::ModelIO<FadeConfigureJob, Input, Output, Config>;

    FadeConfigureJob(FadeCommonParameters::Pointer commonParams);

    const gpu::TexturePointer getFadeMaskMap() const { return _fadeMaskMap; }

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Input& input, Output& output);

private:

    FadeCommonParameters::Pointer _parameters;
    bool _isBufferDirty{ true };
    gpu::TexturePointer _fadeMaskMap;
    FadeParameters _configurations[FadeJobConfig::EVENT_CATEGORY_COUNT];
};


class FadeRenderJobConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(float threshold MEMBER threshold NOTIFY dirty)

public:

    float threshold{ 0.f };

signals:
    void dirty();

};

class FadeRenderJob {

public:

    using Config = FadeRenderJobConfig;
    using Input = render::VaryingSet3<render::ItemBounds, LightingModelPointer, FadeConfigureJob::Output>;
    using JobModel = render::Job::ModelI<FadeRenderJob, Input, Config>;

    FadeRenderJob(FadeCommonParameters::Pointer commonParams, render::ShapePlumberPointer shapePlumber) : _shapePlumber{ shapePlumber }, _parameters{ commonParams }  {}

    void configure(const Config& config) {}
    void run(const render::RenderContextPointer& renderContext, const Input& inputs);

    static void bindPerBatch(gpu::Batch& batch, int fadeMaskMapLocation, int fadeBufferLocation);
    static void bindPerBatch(gpu::Batch& batch, gpu::TexturePointer texture, int fadeMaskMapLocation, const gpu::BufferView* buffer, int fadeBufferLocation);
    static void bindPerBatch(gpu::Batch& batch, gpu::TexturePointer texture, const gpu::BufferView* buffer, const gpu::PipelinePointer& pipeline);

    static bool bindPerItem(gpu::Batch& batch, RenderArgs* args, glm::vec3 offset, quint64 startTime);
    static bool bindPerItem(gpu::Batch& batch, const gpu::Pipeline* pipeline, glm::vec3 offset, quint64 startTime);

    static float computeFadePercent(quint64 startTime);

private:

    static const FadeRenderJob* _currentInstance;
    static gpu::TexturePointer _currentFadeMaskMap;
    static const gpu::BufferView* _currentFadeBuffer;

    render::ShapePlumberPointer _shapePlumber;
    FadeCommonParameters::Pointer _parameters;

    float computeElementEnterThreshold(double time, const double period, FadeJobConfig::Timing timing) const;

    // Everything needed for interactive edition
    uint64_t _editPreviousTime{ 0 };
    double _editTime{ 0.0 };
    float _editThreshold{ 0.f };
    glm::vec3 _editNoiseOffset{ 0.f, 0.f, 0.f };
    glm::vec3 _editBaseOffset{ 0.f, 0.f, 0.f };

    void updateFadeEdit(const render::RenderContextPointer& renderContext, const render::ItemBound& itemBounds);
};

#endif // hifi_FadeEffect_h
