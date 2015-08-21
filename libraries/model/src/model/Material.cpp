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
using namespace gpu;

Material::Material() :
    _key(0),
    _schemaBuffer(),
    _textureMap() {
       
        // only if created from nothing shall we create the Buffer to store the properties
        Schema schema;
        _schemaBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(Schema), (const gpu::Byte*) &schema));
        

}

Material::Material(const Material& material) :
    _key(material._key),
    _schemaBuffer(material._schemaBuffer),
    _textureMap(material._textureMap) {
}

Material& Material::operator= (const Material& material) {
    _key = (material._key);
    _schemaBuffer = (material._schemaBuffer);
    _textureMap = (material._textureMap);

    return (*this);
}

Material::~Material() {
}

void Material::setDiffuse(const Color& diffuse) {
    _key.setDiffuse(glm::any(glm::greaterThan(diffuse, Color(0.0f))));
    _schemaBuffer.edit<Schema>()._diffuse = diffuse;
}

void Material::setMetallic(float metallic) {
    _key.setMetallic(metallic > 0.0f);
    _schemaBuffer.edit<Schema>()._metallic = glm::vec3(metallic);
}

void Material::setEmissive(const Color&  emissive) {
    _key.setEmissive(glm::any(glm::greaterThan(emissive, Color(0.0f))));
    _schemaBuffer.edit<Schema>()._emissive = emissive;
}

void Material::setGloss(float gloss) {
    _key.setGloss((gloss > 0.0f));
    _schemaBuffer.edit<Schema>()._gloss = gloss;
}

void Material::setOpacity(float opacity) {
    _key.setTransparent((opacity < 1.0f));
    _schemaBuffer.edit<Schema>()._opacity = opacity;
}

void Material::setTextureView(MapChannel channel, const gpu::TextureView& view) {
    _key.setMapChannel(channel, (view.isValid()));
    _textureMap[channel] = view;
}

