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

#include "model/Light.h"
#include "model/Geometry.h"

#include "render/Context.h"
#include <render/CullTask.h>

#include "DeferredFrameTransform.h"
#include "DeferredFramebuffer.h"
#include "LightingModel.h"

#include "LightStage.h"
#include "LightClusters.h"

#include "SurfaceGeometryPass.h"
#include "SubsurfaceScattering.h"
#include "AmbientOcclusionEffect.h"

class RenderArgs;
struct LightLocations;
using LightLocationsPtr = std::shared_ptr<LightLocations>;

// THis is where we currently accumulate the local lights, let s change that sooner than later
class DeferredLightingEffect : public Dependency {
    SINGLETON_DEPENDENCY
    
public:
    void init();
    
    void addLight(const model::LightPointer& light);

    /// Adds a point light to render for the current frame.
    void addPointLight(const glm::vec3& position, float radius, const glm::vec3& color = glm::vec3(0.0f, 0.0f, 0.0f),
        float intensity = 0.5f, float falloffRadius = 0.01f);
        
    /// Adds a spot light to render for the current frame.
    void addSpotLight(const glm::vec3& position, float radius, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f),
        float intensity = 0.5f, float falloffRadius = 0.01f,
        const glm::quat& orientation = glm::quat(), float exponent = 0.0f, float cutoff = PI);

    void setupKeyLightBatch(gpu::Batch& batch, int lightBufferUnit, int ambientBufferUnit, int skyboxCubemapUnit);
    void unsetKeyLightBatch(gpu::Batch& batch, int lightBufferUnit, int ambientBufferUnit, int skyboxCubemapUnit);

    // update global lighting
    void setGlobalLight(const model::LightPointer& light);

    const LightStagePointer getLightStage() { return _lightStage; }

    void setShadowMapEnabled(bool enable) { _shadowMapEnabled = enable; };
    void setAmbientOcclusionEnabled(bool enable) { _ambientOcclusionEnabled = enable; }
    bool isAmbientOcclusionEnabled() const { return _ambientOcclusionEnabled; }

private:
    DeferredLightingEffect() = default;

    LightStagePointer _lightStage;

    bool _shadowMapEnabled{ false };
    bool _ambientOcclusionEnabled{ false };

    model::MeshPointer _pointLightMesh;
    model::MeshPointer getPointLightMesh();
    model::MeshPointer _spotLightMesh;
    model::MeshPointer getSpotLightMesh();

    gpu::PipelinePointer _directionalSkyboxLight;
    gpu::PipelinePointer _directionalAmbientSphereLight;
    gpu::PipelinePointer _directionalLight;

    gpu::PipelinePointer _directionalSkyboxLightShadow;
    gpu::PipelinePointer _directionalAmbientSphereLightShadow;
    gpu::PipelinePointer _directionalLightShadow;

    gpu::PipelinePointer _localLight;
    gpu::PipelinePointer _localLightOutline;

    gpu::PipelinePointer _pointLightBack;
    gpu::PipelinePointer _pointLightFront;
    gpu::PipelinePointer _spotLightBack;
    gpu::PipelinePointer _spotLightFront;

    LightLocationsPtr _directionalSkyboxLightLocations;
    LightLocationsPtr _directionalAmbientSphereLightLocations;
    LightLocationsPtr _directionalLightLocations;

    LightLocationsPtr _directionalSkyboxLightShadowLocations;
    LightLocationsPtr _directionalAmbientSphereLightShadowLocations;
    LightLocationsPtr _directionalLightShadowLocations;

    LightLocationsPtr _localLightLocations;
    LightLocationsPtr _localLightOutlineLocations;
    LightLocationsPtr _pointLightLocations;
    LightLocationsPtr _spotLightLocations;

    using Lights = std::vector<model::LightPointer>;

    Lights _allocatedLights;
    std::vector<int> _globalLights;
    std::vector<int> _pointLights;
    std::vector<int> _spotLights;

    friend class LightClusteringPass;
    friend class RenderDeferredSetup;
    friend class RenderDeferredLocals;
    friend class RenderDeferredCleanup;
};

class PreparePrimaryFramebuffer {
public:
    using JobModel = render::Job::ModelO<PreparePrimaryFramebuffer, gpu::FramebufferPointer>;

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, gpu::FramebufferPointer& primaryFramebuffer);

    gpu::FramebufferPointer _primaryFramebuffer;
};

class PrepareDeferred {
public:
    // Inputs: primaryFramebuffer and lightingModel
    using Inputs = render::VaryingSet2 <gpu::FramebufferPointer, LightingModelPointer>;
    // Output: DeferredFramebuffer, LightingFramebuffer
    using Outputs = render::VaryingSet2<DeferredFramebufferPointer, gpu::FramebufferPointer>;

    using JobModel = render::Job::ModelIO<PrepareDeferred, Inputs, Outputs>;

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

    DeferredFramebufferPointer _deferredFramebuffer;
};

class RenderDeferredSetup {
public:
  //  using JobModel = render::Job::ModelI<RenderDeferredSetup, DeferredFrameTransformPointer>;
    
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
        const DeferredFrameTransformPointer& frameTransform,
        const DeferredFramebufferPointer& deferredFramebuffer,
        const LightingModelPointer& lightingModel,
        const SurfaceGeometryFramebufferPointer& surfaceGeometryFramebuffer,
        const AmbientOcclusionFramebufferPointer& ambientOcclusionFramebuffer,
        const SubsurfaceScatteringResourcePointer& subsurfaceScatteringResource);
};

class RenderDeferredLocals {
public:
    using JobModel = render::Job::ModelI<RenderDeferredLocals, DeferredFrameTransformPointer>;
    
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
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
    
    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext);
};

using RenderDeferredConfig = render::GPUJobConfig;

class RenderDeferred {
public:
    using Inputs = render::VaryingSet7 < DeferredFrameTransformPointer, DeferredFramebufferPointer, LightingModelPointer, SurfaceGeometryFramebufferPointer, AmbientOcclusionFramebufferPointer, SubsurfaceScatteringResourcePointer, LightClustersPointer>;
    using Config = RenderDeferredConfig;
    using JobModel = render::Job::ModelI<RenderDeferred, Inputs, Config>;

    RenderDeferred();

    void configure(const Config& config);

    void run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs);
    
    RenderDeferredSetup setupJob;
    RenderDeferredLocals lightsJob;
    RenderDeferredCleanup cleanupJob;

protected:
    gpu::RangeTimerPointer _gpuTimer;
};



#endif // hifi_DeferredLightingEffect_h
