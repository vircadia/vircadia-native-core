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
#include "deferred_light_limited_vert.h"
#include "deferred_light_spot_vert.h"

#include "directional_light_frag.h"
#include "directional_ambient_light_frag.h"
#include "directional_skybox_light_frag.h"

#include "directional_light_shadow_frag.h"
#include "directional_ambient_light_shadow_frag.h"
#include "directional_skybox_light_shadow_frag.h"

#include "point_light_frag.h"
#include "spot_light_frag.h"

using namespace render;

struct LightLocations {
    int radius{ -1 };
    int ambientSphere{ -1 };
    int lightBufferUnit{ -1 };
    int texcoordFrameTransform{ -1 };
    int sphereParam{ -1 };
    int coneParam{ -1 };
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
    DEFERRED_BUFFER_CURVATURE_UNIT,
    DEFERRED_BUFFER_DIFFUSED_CURVATURE_UNIT,
    SCATTERING_LUT_UNIT,
    SCATTERING_SPECULAR_UNIT,
};
enum DeferredShader_BufferSlot {
    DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT = 0,
    SCATTERING_PARAMETERS_BUFFER_SLOT,
    LIGHTING_MODEL_BUFFER_SLOT = render::ShapePipeline::Slot::LIGHTING_MODEL,
    LIGHT_GPU_SLOT = render::ShapePipeline::Slot::LIGHT,
};

static void loadLightProgram(const char* vertSource, const char* fragSource, bool lightVolume, gpu::PipelinePointer& program, LightLocationsPtr& locations);

void DeferredLightingEffect::init() {
    _directionalLightLocations = std::make_shared<LightLocations>();
    _directionalAmbientSphereLightLocations = std::make_shared<LightLocations>();
    _directionalSkyboxLightLocations = std::make_shared<LightLocations>();

    _directionalLightShadowLocations = std::make_shared<LightLocations>();
    _directionalAmbientSphereLightShadowLocations = std::make_shared<LightLocations>();
    _directionalSkyboxLightShadowLocations = std::make_shared<LightLocations>();

    _pointLightLocations = std::make_shared<LightLocations>();
    _spotLightLocations = std::make_shared<LightLocations>();

    loadLightProgram(deferred_light_vert, directional_light_frag, false, _directionalLight, _directionalLightLocations);
    loadLightProgram(deferred_light_vert, directional_ambient_light_frag, false, _directionalAmbientSphereLight, _directionalAmbientSphereLightLocations);
    loadLightProgram(deferred_light_vert, directional_skybox_light_frag, false, _directionalSkyboxLight, _directionalSkyboxLightLocations);

    loadLightProgram(deferred_light_vert, directional_light_shadow_frag, false, _directionalLightShadow, _directionalLightShadowLocations);
    loadLightProgram(deferred_light_vert, directional_ambient_light_shadow_frag, false, _directionalAmbientSphereLightShadow, _directionalAmbientSphereLightShadowLocations);
    loadLightProgram(deferred_light_vert, directional_skybox_light_shadow_frag, false, _directionalSkyboxLightShadow, _directionalSkyboxLightShadowLocations);

    loadLightProgram(deferred_light_limited_vert, point_light_frag, true, _pointLight, _pointLightLocations);
    loadLightProgram(deferred_light_spot_vert, spot_light_frag, true, _spotLight, _spotLightLocations);

    // Allocate a global light representing the Global Directional light casting shadow (the sun) and the ambient light
    _globalLights.push_back(0);
    _allocatedLights.push_back(std::make_shared<model::Light>());

    model::LightPointer lp = _allocatedLights[0];
    lp->setType(model::Light::SUN);

    // Add the global light to the light stage (for later shadow rendering)
    _lightStage.addLight(lp);

    lp->setDirection(glm::vec3(-1.0f));
    lp->setColor(glm::vec3(1.0f));
    lp->setIntensity(1.0f);
    lp->setType(model::Light::SUN);
    lp->setAmbientSpherePreset(gpu::SphericalHarmonics::Preset::OLD_TOWN_SQUARE);
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

void DeferredLightingEffect::setupKeyLightBatch(gpu::Batch& batch, int lightBufferUnit, int skyboxCubemapUnit) {
    PerformanceTimer perfTimer("DLE->setupBatch()");
    auto keyLight = _allocatedLights[_globalLights.front()];

    if (lightBufferUnit >= 0) {
        batch.setUniformBuffer(lightBufferUnit, keyLight->getSchemaBuffer());
    }

    if (keyLight->getAmbientMap() && (skyboxCubemapUnit >= 0)) {
        batch.setResourceTexture(skyboxCubemapUnit, keyLight->getAmbientMap());
    }
}

static void loadLightProgram(const char* vertSource, const char* fragSource, bool lightVolume, gpu::PipelinePointer& pipeline, LightLocationsPtr& locations) {
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

    slotBindings.insert(gpu::Shader::Binding(std::string("curvatureMap"), DEFERRED_BUFFER_CURVATURE_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("diffusedCurvatureMap"), DEFERRED_BUFFER_DIFFUSED_CURVATURE_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("scatteringLUT"), SCATTERING_LUT_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("scatteringSpecularBeckmann"), SCATTERING_SPECULAR_UNIT));


    slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightingModelBuffer"), LIGHTING_MODEL_BUFFER_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("subsurfaceScatteringParametersBuffer"), SCATTERING_PARAMETERS_BUFFER_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), LIGHT_GPU_SLOT));

    gpu::Shader::makeProgram(*program, slotBindings);

    locations->radius = program->getUniforms().findLocation("radius");
    locations->ambientSphere = program->getUniforms().findLocation("ambientSphere.L00");

    locations->texcoordFrameTransform = program->getUniforms().findLocation("texcoordFrameTransform");
    locations->sphereParam = program->getUniforms().findLocation("sphereParam");
    locations->coneParam = program->getUniforms().findLocation("coneParam");

    locations->lightBufferUnit = program->getBuffers().findLocation("lightBuffer");
    locations->deferredFrameTransformBuffer = program->getBuffers().findLocation("deferredFrameTransformBuffer");
    locations->subsurfaceScatteringParametersBuffer = program->getBuffers().findLocation("subsurfaceScatteringParametersBuffer");
    locations->shadowTransformBuffer = program->getBuffers().findLocation("shadowTransformBuffer");

    auto state = std::make_shared<gpu::State>();
    state->setColorWriteMask(true, true, true, false);

    // Stencil test all the light passes for objects pixels only, not the background
    state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
    
    if (lightVolume) {
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, false, gpu::LESS_EQUAL);

        // TODO: We should use DepthClamp and avoid changing geometry for inside /outside cases
        // additive blending
        state->setBlendFunction(true, gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

    } else {
        state->setCullMode(gpu::State::CULL_BACK);
        // additive blending
        state->setBlendFunction(true, gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
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

model::MeshPointer DeferredLightingEffect::getSpotLightMesh() {
    if (!_spotLightMesh) {
        _spotLightMesh = std::make_shared<model::Mesh>();

        int slices = 32;
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
        glm::ivec2 frameSize(framebufferSize.width(), framebufferSize.height());

    if (!_primaryFramebuffer) {
        _primaryFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
        auto colorFormat = gpu::Element::COLOR_SRGBA_32;

        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        auto primaryColorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, frameSize.x, frameSize.y, defaultSampler));


        _primaryFramebuffer->setRenderBuffer(0, primaryColorTexture);


        auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL); // Depth24_Stencil8 texel format
        auto primaryDepthTexture = gpu::TexturePointer(gpu::Texture::create2D(depthFormat, frameSize.x, frameSize.y, defaultSampler));

        _primaryFramebuffer->setDepthStencilBuffer(primaryDepthTexture, depthFormat);

    }
    _primaryFramebuffer->resize(frameSize.x, frameSize.y);

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
}


void RenderDeferredSetup::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
    const DeferredFrameTransformPointer& frameTransform,
    const DeferredFramebufferPointer& deferredFramebuffer,
    const LightingModelPointer& lightingModel,
    const SurfaceGeometryFramebufferPointer& surfaceGeometryFramebuffer,
    const gpu::FramebufferPointer& lowCurvatureNormalFramebuffer,
    const SubsurfaceScatteringResourcePointer& subsurfaceScatteringResource) {
    
    auto args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        
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
            batch.setResourceTexture(DEFERRED_BUFFER_OBSCURANCE_UNIT, framebufferCache->getOcclusionTexture());
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
            batch.setResourceTexture(DEFERRED_BUFFER_CURVATURE_UNIT, surfaceGeometryFramebuffer->getCurvatureTexture());
        }
        if (lowCurvatureNormalFramebuffer) {
            batch.setResourceTexture(DEFERRED_BUFFER_DIFFUSED_CURVATURE_UNIT, lowCurvatureNormalFramebuffer->getRenderBuffer(0));
        }
        if (subsurfaceScatteringResource) {
            batch.setUniformBuffer(SCATTERING_PARAMETERS_BUFFER_SLOT, subsurfaceScatteringResource->getParametersBuffer());
            batch.setResourceTexture(SCATTERING_LUT_UNIT, subsurfaceScatteringResource->getScatteringTable());
            batch.setResourceTexture(SCATTERING_SPECULAR_UNIT, subsurfaceScatteringResource->getScatteringSpecular());
        }

        // Global directional light and ambient pass

        assert(deferredLightingEffect->getLightStage().lights.size() > 0);
        const auto& globalShadow = deferredLightingEffect->getLightStage().lights[0]->shadow;

        // Bind the shadow buffer
        batch.setResourceTexture(SHADOW_MAP_UNIT, globalShadow.map);

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
                    program = deferredLightingEffect->_directionalSkyboxLight;
                    locations = deferredLightingEffect->_directionalSkyboxLightLocations;
                } else {
                    program = deferredLightingEffect->_directionalAmbientSphereLight;
                    locations = deferredLightingEffect->_directionalAmbientSphereLightLocations;
                }
            }

            if (locations->shadowTransformBuffer >= 0) {
                batch.setUniformBuffer(locations->shadowTransformBuffer, globalShadow.getBuffer());
            }
            batch.setPipeline(program);
        }

        // Adjust the texcoordTransform in the case we are rendeirng a sub region(mini mirror)
        auto textureFrameTransform = gpu::Framebuffer::evalSubregionTexcoordTransformCoefficients(deferredFramebuffer->getFrameSize(), args->_viewport);
        batch._glUniform4fv(locations->texcoordFrameTransform, 1, reinterpret_cast< const float* >(&textureFrameTransform));

        { // Setup the global lighting
            deferredLightingEffect->setupKeyLightBatch(batch, locations->lightBufferUnit, SKYBOX_MAP_UNIT);
        }

        batch.draw(gpu::TRIANGLE_STRIP, 4);

        if (keyLight->getAmbientMap()) {
            batch.setResourceTexture(SKYBOX_MAP_UNIT, nullptr);
        }
        batch.setResourceTexture(SHADOW_MAP_UNIT, nullptr);
    });
    
}

void RenderDeferredLocals::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
    const DeferredFrameTransformPointer& frameTransform,
    const DeferredFramebufferPointer& deferredFramebuffer,
    const LightingModelPointer& lightingModel) {

    bool points = lightingModel->isPointLightEnabled();
    bool spots = lightingModel->isSpotLightEnabled();

    if (!points && !spots) {
        return;
    }
    auto args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {

        // THe main viewport is assumed to be the mono viewport (or the 2 stereo faces side by side within that viewport)
        auto monoViewport = args->_viewport;

        // The view frustum is the mono frustum base
        auto viewFrustum = args->getViewFrustum();

        // Eval the mono projection
        mat4 monoProjMat;
        viewFrustum.evalProjectionMatrix(monoProjMat);

        // The mono view transform
        Transform monoViewTransform;
        viewFrustum.evalViewTransform(monoViewTransform);

        // THe mono view matrix coming from the mono view transform
        glm::mat4 monoViewMat;
        monoViewTransform.getMatrix(monoViewMat);

        auto geometryCache = DependencyManager::get<GeometryCache>();

        auto eyePoint = viewFrustum.getPosition();
        float nearRadius = glm::distance(eyePoint, viewFrustum.getNearTopLeft());

        auto deferredLightingEffect = DependencyManager::get<DeferredLightingEffect>();

        // Render in this side's viewport
        batch.setViewportTransform(monoViewport);
        batch.setStateScissorRect(monoViewport);

        // enlarge the scales slightly to account for tesselation
        const float SCALE_EXPANSION = 0.05f;

        auto textureFrameTransform = gpu::Framebuffer::evalSubregionTexcoordTransformCoefficients(deferredFramebuffer->getFrameSize(), monoViewport);

        batch.setProjectionTransform(monoProjMat);
        batch.setViewTransform(monoViewTransform);

        // Splat Point lights
        if (points && !deferredLightingEffect->_pointLights.empty()) {
            // POint light pipeline
            batch.setPipeline(deferredLightingEffect->_pointLight);
            batch._glUniform4fv(deferredLightingEffect->_pointLightLocations->texcoordFrameTransform, 1, reinterpret_cast< const float* >(&textureFrameTransform));

            for (auto lightID : deferredLightingEffect->_pointLights) {
                auto& light = deferredLightingEffect->_allocatedLights[lightID];
                // IN DEBUG: light->setShowContour(true);
                batch.setUniformBuffer(deferredLightingEffect->_pointLightLocations->lightBufferUnit, light->getSchemaBuffer());

                float expandedRadius = light->getMaximumRadius() * (1.0f + SCALE_EXPANSION);
                glm::vec4 sphereParam(expandedRadius, 0.0f, 0.0f, 1.0f);

                // TODO: We shouldn;t have to do that test and use a different volume geometry for when inside the vlight volume,
                // we should be able to draw thre same geometry use DepthClamp but for unknown reason it's s not working...
                if (glm::distance(eyePoint, glm::vec3(light->getPosition())) < expandedRadius + nearRadius) {
                    sphereParam.w = 0.0f;
                    batch._glUniform4fv(deferredLightingEffect->_pointLightLocations->sphereParam, 1, reinterpret_cast< const float* >(&sphereParam));
                    batch.draw(gpu::TRIANGLE_STRIP, 4);
                } else {
                    sphereParam.w = 1.0f;
                    batch._glUniform4fv(deferredLightingEffect->_pointLightLocations->sphereParam, 1, reinterpret_cast< const float* >(&sphereParam));
                    
                    Transform model;
                    model.setTranslation(glm::vec3(light->getPosition().x, light->getPosition().y, light->getPosition().z));
                    batch.setModelTransform(model.postScale(expandedRadius));
                    batch._glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                    geometryCache->renderSphere(batch);
                }
            }
        }

        // Splat spot lights
        if (spots && !deferredLightingEffect->_spotLights.empty()) {
            // Spot light pipeline
            batch.setPipeline(deferredLightingEffect->_spotLight);
            batch._glUniform4fv(deferredLightingEffect->_spotLightLocations->texcoordFrameTransform, 1, reinterpret_cast< const float* >(&textureFrameTransform));

            // Spot mesh
            auto mesh = deferredLightingEffect->getSpotLightMesh();
            batch.setIndexBuffer(mesh->getIndexBuffer());
            batch.setInputBuffer(0, mesh->getVertexBuffer());
            batch.setInputFormat(mesh->getVertexFormat());
            auto& conePart = mesh->getPartBuffer().get<model::Mesh::Part>(0);

            for (auto lightID : deferredLightingEffect->_spotLights) {
                auto light = deferredLightingEffect->_allocatedLights[lightID];
                // IN DEBUG: 
                light->setShowContour(true);
                batch.setUniformBuffer(deferredLightingEffect->_spotLightLocations->lightBufferUnit, light->getSchemaBuffer());

                auto eyeLightPos = eyePoint - light->getPosition();
                auto eyeHalfPlaneDistance = glm::dot(eyeLightPos, light->getDirection());

                const float TANGENT_LENGTH_SCALE = 0.666f;
                glm::vec4 coneParam(light->getSpotAngleCosSin(), TANGENT_LENGTH_SCALE * tanf(0.5f * light->getSpotAngle()), 1.0f);

                float expandedRadius = light->getMaximumRadius() * (1.0f + SCALE_EXPANSION);
                // TODO: We shouldn;t have to do that test and use a different volume geometry for when inside the vlight volume,
                // we should be able to draw thre same geometry use DepthClamp but for unknown reason it's s not working...
                const float OVER_CONSERVATIVE_SCALE = 1.1f;
                if ((eyeHalfPlaneDistance > -nearRadius) &&
                    (glm::distance(eyePoint, glm::vec3(light->getPosition())) < (expandedRadius * OVER_CONSERVATIVE_SCALE) + nearRadius)) {
                    coneParam.w = 0.0f;
                    batch._glUniform4fv(deferredLightingEffect->_spotLightLocations->coneParam, 1, reinterpret_cast< const float* >(&coneParam));
                    batch.draw(gpu::TRIANGLE_STRIP, 4);
                } else {
                    coneParam.w = 1.0f;
                    batch._glUniform4fv(deferredLightingEffect->_spotLightLocations->coneParam, 1, reinterpret_cast< const float* >(&coneParam));

                    Transform model;
                    model.setTranslation(light->getPosition());
                    model.postRotate(light->getOrientation());
                    model.postScale(glm::vec3(expandedRadius, expandedRadius, expandedRadius));

                    batch.setModelTransform(model);

                    batch.drawIndexed(model::Mesh::topologyToPrimitive(conePart._topology), conePart._numIndices, conePart._startIndex);
                }
            }
        }
    });
}

void RenderDeferredCleanup::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
    auto args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        // Probably not necessary in the long run because the gpu layer would unbound this texture if used as render target
        batch.setResourceTexture(DEFERRED_BUFFER_COLOR_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_NORMAL_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_EMISSIVE_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_DEPTH_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_OBSCURANCE_UNIT, nullptr);

        batch.setResourceTexture(DEFERRED_BUFFER_CURVATURE_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_DIFFUSED_CURVATURE_UNIT, nullptr);
        batch.setResourceTexture(SCATTERING_LUT_UNIT, nullptr);
        batch.setResourceTexture(SCATTERING_SPECULAR_UNIT, nullptr);
        
        batch.setUniformBuffer(SCATTERING_PARAMETERS_BUFFER_SLOT, nullptr);
   //     batch.setUniformBuffer(LIGHTING_MODEL_BUFFER_SLOT, nullptr);
        batch.setUniformBuffer(DEFERRED_FRAME_TRANSFORM_BUFFER_SLOT, nullptr);
    });
    
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
    auto deferredTransform = inputs.get0();
    auto deferredFramebuffer = inputs.get1();
    auto lightingModel = inputs.get2();
    auto surfaceGeometryFramebuffer = inputs.get3();
    auto lowCurvatureNormalFramebuffer = inputs.get4();
    auto subsurfaceScatteringResource = inputs.get5();



    setupJob.run(sceneContext, renderContext, deferredTransform, deferredFramebuffer, lightingModel, surfaceGeometryFramebuffer, lowCurvatureNormalFramebuffer, subsurfaceScatteringResource);
    
    lightsJob.run(sceneContext, renderContext, deferredTransform, deferredFramebuffer, lightingModel);

    cleanupJob.run(sceneContext, renderContext);
}
