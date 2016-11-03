//
//  Material.cpp
//  libraries/model/src/model
//
//  Created by Sam Gateau on 12/10/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Material.h"

#include "TextureMap.h"

using namespace model;
using namespace gpu;

Material::Material() :
    _key(0),
    _schemaBuffer(),
    _texMapArrayBuffer(),
    _textureMaps()
{
    // created from nothing: create the Buffer to store the properties
    Schema schema;
    _schemaBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Schema), (const gpu::Byte*) &schema));

    TexMapArraySchema TexMapArraySchema;
    _texMapArrayBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(TexMapArraySchema), (const gpu::Byte*) &TexMapArraySchema));
}

Material::Material(const Material& material) :
    _key(material._key),
    _textureMaps(material._textureMaps)
{
    // copied: create the Buffer to store the properties, avoid holding a ref to the old Buffer
    Schema schema;
    _schemaBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Schema), (const gpu::Byte*) &schema));
    _schemaBuffer.edit<Schema>() = material._schemaBuffer.get<Schema>();

    TexMapArraySchema texMapArraySchema;
    _texMapArrayBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(TexMapArraySchema), (const gpu::Byte*) &texMapArraySchema));
    _texMapArrayBuffer.edit<TexMapArraySchema>() = material._texMapArrayBuffer.get<TexMapArraySchema>();
}

Material& Material::operator= (const Material& material) {
    QMutexLocker locker(&_textureMapsMutex);

    _key = (material._key);
    _textureMaps = (material._textureMaps);
    _hasCalculatedTextureInfo = false;

    // copied: create the Buffer to store the properties, avoid holding a ref to the old Buffer
    Schema schema;
    _schemaBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Schema), (const gpu::Byte*) &schema));
    _schemaBuffer.edit<Schema>() = material._schemaBuffer.get<Schema>();

    TexMapArraySchema texMapArraySchema;
    _texMapArrayBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(TexMapArraySchema), (const gpu::Byte*) &texMapArraySchema));
    _texMapArrayBuffer.edit<TexMapArraySchema>() = material._texMapArrayBuffer.get<TexMapArraySchema>();

    return (*this);
}

Material::~Material() {
}

void Material::setEmissive(const Color&  emissive, bool isSRGB) {
    _key.setEmissive(glm::any(glm::greaterThan(emissive, Color(0.0f))));
    _schemaBuffer.edit<Schema>()._key = (uint32) _key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._emissive = (isSRGB ? ColorUtils::sRGBToLinearVec3(emissive) : emissive);
}

void Material::setOpacity(float opacity) {
    _key.setTranslucentFactor((opacity < 1.0f));
    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._opacity = opacity;
}

void Material::setUnlit(bool value) {
    _key.setUnlit(value);
    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
}

void Material::setAlbedo(const Color& albedo, bool isSRGB) {
    _key.setAlbedo(glm::any(glm::greaterThan(albedo, Color(0.0f))));
    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._albedo = (isSRGB ? ColorUtils::sRGBToLinearVec3(albedo) : albedo);
}

void Material::setRoughness(float roughness) {
    roughness = std::min(1.0f, std::max(roughness, 0.0f));
    _key.setGlossy((roughness < 1.0f));
    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._roughness = roughness;
}

void Material::setFresnel(const Color& fresnel, bool isSRGB) {
    //_key.setAlbedo(glm::any(glm::greaterThan(albedo, Color(0.0f))));
    _schemaBuffer.edit<Schema>()._fresnel = (isSRGB ? ColorUtils::sRGBToLinearVec3(fresnel) : fresnel);
}

void Material::setMetallic(float metallic) {
    metallic = glm::clamp(metallic, 0.0f, 1.0f);
    _key.setMetallic(metallic > 0.0f);
    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._metallic = metallic;
}

void Material::setScattering(float scattering) {
    scattering = glm::clamp(scattering, 0.0f, 1.0f);
    _key.setMetallic(scattering > 0.0f);
    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._scattering = scattering;
}

void Material::setTextureMap(MapChannel channel, const TextureMapPointer& textureMap) {
    QMutexLocker locker(&_textureMapsMutex);

    if (textureMap) {
        _key.setMapChannel(channel, (true));
        _textureMaps[channel] = textureMap;
    } else {
        _key.setMapChannel(channel, (false));
        _textureMaps.erase(channel);
    }
    _hasCalculatedTextureInfo = false;

    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();

    if (channel == MaterialKey::ALBEDO_MAP) {
        resetOpacityMap();

        // update the texcoord0 with albedo
        _texMapArrayBuffer.edit<TexMapArraySchema>()._texcoordTransforms[0] = (textureMap ? textureMap->getTextureTransform().getMatrix() : glm::mat4());
    }

    if (channel == MaterialKey::OCCLUSION_MAP) {
        _texMapArrayBuffer.edit<TexMapArraySchema>()._texcoordTransforms[1] = (textureMap ? textureMap->getTextureTransform().getMatrix() : glm::mat4());
    }

    if (channel == MaterialKey::LIGHTMAP_MAP) {
        // update the texcoord1 with lightmap
        _texMapArrayBuffer.edit<TexMapArraySchema>()._texcoordTransforms[1] = (textureMap ? textureMap->getTextureTransform().getMatrix() : glm::mat4());
        _texMapArrayBuffer.edit<TexMapArraySchema>()._lightmapParams = (textureMap ? glm::vec4(textureMap->getLightmapOffsetScale(), 0.0, 0.0) : glm::vec4(0.0, 1.0, 0.0, 0.0));
    }

    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();

}

void Material::resetOpacityMap() const {
    // Clear the previous flags
    _key.setOpacityMaskMap(false);
    _key.setTranslucentMap(false);

    const auto& textureMap = getTextureMap(MaterialKey::ALBEDO_MAP);
    if (textureMap &&
        textureMap->useAlphaChannel() &&
        textureMap->isDefined() &&
        textureMap->getTextureView().isValid()) {

        auto usage = textureMap->getTextureView()._texture->getUsage();
        if (usage.isAlpha()) {
            if (usage.isAlphaMask()) {
                // Texture has alpha, but it is just a mask
                _key.setOpacityMaskMap(true);
                _key.setTranslucentMap(false);
            } else {
                // Texture has alpha, it is a true translucency channel
                _key.setOpacityMaskMap(false);
                _key.setTranslucentMap(true);
            }
        }
    }

    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
}


const TextureMapPointer Material::getTextureMap(MapChannel channel) const {
    QMutexLocker locker(&_textureMapsMutex);

    auto result = _textureMaps.find(channel);
    if (result != _textureMaps.end()) {
        return (result->second);
    } else {
        return TextureMapPointer();
    }
}


bool Material::calculateMaterialInfo() const {
    if (!_hasCalculatedTextureInfo) {
        QMutexLocker locker(&_textureMapsMutex);

        bool allTextures = true; // assume we got this...
        _textureSize = 0;
        _textureCount = 0;

        for (auto const &textureMapItem : _textureMaps) {
            auto textureMap = textureMapItem.second;
            if (textureMap) {
                auto textureSoure = textureMap->getTextureSource();
                if (textureSoure) {
                    auto texture = textureSoure->getGPUTexture();
                    if (texture) {
                        auto size = texture->getSize();
                        _textureSize += size;
                        _textureCount++;
                    } else {
                        allTextures = false;
                    }
                } else {
                    allTextures = false;
                }
            } else {
                allTextures = false;
            }
        }
        _hasCalculatedTextureInfo = allTextures;
    }
    return _hasCalculatedTextureInfo;
}
