//
//  FadeEffect.h

//  Created by Olivier Prat on 07/07/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_FadeEffect_h
#define hifi_render_utils_FadeEffect_h

#include <gpu/Pipeline.h>
#include <render/ShapePipeline.h>
#include <render/RenderFetchCullSortTask.h>
#include <render/Transition.h>

class FadeConfig : public render::Job::Config {
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
        Q_PROPERTY(float threshold MEMBER threshold NOTIFY dirty)
        Q_PROPERTY(bool editFade MEMBER editFade NOTIFY dirty)

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

    Event events[render::Transition::EVENT_CATEGORY_COUNT];
    float threshold{ 0.f };
    float manualThreshold{ 0.f };
    int editedCategory{ render::Transition::ELEMENT_ENTER_LEAVE_DOMAIN };
    bool editFade{ false };
    bool manualFade{ false };

    Q_INVOKABLE void save() const;
    Q_INVOKABLE void load();

    static QString eventNames[render::Transition::EVENT_CATEGORY_COUNT];

signals:

    void dirty();
    void dirtyCategory();

};

class FadeJob {

public:

    using Config = FadeConfig;
    using JobModel = render::Job::Model<FadeJob, Config>;

    FadeJob();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext);

    render::ShapePipeline::BatchSetter getBatchSetter() const;
    render::ShapePipeline::ItemSetter getItemSetter() const;

private:

#include "Fade_shared.slh"

    struct FadeConfiguration
    {
        FadeParameters  parameters[render::Transition::EVENT_CATEGORY_COUNT];
    };

    gpu::StructBuffer<FadeConfiguration> _configurations;
    gpu::TexturePointer _fadeMaskMap;
    float _thresholdScale[render::Transition::EVENT_CATEGORY_COUNT];
    uint64_t _previousTime{ 0 };

    bool update(const Config& config, const render::ScenePointer& scene, render::Transition& transition, const double deltaTime) const;
    static float computeElementEnterRatio(double time, const double period, FadeConfig::Timing timing);

    const render::Item* findNearestItem(const render::RenderContextPointer& renderContext, const render::Varying& input, float& minIsectDistance) const;
};

#endif // hifi_render_utils_FadeEffect_h
