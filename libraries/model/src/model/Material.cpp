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
    _textureMaps() {
       
        // only if created from nothing shall we create the Buffer to store the properties
        Schema schema;
        _schemaBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Schema), (const gpu::Byte*) &schema));
        

}

Material::Material(const Material& material) :
    _key(material._key),
    _schemaBuffer(material._schemaBuffer),
    _textureMaps(material._textureMaps) {
}

Material& Material::operator= (const Material& material) {
    _key = (material._key);
    _schemaBuffer = (material._schemaBuffer);
    _textureMaps = (material._textureMaps);

    return (*this);
}

Material::~Material() {
}

void Material::setEmissive(const Color&  emissive, bool isSRGB) {
    _key.setEmissive(glm::any(glm::greaterThan(emissive, Color(0.0f))));
    _schemaBuffer.edit<Schema>()._key = (uint32) _key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._emissive = (isSRGB ? ColorUtils::toLinearVec3(emissive) : emissive);
}

void Material::setOpacity(float opacity) {
    _key.setTranslucentFactor((opacity < 1.0f));
    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._opacity = opacity;
}

void Material::setAlbedo(const Color& albedo, bool isSRGB) {
    _key.setAlbedo(glm::any(glm::greaterThan(albedo, Color(0.0f))));
    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._albedo = (isSRGB ? ColorUtils::toLinearVec3(albedo) : albedo);
}

void Material::setRoughness(float roughness) {
    roughness = std::min(1.0f, std::max(roughness, 0.0f));
    _key.setGlossy((roughness < 1.0f));
    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._roughness = roughness;
}

void Material::setFresnel(const Color& fresnel, bool isSRGB) {
    //_key.setAlbedo(glm::any(glm::greaterThan(albedo, Color(0.0f))));
    _schemaBuffer.edit<Schema>()._fresnel = (isSRGB ? ColorUtils::toLinearVec3(fresnel) : fresnel);
}

void Material::setMetallic(float metallic) {
    _key.setMetallic(metallic > 0.0f);
    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();
    _schemaBuffer.edit<Schema>()._metallic = metallic;
}

void Material::setTextureMap(MapChannel channel, const TextureMapPointer& textureMap) {
    if (textureMap) {
        _key.setMapChannel(channel, (true));
 
        if (channel == MaterialKey::ALBEDO_MAP) {
            // clear the previous flags whatever they were:
            _key.setOpacityMaskMap(false);
            _key.setTranslucentMap(false);

            if (textureMap->useAlphaChannel() && textureMap->isDefined() && textureMap->getTextureView().isValid()) {
                auto usage = textureMap->getTextureView()._texture->getUsage();
                if (usage.isAlpha()) {
                    // Texture has alpha, is not just a mask or a true transparent channel
                    if (usage.isAlphaMask()) {
                        _key.setOpacityMaskMap(true);
                        _key.setTranslucentMap(false);
                    } else {
                        _key.setOpacityMaskMap(false);
                        _key.setTranslucentMap(true);
                    }
                }
            }
        }

        _textureMaps[channel] = textureMap;
    } else {
        _key.setMapChannel(channel, (false));

        if (channel == MaterialKey::ALBEDO_MAP) {
            _key.setOpacityMaskMap(false);
            _key.setTranslucentMap(false);
        }

        _textureMaps.erase(channel);
    }

    _schemaBuffer.edit<Schema>()._key = (uint32)_key._flags.to_ulong();

}


const TextureMapPointer Material::getTextureMap(MapChannel channel) const {
    auto result = _textureMaps.find(channel);
    if (result != _textureMaps.end()) {
        return (result->second);
    } else {
        return TextureMapPointer();
    }
}
