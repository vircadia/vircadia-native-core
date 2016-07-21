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

#include <ColorUtils.h>

#include <gpu/Resource.h>

namespace model {

class TextureMap;
typedef std::shared_ptr< TextureMap > TextureMapPointer;

// Material Key is a coarse trait description of a material used to classify the materials
class MaterialKey {
public:
   enum FlagBit {
        EMISSIVE_VAL_BIT = 0,
        UNLIT_VAL_BIT,
        ALBEDO_VAL_BIT,
        METALLIC_VAL_BIT,
        GLOSSY_VAL_BIT,
        OPACITY_VAL_BIT,
        OPACITY_MASK_MAP_BIT,           // Opacity Map and Opacity MASK map are mutually exclusive
        OPACITY_TRANSLUCENT_MAP_BIT,
        SCATTERING_VAL_BIT,

        // THe map bits must be in the same sequence as the enum names for the map channels
        EMISSIVE_MAP_BIT,
        ALBEDO_MAP_BIT,
        METALLIC_MAP_BIT,
        ROUGHNESS_MAP_BIT,
        NORMAL_MAP_BIT,
        OCCLUSION_MAP_BIT,
        LIGHTMAP_MAP_BIT,
        SCATTERING_MAP_BIT,

        NUM_FLAGS,
    };
    typedef std::bitset<NUM_FLAGS> Flags;

    enum MapChannel {
        EMISSIVE_MAP = 0,
        ALBEDO_MAP,
        METALLIC_MAP,
        ROUGHNESS_MAP,
        NORMAL_MAP,
        OCCLUSION_MAP,
        LIGHTMAP_MAP,
        SCATTERING_MAP,

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
        Builder& withUnlit() { _flags.set(UNLIT_VAL_BIT); return (*this); }

        Builder& withAlbedo() { _flags.set(ALBEDO_VAL_BIT); return (*this); }
        Builder& withMetallic() { _flags.set(METALLIC_VAL_BIT); return (*this); }
        Builder& withGlossy() { _flags.set(GLOSSY_VAL_BIT); return (*this); }

        Builder& withTranslucentFactor() { _flags.set(OPACITY_VAL_BIT); return (*this); }

        Builder& withScattering() { _flags.set(SCATTERING_VAL_BIT); return (*this); }

        Builder& withEmissiveMap() { _flags.set(EMISSIVE_MAP_BIT); return (*this); }
        Builder& withAlbedoMap() { _flags.set(ALBEDO_MAP_BIT); return (*this); }
        Builder& withMetallicMap() { _flags.set(METALLIC_MAP_BIT); return (*this); }
        Builder& withRoughnessMap() { _flags.set(ROUGHNESS_MAP_BIT); return (*this); }

        Builder& withTranslucentMap() { _flags.set(OPACITY_TRANSLUCENT_MAP_BIT); return (*this); }
        Builder& withMaskMap() { _flags.set(OPACITY_MASK_MAP_BIT); return (*this); }

        Builder& withNormalMap() { _flags.set(NORMAL_MAP_BIT); return (*this); }
        Builder& withOcclusionMap() { _flags.set(OCCLUSION_MAP_BIT); return (*this); }
        Builder& withLightmapMap() { _flags.set(LIGHTMAP_MAP_BIT); return (*this); }
        Builder& withScatteringMap() { _flags.set(SCATTERING_MAP_BIT); return (*this); }

        // Convenient standard keys that we will keep on using all over the place
        static MaterialKey opaqueAlbedo() { return Builder().withAlbedo().build(); }
    };

    void setEmissive(bool value) { _flags.set(EMISSIVE_VAL_BIT, value); }
    bool isEmissive() const { return _flags[EMISSIVE_VAL_BIT]; }

    void setUnlit(bool value) { _flags.set(UNLIT_VAL_BIT, value); }
    bool isUnlit() const { return _flags[UNLIT_VAL_BIT]; }

    void setEmissiveMap(bool value) { _flags.set(EMISSIVE_MAP_BIT, value); }
    bool isEmissiveMap() const { return _flags[EMISSIVE_MAP_BIT]; }
 
    void setAlbedo(bool value) { _flags.set(ALBEDO_VAL_BIT, value); }
    bool isAlbedo() const { return _flags[ALBEDO_VAL_BIT]; }

    void setAlbedoMap(bool value) { _flags.set(ALBEDO_MAP_BIT, value); }
    bool isAlbedoMap() const { return _flags[ALBEDO_MAP_BIT]; }

    void setMetallic(bool value) { _flags.set(METALLIC_VAL_BIT, value); }
    bool isMetallic() const { return _flags[METALLIC_VAL_BIT]; }

    void setMetallicMap(bool value) { _flags.set(METALLIC_MAP_BIT, value); }
    bool isMetallicMap() const { return _flags[METALLIC_MAP_BIT]; }

    void setGlossy(bool value) { _flags.set(GLOSSY_VAL_BIT, value); }
    bool isGlossy() const { return _flags[GLOSSY_VAL_BIT]; }
    bool isRough() const { return !_flags[GLOSSY_VAL_BIT]; }

    void setRoughnessMap(bool value) { _flags.set(ROUGHNESS_MAP_BIT, value); }
    bool isRoughnessMap() const { return _flags[ROUGHNESS_MAP_BIT]; }

    void setTranslucentFactor(bool value) { _flags.set(OPACITY_VAL_BIT, value); }
    bool isTranslucentFactor() const { return _flags[OPACITY_VAL_BIT]; }

    void setTranslucentMap(bool value) { _flags.set(OPACITY_TRANSLUCENT_MAP_BIT, value); }
    bool isTranslucentMap() const { return _flags[OPACITY_TRANSLUCENT_MAP_BIT]; }

    void setOpacityMaskMap(bool value) { _flags.set(OPACITY_MASK_MAP_BIT, value); }
    bool isOpacityMaskMap() const { return _flags[OPACITY_MASK_MAP_BIT]; }

    void setNormalMap(bool value) { _flags.set(NORMAL_MAP_BIT, value); }
    bool isNormalMap() const { return _flags[NORMAL_MAP_BIT]; }

    void setOcclusionMap(bool value) { _flags.set(OCCLUSION_MAP_BIT, value); }
    bool isOcclusionMap() const { return _flags[OCCLUSION_MAP_BIT]; }

    void setLightmapMap(bool value) { _flags.set(LIGHTMAP_MAP_BIT, value); }
    bool isLightmapMap() const { return _flags[LIGHTMAP_MAP_BIT]; }

    void setScattering(bool value) { _flags.set(SCATTERING_VAL_BIT, value); }
    bool isScattering() const { return _flags[SCATTERING_VAL_BIT]; }

    void setScatteringMap(bool value) { _flags.set(SCATTERING_MAP_BIT, value); }
    bool isScatteringMap() const { return _flags[SCATTERING_MAP_BIT]; }

    void setMapChannel(MapChannel channel, bool value) { _flags.set(EMISSIVE_MAP_BIT + channel, value); }
    bool isMapChannel(MapChannel channel) const { return _flags[EMISSIVE_MAP_BIT + channel]; }


    // Translucency and Opacity Heuristics are combining several flags:
    bool isTranslucent() const { return isTranslucentFactor() || isTranslucentMap(); }
    bool isOpaque() const { return !isTranslucent(); }
    bool isSurfaceOpaque() const { return isOpaque() && !isOpacityMaskMap(); }
    bool isTexelOpaque() const { return isOpaque() && isOpacityMaskMap(); }
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

        Builder& withoutUnlit()       { _value.reset(MaterialKey::UNLIT_VAL_BIT); _mask.set(MaterialKey::UNLIT_VAL_BIT); return (*this); }
        Builder& withUnlit()        { _value.set(MaterialKey::UNLIT_VAL_BIT);  _mask.set(MaterialKey::UNLIT_VAL_BIT); return (*this); }

        Builder& withoutAlbedo()       { _value.reset(MaterialKey::ALBEDO_VAL_BIT); _mask.set(MaterialKey::ALBEDO_VAL_BIT); return (*this); }
        Builder& withAlbedo()        { _value.set(MaterialKey::ALBEDO_VAL_BIT);  _mask.set(MaterialKey::ALBEDO_VAL_BIT); return (*this); }

        Builder& withoutAlbedoMap()       { _value.reset(MaterialKey::ALBEDO_MAP_BIT); _mask.set(MaterialKey::ALBEDO_MAP_BIT); return (*this); }
        Builder& withAlbedoMap()        { _value.set(MaterialKey::ALBEDO_MAP_BIT);  _mask.set(MaterialKey::ALBEDO_MAP_BIT); return (*this); }

        Builder& withoutMetallic()       { _value.reset(MaterialKey::METALLIC_VAL_BIT); _mask.set(MaterialKey::METALLIC_VAL_BIT); return (*this); }
        Builder& withMetallic()        { _value.set(MaterialKey::METALLIC_VAL_BIT);  _mask.set(MaterialKey::METALLIC_VAL_BIT); return (*this); }

        Builder& withoutMetallicMap()       { _value.reset(MaterialKey::METALLIC_MAP_BIT); _mask.set(MaterialKey::METALLIC_MAP_BIT); return (*this); }
        Builder& withMetallicMap()        { _value.set(MaterialKey::METALLIC_MAP_BIT);  _mask.set(MaterialKey::METALLIC_MAP_BIT); return (*this); }

        Builder& withoutGlossy()       { _value.reset(MaterialKey::GLOSSY_VAL_BIT); _mask.set(MaterialKey::GLOSSY_VAL_BIT); return (*this); }
        Builder& withGlossy()        { _value.set(MaterialKey::GLOSSY_VAL_BIT);  _mask.set(MaterialKey::GLOSSY_VAL_BIT); return (*this); }

        Builder& withoutRoughnessMap()       { _value.reset(MaterialKey::ROUGHNESS_MAP_BIT); _mask.set(MaterialKey::ROUGHNESS_MAP_BIT); return (*this); }
        Builder& withRoughnessMap()        { _value.set(MaterialKey::ROUGHNESS_MAP_BIT);  _mask.set(MaterialKey::ROUGHNESS_MAP_BIT); return (*this); }

        Builder& withoutTranslucentFactor()       { _value.reset(MaterialKey::OPACITY_VAL_BIT); _mask.set(MaterialKey::OPACITY_VAL_BIT); return (*this); }
        Builder& withTranslucentFactor()        { _value.set(MaterialKey::OPACITY_VAL_BIT);  _mask.set(MaterialKey::OPACITY_VAL_BIT); return (*this); }

        Builder& withoutTranslucentMap()       { _value.reset(MaterialKey::OPACITY_TRANSLUCENT_MAP_BIT); _mask.set(MaterialKey::OPACITY_TRANSLUCENT_MAP_BIT); return (*this); }
        Builder& withTranslucentMap()        { _value.set(MaterialKey::OPACITY_TRANSLUCENT_MAP_BIT);  _mask.set(MaterialKey::OPACITY_TRANSLUCENT_MAP_BIT); return (*this); }

        Builder& withoutMaskMap()       { _value.reset(MaterialKey::OPACITY_MASK_MAP_BIT); _mask.set(MaterialKey::OPACITY_MASK_MAP_BIT); return (*this); }
        Builder& withMaskMap()        { _value.set(MaterialKey::OPACITY_MASK_MAP_BIT);  _mask.set(MaterialKey::OPACITY_MASK_MAP_BIT); return (*this); }

        Builder& withoutNormalMap()       { _value.reset(MaterialKey::NORMAL_MAP_BIT); _mask.set(MaterialKey::NORMAL_MAP_BIT); return (*this); }
        Builder& withNormalMap()        { _value.set(MaterialKey::NORMAL_MAP_BIT);  _mask.set(MaterialKey::NORMAL_MAP_BIT); return (*this); }

        Builder& withoutOcclusionMap()       { _value.reset(MaterialKey::OCCLUSION_MAP_BIT); _mask.set(MaterialKey::OCCLUSION_MAP_BIT); return (*this); }
        Builder& withOcclusionMap()        { _value.set(MaterialKey::OCCLUSION_MAP_BIT);  _mask.set(MaterialKey::OCCLUSION_MAP_BIT); return (*this); }

        Builder& withoutLightmapMap()       { _value.reset(MaterialKey::LIGHTMAP_MAP_BIT); _mask.set(MaterialKey::LIGHTMAP_MAP_BIT); return (*this); }
        Builder& withLightmapMap()        { _value.set(MaterialKey::LIGHTMAP_MAP_BIT);  _mask.set(MaterialKey::LIGHTMAP_MAP_BIT); return (*this); }

        Builder& withoutScattering()       { _value.reset(MaterialKey::SCATTERING_VAL_BIT); _mask.set(MaterialKey::SCATTERING_VAL_BIT); return (*this); }
        Builder& withScattering()        { _value.set(MaterialKey::SCATTERING_VAL_BIT);  _mask.set(MaterialKey::SCATTERING_VAL_BIT); return (*this); }

        Builder& withoutScatteringMap()       { _value.reset(MaterialKey::SCATTERING_MAP_BIT); _mask.set(MaterialKey::SCATTERING_MAP_BIT); return (*this); }
        Builder& withScatteringMap()        { _value.set(MaterialKey::SCATTERING_MAP_BIT);  _mask.set(MaterialKey::SCATTERING_MAP_BIT); return (*this); }


        // Convenient standard keys that we will keep on using all over the place
        static MaterialFilter opaqueAlbedo() { return Builder().withAlbedo().withoutTranslucentFactor().build(); }
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

    void setEmissive(const Color& emissive, bool isSRGB = true);
    Color getEmissive(bool SRGB = true) const { return (SRGB ? ColorUtils::tosRGBVec3(_schemaBuffer.get<Schema>()._emissive) : _schemaBuffer.get<Schema>()._emissive); }

    void setOpacity(float opacity);
    float getOpacity() const { return _schemaBuffer.get<Schema>()._opacity; }

    void setUnlit(bool value);
    bool isUnlit() const { return _key.isUnlit(); }

    void setAlbedo(const Color& albedo, bool isSRGB = true);
    Color getAlbedo(bool SRGB = true) const { return (SRGB ? ColorUtils::tosRGBVec3(_schemaBuffer.get<Schema>()._albedo) : _schemaBuffer.get<Schema>()._albedo); }

    void setFresnel(const Color& fresnel, bool isSRGB = true);
    Color getFresnel(bool SRGB = true) const { return (SRGB ? ColorUtils::tosRGBVec3(_schemaBuffer.get<Schema>()._fresnel) : _schemaBuffer.get<Schema>()._fresnel); }

    void setMetallic(float metallic);
    float getMetallic() const { return _schemaBuffer.get<Schema>()._metallic; }

    void setRoughness(float roughness);
    float getRoughness() const { return _schemaBuffer.get<Schema>()._roughness; }

    void setScattering(float scattering);
    float getScattering() const { return _schemaBuffer.get<Schema>()._scattering; }

    // Schema to access the attribute values of the material
    class Schema {
    public:
        glm::vec3 _emissive{ 0.0f }; // No Emissive
        float _opacity{ 1.0f }; // Opacity = 1 => Not Transparent

        glm::vec3 _albedo{ 0.5f }; // Grey albedo => isAlbedo
        float _roughness{ 1.0f }; // Roughness = 1 => Not Glossy

        glm::vec3 _fresnel{ 0.03f }; // Fresnel value for a default non metallic
        float _metallic{ 0.0f }; // Not Metallic

        float _scattering{ 0.0f }; // Scattering info

        glm::vec2 _spare{ 0.0f };

        uint32_t _key{ 0 }; // a copy of the materialKey

        // for alignment beauty, Material size == Mat4x4

        Schema() {}
    };

    const UniformBufferView& getSchemaBuffer() const { return _schemaBuffer; }

    // The texture map to channel association
    void setTextureMap(MapChannel channel, const TextureMapPointer& textureMap);
    const TextureMaps& getTextureMaps() const { return _textureMaps; }
    const TextureMapPointer getTextureMap(MapChannel channel) const;

    // Albedo maps cannot have opacity detected until they are loaded
    // This method allows const changing of the key/schemaBuffer without touching the map
    void resetOpacityMap() const;

    // conversion from legacy material properties to PBR equivalent
    static float shininessToRoughness(float shininess) { return 1.0f - shininess / 100.0f; }

    // Texture Map Array Schema
    static const int NUM_TEXCOORD_TRANSFORMS{ 2 };
    class TexMapArraySchema {
    public:
        glm::mat4 _texcoordTransforms[NUM_TEXCOORD_TRANSFORMS];
        glm::vec4 _lightmapParams{ 0.0, 1.0, 0.0, 0.0 };
        TexMapArraySchema() {}
    };

    const UniformBufferView& getTexMapArrayBuffer() const { return _texMapArrayBuffer; }
private:
    mutable MaterialKey _key;
    mutable UniformBufferView _schemaBuffer;
    mutable UniformBufferView _texMapArrayBuffer;

    TextureMaps _textureMaps;
};
typedef std::shared_ptr< Material > MaterialPointer;

};

#endif
