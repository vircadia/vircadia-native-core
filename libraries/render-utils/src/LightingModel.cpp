//
//  LightingModel.cpp
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 7/1/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "LightingModel.h"
#include "RandomAndNoise.h"
#include "BRDF.h"

#include "render-utils/ShaderConstants.h"

#include <TBBHelpers.h>

gpu::TexturePointer LightingModel::_ambientFresnelLUT;

LightingModel::LightingModel() {
    Parameters parameters;
    _parametersBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Parameters), (const gpu::Byte*) &parameters, sizeof(Parameters)));

#if RENDER_UTILS_ENABLE_AMBIENT_FRESNEL_LUT
    if (!_ambientFresnelLUT) {
        // Code taken from the IntegrateBRDF method as described in this talk :
        // https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
        const auto N_roughness = 32;
        const auto N_NdotV = 256;

        using LUTVector = std::vector<glm::u16vec2>;
        using LUTValueType = LUTVector::value_type::value_type;

        LUTVector lut(N_roughness * N_NdotV);

        _ambientFresnelLUT = gpu::Texture::create2D(gpu::Element{ gpu::VEC2, gpu::NUINT16, gpu::XY }, N_roughness, N_NdotV, 1U,
                                                    gpu::Sampler(gpu::Sampler::FILTER_MIN_POINT_MAG_LINEAR, gpu::Sampler::WRAP_CLAMP));

        tbb::parallel_for(tbb::blocked_range2d<int, int>(0, N_NdotV, 8, 0, N_roughness, 8), [&](const tbb::blocked_range2d<int, int>& range) {
            auto roughnessRange = range.cols();
            auto ndotvRange = range.rows();

            for (auto j = ndotvRange.begin(); j < ndotvRange.end(); j++) {
                const float NdotV = j / float(N_NdotV - 1);

                glm::vec3 V;
                V.x = std::sqrt(1.0f - NdotV * NdotV);  // sin
                V.y = 0;
                V.z = NdotV;                            // cos

                for (auto k = roughnessRange.begin(); k < roughnessRange.end(); k++) {
                    const float roughness = k / float(N_roughness - 1);
                    const float alpha = roughness * roughness;
                    const float alphaSquared = alpha * alpha;

                    float A = 0.0f;
                    float B = 0.0f;

                    const uint NumSamples = 1024;
                    for (uint i = 0; i < NumSamples; i++) {
                        glm::vec2 Xi = hammersley::evaluate(i, NumSamples);
                        glm::vec3 H = ggx::sample(Xi, roughness);
                        float VdotH = glm::dot(V, H);
                        glm::vec3 L = 2.0f * VdotH * H - V;
                        float NdotL = L.z;

                        if (NdotL > 0.0f) {
                            VdotH = glm::clamp(VdotH, 0.0f, 1.0f);

                            float NdotH = glm::clamp(H.z, 0.0f, 1.0f);
                            float G = smith::evaluateFastWithoutNdotV(alphaSquared, NdotV, NdotL);
                            float G_Vis = (G * VdotH) / NdotH;
                            float Fc = std::pow(1.0f - VdotH, 5.0f);

                            A += (1.0f - Fc) * G_Vis;
                            B += Fc * G_Vis;
                        }
                    }

                    A /= NumSamples;
                    B /= NumSamples;

                    auto& lutValue = lut[k + j * N_roughness];
                    lutValue.x = (LUTValueType)(glm::min(1.0f, A) * std::numeric_limits<LUTValueType>::max());
                    lutValue.y = (LUTValueType)(glm::min(1.0f, B) * std::numeric_limits<LUTValueType>::max());
                }
            }
        });

        _ambientFresnelLUT->assignStoredMip(0, N_roughness * N_NdotV * sizeof(LUTVector::value_type), (const gpu::Byte*)lut.data());
    }
#endif
}

void LightingModel::setUnlit(bool enable) {
    if (enable != isUnlitEnabled()) {
        _parametersBuffer.edit<Parameters>().enableUnlit = (float) enable;
    }
}
bool LightingModel::isUnlitEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableUnlit;
}

void LightingModel::setEmissive(bool enable) {
    if (enable != isEmissiveEnabled()) {
        _parametersBuffer.edit<Parameters>().enableEmissive = (float)enable;
    }
}
bool LightingModel::isEmissiveEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableEmissive;
}
void LightingModel::setLightmap(bool enable) {
    if (enable != isLightmapEnabled()) {
        _parametersBuffer.edit<Parameters>().enableLightmap = (float)enable;
    }
}
bool LightingModel::isLightmapEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableLightmap;
}

void LightingModel::setBackground(bool enable) {
    if (enable != isBackgroundEnabled()) {
        _parametersBuffer.edit<Parameters>().enableBackground = (float)enable;
    }
}
bool LightingModel::isBackgroundEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableBackground;
}
void LightingModel::setHaze(bool enable) {
    if (enable != isHazeEnabled()) {
        _parametersBuffer.edit<Parameters>().enableHaze = (float)enable;
    }
}
bool LightingModel::isHazeEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableHaze;
}
void LightingModel::setObscurance(bool enable) {
    if (enable != isObscuranceEnabled()) {
        _parametersBuffer.edit<Parameters>().enableObscurance = (float)enable;
    }
}
bool LightingModel::isObscuranceEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableObscurance;
}

void LightingModel::setScattering(bool enable) {
    if (enable != isScatteringEnabled()) {
        _parametersBuffer.edit<Parameters>().enableScattering = (float)enable;
    }
}
bool LightingModel::isScatteringEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableScattering;
}

void LightingModel::setDiffuse(bool enable) {
    if (enable != isDiffuseEnabled()) {
        _parametersBuffer.edit<Parameters>().enableDiffuse = (float)enable;
    }
}
bool LightingModel::isDiffuseEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableDiffuse;
}
void LightingModel::setSpecular(bool enable) {
    if (enable != isSpecularEnabled()) {
        _parametersBuffer.edit<Parameters>().enableSpecular = (float)enable;
    }
}
bool LightingModel::isSpecularEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableSpecular;
}
void LightingModel::setAlbedo(bool enable) {
    if (enable != isAlbedoEnabled()) {
        _parametersBuffer.edit<Parameters>().enableAlbedo = (float)enable;
    }
}
bool LightingModel::isAlbedoEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableAlbedo;
}

void LightingModel::setMaterialTexturing(bool enable) {
    if (enable != isMaterialTexturingEnabled()) {
        _parametersBuffer.edit<Parameters>().enableMaterialTexturing = (float)enable;
    }
}
bool LightingModel::isMaterialTexturingEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableMaterialTexturing;
}

void LightingModel::setAmbientLight(bool enable) {
    if (enable != isAmbientLightEnabled()) {
        _parametersBuffer.edit<Parameters>().enableAmbientLight = (float)enable;
    }
}
bool LightingModel::isAmbientLightEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableAmbientLight;
}
void LightingModel::setDirectionalLight(bool enable) {
    if (enable != isDirectionalLightEnabled()) {
        _parametersBuffer.edit<Parameters>().enableDirectionalLight = (float)enable;
    }
}
bool LightingModel::isDirectionalLightEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableDirectionalLight;
}
void LightingModel::setPointLight(bool enable) {
    if (enable != isPointLightEnabled()) {
        _parametersBuffer.edit<Parameters>().enablePointLight = (float)enable;
    }
}
bool LightingModel::isPointLightEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enablePointLight;
}
void LightingModel::setSpotLight(bool enable) {
    if (enable != isSpotLightEnabled()) {
        _parametersBuffer.edit<Parameters>().enableSpotLight = (float)enable;
    }
}
bool LightingModel::isSpotLightEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableSpotLight;
}

void LightingModel::setShowLightContour(bool enable) {
    if (enable != isShowLightContourEnabled()) {
        _parametersBuffer.edit<Parameters>().showLightContour = (float)enable;
    }
}
bool LightingModel::isShowLightContourEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().showLightContour;
}

void LightingModel::setWireframe(bool enable) {
    if (enable != isWireframeEnabled()) {
        _parametersBuffer.edit<Parameters>().enableWireframe = (float)enable;
    }
}
bool LightingModel::isWireframeEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableWireframe;
}

void LightingModel::setBloom(bool enable) {
    if (enable != isBloomEnabled()) {
        _parametersBuffer.edit<Parameters>().enableBloom = (float)enable;
    }
}
bool LightingModel::isBloomEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableBloom;
}

void LightingModel::setSkinning(bool enable) {
    if (enable != isSkinningEnabled()) {
        _parametersBuffer.edit<Parameters>().enableSkinning = (float)enable;
    }
}
bool LightingModel::isSkinningEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableSkinning;
}

void LightingModel::setBlendshape(bool enable) {
    if (enable != isBlendshapeEnabled()) {
        _parametersBuffer.edit<Parameters>().enableBlendshape = (float)enable;
    }
}
bool LightingModel::isBlendshapeEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableBlendshape;
}

void LightingModel::setAmbientOcclusion(bool enable) {
    if (enable != isAmbientOcclusionEnabled()) {
        _parametersBuffer.edit<Parameters>().enableAmbientOcclusion = (float)enable;
    }
}
bool LightingModel::isAmbientOcclusionEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableAmbientOcclusion;
}

void LightingModel::setShadow(bool enable) {
    if (enable != isShadowEnabled()) {
        _parametersBuffer.edit<Parameters>().enableShadow = (float)enable;
    }
}
bool LightingModel::isShadowEnabled() const {
    return (bool)_parametersBuffer.get<Parameters>().enableShadow;
}

MakeLightingModel::MakeLightingModel() {
    _lightingModel = std::make_shared<LightingModel>();
}

void MakeLightingModel::configure(const Config& config) {
    _lightingModel->setUnlit(config.enableUnlit);
    _lightingModel->setEmissive(config.enableEmissive);
    _lightingModel->setLightmap(config.enableLightmap);
    _lightingModel->setBackground(config.enableBackground);
    _lightingModel->setHaze(config.enableHaze);
    _lightingModel->setBloom(config.enableBloom);

    _lightingModel->setObscurance(config.enableObscurance);

    _lightingModel->setScattering(config.enableScattering);
    _lightingModel->setDiffuse(config.enableDiffuse);
    _lightingModel->setSpecular(config.enableSpecular);
    _lightingModel->setAlbedo(config.enableAlbedo);

    _lightingModel->setMaterialTexturing(config.enableMaterialTexturing);

    _lightingModel->setAmbientLight(config.enableAmbientLight);
    _lightingModel->setDirectionalLight(config.enableDirectionalLight);
    _lightingModel->setPointLight(config.enablePointLight);
    _lightingModel->setSpotLight(config.enableSpotLight);

    _lightingModel->setShowLightContour(config.showLightContour);
    _lightingModel->setWireframe(config.enableWireframe);

    _lightingModel->setSkinning(config.enableSkinning);
    _lightingModel->setBlendshape(config.enableBlendshape);

    _lightingModel->setAmbientOcclusion(config.enableAmbientOcclusion);
    _lightingModel->setShadow(config.enableShadow);
}

void MakeLightingModel::run(const render::RenderContextPointer& renderContext, LightingModelPointer& lightingModel) {

    lightingModel = _lightingModel;

    // make sure the enableTexturing flag of the render ARgs is in sync
    renderContext->args->_enableTexturing = _lightingModel->isMaterialTexturingEnabled();
    renderContext->args->_enableBlendshape = _lightingModel->isBlendshapeEnabled();
    renderContext->args->_enableSkinning = _lightingModel->isSkinningEnabled();
}