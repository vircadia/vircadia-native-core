//
//  HazeStage.h

//  Created by Nissim Hadar on 9/26/2017.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_HazeStage_h
#define hifi_render_utils_HazeStage_h

#include <model/Stage.h>
#include <set>
#include <unordered_map>
#include <render/IndexedContainer.h>
#include <render/Stage.h>

#include <render/Forward.h>
#include <render/DrawTask.h>
#include <model/Haze.h>

// Haze stage to set up haze-related rendering tasks
class HazeStage : public render::Stage {
public:
    static std::string _stageName;
    static const std::string& getName() { return _stageName; }

    using Index = render::indexed_container::Index;
    static const Index INVALID_INDEX { render::indexed_container::INVALID_INDEX };
    static bool isIndexInvalid(Index index) { return index == INVALID_INDEX; }
    
    using HazePointer = model::HazePointer;
    using Hazes = render::indexed_container::IndexedPointerVector<model::Haze>;
    using HazeMap = std::unordered_map<HazePointer, Index>;

    using HazeIndices = std::vector<Index>;

    Index findHaze(const HazePointer& haze) const;
    Index addHaze(const HazePointer& haze);

    HazePointer removeHaze(Index index);
    
    bool checkHazeId(Index index) const { return _hazes.checkIndex(index); }

    Index getNumHazes() const { return _hazes.getNumElements(); }
    Index getNumFreeHazes() const { return _hazes.getNumFreeIndices(); }
    Index getNumAllocatedHazes() const { return _hazes.getNumAllocatedIndices(); }

    HazePointer getHaze(Index hazeId) const {
        return _hazes.get(hazeId);
    }

    Hazes _hazes;
    HazeMap _hazeMap;

    class Frame {
    public:
        Frame() {}
        
        void clear() { _hazes.clear(); }

        void pushHaze(HazeStage::Index index) { _hazes.emplace_back(index); }

        HazeStage::HazeIndices _hazes;
    };
    
    Frame _currentFrame;
};
using HazeStagePointer = std::shared_ptr<HazeStage>;

class HazeStageSetup {
public:
    using JobModel = render::Job::Model<HazeStageSetup>;

    HazeStageSetup();
    void run(const render::RenderContextPointer& renderContext);

protected:
};

class FetchHazeConfig : public render::Job::Config {
    Q_OBJECT

    Q_PROPERTY(float hazeColorR MEMBER hazeColorR WRITE setHazeColorR NOTIFY dirty);
    Q_PROPERTY(float hazeColorG MEMBER hazeColorG WRITE setHazeColorG NOTIFY dirty);
    Q_PROPERTY(float hazeColorB MEMBER hazeColorB WRITE setHazeColorB NOTIFY dirty);
    Q_PROPERTY(float hazeGlareAngle_degs MEMBER hazeGlareAngle_degs WRITE setHazeGlareAngle_degs NOTIFY dirty);

    Q_PROPERTY(float hazeGlareColorR MEMBER hazeGlareColorR WRITE setHazeGlareColorR NOTIFY dirty);
    Q_PROPERTY(float hazeGlareColorG MEMBER hazeGlareColorG WRITE setHazeGlareColorG NOTIFY dirty);
    Q_PROPERTY(float hazeGlareColorB MEMBER hazeGlareColorB WRITE setHazeGlareColorB NOTIFY dirty);
    Q_PROPERTY(float hazeBaseReference_m MEMBER hazeBaseReference_m WRITE setHazeBaseReference NOTIFY dirty);

    Q_PROPERTY(bool isHazeActive MEMBER isHazeActive WRITE setHazeActive NOTIFY dirty);
    Q_PROPERTY(bool isAltitudeBased MEMBER isAltitudeBased WRITE setAltitudeBased NOTIFY dirty);
    Q_PROPERTY(bool isHazeAttenuateKeyLight MEMBER isHazeAttenuateKeyLight WRITE setHazeAttenuateKeyLight NOTIFY dirty);
    Q_PROPERTY(bool isModulateColorActive MEMBER isModulateColorActive WRITE setModulateColorActive NOTIFY dirty);
    Q_PROPERTY(bool isHazeEnableGlare MEMBER isHazeEnableGlare WRITE setHazeEnableGlare NOTIFY dirty);

    Q_PROPERTY(float hazeRange_m MEMBER hazeRange_m WRITE setHazeRange_m NOTIFY dirty);
    Q_PROPERTY(float hazeHeight_m MEMBER hazeHeight_m WRITE setHazeAltitude_m NOTIFY dirty);

    Q_PROPERTY(float hazeKeyLightRange_m MEMBER hazeKeyLightRange_m WRITE setHazeKeyLightRange_m NOTIFY dirty);
    Q_PROPERTY(float hazeKeyLightAltitude_m MEMBER hazeKeyLightAltitude_m WRITE setHazeKeyLightAltitude_m NOTIFY dirty);

    Q_PROPERTY(float hazeBackgroundBlend MEMBER hazeBackgroundBlend WRITE setHazeBackgroundBlend NOTIFY dirty);

public:
    FetchHazeConfig() : render::Job::Config() {}

    float hazeColorR{ model::initialHazeColor.r };
    float hazeColorG{ model::initialHazeColor.g };
    float hazeColorB{ model::initialHazeColor.b };
    float hazeGlareAngle_degs{ model::initialGlareAngle_degs };

    float hazeGlareColorR{ model::initialHazeGlareColor.r };
    float hazeGlareColorG{ model::initialHazeGlareColor.g };
    float hazeGlareColorB{ model::initialHazeGlareColor.b };
    float hazeBaseReference_m{ model::initialHazeBaseReference_m };

    bool isHazeActive{ false };
    bool isAltitudeBased{ false };
    bool isHazeAttenuateKeyLight{ false };
    bool isModulateColorActive{ false };
    bool isHazeEnableGlare{ false };

    float hazeRange_m{ model::initialHazeRange_m };
    float hazeHeight_m{ model::initialHazeHeight_m };

    float hazeKeyLightRange_m{ model::initialHazeKeyLightRange_m };
    float hazeKeyLightAltitude_m{ model::initialHazeKeyLightAltitude_m };

    float hazeBackgroundBlend{ model::initialHazeBackgroundBlend };

public slots:
    void setHazeColorR(const float value) { hazeColorR = value; emit dirty(); }
    void setHazeColorG(const float value) { hazeColorG = value; emit dirty(); }
    void setHazeColorB(const float value) { hazeColorB = value; emit dirty(); }
    void setHazeGlareAngle_degs(const float value) { hazeGlareAngle_degs = value; emit dirty(); }

    void setHazeGlareColorR(const float value) { hazeGlareColorR = value; emit dirty(); }
    void setHazeGlareColorG(const float value) { hazeGlareColorG = value; emit dirty(); }
    void setHazeGlareColorB(const float value) { hazeGlareColorB = value; emit dirty(); }
    void setHazeBaseReference(const float value) { hazeBaseReference_m = value; ; emit dirty(); }

    void setHazeActive(const bool active) { isHazeActive = active; emit dirty(); }
    void setAltitudeBased(const bool active) { isAltitudeBased = active; emit dirty(); }
    void setHazeAttenuateKeyLight(const bool active) { isHazeAttenuateKeyLight = active; emit dirty(); }
    void setModulateColorActive(const bool active) { isModulateColorActive = active; emit dirty(); }
    void setHazeEnableGlare(const bool active) { isHazeEnableGlare = active; emit dirty(); }

    void setHazeRange_m(const float value) { hazeRange_m = value; emit dirty(); }
    void setHazeAltitude_m(const float value) { hazeHeight_m = value; emit dirty(); }

    void setHazeKeyLightRange_m(const float value) { hazeKeyLightRange_m = value; emit dirty(); }
    void setHazeKeyLightAltitude_m(const float value) { hazeKeyLightAltitude_m = value; emit dirty(); }

    void setHazeBackgroundBlend(const float value) { hazeBackgroundBlend = value; ; emit dirty(); }

signals:
    void dirty();
};

class FetchHazeStage {
public:
    using Config = FetchHazeConfig;
    using JobModel = render::Job::ModelO<FetchHazeStage, model::HazePointer, Config>;

    FetchHazeStage();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, model::HazePointer& haze);

private:
    model::HazePointer _haze;
    gpu::PipelinePointer _hazePipeline;
};
#endif
