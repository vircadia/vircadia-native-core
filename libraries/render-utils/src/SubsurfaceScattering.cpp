//
//  SubsurfaceScattering.cpp
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 6/3/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "SubsurfaceScattering.h"

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "FramebufferCache.h"

#include "DeferredLightingEffect.h"

#include "subsurfaceScattering_makeProfile_frag.h"
#include "subsurfaceScattering_makeLUT_frag.h"
#include "subsurfaceScattering_drawScattering_frag.h"

enum ScatteringShaderBufferSlots {
    ScatteringTask_FrameTransformSlot = 0,
    ScatteringTask_ParamSlot,
    ScatteringTask_LightSlot,
};
enum ScatteringShaderMapSlots {
    ScatteringTask_ScatteringTableSlot = 0,
    ScatteringTask_CurvatureMapSlot,
    ScatteringTask_DiffusedCurvatureMapSlot,
    ScatteringTask_NormalMapSlot,

    ScatteringTask_AlbedoMapSlot,
    ScatteringTask_LinearMapSlot,
    
    ScatteringTask_IBLMapSlot,

};

SubsurfaceScatteringResource::SubsurfaceScatteringResource() {
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters));

}

void SubsurfaceScatteringResource::setBentNormalFactors(const glm::vec4& rgbsBentFactors) {
    if (rgbsBentFactors != getBentNormalFactors()) {
        _parametersBuffer.edit<Parameters>().normalBentInfo = rgbsBentFactors;
    }
}

glm::vec4 SubsurfaceScatteringResource::getBentNormalFactors() const {
    return _parametersBuffer.get<Parameters>().normalBentInfo;
}

void SubsurfaceScatteringResource::setCurvatureFactors(const glm::vec2& sbCurvatureFactors) {
    if (sbCurvatureFactors != getCurvatureFactors()) {
        _parametersBuffer.edit<Parameters>().curvatureInfo = sbCurvatureFactors;
    }
}

glm::vec2 SubsurfaceScatteringResource::getCurvatureFactors() const {
    return _parametersBuffer.get<Parameters>().curvatureInfo;
}


void SubsurfaceScatteringResource::setLevel(float level) {
    if (level != getLevel()) {
        _parametersBuffer.edit<Parameters>().level = level;
    }
}
float SubsurfaceScatteringResource::getLevel() const {
    return _parametersBuffer.get<Parameters>().level;
}

void SubsurfaceScatteringResource::setShowBRDF(bool show) {
    if (show != isShowBRDF()) {
        _parametersBuffer.edit<Parameters>().showBRDF = show;
    }
}
bool SubsurfaceScatteringResource::isShowBRDF() const {
    return (bool)_parametersBuffer.get<Parameters>().showBRDF;
}


void SubsurfaceScatteringResource::generateScatteringTable(RenderArgs* args) {
    if (!_scatteringProfile) {
        _scatteringProfile = generateScatteringProfile(args);
    }
    if (!_scatteringTable) {
        _scatteringTable = generatePreIntegratedScattering(_scatteringProfile, args);
    }
}

SubsurfaceScattering::SubsurfaceScattering() {
    _scatteringResource = std::make_shared<SubsurfaceScatteringResource>();
}

void SubsurfaceScattering::configure(const Config& config) {

    glm::vec4 bentInfo(config.bentRed, config.bentGreen, config.bentBlue, config.bentScale);
    _scatteringResource->setBentNormalFactors(bentInfo);

    glm::vec2 curvatureInfo(config.curvatureOffset, config.curvatureScale);
    _scatteringResource->setCurvatureFactors(curvatureInfo);

    _showProfile = config.showProfile;
    _showLUT = config.showLUT;
}



gpu::PipelinePointer SubsurfaceScattering::getScatteringPipeline() {
    if (!_scatteringPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
     //   auto ps = gpu::StandardShaderLib::getDrawTextureOpaquePS();
        auto ps = gpu::Shader::createPixel(std::string(subsurfaceScattering_drawScattering_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), ScatteringTask_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("scatteringParamsBuffer"), ScatteringTask_ParamSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), ScatteringTask_LightSlot));

        slotBindings.insert(gpu::Shader::Binding(std::string("scatteringLUT"), ScatteringTask_ScatteringTableSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("curvatureMap"), ScatteringTask_CurvatureMapSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("diffusedCurvatureMap"), ScatteringTask_DiffusedCurvatureMapSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), ScatteringTask_NormalMapSlot));

        slotBindings.insert(gpu::Shader::Binding(std::string("albedoMap"), ScatteringTask_AlbedoMapSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("linearDepthMap"), ScatteringTask_LinearMapSlot));

        slotBindings.insert(gpu::Shader::Binding(std::string("skyboxMap"), ScatteringTask_IBLMapSlot));

        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the curvature pass for objects pixels only, not the background
      //  state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _scatteringPipeline = gpu::Pipeline::create(program, state);
    }

    return _scatteringPipeline;
}


gpu::PipelinePointer _showLUTPipeline;
gpu::PipelinePointer getShowLUTPipeline();
gpu::PipelinePointer SubsurfaceScattering::getShowLUTPipeline() {
    if (!_showLUTPipeline) {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::StandardShaderLib::getDrawTextureOpaquePS();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
      //  slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), SubsurfaceScattering_FrameTransformSlot));
        // slotBindings.insert(gpu::Shader::Binding(std::string("sourceMap"), BlurTask_SourceSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the curvature pass for objects pixels only, not the background
        //  state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

        _showLUTPipeline = gpu::Pipeline::create(program, state);
    }

    return _showLUTPipeline;
}

bool SubsurfaceScattering::updateScatteringFramebuffer(const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& scatteringFramebuffer) {
    if (!sourceFramebuffer) {
        return false;
    }

    if (!_scatteringFramebuffer) {
        _scatteringFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());

        // attach depthStencil if present in source
        if (sourceFramebuffer->hasDepthStencil()) {
     //       _scatteringFramebuffer->setDepthStencilBuffer(sourceFramebuffer->getDepthStencilBuffer(), sourceFramebuffer->getDepthStencilBufferFormat());
        }
        auto blurringSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT);
        auto blurringTarget = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_SRGBA_32, sourceFramebuffer->getWidth(), sourceFramebuffer->getHeight(), blurringSampler));
        _scatteringFramebuffer->setRenderBuffer(0, blurringTarget);
    } else {
        // it would be easier to just call resize on the bluredFramebuffer and let it work if needed but the source might loose it's depth buffer when doing so
        if ((_scatteringFramebuffer->getWidth() != sourceFramebuffer->getWidth()) || (_scatteringFramebuffer->getHeight() != sourceFramebuffer->getHeight())) {
            _scatteringFramebuffer->resize(sourceFramebuffer->getWidth(), sourceFramebuffer->getHeight(), sourceFramebuffer->getNumSamples());
            if (sourceFramebuffer->hasDepthStencil()) {
           //     _scatteringFramebuffer->setDepthStencilBuffer(sourceFramebuffer->getDepthStencilBuffer(), sourceFramebuffer->getDepthStencilBufferFormat());
            }
        }
    }

    if (!scatteringFramebuffer) {
        scatteringFramebuffer = _scatteringFramebuffer;
    }

    return true;

}


void SubsurfaceScattering::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs, gpu::FramebufferPointer& scatteringFramebuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;

    _scatteringResource->generateScatteringTable(args);
    auto scatteringProfile = _scatteringResource->getScatteringProfile();
    auto scatteringTable = _scatteringResource->getScatteringTable();


    auto pipeline = getScatteringPipeline();
    
    auto& frameTransform = inputs.get0();
    auto& curvatureFramebuffer = inputs.get1();
    auto& diffusedFramebuffer = inputs.get2();
    

    auto framebufferCache = DependencyManager::get<FramebufferCache>();
 
 /*   if (!updateScatteringFramebuffer(curvatureFramebuffer, scatteringFramebuffer)) {
        return;
    }
*/
    const auto theLight = DependencyManager::get<DeferredLightingEffect>()->getLightStage().lights[0];
    
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
   //     batch.enableStereo(false);

        batch.setViewportTransform(args->_viewport);

   /*     batch.setFramebuffer(_scatteringFramebuffer);

        batch.setPipeline(pipeline);

        batch.setUniformBuffer(ScatteringTask_FrameTransformSlot, frameTransform->getFrameTransformBuffer());
        batch.setUniformBuffer(ScatteringTask_ParamSlot, _scatteringResource->getParametersBuffer());
        if (theLight->light) {
            batch.setUniformBuffer(ScatteringTask_LightSlot, theLight->light->getSchemaBuffer());
            if (theLight->light->getAmbientMap()) {
                batch.setResourceTexture(ScatteringTask_IBLMapSlot, theLight->light->getAmbientMap());
            }
        }
        batch.setResourceTexture(ScatteringTask_ScatteringTableSlot, scatteringTable);
        batch.setResourceTexture(ScatteringTask_CurvatureMapSlot, curvatureFramebuffer->getRenderBuffer(0));
        batch.setResourceTexture(ScatteringTask_DiffusedCurvatureMapSlot, diffusedFramebuffer->getRenderBuffer(0));
        batch.setResourceTexture(ScatteringTask_NormalMapSlot, framebufferCache->getDeferredNormalTexture());
        batch.setResourceTexture(ScatteringTask_AlbedoMapSlot, framebufferCache->getDeferredColorTexture());
        batch.setResourceTexture(ScatteringTask_LinearMapSlot, framebufferCache->getDepthPyramidTexture());

        batch.draw(gpu::TRIANGLE_STRIP, 4);
*/
        auto viewportSize = std::min(args->_viewport.z, args->_viewport.w) >> 1;
        auto offsetViewport = viewportSize * 0.1;

        if (_showProfile) {
            batch.setViewportTransform(glm::ivec4(0, 0, viewportSize, offsetViewport));
            batch.setPipeline(getShowLUTPipeline());
            batch.setResourceTexture(0, scatteringProfile);
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }

        if (_showLUT) {
            batch.setViewportTransform(glm::ivec4(0, offsetViewport * 1.5, viewportSize, viewportSize));
            batch.setPipeline(getShowLUTPipeline());
            batch.setResourceTexture(0, scatteringTable);
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }
    });
}





// Reference: http://www.altdevblogaday.com/2011/12/31/skin-shading-in-unity3d/
#include <cstdio>
#include <cmath>
#include <algorithm>

#define _PI 3.14159265358979523846

using namespace std;

double gaussian(float v, float r) {
    double g = (1.0 / sqrt(2.0 * _PI * v)) * exp(-(r*r) / (2.0 * v));
    return g;
}

vec3 scatter(double r) {
    // Values from GPU Gems 3 "Advanced Skin Rendering".
    // Originally taken from real life samples.
    static const double profile[][4] = {
        { 0.0064, 0.233, 0.455, 0.649 },
        { 0.0484, 0.100, 0.336, 0.344 },
        { 0.1870, 0.118, 0.198, 0.000 },
        { 0.5670, 0.113, 0.007, 0.007 },
        { 1.9900, 0.358, 0.004, 0.000 },
        { 7.4100, 0.078, 0.000, 0.000 }
    };
    static const int profileNum = 6;
    vec3 ret(0.0);
    for (int i = 0; i < profileNum; i++) {
        double g = gaussian(profile[i][0] * 1.414f, r);
        ret.x += g * profile[i][1];
        ret.y += g * profile[i][2];
        ret.z += g * profile[i][3];
    }

    return ret;
}

vec3 integrate(double cosTheta, double skinRadius) {
    // Angle from lighting direction.
    double theta = acos(cosTheta);
    vec3 totalWeights(0.0);
    vec3 totalLight(0.0);
    vec3 skinColour(1.0);

    double a = -(_PI);

    double inc = 0.005;

    while (a <= (_PI)) {
        double sampleAngle = theta + a;
        double diffuse = cos(sampleAngle);
        if (diffuse < 0.0) diffuse = 0.0;
        if (diffuse > 1.0) diffuse = 1.0;

        // Distance.
        double sampleDist = abs(2.0 * skinRadius * sin(a * 0.5));

        // Profile Weight.
        vec3 weights = scatter(sampleDist);

        totalWeights += weights;
        totalLight.x += diffuse * weights.x * (skinColour.x * skinColour.x);
        totalLight.y += diffuse * weights.y * (skinColour.y * skinColour.y);
        totalLight.z += diffuse * weights.z * (skinColour.z * skinColour.z);
        a += inc;
    }

    vec3 result;
    result.x = totalLight.x / totalWeights.x;
    result.y = totalLight.y / totalWeights.y;
    result.z = totalLight.z / totalWeights.z;

    return result;
}

void diffuseScatter(gpu::TexturePointer& lut) {
    int width = lut->getWidth();
    int height = lut->getHeight();
    
    const int COMPONENT_COUNT = 4;
    std::vector<unsigned char> bytes(COMPONENT_COUNT * height * width);
    
    int index = 0;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            // Lookup by: x: NDotL y: 1 / r
            float y = 2.0 * 1.0 / ((j + 1.0) / (double)height);
            float x = ((i / (double)width) * 2.0) - 1.0;
            vec3 val = integrate(x, y);

            // Convert to linear
           // val.x = sqrt(val.x);
           // val.y = sqrt(val.y);
           // val.z = sqrt(val.z);

            // Convert to 24-bit image.
            unsigned char valI[3];
            if (val.x > 1.0) val.x = 1.0;
            if (val.y > 1.0) val.y = 1.0;
            if (val.z > 1.0) val.z = 1.0;
            valI[0] = (unsigned char)(val.x * 256.0);
            valI[1] = (unsigned char)(val.y * 256.0);
            valI[2] = (unsigned char)(val.z * 256.0);
            
            bytes[COMPONENT_COUNT * index] = valI[0];
            bytes[COMPONENT_COUNT * index + 1] = valI[1];
            bytes[COMPONENT_COUNT * index + 2] = valI[2];
            bytes[COMPONENT_COUNT * index + 3] = 255.0;
            
            index++;
        }
    }

    lut->assignStoredMip(0, gpu::Element::COLOR_RGBA_32, bytes.size(), bytes.data());
}


void diffuseProfile(gpu::TexturePointer& profile) {
    int width = profile->getWidth();
    int height = profile->getHeight();
    
    const int COMPONENT_COUNT = 4;
    std::vector<unsigned char> bytes(COMPONENT_COUNT * height * width);

    int index = 0;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            float y = (double)(i + 1.0) / (double)width;
            vec3 val = scatter(y * 2.0f);

            // Convert to 24-bit image.
            unsigned char valI[3];
            if (val.x > 1.0) val.x = 1.0;
            if (val.y > 1.0) val.y = 1.0;
            if (val.z > 1.0) val.z = 1.0;
            valI[0] = (unsigned char)(val.x * 255.0);
            valI[1] = (unsigned char)(val.y * 255.0);
            valI[2] = (unsigned char)(val.z * 255.0);

            bytes[COMPONENT_COUNT * index] = valI[0];
            bytes[COMPONENT_COUNT * index + 1] = valI[1];
            bytes[COMPONENT_COUNT * index + 2] = valI[2];
            bytes[COMPONENT_COUNT * index + 3] = 255.0;

            index++;
        }
    }

    
    profile->assignStoredMip(0, gpu::Element::COLOR_RGBA_32, bytes.size(), bytes.data());
}

void diffuseProfileGPU(gpu::TexturePointer& profileMap, RenderArgs* args) {
    int width = profileMap->getWidth();
    int height = profileMap->getHeight();

    gpu::PipelinePointer makePipeline;
    {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(subsurfaceScattering_makeProfile_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        //slotBindings.insert(gpu::Shader::Binding(std::string("profileMap"), 0));
        // slotBindings.insert(gpu::Shader::Binding(std::string("sourceMap"), BlurTask_SourceSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        makePipeline = gpu::Pipeline::create(program, state);
    }

    auto makeFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    makeFramebuffer->setRenderBuffer(0, profileMap);

    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setViewportTransform(glm::ivec4(0, 0, width, height));

        batch.setFramebuffer(makeFramebuffer);
        batch.setPipeline(makePipeline);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
        batch.setResourceTexture(0, nullptr);
        batch.setPipeline(nullptr);
        batch.setFramebuffer(nullptr);
    });
}


void diffuseScatterGPU(const gpu::TexturePointer& profileMap, gpu::TexturePointer& lut, RenderArgs* args) {
    int width = lut->getWidth();
    int height = lut->getHeight();

    gpu::PipelinePointer makePipeline;
    {
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(subsurfaceScattering_makeLUT_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("scatteringProfile"), 0));
        // slotBindings.insert(gpu::Shader::Binding(std::string("sourceMap"), BlurTask_SourceSlot));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        makePipeline = gpu::Pipeline::create(program, state);
    }

    auto makeFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    makeFramebuffer->setRenderBuffer(0, lut);

    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setViewportTransform(glm::ivec4(0, 0, width, height));

        batch.setFramebuffer(makeFramebuffer);
        batch.setPipeline(makePipeline);
        batch.setResourceTexture(0, profileMap);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
        batch.setResourceTexture(0, nullptr);
        batch.setPipeline(nullptr);
        batch.setFramebuffer(nullptr);

    });
}

gpu::TexturePointer SubsurfaceScatteringResource::generateScatteringProfile(RenderArgs* args) {
    const int PROFILE_RESOLUTION = 512;
    auto profileMap = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_SRGBA_32, PROFILE_RESOLUTION, 1, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
    diffuseProfileGPU(profileMap, args);
    return profileMap;
}

gpu::TexturePointer SubsurfaceScatteringResource::generatePreIntegratedScattering(const gpu::TexturePointer& profile, RenderArgs* args) {

    const int TABLE_RESOLUTION = 512;
    auto scatteringLUT = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_SRGBA_32, TABLE_RESOLUTION, TABLE_RESOLUTION, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
    //diffuseScatter(scatteringLUT);
    diffuseScatterGPU(profile, scatteringLUT, args);
    return scatteringLUT;
}
