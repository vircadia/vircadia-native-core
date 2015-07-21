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
#include "model/Stage.h"
#include "model/Geometry.h"

class AbstractViewStateInterface;
class RenderArgs;
class SimpleProgramKey;

/// Handles deferred lighting for the bits that require it (voxels...)
class DeferredLightingEffect : public Dependency {
    SINGLETON_DEPENDENCY
    
public:
    
    void init(AbstractViewStateInterface* viewState);

    /// Sets up the state necessary to render static untextured geometry with the simple program.
    void bindSimpleProgram(gpu::Batch& batch, bool textured = false, bool culled = true,
                           bool emmisive = false, bool depthBias = false);

    //// Renders a solid sphere with the simple program.
    void renderSolidSphere(gpu::Batch& batch, float radius, int slices, int stacks, const glm::vec4& color);

    //// Renders a wireframe sphere with the simple program.
    void renderWireSphere(gpu::Batch& batch, float radius, int slices, int stacks, const glm::vec4& color);
    
    //// Renders a solid cube with the simple program.
    void renderSolidCube(gpu::Batch& batch, float size, const glm::vec4& color);

    //// Renders a wireframe cube with the simple program.
    void renderWireCube(gpu::Batch& batch, float size, const glm::vec4& color);
    
    //// Renders a quad with the simple program.
    void renderQuad(gpu::Batch& batch, const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec4& color);

    //// Renders a line with the simple program.
    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec4& color1, const glm::vec4& color2);
    
    /// Adds a point light to render for the current frame.
    void addPointLight(const glm::vec3& position, float radius, const glm::vec3& color = glm::vec3(0.0f, 0.0f, 0.0f),
        float intensity = 0.5f);
        
    /// Adds a spot light to render for the current frame.
    void addSpotLight(const glm::vec3& position, float radius, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f),
        float intensity = 0.5f, const glm::quat& orientation = glm::quat(), float exponent = 0.0f, float cutoff = PI);
    
    void prepare(RenderArgs* args);
    void render(RenderArgs* args);
    void copyBack(RenderArgs* args);

    void setupTransparent(RenderArgs* args, int lightBufferUnit);

    // update global lighting
    void setAmbientLightMode(int preset);
    void setGlobalLight(const glm::vec3& direction, const glm::vec3& diffuse, float intensity, float ambientIntensity);
    void setGlobalAtmosphere(const model::AtmospherePointer& atmosphere) { _atmosphere = atmosphere; }

    void setGlobalSkybox(const model::SkyboxPointer& skybox);
private:
    DeferredLightingEffect() {}
    virtual ~DeferredLightingEffect() { }

    class LightLocations {
    public:
        int shadowDistances;
        int shadowScale;
        int nearLocation;
        int depthScale;
        int depthTexCoordOffset;
        int depthTexCoordScale;
        int radius;
        int ambientSphere;
        int lightBufferUnit;
        int atmosphereBufferUnit;
        int invViewMat;
        int texcoordMat;
        int coneParam;
    };

    model::MeshPointer _spotLightMesh;
    model::MeshPointer getSpotLightMesh();

    static void loadLightProgram(const char* vertSource, const char* fragSource, bool lightVolume, gpu::PipelinePointer& program, LightLocations& locations);
  
    gpu::PipelinePointer getPipeline(SimpleProgramKey config);
    
    gpu::ShaderPointer _simpleShader;
    gpu::ShaderPointer _emissiveShader;
    QHash<SimpleProgramKey, gpu::PipelinePointer> _simplePrograms;

    gpu::PipelinePointer _blitLightBuffer;

    gpu::PipelinePointer _directionalSkyboxLight;
    LightLocations _directionalSkyboxLightLocations;
    gpu::PipelinePointer _directionalSkyboxLightShadowMap;
    LightLocations _directionalSkyboxLightShadowMapLocations;
    gpu::PipelinePointer _directionalSkyboxLightCascadedShadowMap;
    LightLocations _directionalSkyboxLightCascadedShadowMapLocations;

    gpu::PipelinePointer _directionalAmbientSphereLight;
    LightLocations _directionalAmbientSphereLightLocations;
    gpu::PipelinePointer _directionalAmbientSphereLightShadowMap;
    LightLocations _directionalAmbientSphereLightShadowMapLocations;
    gpu::PipelinePointer _directionalAmbientSphereLightCascadedShadowMap;
    LightLocations _directionalAmbientSphereLightCascadedShadowMapLocations;

    gpu::PipelinePointer _directionalLight;
    LightLocations _directionalLightLocations;
    gpu::PipelinePointer _directionalLightShadowMap;
    LightLocations _directionalLightShadowMapLocations;
    gpu::PipelinePointer _directionalLightCascadedShadowMap;
    LightLocations _directionalLightCascadedShadowMapLocations;

    gpu::PipelinePointer _pointLight;
    LightLocations _pointLightLocations;
    gpu::PipelinePointer _spotLight;
    LightLocations _spotLightLocations;

    class PointLight {
    public:
        glm::vec4 position;
        float radius;
        glm::vec4 ambient;
        glm::vec4 diffuse;
        glm::vec4 specular;
        float constantAttenuation;
        float linearAttenuation;
        float quadraticAttenuation;
    };
    
    class SpotLight : public PointLight {
    public:
        glm::vec3 direction;
        float exponent;
        float cutoff;
    };

    typedef std::vector< model::LightPointer > Lights;

    Lights _allocatedLights;
    std::vector<int> _globalLights;
    std::vector<int> _pointLights;
    std::vector<int> _spotLights;
    
    AbstractViewStateInterface* _viewState;

    int _ambientLightMode = 0;
    model::AtmospherePointer _atmosphere;
    model::SkyboxPointer _skybox;
};

class SimpleProgramKey {
public:
    enum FlagBit {
        IS_TEXTURED_FLAG = 0,
        IS_CULLED_FLAG,
        IS_EMISSIVE_FLAG,
        HAS_DEPTH_BIAS_FLAG,
        
        NUM_FLAGS,
    };
    
    enum Flag {
        IS_TEXTURED = (1 << IS_TEXTURED_FLAG),
        IS_CULLED = (1 << IS_CULLED_FLAG),
        IS_EMISSIVE = (1 << IS_EMISSIVE_FLAG),
        HAS_DEPTH_BIAS = (1 << HAS_DEPTH_BIAS_FLAG),
    };
    typedef unsigned short Flags;
    
    bool isFlag(short flagNum) const { return bool((_flags & flagNum) != 0); }
    
    bool isTextured() const { return isFlag(IS_TEXTURED); }
    bool isCulled() const { return isFlag(IS_CULLED); }
    bool isEmissive() const { return isFlag(IS_EMISSIVE); }
    bool hasDepthBias() const { return isFlag(HAS_DEPTH_BIAS); }
    
    Flags _flags = 0;
    short _spare = 0;
    
    int getRaw() const { return *reinterpret_cast<const int*>(this); }
    
    
    SimpleProgramKey(bool textured = false, bool culled = true,
                     bool emissive = false, bool depthBias = false) {
        _flags = (textured ? IS_TEXTURED : 0) | (culled ? IS_CULLED : 0) |
        (emissive ? IS_EMISSIVE : 0) | (depthBias ? HAS_DEPTH_BIAS : 0);
    }
    
    SimpleProgramKey(int bitmask) : _flags(bitmask) {}
};

inline uint qHash(const SimpleProgramKey& key, uint seed) {
    return qHash(key.getRaw(), seed);
}

inline bool operator==(const SimpleProgramKey& a, const SimpleProgramKey& b) {
    return a.getRaw() == b.getRaw();
}

#endif // hifi_DeferredLightingEffect_h
