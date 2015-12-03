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

#include <gpu/Resource.h>

namespace model {

class TextureMap;
typedef std::shared_ptr< TextureMap > TextureMapPointer;

// Material Key is a coarse trait description of a material used to classify the materials
class MaterialKey {
public:
   enum FlagBit {
        EMISSIVE_VAL_BIT = 0,
        DIFFUSE_VAL_BIT,
        METALLIC_VAL_BIT,
        GLOSS_VAL_BIT,
        TRANSPARENT_VAL_BIT,

        EMISSIVE_MAP_BIT,
        DIFFUSE_MAP_BIT,
        METALLIC_MAP_BIT,
        GLOSS_MAP_BIT,
        TRANSPARENT_MAP_BIT,
        NORMAL_MAP_BIT,
        LIGHTMAP_MAP_BIT,

        NUM_FLAGS,
    };
    typedef std::bitset<NUM_FLAGS> Flags;

    enum MapChannel {
        EMISSIVE_MAP = 0,
        DIFFUSE_MAP,
        METALLIC_MAP,
        GLOSS_MAP,
        TRANSPARENT_MAP,
        NORMAL_MAP,
        LIGHTMAP_MAP,

        NUM_MAP_CHANNELS,
    };

    // The signature is the Flags
    Flags _flags;

    MaterialKey() : _flags(0) {}
    MaterialKey(const Flags& flags) : _flags(flags) {}

    class Builder {
        Flags _flags{ 0 };
    public:
        Builder() {}

        MaterialKey build() const { return MaterialKey(_flags); }

        Builder& withEmissive() { _flags.set(EMISSIVE_VAL_BIT); return (*this); }
        Builder& withDiffuse() { _flags.set(DIFFUSE_VAL_BIT); return (*this); }
        Builder& withMetallic() { _flags.set(METALLIC_VAL_BIT); return (*this); }
        Builder& withGloss() { _flags.set(GLOSS_VAL_BIT); return (*this); }
        Builder& withTransparent() { _flags.set(TRANSPARENT_VAL_BIT); return (*this); }

        Builder& withEmissiveMap() { _flags.set(EMISSIVE_MAP_BIT); return (*this); }
        Builder& withDiffuseMap() { _flags.set(DIFFUSE_MAP_BIT); return (*this); }
        Builder& withMetallicMap() { _flags.set(METALLIC_MAP_BIT); return (*this); }
        Builder& withGlossMap() { _flags.set(GLOSS_MAP_BIT); return (*this); }
        Builder& withTransparentMap() { _flags.set(TRANSPARENT_MAP_BIT); return (*this); }

        Builder& withNormalMap() { _flags.set(NORMAL_MAP_BIT); return (*this); }
        Builder& withLightmapMap() { _flags.set(LIGHTMAP_MAP_BIT); return (*this); }

        // Convenient standard keys that we will keep on using all over the place
        static MaterialKey opaqueDiffuse() { return Builder().withDiffuse().build(); }
    };

    void setEmissive(bool value) { _flags.set(EMISSIVE_VAL_BIT, value); }
    bool isEmissive() const { return _flags[EMISSIVE_VAL_BIT]; }

    void setEmissiveMap(bool value) { _flags.set(EMISSIVE_MAP_BIT, value); }
    bool isEmissiveMap() const { return _flags[EMISSIVE_MAP_BIT]; }
 
    void setDiffuse(bool value) { _flags.set(DIFFUSE_VAL_BIT, value); }
    bool isDiffuse() const { return _flags[DIFFUSE_VAL_BIT]; }

    void setDiffuseMap(bool value) { _flags.set(DIFFUSE_MAP_BIT, value); }
    bool isDiffuseMap() const { return _flags[DIFFUSE_MAP_BIT]; }

    void setMetallic(bool value) { _flags.set(METALLIC_VAL_BIT, value); }
    bool isMetallic() const { return _flags[METALLIC_VAL_BIT]; }

    void setMetallicMap(bool value) { _flags.set(METALLIC_MAP_BIT, value); }
    bool isMetallicMap() const { return _flags[METALLIC_MAP_BIT]; }

    void setGloss(bool value) { _flags.set(GLOSS_VAL_BIT, value); }
    bool isGloss() const { return _flags[GLOSS_VAL_BIT]; }

    void setGlossMap(bool value) { _flags.set(GLOSS_MAP_BIT, value); }
    bool isGlossMap() const { return _flags[GLOSS_MAP_BIT]; }

    void setTransparent(bool value) { _flags.set(TRANSPARENT_VAL_BIT, value); }
    bool isTransparent() const { return _flags[TRANSPARENT_VAL_BIT]; }
    bool isOpaque() const { return !_flags[TRANSPARENT_VAL_BIT]; }

    void setTransparentMap(bool value) { _flags.set(TRANSPARENT_MAP_BIT, value); }
    bool isTransparentMap() const { return _flags[TRANSPARENT_MAP_BIT]; }

    void setNormalMap(bool value) { _flags.set(NORMAL_MAP_BIT, value); }
    bool isNormalMap() const { return _flags[NORMAL_MAP_BIT]; }

    void setLightmapMap(bool value) { _flags.set(LIGHTMAP_MAP_BIT, value); }
    bool isLightmapMap() const { return _flags[LIGHTMAP_MAP_BIT]; }

    void setMapChannel(MapChannel channel, bool value) { _flags.set(EMISSIVE_MAP_BIT + channel, value); }
    bool isMapChannel(MapChannel channel) const { return _flags[EMISSIVE_MAP_BIT + channel]; }

};


class MaterialFilter {
public:
    MaterialKey::Flags _value{ 0 };
    MaterialKey::Flags _mask{ 0 };


    MaterialFilter(const MaterialKey::Flags& value = MaterialKey::Flags(0), const MaterialKey::Flags& mask = MaterialKey::Flags(0)) : _value(value), _mask(mask) {}

    class Builder {
        MaterialKey::Flags _value{ 0 };
        MaterialKey::Flags _mask{ 0 };
    public:
        Builder() {}

        MaterialFilter build() const { return MaterialFilter(_value, _mask); }

        Builder& withoutEmissive()       { _value.reset(MaterialKey::EMISSIVE_VAL_BIT); _mask.set(MaterialKey::EMISSIVE_VAL_BIT); return (*this); }
        Builder& withEmissive()        { _value.set(MaterialKey::EMISSIVE_VAL_BIT);  _mask.set(MaterialKey::EMISSIVE_VAL_BIT); return (*this); }

        Builder& withoutEmissiveMap()       { _value.reset(MaterialKey::EMISSIVE_MAP_BIT); _mask.set(MaterialKey::EMISSIVE_MAP_BIT); return (*this); }
        Builder& withEmissiveMap()        { _value.set(MaterialKey::EMISSIVE_MAP_BIT);  _mask.set(MaterialKey::EMISSIVE_MAP_BIT); return (*this); }

        Builder& withoutDiffuse()       { _value.reset(MaterialKey::DIFFUSE_VAL_BIT); _mask.set(MaterialKey::DIFFUSE_VAL_BIT); return (*this); }
        Builder& withDiffuse()        { _value.set(MaterialKey::DIFFUSE_VAL_BIT);  _mask.set(MaterialKey::DIFFUSE_VAL_BIT); return (*this); }

        Builder& withoutDiffuseMap()       { _value.reset(MaterialKey::DIFFUSE_MAP_BIT); _mask.set(MaterialKey::DIFFUSE_MAP_BIT); return (*this); }
        Builder& withDiffuseMap()        { _value.set(MaterialKey::DIFFUSE_MAP_BIT);  _mask.set(MaterialKey::DIFFUSE_MAP_BIT); return (*this); }

        Builder& withoutMetallic()       { _value.reset(MaterialKey::METALLIC_VAL_BIT); _mask.set(MaterialKey::METALLIC_VAL_BIT); return (*this); }
        Builder& withMetallic()        { _value.set(MaterialKey::METALLIC_VAL_BIT);  _mask.set(MaterialKey::METALLIC_VAL_BIT); return (*this); }

        Builder& withoutMetallicMap()       { _value.reset(MaterialKey::METALLIC_MAP_BIT); _mask.set(MaterialKey::METALLIC_MAP_BIT); return (*this); }
        Builder& withMetallicMap()        { _value.set(MaterialKey::METALLIC_MAP_BIT);  _mask.set(MaterialKey::METALLIC_MAP_BIT); return (*this); }

        Builder& withoutGloss()       { _value.reset(MaterialKey::GLOSS_VAL_BIT); _mask.set(MaterialKey::GLOSS_VAL_BIT); return (*this); }
        Builder& withGloss()        { _value.set(MaterialKey::GLOSS_VAL_BIT);  _mask.set(MaterialKey::GLOSS_VAL_BIT); return (*this); }

        Builder& withoutGlossMap()       { _value.reset(MaterialKey::GLOSS_MAP_BIT); _mask.set(MaterialKey::GLOSS_MAP_BIT); return (*this); }
        Builder& withGlossMap()        { _value.set(MaterialKey::GLOSS_MAP_BIT);  _mask.set(MaterialKey::GLOSS_MAP_BIT); return (*this); }

        Builder& withoutTransparent()       { _value.reset(MaterialKey::TRANSPARENT_VAL_BIT); _mask.set(MaterialKey::TRANSPARENT_VAL_BIT); return (*this); }
        Builder& withTransparent()        { _value.set(MaterialKey::TRANSPARENT_VAL_BIT);  _mask.set(MaterialKey::TRANSPARENT_VAL_BIT); return (*this); }

        Builder& withoutTransparentMap()       { _value.reset(MaterialKey::TRANSPARENT_MAP_BIT); _mask.set(MaterialKey::TRANSPARENT_MAP_BIT); return (*this); }
        Builder& withTransparentMap()        { _value.set(MaterialKey::TRANSPARENT_MAP_BIT);  _mask.set(MaterialKey::TRANSPARENT_MAP_BIT); return (*this); }

        Builder& withoutNormalMap()       { _value.reset(MaterialKey::NORMAL_MAP_BIT); _mask.set(MaterialKey::NORMAL_MAP_BIT); return (*this); }
        Builder& withNormalMap()        { _value.set(MaterialKey::NORMAL_MAP_BIT);  _mask.set(MaterialKey::NORMAL_MAP_BIT); return (*this); }

        Builder& withoutLightmapMap()       { _value.reset(MaterialKey::LIGHTMAP_MAP_BIT); _mask.set(MaterialKey::LIGHTMAP_MAP_BIT); return (*this); }
        Builder& withLightmapMap()        { _value.set(MaterialKey::LIGHTMAP_MAP_BIT);  _mask.set(MaterialKey::LIGHTMAP_MAP_BIT); return (*this); }

        // Convenient standard keys that we will keep on using all over the place
        static MaterialFilter opaqueDiffuse() { return Builder().withDiffuse().withoutTransparent().build(); }
    };

    // Item Filter operator testing if a key pass the filter
    bool test(const MaterialKey& key) const { return (key._flags & _mask) == (_value & _mask); }

    class Less {
    public:
        bool operator() (const MaterialFilter& left, const MaterialFilter& right) const {
            if (left._value.to_ulong() == right._value.to_ulong()) {
                return left._mask.to_ulong() < right._mask.to_ulong();
            } else {
                return left._value.to_ulong() < right._value.to_ulong();
            }
        }
    };
};

class Material {
public:
    typedef gpu::BufferView UniformBufferView;

    typedef glm::vec3 Color;

    typedef MaterialKey::MapChannel MapChannel;
    typedef std::map<MapChannel, TextureMapPointer> TextureMaps;
    typedef std::bitset<MaterialKey::NUM_MAP_CHANNELS> MapFlags;

    Material();
    Material(const Material& material);
    Material& operator= (const Material& material);
    virtual ~Material();

    const MaterialKey& getKey() const { return _key; }
     
    const Color& getEmissive() const { return _schemaBuffer.get<Schema>()._emissive; }
    const Color& getDiffuse() const { return _schemaBuffer.get<Schema>()._diffuse; }
    float getMetallic() const { return _schemaBuffer.get<Schema>()._metallic.x; }
    float getGloss() const { return _schemaBuffer.get<Schema>()._gloss; }
    float getOpacity() const { return _schemaBuffer.get<Schema>()._opacity; }

    void setEmissive(const Color& emissive);
    void setDiffuse(const Color& diffuse);
    void setMetallic(float metallic);
    void setGloss(float gloss);
    void setOpacity(float opacity);

    // Schema to access the attribute values of the material
    class Schema {
    public:
        
        Color _diffuse{0.5f};
        float _opacity{1.f};
        Color _metallic{0.03f};
        float _gloss{0.1f};
        Color _emissive{0.0f};
        float _spare0{0.0f};
        glm::vec4  _spareVec4{0.0f}; // for alignment beauty, Material size == Mat4x4

        Schema() {}
    };

    const UniformBufferView& getSchemaBuffer() const { return _schemaBuffer; }

    // The texture map to channel association
    void setTextureMap(MapChannel channel, const TextureMapPointer& textureMap);
    const TextureMaps& getTextureMaps() const { return _textureMaps; }

protected:

    MaterialKey _key;
    UniformBufferView _schemaBuffer;
    TextureMaps _textureMaps;

};
typedef std::shared_ptr< Material > MaterialPointer;

};

#endif
