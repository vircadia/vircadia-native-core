//
//  Created by HifiExperiments on 3/14/2021
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ReferenceMaterial.h"

std::function<graphics::MaterialPointer(QUuid)> ReferenceMaterial::_unboundMaterialForUUIDOperator = nullptr;

ReferenceMaterial::ReferenceMaterial(QUuid uuid) :
    graphics::ProceduralMaterial() {
    if (_unboundMaterialForUUIDOperator) {
        _materialForUUIDOperator = std::bind(_unboundMaterialForUUIDOperator, uuid);
    }
}

// Material
const graphics::MaterialKey& ReferenceMaterial::getKey() const {
    auto material = getMaterial();
    return material ? material->getKey() : Parent::getKey();
}

glm::vec3 ReferenceMaterial::getEmissive(bool SRGB) const {
    auto material = getMaterial();
    return material ? material->getEmissive(SRGB) : glm::vec3(DEFAULT_EMISSIVE);
}

float ReferenceMaterial::getOpacity() const {
    auto material = getMaterial();
    return material ? material->getOpacity() : DEFAULT_OPACITY;
}

graphics::MaterialKey::OpacityMapMode ReferenceMaterial::getOpacityMapMode() const {
    auto material = getMaterial();
    return material ? material->getOpacityMapMode() : DEFAULT_OPACITY_MAP_MODE;
}

float ReferenceMaterial::getOpacityCutoff() const {
    auto material = getMaterial();
    return material ? material->getOpacityCutoff() : DEFAULT_OPACITY_CUTOFF;
}

graphics::MaterialKey::CullFaceMode ReferenceMaterial::getCullFaceMode() const {
    auto material = getMaterial();
    return material ? material->getCullFaceMode() : DEFAULT_CULL_FACE_MODE;
}

bool ReferenceMaterial::isUnlit() const {
    auto material = getMaterial();
    return material ? material->isUnlit() : false;
}

glm::vec3 ReferenceMaterial::getAlbedo(bool SRGB) const {
    auto material = getMaterial();
    return material ? material->getAlbedo(SRGB) : glm::vec3(DEFAULT_ALBEDO);
}

float ReferenceMaterial::getMetallic() const {
    auto material = getMaterial();
    return material ? material->getMetallic() : DEFAULT_METALLIC;
}

float ReferenceMaterial::getRoughness() const {
    auto material = getMaterial();
    return material ? material->getRoughness() : DEFAULT_ROUGHNESS;
}

float ReferenceMaterial::getScattering() const {
    auto material = getMaterial();
    return material ? material->getScattering() : DEFAULT_SCATTERING;
}

bool ReferenceMaterial::resetOpacityMap() const {
    auto material = getMaterial();
    return material ? material->resetOpacityMap() : false;
}

const graphics::Material::TextureMaps& ReferenceMaterial::getTextureMaps() const {
    auto material = getMaterial();
    return material ? material->getTextureMaps() : Parent::getTextureMaps();
}

glm::vec2 ReferenceMaterial::getLightmapParams() const {
    auto material = getMaterial();
    return material ? material->getLightmapParams() : glm::vec2(0.0f, 1.0f);
}

bool ReferenceMaterial::getDefaultFallthrough() const {
    auto material = getMaterial();
    return material ? material->getDefaultFallthrough() : false;
}

// NetworkMaterial
bool ReferenceMaterial::isMissingTexture() {
    auto material = getNetworkMaterial();
    return material ? material->isMissingTexture() : false;
}

bool ReferenceMaterial::checkResetOpacityMap() {
    auto material = getNetworkMaterial();
    return material ? material->checkResetOpacityMap() : false;
}

// ProceduralMaterial
bool ReferenceMaterial::isProcedural() const {
    auto material = getMaterial();
    return material ? material->isProcedural() : false;
}

bool ReferenceMaterial::isEnabled() const {
    auto material = getMaterial();
    return material ? material->isEnabled() : false;
}

bool ReferenceMaterial::isReady() const {
    auto material = getMaterial();
    return material ? material->isReady() : false;
}

QString ReferenceMaterial::getProceduralString() const {
    auto material = getMaterial();
    return material ? material->getProceduralString() : QString();
}

glm::vec4 ReferenceMaterial::getColor(const glm::vec4& color) const {
    auto material = getProceduralMaterial();
    return material ? material->getColor(color) : glm::vec4();
}

bool ReferenceMaterial::isFading() const {
    auto material = getProceduralMaterial();
    return material ? material->isFading() : false;
}

uint64_t ReferenceMaterial::getFadeStartTime() const {
    auto material = getProceduralMaterial();
    return material ? material->getFadeStartTime() : 0;
}

bool ReferenceMaterial::hasVertexShader() const {
    auto material = getProceduralMaterial();
    return material ? material->hasVertexShader() : false;
}

void ReferenceMaterial::prepare(gpu::Batch& batch, const glm::vec3& position, const glm::vec3& size, const glm::quat& orientation,
                const uint64_t& created, const ProceduralProgramKey key) {
    if (auto material = getProceduralMaterial()) {
        material->prepare(batch, position, size, orientation, created, key);
    }
}

void ReferenceMaterial::initializeProcedural() {
    if (auto material = getProceduralMaterial()) {
        material->initializeProcedural();
    }
}

graphics::MaterialPointer ReferenceMaterial::getMaterial() const {
    if (_materialForUUIDOperator) {
        return _materialForUUIDOperator();
    }
    return nullptr;
}

std::shared_ptr<NetworkMaterial> ReferenceMaterial::getNetworkMaterial() const {
    if (_materialForUUIDOperator) {
        auto material = _materialForUUIDOperator();
        if (material && material->isProcedural()) {
            return std::static_pointer_cast<NetworkMaterial>(material);
        }
    }
    return nullptr;
}

graphics::ProceduralMaterialPointer ReferenceMaterial::getProceduralMaterial() const {
    if (_materialForUUIDOperator) {
        auto material = _materialForUUIDOperator();
        if (material && material->isProcedural()) {
            return std::static_pointer_cast<graphics::ProceduralMaterial>(material);
        }
    }
    return nullptr;
}
