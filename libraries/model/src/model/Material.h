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
#include "gpu/Texture.h"

#include <qurl.h>

namespace model {

typedef glm::vec3 Color;

// TextureStorage is a specialized version of the gpu::Texture::Storage
// It adds the URL and the notion that it owns the gpu::Texture
class TextureStorage : public gpu::Texture::Storage {
public:
    TextureStorage(const QUrl& url);
    ~TextureStorage();

    const QUrl& getUrl() const { return _url; }
    const gpu::TexturePointer& getGPUTexture() const { return _gpuTexture; }

protected:
    gpu::TexturePointer _gpuTexture;
    QUrl _url;
    void init();
};
typedef std::shared_ptr< TextureStorage > TextureStoragePointer;



class Material {
public:
    typedef gpu::BufferView UniformBufferView;
    typedef gpu::TextureView TextureView;

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
        
        Color _diffuse{0.5f};
        float _opacity{1.f};
        Color _specular{0.03f};
        float _shininess{0.1f};
        Color _emissive{0.0f};
        float _spare0{0.0f};
        glm::vec4  _spareVec4{0.0f}; // for alignment beauty, Material size == Mat4x4

        Schema() {}
    };

    const UniformBufferView& getSchemaBuffer() const { return _schemaBuffer; }

    void setTextureView(MapChannel channel, const TextureView& texture);
    const TextureMap& getTextureMap() const { return _textureMap; }

protected:

    Flags _flags;
    UniformBufferView _schemaBuffer;
    TextureMap _textureMap;

};
typedef std::shared_ptr< Material > MaterialPointer;

};

#endif
