//
//  DeferredLightingEffect.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 9/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DeferredLightingEffect.h"

#include <GLMHelpers.h>
#include <PathUtils.h>
#include <ViewFrustum.h>

#include <gpu/Batch.h>
#include <gpu/Context.h>

#include "AbstractViewStateInterface.h"
#include "GeometryCache.h"
#include "TextureCache.h"
#include "FramebufferCache.h"

#include "deferred_light_vert.h"
#include "deferred_light_point_vert.h"
#include "deferred_light_spot_vert.h"

#include "directional_light_frag.h"
#include "directional_ambient_light_frag.h"
#include "directional_skybox_light_frag.h"

#include "directional_light_shadow_frag.h"
#include "directional_ambient_light_shadow_frag.h"
#include "directional_skybox_light_shadow_frag.h"

#include "local_lights_shading_frag.h"
#include "point_light_frag.h"
#include "spot_light_frag.h"

using namespace render;

struct LightLocations {
    int radius{ -1 };
    int ambientSphere{ -1 };
    int lightBufferUnit{ -1 };
    int ambientBufferUnit { -1 };
    int lightIndexBufferUnit { -1 };
    int texcoordFrameTransform{ -1 };
    int deferredFrameTransformBuffer{ -1 };
    int subsurfaceScatteringParametersBuffer{ -1 };
    int shadowTransformBuffer{ -1 };
};

enum DeferredShader_MapSlot {
    DEFERRED_BUFFER_COLOR_UNIT = 0,
    DEFERRED_BUFFER_NORMAL_UNIT = 1,
    DEFERRED_BUFFER_EMISSIVE_UNIT = 2,
    DEFERRED_BUFFER_DEPTH_UNIT = 3,
    DEFERRED_BUFFER_OBSCURANCE_UNIT = 4,
    SHADOW_MAP_UNIT = 5,
    SKYBOX_MAP_UNIT = 6,
    DEFERRED_BUFFER_LINEAR_DEPTH_UNIT,
    DEFERRED_BUFFER_CURVATURE_UNIT,
    DEFERRED_BUFFER_DIFFUSED_CURVATURE_UNIT,
    SCATTERING_LUT_UNIT,
    SCATTERING_SPECULAR_UNIT,
};
enum DeferredShader_BufferSlot {
    DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT = 0,
    CAMERA_CORRECTION_BUFFER_SLOT,
    SCATTERING_PARAMETERS_BUFFER_SLOT,
    LIGHTING_MODEL_BUFFER_SLOT = render::ShapePipeline::Slot::LIGHTING_MODEL,
    LIGHT_GPU_SLOT = render::ShapePipeline::Slot::LIGHT,
    LIGHT_AMBIENT_SLOT,
    LIGHT_INDEX_GPU_SLOT,
    LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT,
    LIGHT_CLUSTER_GRID_CLUSTER_GRID_SLOT,
    LIGHT_CLUSTER_GRID_CLUSTER_CONTENT_SLOT,

};

static void loadLightProgram(const char* vertSource, const char* fragSource, bool lightVolume, gpu::PipelinePointer& program, LightLocationsPtr& locations);
static void loadLightVolumeProgram(const char* vertSource, const char* fragSource, bool front, gpu::PipelinePointer& program, LightLocationsPtr& locations);

const char no_light_frag[] =
R"SCRIBE(
out vec4 _fragColor;

void main(void) {
    _fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)SCRIBE"
;

void DeferredLightingEffect::init() {
    _directionalLightLocations = std::make_shared<LightLocations>();
    _directionalAmbientSphereLightLocations = std::make_shared<LightLocations>();
    _directionalSkyboxLightLocations = std::make_shared<LightLocations>();

    _directionalLightShadowLocations = std::make_shared<LightLocations>();
    _directionalAmbientSphereLightShadowLocations = std::make_shared<LightLocations>();
    _directionalSkyboxLightShadowLocations = std::make_shared<LightLocations>();

    _localLightLocations = std::make_shared<LightLocations>();
    _pointLightLocations = std::make_shared<LightLocations>();
    _spotLightLocations = std::make_shared<LightLocations>();

    loadLightProgram(deferred_light_vert, directional_light_frag, false, _directionalLight, _directionalLightLocations);
    loadLightProgram(deferred_light_vert, directional_ambient_light_frag, false, _directionalAmbientSphereLight, _directionalAmbientSphereLightLocations);
    loadLightProgram(deferred_light_vert, directional_skybox_light_frag, false, _directionalSkyboxLight, _directionalSkyboxLightLocations);

    loadLightProgram(deferred_light_vert, directional_light_shadow_frag, false, _directionalLightShadow, _directionalLightShadowLocations);
    loadLightProgram(deferred_light_vert, directional_ambient_light_shadow_frag, false, _directionalAmbientSphereLightShadow, _directionalAmbientSphereLightShadowLocations);
    loadLightProgram(deferred_light_vert, directional_skybox_light_shadow_frag, false, _directionalSkyboxLightShadow, _directionalSkyboxLightShadowLocations);

    loadLightProgram(deferred_light_vert, local_lights_shading_frag, true, _localLight, _localLightLocations);

    loadLightVolumeProgram(deferred_light_point_vert, no_light_frag, false, _pointLightBack, _pointLightLocations);
    loadLightVolumeProgram(deferred_light_point_vert, no_light_frag, true, _pointLightFront, _pointLightLocations);
    loadLightVolumeProgram(deferred_light_spot_vert, no_light_frag, false, _spotLightBack, _spotLightLocations);
    loadLightVolumeProgram(deferred_light_spot_vert, no_light_frag, true, _spotLightFront, _spotLightLocations);

    // Light Stage and clusters
    _lightStage = std::make_shared<LightStage>();
    
    // Allocate a global light representing the Global Directional light casting shadow (the sun) and the ambient light
    _allocatedLights.push_back(std::make_shared<model::Light>());
    model::LightPointer lp = _allocatedLights[0];
    lp->setType(model::Light::SUN);
    lp->setDirection(glm::vec3(-1.0f));
    lp->setColor(glm::vec3(1.0f));
    lp->setIntensity(1.0f);
    lp->setType(model::Light::SUN);
    lp->setAmbientSpherePreset(gpu::SphericalHarmonics::Preset::OLD_TOWN_SQUARE);

    // Add the global light to the light stage (for later shadow rendering)
    _globalLights.push_back(_lightStage->addLight(lp));

}

void DeferredLightingEffect::addLight(const model::LightPointer& light) {
    assert(light);
    auto lightID = _lightStage->addLight(light);
    if (light->getType() == model::Light::POINT) {
        _pointLights.push_back(lightID);
    } else {
        _spotLights.push_back(lightID);
    }
}


void DeferredLightingEffect::addPointLight(const glm::vec3& position, float radius, const glm::vec3& color,
        float intensity, float falloffRadius) {
    addSpotLight(position, radius, color, intensity, falloffRadius);
}

void DeferredLightingEffect::addSpotLight(const glm::vec3& position, float radius, const glm::vec3& color,
    float intensity, float falloffRadius, const glm::quat& orientation, float exponent, float cutoff) {
    
    unsigned int lightID = (unsigned int)(_pointLights.size() + _spotLights.size() + _globalLights.size());
    if (lightID >= _allocatedLights.size()) {
        _allocatedLights.push_back(std::make_shared<model::Light>());
    }
    model::LightPointer lp = _allocatedLights[lightID];

    lp->setPosition(position);
    lp->setMaximumRadius(radius);
    lp->setColor(color);
    lp->setIntensity(intensity);
    lp->setFalloffRadius(falloffRadius);

    if (exponent == 0.0f && cutoff == PI) {
        lp->setType(model::Light::POINT);
        _pointLights.push_back(lightID);
        
    } else {
        lp->setOrientation(orientation);
        lp->setSpotAngle(cutoff);
        lp->setSpotExponent(exponent);
        lp->setType(model::Light::SPOT);
        _spotLights.push_back(lightID);
    }
}

void DeferredLightingEffect::setupKeyLightBatch(gpu::Batch& batch, int lightBufferUnit, int ambientBufferUnit, int skyboxCubemapUnit) {
    PerformanceTimer perfTimer("DLE->setupBatch()");
    auto keyLight = _allocatedLights[_globalLights.front()];

    if (lightBufferUnit >= 0) {
        batch.setUniformBuffer(lightBufferUnit, keyLight->getLightSchemaBuffer());
    }
    if (keyLight->hasAmbient() && (ambientBufferUnit >= 0)) {
        batch.setUniformBuffer(ambientBufferUnit, keyLight->getAmbientSchemaBuffer());
    }

    if (keyLight->getAmbientMap() && (skyboxCubemapUnit >= 0)) {
        batch.setResourceTexture(skyboxCubemapUnit, keyLight->getAmbientMap());
    }
}
static gpu::ShaderPointer makeLightProgram(const char* vertSource, const char* fragSource, LightLocationsPtr& locations) {
    auto VS = gpu::Shader::createVertex(std::string(vertSource));
    auto PS = gpu::Shader::createPixel(std::string(fragSource));
    
    gpu::ShaderPointer program = gpu::Shader::createProgram(VS, PS);

    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding(std::string("colorMap"), DEFERRED_BUFFER_COLOR_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), DEFERRED_BUFFER_NORMAL_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("specularMap"), DEFERRED_BUFFER_EMISSIVE_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), DEFERRED_BUFFER_DEPTH_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("obscuranceMap"), DEFERRED_BUFFER_OBSCURANCE_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("shadowMap"), SHADOW_MAP_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("skyboxMap"), SKYBOX_MAP_UNIT));

    slotBindings.insert(gpu::Shader::Binding(std::string("linearZeyeMap"), DEFERRED_BUFFER_LINEAR_DEPTH_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("curvatureMap"), DEFERRED_BUFFER_CURVATURE_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("diffusedCurvatureMap"), DEFERRED_BUFFER_DIFFUSED_CURVATURE_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("scatteringLUT"), SCATTERING_LUT_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("scatteringSpecularBeckmann"), SCATTERING_SPECULAR_UNIT));


    slotBindings.insert(gpu::Shader::Binding(std::string("cameraCorrectionBuffer"), CAMERA_CORRECTION_BUFFER_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightingModelBuffer"), LIGHTING_MODEL_BUFFER_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("subsurfaceScatteringParametersBuffer"), SCATTERING_PARAMETERS_BUFFER_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), LIGHT_GPU_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightAmbientBuffer"), LIGHT_AMBIENT_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightIndexBuffer"), LIGHT_INDEX_GPU_SLOT));


    slotBindings.insert(gpu::Shader::Binding(std::string("frustumGridBuffer"), LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("clusterGridBuffer"), LIGHT_CLUSTER_GRID_CLUSTER_GRID_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("clusterContentBuffer"), LIGHT_CLUSTER_GRID_CLUSTER_CONTENT_SLOT));

    gpu::Shader::makeProgram(*program, slotBindings);

    locations->radius = program->getUniforms().findLocation("radius");
    locations->ambientSphere = program->getUniforms().findLocation("ambientSphere.L00");

    locations->texcoordFrameTransform = program->getUniforms().findLocation("texcoordFrameTransform");

    locations->lightBufferUnit = program->getBuffers().findLocation("lightBuffer");
    locations->ambientBufferUnit = program->getBuffers().findLocation("lightAmbientBuffer");
    locations->lightIndexBufferUnit = program->getBuffers().findLocation("lightIndexBuffer");
    locations->deferredFrameTransformBuffer = program->getBuffers().findLocation("deferredFrameTransformBuffer");
    locations->subsurfaceScatteringParametersBuffer = program->getBuffers().findLocation("subsurfaceScatteringParametersBuffer");
    locations->shadowTransformBuffer = program->getBuffers().findLocation("shadowTransformBuffer");

    return program;
}

static void loadLightProgram(const char* vertSource, const char* fragSource, bool lightVolume, gpu::PipelinePointer& pipeline, LightLocationsPtr& locations) {

    gpu::ShaderPointer program = makeLightProgram(vertSource, fragSource, locations);

    auto state = std::make_shared<gpu::State>();
    state->setColorWriteMask(true, true, true, false);

    if (lightVolume) {
        state->setStencilTest(true, 0x00, gpu::State::StencilTest(1, 0xFF, gpu::LESS_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
       
        state->setCullMode(gpu::State::CULL_BACK);
   //     state->setCullMode(gpu::State::CULL_FRONT);
     //   state->setDepthTest(true, false, gpu::GREATER_EQUAL);

        //state->setDepthClampEnable(true);
        // TODO: We should use DepthClamp and avoid changing geometry for inside /outside cases
        // additive blending
        state->setBlendFunction(true, gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

    } else {
        // Stencil test all the light passes for objects pixels only, not the background
        state->setStencilTest(true, 0x00, gpu::State::StencilTest(0, 0x01, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        state->setCullMode(gpu::State::CULL_BACK);
        // additive blending
        state->setBlendFunction(true, gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    }
    pipeline = gpu::Pipeline::create(program, state);

}


static void loadLightVolumeProgram(const char* vertSource, const char* fragSource, bool front, gpu::PipelinePointer& pipeline, LightLocationsPtr& locations) {
    gpu::ShaderPointer program = makeLightProgram(vertSource, fragSource, locations);

    auto state = std::make_shared<gpu::State>();

    // Stencil test all the light passes for objects pixels only, not the background

    if (front) {
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_DECR, gpu::State::STENCIL_OP_KEEP));

      //  state->setDepthClampEnable(true);
        // TODO: We should use DepthClamp and avoid changing geometry for inside /outside cases
        // additive blending
       // state->setBlendFunction(true, gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        //state->setColorWriteMask(true, true, true, false);
        state->setColorWriteMask(false, false, false, false);
    } else {
        state->setCullMode(gpu::State::CULL_FRONT);
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_INCR, gpu::State::STENCIL_OP_KEEP));
        // additive blending
       // state->setBlendFunction(true, gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        // state->setColorWriteMask(true, true, true, false);
        state->setColorWriteMask(false, false, false, false);
    }
    pipeline = gpu::Pipeline::create(program, state);

}

void DeferredLightingEffect::setGlobalLight(const model::LightPointer& light) {
    auto globalLight = _allocatedLights.front();
    globalLight->setDirection(light->getDirection());
    globalLight->setColor(light->getColor());
    globalLight->setIntensity(light->getIntensity());
    globalLight->setAmbientIntensity(light->getAmbientIntensity());
    globalLight->setAmbientSphere(light->getAmbientSphere());
    globalLight->setAmbientMap(light->getAmbientMap());
}

#include <shared/Shapes.h>

model::MeshPointer DeferredLightingEffect::getPointLightMesh() {
    if (!_pointLightMesh) {
        _pointLightMesh = std::make_shared<model::Mesh>();

        // let's use a icosahedron
        auto solid = geometry::icosahedron();
        solid.fitDimension(1.05f); // scaled to 1.05 meters, it will be scaled by the shader accordingly to the light size

        int verticesSize = (int) (solid.vertices.size() * 3 * sizeof(float));
        float* vertexData = (float*) solid.vertices.data();

        _pointLightMesh->setVertexBuffer(gpu::BufferView(new gpu::Buffer(verticesSize, (gpu::Byte*) vertexData), gpu::Element::VEC3F_XYZ));

        int nbIndices = (int) solid.faces.size() * 3;

        gpu::uint16* indexData = new gpu::uint16[nbIndices];
        gpu::uint16* index = indexData;
        for (auto face : solid.faces) {
            *(index++) = face[0];
            *(index++) = face[1];
            *(index++) = face[2];
        }

        _pointLightMesh->setIndexBuffer(gpu::BufferView(new gpu::Buffer(sizeof(unsigned short) * nbIndices, (gpu::Byte*) indexData), gpu::Element::INDEX_UINT16));
        delete[] indexData;


        std::vector<model::Mesh::Part> parts;
        parts.push_back(model::Mesh::Part(0, nbIndices, 0, model::Mesh::TRIANGLES));
        parts.push_back(model::Mesh::Part(0, nbIndices, 0, model::Mesh::LINE_STRIP)); // outline version


        _pointLightMesh->setPartBuffer(gpu::BufferView(new gpu::Buffer(parts.size() * sizeof(model::Mesh::Part), (gpu::Byte*) parts.data()), gpu::Element::PART_DRAWCALL));
    }
    return _pointLightMesh;
}

model::MeshPointer DeferredLightingEffect::getSpotLightMesh() {
    if (!_spotLightMesh) {
        _spotLightMesh = std::make_shared<model::Mesh>();

        int slices = 16;
        int rings = 3;
        int vertices = 2 + rings * slices;
        int originVertex = vertices - 2;
        int capVertex = vertices - 1;
        int verticesSize = vertices * 3 * sizeof(float);
        int indices = 3 * slices * (1 + 1 + 2 * (rings -1));
        int ringFloatOffset = slices * 3;


        float* vertexData = new float[verticesSize];
        float* vertexRing0 = vertexData;
        float* vertexRing1 = vertexRing0 + ringFloatOffset;
        float* vertexRing2 = vertexRing1 + ringFloatOffset;
        
        for (int i = 0; i < slices; i++) {
            float theta = TWO_PI * i / slices;
            auto cosin = glm::vec2(cosf(theta), sinf(theta));

            *(vertexRing0++) = cosin.x;
            *(vertexRing0++) = cosin.y;
            *(vertexRing0++) = 0.0f;

            *(vertexRing1++) = cosin.x;
            *(vertexRing1++) = cosin.y;
            *(vertexRing1++) = 0.33f;

            *(vertexRing2++) = cosin.x;
            *(vertexRing2++) = cosin.y;
            *(vertexRing2++) = 0.66f;
        }
        
        *(vertexRing2++) = 0.0f;
        *(vertexRing2++) = 0.0f;
        *(vertexRing2++) = -1.0f;
        
        *(vertexRing2++) = 0.0f;
        *(vertexRing2++) = 0.0f;
        *(vertexRing2++) = 1.0f;
        
        _spotLightMesh->setVertexBuffer(gpu::BufferView(new gpu::Buffer(verticesSize, (gpu::Byte*) vertexData), gpu::Element::VEC3F_XYZ));
        delete[] vertexData;

        gpu::uint16* indexData = new gpu::uint16[indices];
        gpu::uint16* index = indexData;
        for (int i = 0; i < slices; i++) {
            *(index++) = originVertex;
            
            int s0 = i;
            int s1 = ((i + 1) % slices);
            *(index++) = s0;
            *(index++) = s1;

            int s2 = s0 + slices;
            int s3 = s1 + slices;
            *(index++) = s1;
            *(index++) = s0;
            *(index++) = s2;

            *(index++) = s1;
            *(index++) = s2;
            *(index++) = s3;

            int s4 = s2 + slices;
            int s5 = s3 + slices;
            *(index++) = s3;
            *(index++) = s2;
            *(index++) = s4;

            *(index++) = s3;
            *(index++) = s4;
            *(index++) = s5;


            *(index++) = s5;
            *(index++) = s4;
            *(index++) = capVertex;
        }

        _spotLightMesh->setIndexBuffer(gpu::BufferView(new gpu::Buffer(sizeof(unsigned short) * indices, (gpu::Byte*) indexData), gpu::Element::INDEX_UINT16));
        delete[] indexData;

        
        std::vector<model::Mesh::Part> parts;
        parts.push_back(model::Mesh::Part(0, indices, 0, model::Mesh::TRIANGLES));
        parts.push_back(model::Mesh::Part(0, indices, 0, model::Mesh::LINE_STRIP)); // outline version

        
        _spotLightMesh->setPartBuffer(gpu::BufferView(new gpu::Buffer(parts.size() * sizeof(model::Mesh::Part), (gpu::Byte*) parts.data()), gpu::Element::PART_DRAWCALL));
    }
    return _spotLightMesh;
}

void PreparePrimaryFramebuffer::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, gpu::FramebufferPointer& primaryFramebuffer) {

    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    auto framebufferSize = framebufferCache->getFrameBufferSize();
    glm::uvec2 frameSize(framebufferSize.width(), framebufferSize.height());

    // Resizing framebuffers instead of re-building them seems to cause issues with threaded 
    // rendering
    if (_primaryFramebuffer && _primaryFramebuffer->getSize() != frameSize) {
        _primaryFramebuffer.reset();
    }

    if (!_primaryFramebuffer) {
        _primaryFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
        auto colorFormat = gpu::Element::COLOR_SRGBA_32;

        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        auto primaryColorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, frameSize.x, frameSize.y, defaultSampler));
        primaryColorTexture->setSource("PreparePrimaryFramebuffer::primaryColorTexture");


        _primaryFramebuffer->setRenderBuffer(0, primaryColorTexture);


        auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL); // Depth24_Stencil8 texel format
        auto primaryDepthTexture = gpu::TexturePointer(gpu::Texture::create2D(depthFormat, frameSize.x, frameSize.y, defaultSampler));
        primaryDepthTexture->setSource("PreparePrimaryFramebuffer::primaryDepthTexture");

        _primaryFramebuffer->setDepthStencilBuffer(primaryDepthTexture, depthFormat);
    }

    
    primaryFramebuffer = _primaryFramebuffer;
}

void PrepareDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    auto args = renderContext->args;

    auto primaryFramebuffer = inputs.get0();
    auto lightingModel = inputs.get1();

    if (!_deferredFramebuffer) {
        _deferredFramebuffer = std::make_shared<DeferredFramebuffer>();
    }
    _deferredFramebuffer->updatePrimaryDepth(primaryFramebuffer->getDepthStencilBuffer());

    outputs.edit0() = _deferredFramebuffer;
    outputs.edit1() = _deferredFramebuffer->getLightingFramebuffer();

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        // Clear deferred
        auto deferredFbo = _deferredFramebuffer->getDeferredFramebuffer();
        batch.setFramebuffer(deferredFbo);

        // Clear Color, Depth and Stencil for deferred buffer
        batch.clearFramebuffer(
            gpu::Framebuffer::BUFFER_COLOR0 | gpu::Framebuffer::BUFFER_COLOR1 | gpu::Framebuffer::BUFFER_COLOR2 | gpu::Framebuffer::BUFFER_COLOR3 |
            gpu::Framebuffer::BUFFER_DEPTH |
            gpu::Framebuffer::BUFFER_STENCIL,
            vec4(vec3(0), 0), 1.0, 0.0, true);

        // For the rest of the rendering, bind the lighting model
        batch.setUniformBuffer(LIGHTING_MODEL_BUFFER_SLOT, lightingModel->getParametersBuffer());
    });
    
    
    // Prepare a fresh Light Frame
    auto deferredLightingEffect = DependencyManager::get<DeferredLightingEffect>();
    deferredLightingEffect->getLightStage()->_currentFrame.clear();
}


void RenderDeferredSetup::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
    const DeferredFrameTransformPointer& frameTransform,
    const DeferredFramebufferPointer& deferredFramebuffer,
    const LightingModelPointer& lightingModel,
    const SurfaceGeometryFramebufferPointer& surfaceGeometryFramebuffer,
    const AmbientOcclusionFramebufferPointer& ambientOcclusionFramebuffer,
    const SubsurfaceScatteringResourcePointer& subsurfaceScatteringResource) {

    auto args = renderContext->args;
    auto& batch = (*args->_batch);
    {
        // Framebuffer copy operations cannot function as multipass stereo operations.
        batch.enableStereo(false);
        
        // perform deferred lighting, rendering to free fbo
        auto framebufferCache = DependencyManager::get<FramebufferCache>();
            
        auto textureCache = DependencyManager::get<TextureCache>();
        auto deferredLightingEffect = DependencyManager::get<DeferredLightingEffect>();

        // binding the first framebuffer
        auto lightingFBO = deferredFramebuffer->getLightingFramebuffer();
        batch.setFramebuffer(lightingFBO);
        
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);
        
        
        // Bind the G-Buffer surfaces
        batch.setResourceTexture(DEFERRED_BUFFER_COLOR_UNIT, deferredFramebuffer->getDeferredColorTexture());
        batch.setResourceTexture(DEFERRED_BUFFER_NORMAL_UNIT, deferredFramebuffer->getDeferredNormalTexture());
        batch.setResourceTexture(DEFERRED_BUFFER_EMISSIVE_UNIT, deferredFramebuffer->getDeferredSpecularTexture());
        batch.setResourceTexture(DEFERRED_BUFFER_DEPTH_UNIT, deferredFramebuffer->getPrimaryDepthTexture());
        
        // FIXME: Different render modes should have different tasks
        if (args->_renderMode == RenderArgs::DEFAULT_RENDER_MODE && deferredLightingEffect->isAmbientOcclusionEnabled()) {
            batch.setResourceTexture(DEFERRED_BUFFER_OBSCURANCE_UNIT, ambientOcclusionFramebuffer->getOcclusionTexture());
        } else {
            // need to assign the white texture if ao is off
            batch.setResourceTexture(DEFERRED_BUFFER_OBSCURANCE_UNIT, textureCache->getWhiteTexture());
        }

        // The Deferred Frame Transform buffer
        batch.setUniformBuffer(DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT, frameTransform->getFrameTransformBuffer());

        // THe lighting model
        batch.setUniformBuffer(LIGHTING_MODEL_BUFFER_SLOT, lightingModel->getParametersBuffer());

        // Subsurface scattering specific
        if (surfaceGeometryFramebuffer) {
            batch.setResourceTexture(DEFERRED_BUFFER_LINEAR_DEPTH_UNIT, surfaceGeometryFramebuffer->getLinearDepthTexture());
            batch.setResourceTexture(DEFERRED_BUFFER_CURVATURE_UNIT, surfaceGeometryFramebuffer->getCurvatureTexture());
            batch.setResourceTexture(DEFERRED_BUFFER_DIFFUSED_CURVATURE_UNIT, surfaceGeometryFramebuffer->getLowCurvatureTexture());
        }
        if (subsurfaceScatteringResource) {
            batch.setUniformBuffer(SCATTERING_PARAMETERS_BUFFER_SLOT, subsurfaceScatteringResource->getParametersBuffer());
            batch.setResourceTexture(SCATTERING_LUT_UNIT, subsurfaceScatteringResource->getScatteringTable());
            batch.setResourceTexture(SCATTERING_SPECULAR_UNIT, subsurfaceScatteringResource->getScatteringSpecular());
        }

        // Global directional light and ambient pass

        assert(deferredLightingEffect->getLightStage()->getNumLights() > 0);
        auto lightAndShadow = deferredLightingEffect->getLightStage()->getLightAndShadow(0);
        const auto& globalShadow = lightAndShadow.second;

        // Bind the shadow buffer
        if (globalShadow) {
            batch.setResourceTexture(SHADOW_MAP_UNIT, globalShadow->map);
        }

        auto& program = deferredLightingEffect->_shadowMapEnabled ? deferredLightingEffect->_directionalLightShadow : deferredLightingEffect->_directionalLight;
        LightLocationsPtr locations = deferredLightingEffect->_shadowMapEnabled ? deferredLightingEffect->_directionalLightShadowLocations : deferredLightingEffect->_directionalLightLocations;
        const auto& keyLight = deferredLightingEffect->_allocatedLights[deferredLightingEffect->_globalLights.front()];

        // Setup the global directional pass pipeline
        {
            if (deferredLightingEffect->_shadowMapEnabled) {
                if (keyLight->getAmbientMap()) {
                    program = deferredLightingEffect->_directionalSkyboxLightShadow;
                    locations = deferredLightingEffect->_directionalSkyboxLightShadowLocations;
                } else {
                    program = deferredLightingEffect->_directionalAmbientSphereLightShadow;
                    locations = deferredLightingEffect->_directionalAmbientSphereLightShadowLocations;
                }
            } else {
                if (keyLight->getAmbientMap()) {
                    program = deferredLightingEffect->_directionalAmbientSphereLight;
                    locations = deferredLightingEffect->_directionalAmbientSphereLightLocations;
                    //program = deferredLightingEffect->_directionalSkyboxLight;
                    //locations = deferredLightingEffect->_directionalSkyboxLightLocations;
                } else {
                    program = deferredLightingEffect->_directionalAmbientSphereLight;
                    locations = deferredLightingEffect->_directionalAmbientSphereLightLocations;
                }
            }

            if (locations->shadowTransformBuffer >= 0) {
                if (globalShadow) {
                    batch.setUniformBuffer(locations->shadowTransformBuffer, globalShadow->getBuffer());
                }
            }
            batch.setPipeline(program);
        }

        // Adjust the texcoordTransform in the case we are rendeirng a sub region(mini mirror)
        auto textureFrameTransform = gpu::Framebuffer::evalSubregionTexcoordTransformCoefficients(deferredFramebuffer->getFrameSize(), args->_viewport);
        batch._glUniform4fv(locations->texcoordFrameTransform, 1, reinterpret_cast< const float* >(&textureFrameTransform));

        { // Setup the global lighting
            deferredLightingEffect->setupKeyLightBatch(batch, locations->lightBufferUnit, locations->ambientBufferUnit, SKYBOX_MAP_UNIT);
        }

        batch.draw(gpu::TRIANGLE_STRIP, 4);

        if (keyLight->getAmbientMap()) {
            batch.setResourceTexture(SKYBOX_MAP_UNIT, nullptr);
        }
        batch.setResourceTexture(SHADOW_MAP_UNIT, nullptr);
    }
}

RenderDeferredLocals::RenderDeferredLocals() :
    _localLightsBuffer(std::make_shared<gpu::Buffer>()) {

}

void RenderDeferredLocals::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
    const DeferredFrameTransformPointer& frameTransform,
    const DeferredFramebufferPointer& deferredFramebuffer,
    const LightingModelPointer& lightingModel,
    const SurfaceGeometryFramebufferPointer& surfaceGeometryFramebuffer, const LightClustersPointer& lightClusters) {

    bool points = lightingModel->isPointLightEnabled();
    bool spots = lightingModel->isSpotLightEnabled();

    if (!points && !spots) {
        return;
    }
    auto args = renderContext->args;
    auto& batch = (*args->_batch);
    {
        // THe main viewport is assumed to be the mono viewport (or the 2 stereo faces side by side within that viewport)
        auto viewport = args->_viewport;

        // The view frustum is the mono frustum base
        auto viewFrustum = args->getViewFrustum();

        // Eval the mono projection
        mat4 projMat;
        viewFrustum.evalProjectionMatrix(projMat);

        // The view transform
        Transform viewTransform;
        viewFrustum.evalViewTransform(viewTransform);


        auto deferredLightingEffect = DependencyManager::get<DeferredLightingEffect>();

        // Render in this viewport
        batch.setViewportTransform(viewport);
        batch.setStateScissorRect(viewport);

        auto textureFrameTransform = gpu::Framebuffer::evalSubregionTexcoordTransformCoefficients(deferredFramebuffer->getFrameSize(), viewport);

    //    batch.setProjectionTransform(projMat);
     //   batch.setViewTransform(viewTransform, true);

        // gather lights
     /*   auto& srcPointLights = deferredLightingEffect->_pointLights;
        auto& srcSpotLights = deferredLightingEffect->_spotLights;
        int numPointLights = (int) srcPointLights.size();
        int offsetPointLights = 0;
        int numSpotLights = (int) srcSpotLights.size();
        int offsetSpotLights = numPointLights;


        std::vector<int> lightIndices(numPointLights + numSpotLights + 1);
        lightIndices[0] = 0;

        if (points && !srcPointLights.empty()) {
            memcpy(lightIndices.data() + (lightIndices[0] + 1), srcPointLights.data(), srcPointLights.size() * sizeof(int));
            lightIndices[0] += (int)srcPointLights.size();
        }
        if (spots && !srcSpotLights.empty()) {
            memcpy(lightIndices.data() + (lightIndices[0] + 1), srcSpotLights.data(), srcSpotLights.size() * sizeof(int));
            lightIndices[0] += (int)srcSpotLights.size();
        }*/
        //auto lightClusters = deferredLightingEffect->_lightClusters;
        auto& lightIndices = lightClusters->_visibleLightIndices;
        if (!lightIndices.empty() && lightIndices[0] > 0) {
           // _localLightsBuffer._buffer->setData(lightIndices.size() * sizeof(int), (const gpu::Byte*) lightIndices.data());
           // _localLightsBuffer._size = lightIndices.size() * sizeof(int);

            
            // Bind the global list of lights and the visible lights this frame
            batch.setUniformBuffer(deferredLightingEffect->_localLightLocations->lightBufferUnit, lightClusters->_lightStage->_lightArrayBuffer);

            batch.setUniformBuffer(LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT, lightClusters->_frustumGridBuffer);
            batch.setUniformBuffer(LIGHT_CLUSTER_GRID_CLUSTER_GRID_SLOT, lightClusters->_clusterGridBuffer);
            batch.setUniformBuffer(LIGHT_CLUSTER_GRID_CLUSTER_CONTENT_SLOT, lightClusters->_clusterContentBuffer);


            // before we get to the real lighting, let s try to cull down the number of pixels
            if (false) {/*
                if (numPointLights > 0) {
                    auto mesh = deferredLightingEffect->getPointLightMesh();
                    batch.setIndexBuffer(mesh->getIndexBuffer());
                    batch.setInputBuffer(0, mesh->getVertexBuffer());
                    batch.setInputFormat(mesh->getVertexFormat());
                    auto& pointPart = mesh->getPartBuffer().get<model::Mesh::Part>(0);

                    // Point light pipeline
                    batch.setPipeline(deferredLightingEffect->_pointLightBack);

                    batch.drawIndexedInstanced(numPointLights, model::Mesh::topologyToPrimitive(pointPart._topology), pointPart._numIndices, pointPart._startIndex, offsetPointLights);

                    batch.setPipeline(deferredLightingEffect->_pointLightFront);

                    batch.drawIndexedInstanced(numPointLights, model::Mesh::topologyToPrimitive(pointPart._topology), pointPart._numIndices, pointPart._startIndex, offsetPointLights);
                }
                
                if (numSpotLights > 0) {
                    auto mesh = deferredLightingEffect->getSpotLightMesh();
                    batch.setIndexBuffer(mesh->getIndexBuffer());
                    batch.setInputBuffer(0, mesh->getVertexBuffer());
                    batch.setInputFormat(mesh->getVertexFormat());
                    auto& conePart = mesh->getPartBuffer().get<model::Mesh::Part>(0);

                    // Spot light pipeline
                    batch.setPipeline(deferredLightingEffect->_spotLightBack);

                    batch.drawIndexedInstanced(numSpotLights, model::Mesh::topologyToPrimitive(conePart._topology), conePart._numIndices, conePart._startIndex, offsetSpotLights);

                    batch.setPipeline(deferredLightingEffect->_spotLightFront);

                    batch.drawIndexedInstanced(numSpotLights, model::Mesh::topologyToPrimitive(conePart._topology), conePart._numIndices, conePart._startIndex, offsetSpotLights);
                }*/
            }

            // Local light pipeline
            batch.setPipeline(deferredLightingEffect->_localLight);
            batch._glUniform4fv(deferredLightingEffect->_localLightLocations->texcoordFrameTransform, 1, reinterpret_cast<const float*>(&textureFrameTransform));

            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }
    }
}

void RenderDeferredCleanup::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
    auto args = renderContext->args;
    auto& batch = (*args->_batch);
    {
        // Probably not necessary in the long run because the gpu layer would unbound this texture if used as render target
        batch.setResourceTexture(DEFERRED_BUFFER_COLOR_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_NORMAL_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_EMISSIVE_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_DEPTH_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_OBSCURANCE_UNIT, nullptr);

        batch.setResourceTexture(DEFERRED_BUFFER_LINEAR_DEPTH_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_CURVATURE_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_DIFFUSED_CURVATURE_UNIT, nullptr);
        batch.setResourceTexture(SCATTERING_LUT_UNIT, nullptr);
        batch.setResourceTexture(SCATTERING_SPECULAR_UNIT, nullptr);

        batch.setUniformBuffer(SCATTERING_PARAMETERS_BUFFER_SLOT, nullptr);
        //     batch.setUniformBuffer(LIGHTING_MODEL_BUFFER_SLOT, nullptr);
        batch.setUniformBuffer(DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT, nullptr);
    }

    auto deferredLightingEffect = DependencyManager::get<DeferredLightingEffect>();

    // End of the Lighting pass
    if (!deferredLightingEffect->_pointLights.empty()) {
        deferredLightingEffect->_pointLights.clear();
    }
    if (!deferredLightingEffect->_spotLights.empty()) {
        deferredLightingEffect->_spotLights.clear();
    }
}

RenderDeferred::RenderDeferred() {

}


void RenderDeferred::configure(const Config& config) {
}

void RenderDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const Inputs& inputs) {
    PROFILE_RANGE("DeferredLighting");

    auto deferredTransform = inputs.get0();
    auto deferredFramebuffer = inputs.get1();
    auto lightingModel = inputs.get2();
    auto surfaceGeometryFramebuffer = inputs.get3();
    auto ssaoFramebuffer = inputs.get4();
    auto subsurfaceScatteringResource = inputs.get5();
    auto lightClusters = inputs.get6();
    auto args = renderContext->args;

    if (!_gpuTimer) {
        _gpuTimer = std::make_shared < gpu::RangeTimer>(__FUNCTION__);
    }

    auto previousBatch = args->_batch;
    gpu::Batch batch;
    args->_batch = &batch;
     _gpuTimer->begin(batch);

    setupJob.run(sceneContext, renderContext, deferredTransform, deferredFramebuffer, lightingModel, surfaceGeometryFramebuffer, ssaoFramebuffer, subsurfaceScatteringResource);
    
    lightsJob.run(sceneContext, renderContext, deferredTransform, deferredFramebuffer, lightingModel, surfaceGeometryFramebuffer, lightClusters);

    cleanupJob.run(sceneContext, renderContext);

    _gpuTimer->end(batch);
     args->_context->appendFrameBatch(batch);
     args->_batch = previousBatch;


    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->setGPUBatchRunTime(_gpuTimer->getGPUAverage(), _gpuTimer->getBatchAverage());
}
