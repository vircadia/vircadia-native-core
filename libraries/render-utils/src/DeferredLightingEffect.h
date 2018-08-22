//
//  DeferredLightingEffect.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 9/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DeferredLightingEffect_h
#define hifi_DeferredLightingEffect_h

#include <QVector>

#include <DependencyManager.h>
#include <NumericalConstants.h>

#include "graphics/Light.h"
#include "graphics/Geometry.h"

#include <procedural/ProceduralSkybox.h>

#include <render/CullTask.h>

#include "DeferredFrameTransform.h"
#include "DeferredFramebuffer.h"
#include "LightingModel.h"

#include "LightStage.h"
#include "LightClusters.h"
#include "BackgroundStage.h"
#include "HazeStage.h"

#include "SurfaceGeometryPass.h"
#include "SubsurfaceScattering.h"
#include "AmbientOcclusionEffect.h"


struct LightLocations;
using LightLocationsPtr = std::shared_ptr<LightLocations>;

// THis is where we currently accumulate the local lights, let s change that sooner than later
class DeferredLightingEffect : public Dependency {
    SINGLETON_DEPENDENCY
    
public:
    void init();
 
    static void setupKeyLightBatch(const RenderArgs* args, gpu::Batch& batch);
    static void unsetKeyLightBatch(gpu::Batch& batch);

    static void setupLocalLightsBatch(gpu::Batch& batch, const LightClustersPointer& lightClusters);
    static void unsetLocalLightsBatch(gpu::Batch& batch);

    void setShadowMapEnabled(bool enable) { _shadowMapEnabled = enable; };
    void setAmbientOcclusionEnabled(bool enable) { _ambientOcclusionEnabled = enable; }
    bool isAmbientOcclusionEnabled() const { return _ambientOcclusionEnabled; }

private:
    DeferredLightingEffect() = default;

    bool _shadowMapEnabled{ true };  // note that this value is overwritten in the ::configure method
    bool _ambientOcclusionEnabled{ false };

    graphics::MeshPointer _pointLightMesh;
    graphics::MeshPointer getPointLightMesh();
    graphics::MeshPointer _spotLightMesh;
    graphics::MeshPointer getSpotLightMesh();

    gpu::PipelinePointer _directionalSkyboxLight;
    gpu::PipelinePointer _directionalAmbientSphereLight;

    gpu::PipelinePointer _directionalSkyboxLightShadow;
    gpu::PipelinePointer _directionalAmbientSphereLightShadow;

    gpu::PipelinePointer _localLight;
    gpu::PipelinePointer _localLightOutline;

    LightLocationsPtr _directionalSkyboxLightLocations;
    LightLocationsPtr _directionalAmbientSphereLightLocations;

    LightLocationsPtr _directionalSkyboxLightShadowLocations;
    LightLocationsPtr _directionalAmbientSphereLightShadowLocations;

    LightLocationsPtr _localLightLocations;
    LightLocationsPtr _localLightOutlineLocations;

    friend class LightClusteringPass;
    friend class RenderDeferredSetup;
    friend class RenderDeferredLocals;
    friend class RenderDeferredCleanup;
};

class PreparePrimaryFramebufferConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(float resolutionScale MEMBER resolutionScale NOTIFY dirty)
public:

    float resolutionScale{ 1.0f };

signals:
    void dirty();
};

class PreparePrimaryFramebuffer {
public:

    using Output = gpu::FramebufferPointer;
    using Config = PreparePrimaryFramebufferConfig;
    using JobModel = render::Job::ModelO<PreparePrimaryFramebuffer, Output, Config>;

    PreparePrimaryFramebuffer(float resolutionScale = 1.0f) : _resolutionScale{resolutionScale} {}
    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, Output& primaryFramebuffer);

    gpu::FramebufferPointer _primaryFramebuffer;
    float _resolutionScale{ 1.0f };

private:

    static gpu::FramebufferPointer createFramebuffer(const char* name, const glm::uvec2& size);
};

class PrepareDeferred {
public:
    // Inputs: primaryFramebuffer and lightingModel
    using Inputs = render::VaryingSet2 <gpu::FramebufferPointer, LightingModelPointer>;
    // Output: DeferredFramebuffer, LightingFramebuffer
    using Outputs = render::VaryingSet2<DeferredFramebufferPointer, gpu::FramebufferPointer>;

    using JobModel = render::Job::ModelIO<PrepareDeferred, Inputs, Outputs>;

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

    DeferredFramebufferPointer _deferredFramebuffer;
};

class RenderDeferredSetup {
public:
  //  using JobModel = render::Job::ModelI<RenderDeferredSetup, DeferredFrameTransformPointer>;
    
    void run(const render::RenderContextPointer& renderContext,
        const DeferredFrameTransformPointer& frameTransform,
        const DeferredFramebufferPointer& deferredFramebuffer,
        const LightingModelPointer& lightingModel,
        const graphics::HazePointer& haze,
        const SurfaceGeometryFramebufferPointer& surfaceGeometryFramebuffer,
        const AmbientOcclusionFramebufferPointer& ambientOcclusionFramebuffer,
        const SubsurfaceScatteringResourcePointer& subsurfaceScatteringResource,
        bool renderShadows);
};

class RenderDeferredLocals {
public:
    using JobModel = render::Job::ModelI<RenderDeferredLocals, DeferredFrameTransformPointer>;
    
    void run(const render::RenderContextPointer& renderContext,
        const DeferredFrameTransformPointer& frameTransform,
        const DeferredFramebufferPointer& deferredFramebuffer,
        const LightingModelPointer& lightingModel,
        const SurfaceGeometryFramebufferPointer& surfaceGeometryFramebuffer,
        const LightClustersPointer& lightClusters);

    gpu::BufferView _localLightsBuffer;

    RenderDeferredLocals();

};


class RenderDeferredCleanup {
public:
    using JobModel = render::Job::Model<RenderDeferredCleanup>;
    
    void run(const render::RenderContextPointer& renderContext);
};

using RenderDeferredConfig = render::GPUJobConfig;

class RenderDeferred {
public:
    using Inputs = render::VaryingSet8 < 
        DeferredFrameTransformPointer, DeferredFramebufferPointer, LightingModelPointer, SurfaceGeometryFramebufferPointer, 
        AmbientOcclusionFramebufferPointer, SubsurfaceScatteringResourcePointer, LightClustersPointer, graphics::HazePointer>;

    using Config = RenderDeferredConfig;
    using JobModel = render::Job::ModelI<RenderDeferred, Inputs, Config>;

    RenderDeferred() {}
    RenderDeferred(bool renderShadows) : _renderShadows(renderShadows) {}

    void configure(const Config& config);

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);
    
    RenderDeferredSetup setupJob;
    RenderDeferredLocals lightsJob;
    RenderDeferredCleanup cleanupJob;

protected:
    gpu::RangeTimerPointer _gpuTimer;

private:
    bool _renderShadows { false };
};

class DefaultLightingSetup {
public:
    using JobModel = render::Job::Model<DefaultLightingSetup>;

    void run(const render::RenderContextPointer& renderContext);

protected:
    graphics::LightPointer _defaultLight;
    LightStage::Index _defaultLightID{ LightStage::INVALID_INDEX };
    graphics::SunSkyStagePointer _defaultBackground;
    BackgroundStage::Index _defaultBackgroundID{ BackgroundStage::INVALID_INDEX };
    graphics::HazePointer _defaultHaze{ nullptr };
    HazeStage::Index _defaultHazeID{ HazeStage::INVALID_INDEX };
    graphics::SkyboxPointer _defaultSkybox { new ProceduralSkybox() };
    gpu::TexturePointer _defaultSkyboxTexture;
    gpu::TexturePointer _defaultSkyboxAmbientTexture;
};

#endif // hifi_DeferredLightingEffect_h
