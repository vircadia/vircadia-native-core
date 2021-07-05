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
graphics::MaterialKey ReferenceMaterial::getKey() const {
    return resultWithLock<graphics::MaterialKey>([&] {
        auto material = getMaterial();
        return material ? material->getKey() : Parent::getKey();
    });
}

glm::vec3 ReferenceMaterial::getEmissive(bool SRGB) const {
    return resultWithLock<glm::vec3>([&] {
        auto material = getMaterial();
        return material ? material->getEmissive(SRGB) : glm::vec3(DEFAULT_EMISSIVE);
    });
}

float ReferenceMaterial::getOpacity() const {
    return resultWithLock<float>([&] {
        auto material = getMaterial();
        return material ? material->getOpacity() : DEFAULT_OPACITY;
    });
}

graphics::MaterialKey::OpacityMapMode ReferenceMaterial::getOpacityMapMode() const {
    return resultWithLock<graphics::MaterialKey::OpacityMapMode>([&] {
        auto material = getMaterial();
        return material ? material->getOpacityMapMode() : DEFAULT_OPACITY_MAP_MODE;
    });
}

float ReferenceMaterial::getOpacityCutoff() const {
    return resultWithLock<float>([&] {
        auto material = getMaterial();
        return material ? material->getOpacityCutoff() : DEFAULT_OPACITY_CUTOFF;
    });
}

graphics::MaterialKey::CullFaceMode ReferenceMaterial::getCullFaceMode() const {
    return resultWithLock<graphics::MaterialKey::CullFaceMode>([&] {
        auto material = getMaterial();
        return material ? material->getCullFaceMode() : DEFAULT_CULL_FACE_MODE;
    });
}

bool ReferenceMaterial::isUnlit() const {
    return resultWithLock<bool>([&] {
        auto material = getMaterial();
        return material ? material->isUnlit() : false;
    });
}

glm::vec3 ReferenceMaterial::getAlbedo(bool SRGB) const {
    return resultWithLock<glm::vec3>([&] {
        auto material = getMaterial();
        return material ? material->getAlbedo(SRGB) : glm::vec3(DEFAULT_ALBEDO);
    });
}

float ReferenceMaterial::getMetallic() const {
    return resultWithLock<float>([&] {
        auto material = getMaterial();
        return material ? material->getMetallic() : DEFAULT_METALLIC;
    });
}

float ReferenceMaterial::getRoughness() const {
    return resultWithLock<float>([&] {
        auto material = getMaterial();
        return material ? material->getRoughness() : DEFAULT_ROUGHNESS;
    });
}

float ReferenceMaterial::getScattering() const {
    return resultWithLock<float>([&] {
        auto material = getMaterial();
        return material ? material->getScattering() : DEFAULT_SCATTERING;
    });
}

bool ReferenceMaterial::resetOpacityMap() const {
    return resultWithLock<bool>([&] {
        auto material = getMaterial();
        return material ? material->resetOpacityMap() : false;
    });
}

graphics::Material::TextureMaps ReferenceMaterial::getTextureMaps() const {
    return resultWithLock<graphics::Material::TextureMaps>([&] {
        auto material = getMaterial();
        return material ? material->getTextureMaps() : Parent::getTextureMaps();
    });
} 

glm::vec2 ReferenceMaterial::getLightmapParams() const {
    return resultWithLock<glm::vec2>([&] {
        auto material = getMaterial();
        return material ? material->getLightmapParams() : glm::vec2(0.0f, 1.0f);
    });
}

bool ReferenceMaterial::getDefaultFallthrough() const {
    return resultWithLock<bool>([&] {
        auto material = getMaterial();
        return material ? material->getDefaultFallthrough() : false;
    });
}

// NetworkMaterial
bool ReferenceMaterial::isMissingTexture() {
    return resultWithLock<bool>([&] {
        auto material = getNetworkMaterial();
        return material ? material->isMissingTexture() : false;
    });
}

bool ReferenceMaterial::checkResetOpacityMap() {
    return resultWithLock<bool>([&] {
        auto material = getNetworkMaterial();
        return material ? material->checkResetOpacityMap() : false;
    });
}

// ProceduralMaterial
bool ReferenceMaterial::isProcedural() const {
    return resultWithLock<bool>([&] {
        auto material = getMaterial();
        return material ? material->isProcedural() : false;
    });
}

bool ReferenceMaterial::isEnabled() const {
    return resultWithLock<bool>([&] {
        auto material = getMaterial();
        return material ? material->isEnabled() : false;
    });
}

bool ReferenceMaterial::isReady() const {
    return resultWithLock<bool>([&] {
        auto material = getMaterial();
        return material ? material->isReady() : false;
    });
}

QString ReferenceMaterial::getProceduralString() const {
    return resultWithLock<QString>([&] {
        auto material = getMaterial();
        return material ? material->getProceduralString() : QString();
    });
}

glm::vec4 ReferenceMaterial::getColor(const glm::vec4& color) const {
    return resultWithLock<glm::vec4>([&] {
        auto material = getProceduralMaterial();
        return material ? material->getColor(color) : glm::vec4();
    });
}

bool ReferenceMaterial::isFading() const {
    return resultWithLock<bool>([&] {
        auto material = getProceduralMaterial();
        return material ? material->isFading() : false;
    });
}

uint64_t ReferenceMaterial::getFadeStartTime() const {
    return resultWithLock<uint64_t>([&] {
        auto material = getProceduralMaterial();
        return material ? material->getFadeStartTime() : 0;
    });
}

bool ReferenceMaterial::hasVertexShader() const {
    return resultWithLock<bool>([&] {
        auto material = getProceduralMaterial();
        return material ? material->hasVertexShader() : false;
    });
}

void ReferenceMaterial::prepare(gpu::Batch& batch, const glm::vec3& position, const glm::vec3& size, const glm::quat& orientation,
                                const uint64_t& created, const ProceduralProgramKey key) {
    withLock([&] {
        if (auto material = getProceduralMaterial()) {
            material->prepare(batch, position, size, orientation, created, key);
        }
    });
}

void ReferenceMaterial::initializeProcedural() {
    withLock([&] {
        if (auto material = getProceduralMaterial()) {
            material->initializeProcedural();
        }
    });
}

void ReferenceMaterial::setMaterialForUUIDOperator(std::function<graphics::MaterialPointer(QUuid)> materialForUUIDOperator) {
    _unboundMaterialForUUIDOperator = materialForUUIDOperator;
}

graphics::MaterialPointer ReferenceMaterial::getMaterial() const {
    if (_materialForUUIDOperator) {
        auto material = _materialForUUIDOperator();
        return material;
    }
    return nullptr;
}

std::shared_ptr<NetworkMaterial> ReferenceMaterial::getNetworkMaterial() const {
    if (_materialForUUIDOperator) {
        std::shared_ptr<NetworkMaterial> result = nullptr;
        if (auto material = _materialForUUIDOperator()) {
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

template <typename T, typename F>
inline T ReferenceMaterial::resultWithLock(F&& f) const {
    if (_locked) {
        return T();
    } else {
        _locked = true;
        T result = f();
        _locked = false;
        return result;
    }
}

template <typename F>
inline void ReferenceMaterial::withLock(F&& f) const {
    if (!_locked) {
        _locked = true;
        f();
        _locked = false;
    }
}
