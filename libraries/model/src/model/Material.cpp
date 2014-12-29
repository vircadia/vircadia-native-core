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

using namespace model;

Material::Material() :
    _flags(0),
    _schemaBuffer(),
    _textureMap() {
       
        // only if created from nothing shall we create the Buffer to store the properties
        Schema schema;
        _schemaBuffer = gpu::BufferView(new gpu::Buffer(sizeof(Schema), (const gpu::Buffer::Byte*) &schema));
        

}

Material::Material(const Material& material) :
    _flags(material._flags),
    _schemaBuffer(material._schemaBuffer),
    _textureMap(material._textureMap) {
}

Material& Material::operator= (const Material& material) {
    _flags = (material._flags);
    _schemaBuffer = (material._schemaBuffer);
    _textureMap = (material._textureMap);

    return (*this);
}

Material::~Material() {
}

void Material::setDiffuse(const Color& diffuse) {
    if (glm::any(glm::greaterThan(diffuse, Color(0.0f)))) {
        _flags.set(DIFFUSE_BIT);
    } else {
        _flags.reset(DIFFUSE_BIT);
    }
    _schemaBuffer.edit<Schema>()._diffuse = diffuse;
}

void Material::setSpecular(const Color& specular) {
    if (glm::any(glm::greaterThan(specular, Color(0.0f)))) {
        _flags.set(SPECULAR_BIT);
    } else {
        _flags.reset(SPECULAR_BIT);
    }
    _schemaBuffer.edit<Schema>()._specular = specular;
}

void Material::setEmissive(const Color&  emissive) {
    if (glm::any(glm::greaterThan(emissive, Color(0.0f)))) {
        _flags.set(EMISSIVE_BIT);
    } else {
        _flags.reset(EMISSIVE_BIT);
    }
    _schemaBuffer.edit<Schema>()._emissive = emissive;
}

void Material::setShininess(float shininess) {
    if (shininess > 0.0f) {
        _flags.set(SHININESS_BIT);
    } else {
        _flags.reset(SHININESS_BIT);
    }
    _schemaBuffer.edit<Schema>()._shininess = shininess;
}

void Material::setOpacity(float opacity) {
    if (opacity >= 1.0f) {
        _flags.reset(TRANSPARENT_BIT);
    } else {
        _flags.set(TRANSPARENT_BIT);
    }
    _schemaBuffer.edit<Schema>()._opacity = opacity;
}

void Material::setTextureView(MapChannel channel, const TextureView& view) {
    _flags.set(DIFFUSE_MAP_BIT + channel);
    _textureMap[channel] = view;
}
