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

#include "LightStage.h"

class RenderArgs;
struct LightLocations;
using LightLocationsPtr = std::shared_ptr<LightLocations>;
/// Handles deferred lighting for the bits that require it (voxels...)
class DeferredLightingEffect : public Dependency {
    SINGLETON_DEPENDENCY
    
public:
    void init();
    
    /// Adds a point light to render for the current frame.
    void addPointLight(const glm::vec3& position, float radius, const glm::vec3& color = glm::vec3(0.0f, 0.0f, 0.0f),
        float intensity = 0.5f, float falloffRadius = 0.01f);
        
    /// Adds a spot light to render for the current frame.
    void addSpotLight(const glm::vec3& position, float radius, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f),
        float intensity = 0.5f, float falloffRadius = 0.01f,
        const glm::quat& orientation = glm::quat(), float exponent = 0.0f, float cutoff = PI);
    
    void prepare(RenderArgs* args);
    void render(const render::RenderContextPointer& renderContext);

    void setupKeyLightBatch(gpu::Batch& batch, int lightBufferUnit, int skyboxCubemapUnit);

    // update global lighting
    void setGlobalLight(const model::LightPointer& light);

    const LightStage& getLightStage() { return _lightStage; }
    void setShadowMapEnabled(bool enable) { _shadowMapEnabled = enable; };
    void setAmbientOcclusionEnabled(bool enable) { _ambientOcclusionEnabled = enable; }
    bool isAmbientOcclusionEnabled() const { return _ambientOcclusionEnabled; }

private:
    DeferredLightingEffect() = default;

    LightStage _lightStage;

    bool _shadowMapEnabled{ false };
    bool _ambientOcclusionEnabled{ false };

    model::MeshPointer _spotLightMesh;
    model::MeshPointer getSpotLightMesh();

    gpu::PipelinePointer _directionalSkyboxLight;
    gpu::PipelinePointer _directionalAmbientSphereLight;
    gpu::PipelinePointer _directionalLight;

    gpu::PipelinePointer _directionalSkyboxLightShadow;
    gpu::PipelinePointer _directionalAmbientSphereLightShadow;
    gpu::PipelinePointer _directionalLightShadow;

    gpu::PipelinePointer _pointLight;
    gpu::PipelinePointer _spotLight;

    LightLocationsPtr _directionalSkyboxLightLocations;
    LightLocationsPtr _directionalAmbientSphereLightLocations;
    LightLocationsPtr _directionalLightLocations;

    LightLocationsPtr _directionalSkyboxLightShadowLocations;
    LightLocationsPtr _directionalAmbientSphereLightShadowLocations;
    LightLocationsPtr _directionalLightShadowLocations;

    LightLocationsPtr _pointLightLocations;
    LightLocationsPtr _spotLightLocations;

    using Lights = std::vector<model::LightPointer>;

    Lights _allocatedLights;
    std::vector<int> _globalLights;
    std::vector<int> _pointLights;
    std::vector<int> _spotLights;

    // Class describing the uniform buffer with all the parameters common to the deferred shaders
    class DeferredTransform {
    public:
        glm::mat4 projection;
        glm::mat4 viewInverse;
        float stereoSide { 0.f };
        float spareA, spareB, spareC;

        DeferredTransform() {}
    };
    typedef gpu::BufferView UniformBufferView;
    UniformBufferView _deferredTransformBuffer[2];
};

#endif // hifi_DeferredLightingEffect_h
