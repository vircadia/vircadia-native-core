//
//  FadeEffectJobs.h

//  Created by Olivier Prat on 07/07/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_FadeEffectJobs_h
#define hifi_render_utils_FadeEffectJobs_h

#include "FadeEffect.h"

#include <gpu/Pipeline.h>
#include <render/ShapePipeline.h>
#include <render/RenderFetchCullSortTask.h>
#include <render/Transition.h>

enum FadeCategory {
    FADE_ELEMENT_ENTER_LEAVE_DOMAIN = 0,
    FADE_BUBBLE_ISECT_OWNER,
    FADE_BUBBLE_ISECT_TRESPASSER,
    FADE_USER_ENTER_LEAVE_DOMAIN,
    FADE_AVATAR_CHANGE,

    // Don't forget to modify Fade.slh to reflect the change in number of categories
    FADE_CATEGORY_COUNT,
};

class FadeEditConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(bool editFade MEMBER editFade NOTIFY dirty)

public:

    bool editFade{ false };

signals:

    void dirty();
};

class FadeConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(int editedCategory MEMBER editedCategory WRITE setEditedCategory NOTIFY dirtyCategory)
        Q_PROPERTY(float duration READ getDuration WRITE setDuration NOTIFY dirty)
        Q_PROPERTY(float baseSizeX READ getBaseSizeX WRITE setBaseSizeX NOTIFY dirty)
        Q_PROPERTY(float baseSizeY READ getBaseSizeY WRITE setBaseSizeY NOTIFY dirty)
        Q_PROPERTY(float baseSizeZ READ getBaseSizeZ WRITE setBaseSizeZ NOTIFY dirty)
        Q_PROPERTY(float baseLevel READ getBaseLevel WRITE setBaseLevel NOTIFY dirty)
        Q_PROPERTY(bool isInverted READ isInverted WRITE setInverted NOTIFY dirty)
        Q_PROPERTY(float noiseSizeX READ getNoiseSizeX WRITE setNoiseSizeX NOTIFY dirty)
        Q_PROPERTY(float noiseSizeY READ getNoiseSizeY WRITE setNoiseSizeY NOTIFY dirty)
        Q_PROPERTY(float noiseSizeZ READ getNoiseSizeZ WRITE setNoiseSizeZ NOTIFY dirty)
        Q_PROPERTY(float noiseLevel READ getNoiseLevel WRITE setNoiseLevel NOTIFY dirty)
        Q_PROPERTY(float edgeWidth READ getEdgeWidth WRITE setEdgeWidth NOTIFY dirty)
        Q_PROPERTY(QColor edgeInnerColor READ getEdgeInnerColor WRITE setEdgeInnerColor NOTIFY dirty)
        Q_PROPERTY(float edgeInnerIntensity READ getEdgeInnerIntensity WRITE setEdgeInnerIntensity NOTIFY dirty)
        Q_PROPERTY(QColor edgeOuterColor READ getEdgeOuterColor WRITE setEdgeOuterColor NOTIFY dirty)
        Q_PROPERTY(float edgeOuterIntensity READ getEdgeOuterIntensity WRITE setEdgeOuterIntensity NOTIFY dirty)
        Q_PROPERTY(int timing READ getTiming WRITE setTiming NOTIFY dirty)
        Q_PROPERTY(float noiseSpeedX READ getNoiseSpeedX WRITE setNoiseSpeedX NOTIFY dirty)
        Q_PROPERTY(float noiseSpeedY READ getNoiseSpeedY WRITE setNoiseSpeedY NOTIFY dirty)
        Q_PROPERTY(float noiseSpeedZ READ getNoiseSpeedZ WRITE setNoiseSpeedZ NOTIFY dirty)
        Q_PROPERTY(float threshold MEMBER threshold NOTIFY dirty)
        Q_PROPERTY(bool manualFade MEMBER manualFade NOTIFY dirty)
        Q_PROPERTY(float manualThreshold MEMBER manualThreshold NOTIFY dirty)

public:

    enum Timing {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT,

        TIMING_COUNT
    };

    FadeConfig();

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

    void setEdgeInnerColor(const QColor& value);
    QColor getEdgeInnerColor() const;

    void setEdgeInnerIntensity(float value);
    float getEdgeInnerIntensity() const { return events[editedCategory].edgeInnerColor.a; }

    void setEdgeOuterColor(const QColor& value);
    QColor getEdgeOuterColor() const;

    void setEdgeOuterIntensity(float value);
    float getEdgeOuterIntensity() const { return events[editedCategory].edgeOuterColor.a; }

    void setTiming(int value);
    int getTiming() const { return events[editedCategory].timing; }

    struct Event {
        glm::vec4 edgeInnerColor;
        glm::vec4 edgeOuterColor;
        glm::vec3 noiseSize;
        glm::vec3 noiseSpeed;
        glm::vec3 baseSize;
        float noiseLevel;
        float baseLevel;
        float duration;
        float edgeWidth;
        int timing;
        bool isInverted;
    };

    Event events[FADE_CATEGORY_COUNT];
    int editedCategory{ FADE_ELEMENT_ENTER_LEAVE_DOMAIN };
    float threshold{ 0.f };
    float manualThreshold{ 0.f };
    bool manualFade{ false };

    Q_INVOKABLE void save(const QString& filePath) const;
    Q_INVOKABLE void load(const QString& filePath);

    static QString eventNames[FADE_CATEGORY_COUNT];

signals:

    void dirty();
    void dirtyCategory();

};

class FadeEditJob {

public:

    using Config = FadeEditConfig;
    using Input = render::VaryingSet2<render::ItemBounds, FadeCategory>;
    using JobModel = render::Job::ModelI<FadeEditJob, Input, Config>;

    FadeEditJob() {}

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const FadeEditJob::Input& inputs);

private:

    bool _isEditEnabled{ false };
    render::ItemID _editedItem{ render::Item::INVALID_ITEM_ID };

};

class FadeJob {

public:

    static const FadeCategory transitionToCategory[render::Transition::TYPE_COUNT];

    using Config = FadeConfig;
    using Output = FadeCategory;
    using JobModel = render::Job::ModelO<FadeJob, Output, Config>;

    FadeJob();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, FadeJob::Output& output);

    gpu::BufferView getConfigurationBuffer() const { return _configurations; }

private:

#include "Fade_shared.slh"

    struct FadeConfiguration
    {
        FadeParameters  parameters[FADE_CATEGORY_COUNT];
    };
    using FadeConfigurationBuffer = gpu::StructBuffer<FadeConfiguration>;

    FadeConfigurationBuffer _configurations;
    float _thresholdScale[FADE_CATEGORY_COUNT];
    uint64_t _previousTime{ 0 };

    bool update(RenderArgs* args, const Config& config, const render::ScenePointer& scene, render::Transaction& transaction, render::Transition& transition, const double deltaTime) const;
    static float computeElementEnterRatio(double time, const double period, FadeConfig::Timing timing);

};

#endif // hifi_render_utils_FadeEffectJobs_h
