//
//  Material.h
//  libraries/model/src/model
//
//  Created by Sam Gateau on 12/10/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_Material_h
#define hifi_model_Material_h

#include <bitset>
#include <map>

#include <glm/glm.hpp>

#include "gpu/Resource.h"

namespace model {
typedef gpu::BufferView UniformBufferView;
typedef gpu::TextureView TextureView;

class Material {
public:

    typedef glm::vec3 Color;

    enum MapChannel {
        DIFFUSE_MAP = 0,
        SPECULAR_MAP,
        SHININESS_MAP,
        EMISSIVE_MAP,
        OPACITY_MAP,
        NORMAL_MAP,

        NUM_MAPS,
    };
    typedef std::map<MapChannel, TextureView> TextureMap;

    enum FlagBit {
        DIFFUSE_BIT = 0,
        SPECULAR_BIT,
        SHININESS_BIT,
        EMISSIVE_BIT,
        TRANSPARENT_BIT,

        DIFFUSE_MAP_BIT,
        SPECULAR_MAP_BIT,
        SHININESS_MAP_BIT,
        EMISSIVE_MAP_BIT,
        OPACITY_MAP_BIT,
        NORMAL_MAP_BIT,

        NUM_FLAGS,
    };
    typedef std::bitset<NUM_FLAGS> Flags;

    Material();
    Material(const Material& material);
    Material& operator= (const Material& material);
    virtual ~Material();

    const Color& getEmissive() const { return _schemaBuffer.get<Schema>()._emissive; }
    const Color& getDiffuse() const { return _schemaBuffer.get<Schema>()._diffuse; }
    const Color& getSpecular() const { return _schemaBuffer.get<Schema>()._specular; }
    float getShininess() const { return _schemaBuffer.get<Schema>()._shininess; }
    float getOpacity() const { return _schemaBuffer.get<Schema>()._opacity; }

    void setDiffuse(const Color& diffuse);
    void setSpecular(const Color& specular);
    void setEmissive(const Color& emissive);
    void setShininess(float shininess);
    void setOpacity(float opacity);

    // Schema to access the attribute values of the material
    class Schema {
    public:
        
        Color _diffuse;
        float _opacity;
        Color _specular;
        float _shininess;
        Color _emissive;
        float _spare0;

        Schema() :
            _diffuse(0.5f),
            _opacity(1.0f),
            _specular(0.03f),
            _shininess(0.1f),
            _emissive(0.0f)
            {}
    };

    const UniformBufferView& getSchemaBuffer() const { return _schemaBuffer; }

    void setTextureView(MapChannel channel, const TextureView& texture);
    const TextureMap& getTextureMap() const { return _textureMap; }

    const Schema* getSchema() const { return &_schemaBuffer.get<Schema>(); }
protected:

    Flags _flags;
    UniformBufferView _schemaBuffer;
    TextureMap _textureMap;

};
typedef QSharedPointer< Material > MaterialPointer;

};

#endif
