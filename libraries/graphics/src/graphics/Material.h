//
//  Material.h
//  libraries/graphics/src/graphics
//
//  Created by Sam Gateau on 12/10/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_Material_h
#define hifi_model_Material_h

#include <mutex>
#include <bitset>
#include <map>
#include <unordered_map>
#include <queue>

#include <ColorUtils.h>

#include <gpu/Resource.h>
#include <gpu/TextureTable.h>

#include "MaterialMappingMode.h"

class Transform;

namespace graphics {

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
        OPACITY_MAP_MODE_BIT,           // Opacity map mode bit is set if the value has set explicitely and not deduced from the textures assigned 
        OPACITY_CUTOFF_VAL_BIT,
        SCATTERING_VAL_BIT,

        // THe map bits must be in the same sequence as the enum names for the map channels
        EMISSIVE_MAP_BIT,
        ALBEDO_MAP_BIT,
        METALLIC_MAP_BIT,
        ROUGHNESS_MAP_BIT,
        NORMAL_MAP_BIT,
        OCCLUSION_MAP_BIT,
        LIGHT_MAP_BIT,
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
        LIGHT_MAP,
        SCATTERING_MAP,

        NUM_MAP_CHANNELS,
    };

    enum OpacityMapMode {
        OPACITY_MAP_OPAQUE = 0,
        OPACITY_MAP_MASK,
        OPACITY_MAP_BLEND,
    };
    static std::string getOpacityMapModeName(OpacityMapMode mode);
    // find the enum value from a string, return true if match found
    static bool getOpacityMapModeFromName(const std::string& modeName, OpacityMapMode& mode);

    enum CullFaceMode {
        CULL_NONE = 0,
        CULL_FRONT,
        CULL_BACK,

        NUM_CULL_FACE_MODES
    };
    static std::string getCullFaceModeName(CullFaceMode mode);
    static bool getCullFaceModeFromName(const std::string& modeName, CullFaceMode& mode);

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
        Builder& withTranslucentMap() { _flags.set(OPACITY_TRANSLUCENT_MAP_BIT); return (*this); }
        Builder& withMaskMap() { _flags.set(OPACITY_MASK_MAP_BIT); return (*this); }
        Builder& withOpacityMapMode(OpacityMapMode mode) {
            switch (mode) {
            case OPACITY_MAP_OPAQUE:
                _flags.reset(OPACITY_TRANSLUCENT_MAP_BIT);
                _flags.reset(OPACITY_MASK_MAP_BIT);
                break;
            case OPACITY_MAP_MASK:
                _flags.reset(OPACITY_TRANSLUCENT_MAP_BIT);
                _flags.set(OPACITY_MASK_MAP_BIT);
                break;
            case OPACITY_MAP_BLEND:
                _flags.set(OPACITY_TRANSLUCENT_MAP_BIT);
                _flags.reset(OPACITY_MASK_MAP_BIT);
                break;
            };
            _flags.set(OPACITY_MAP_MODE_BIT); // Intentionally set the mode!
            return (*this);
        }
        Builder& withOpacityCutoff() { _flags.set(OPACITY_CUTOFF_VAL_BIT); return (*this); }

        Builder& withScattering() { _flags.set(SCATTERING_VAL_BIT); return (*this); }

        Builder& withEmissiveMap() { _flags.set(EMISSIVE_MAP_BIT); return (*this); }
        Builder& withAlbedoMap() { _flags.set(ALBEDO_MAP_BIT); return (*this); }
        Builder& withMetallicMap() { _flags.set(METALLIC_MAP_BIT); return (*this); }
        Builder& withRoughnessMap() { _flags.set(ROUGHNESS_MAP_BIT); return (*this); }

        Builder& withNormalMap() { _flags.set(NORMAL_MAP_BIT); return (*this); }
        Builder& withOcclusionMap() { _flags.set(OCCLUSION_MAP_BIT); return (*this); }
        Builder& withLightMap() { _flags.set(LIGHT_MAP_BIT); return (*this); }
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

    void setOpacityCutoff(bool value) { _flags.set(OPACITY_CUTOFF_VAL_BIT, value); }
    bool isOpacityCutoff() const { return _flags[OPACITY_CUTOFF_VAL_BIT]; }

    void setNormalMap(bool value) { _flags.set(NORMAL_MAP_BIT, value); }
    bool isNormalMap() const { return _flags[NORMAL_MAP_BIT]; }

    void setOcclusionMap(bool value) { _flags.set(OCCLUSION_MAP_BIT, value); }
    bool isOcclusionMap() const { return _flags[OCCLUSION_MAP_BIT]; }

    void setLightMap(bool value) { _flags.set(LIGHT_MAP_BIT, value); }
    bool isLightMap() const { return _flags[LIGHT_MAP_BIT]; }

    void setScattering(bool value) { _flags.set(SCATTERING_VAL_BIT, value); }
    bool isScattering() const { return _flags[SCATTERING_VAL_BIT]; }

    void setScatteringMap(bool value) { _flags.set(SCATTERING_MAP_BIT, value); }
    bool isScatteringMap() const { return _flags[SCATTERING_MAP_BIT]; }

    void setMapChannel(MapChannel channel, bool value) { _flags.set(EMISSIVE_MAP_BIT + channel, value); }
    bool isMapChannel(MapChannel channel) const { return _flags[EMISSIVE_MAP_BIT + channel]; }


    // Translucency and Opacity Heuristics are combining several flags:
    void setOpacityMapMode(OpacityMapMode mode) {
        switch (mode) {
        case OPACITY_MAP_OPAQUE:
            _flags.reset(OPACITY_TRANSLUCENT_MAP_BIT);
            _flags.reset(OPACITY_MASK_MAP_BIT);
            break;
        case OPACITY_MAP_MASK:
            _flags.reset(OPACITY_TRANSLUCENT_MAP_BIT);
            _flags.set(OPACITY_MASK_MAP_BIT);
            break;
        case OPACITY_MAP_BLEND:
            _flags.set(OPACITY_TRANSLUCENT_MAP_BIT);
            _flags.reset(OPACITY_MASK_MAP_BIT);
            break;
        };
        _flags.set(OPACITY_MAP_MODE_BIT); // Intentionally set the mode!
    }
    bool isOpacityMapMode() const { return _flags[OPACITY_MAP_MODE_BIT]; }
    OpacityMapMode getOpacityMapMode() const { return (isOpacityMaskMap() ? OPACITY_MAP_MASK : (isTranslucentMap() ? OPACITY_MAP_BLEND : OPACITY_MAP_OPAQUE)); }

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

        Builder& withoutOpacityMapMode() { _value.reset(MaterialKey::OPACITY_MAP_MODE_BIT); _mask.set(MaterialKey::OPACITY_MAP_MODE_BIT); return (*this); }
        Builder& withOpacityMapMode() { _value.set(MaterialKey::OPACITY_MAP_MODE_BIT);  _mask.set(MaterialKey::OPACITY_MAP_MODE_BIT); return (*this); }

        Builder& withoutOpacityCutoff() { _value.reset(MaterialKey::OPACITY_CUTOFF_VAL_BIT); _mask.set(MaterialKey::OPACITY_CUTOFF_VAL_BIT); return (*this); }
        Builder& withOpacityCutoff() { _value.set(MaterialKey::OPACITY_CUTOFF_VAL_BIT);  _mask.set(MaterialKey::OPACITY_CUTOFF_VAL_BIT); return (*this); }

        Builder& withoutNormalMap()       { _value.reset(MaterialKey::NORMAL_MAP_BIT); _mask.set(MaterialKey::NORMAL_MAP_BIT); return (*this); }
        Builder& withNormalMap()        { _value.set(MaterialKey::NORMAL_MAP_BIT);  _mask.set(MaterialKey::NORMAL_MAP_BIT); return (*this); }

        Builder& withoutOcclusionMap()       { _value.reset(MaterialKey::OCCLUSION_MAP_BIT); _mask.set(MaterialKey::OCCLUSION_MAP_BIT); return (*this); }
        Builder& withOcclusionMap()        { _value.set(MaterialKey::OCCLUSION_MAP_BIT);  _mask.set(MaterialKey::OCCLUSION_MAP_BIT); return (*this); }

        Builder& withoutLightMap()       { _value.reset(MaterialKey::LIGHT_MAP_BIT); _mask.set(MaterialKey::LIGHT_MAP_BIT); return (*this); }
        Builder& withLightMap()        { _value.set(MaterialKey::LIGHT_MAP_BIT);  _mask.set(MaterialKey::LIGHT_MAP_BIT); return (*this); }

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
    typedef MaterialKey::MapChannel MapChannel;
    typedef std::map<MapChannel, TextureMapPointer> TextureMaps;

    Material();
    Material(const Material& material);
    virtual ~Material() = default;
    Material& operator= (const Material& material);

    virtual MaterialKey getKey() const { return _key; }

    static const float DEFAULT_EMISSIVE;
    void setEmissive(const glm::vec3& emissive, bool isSRGB = true);
    virtual glm::vec3 getEmissive(bool SRGB = true) const { return (SRGB ? ColorUtils::tosRGBVec3(_emissive) : _emissive); }

    static const float DEFAULT_OPACITY;
    void setOpacity(float opacity);
    virtual float getOpacity() const { return _opacity; }

    static const MaterialKey::OpacityMapMode DEFAULT_OPACITY_MAP_MODE;
    void setOpacityMapMode(MaterialKey::OpacityMapMode opacityMapMode);
    virtual MaterialKey::OpacityMapMode getOpacityMapMode() const;

    static const float DEFAULT_OPACITY_CUTOFF;
    void setOpacityCutoff(float opacityCutoff);
    virtual float getOpacityCutoff() const { return _opacityCutoff; }

    static const MaterialKey::CullFaceMode DEFAULT_CULL_FACE_MODE;
    void setCullFaceMode(MaterialKey::CullFaceMode cullFaceMode) { _cullFaceMode = cullFaceMode; }
    virtual MaterialKey::CullFaceMode getCullFaceMode() const { return _cullFaceMode; }

    void setUnlit(bool value);
    virtual bool isUnlit() const { return _key.isUnlit(); }

    static const float DEFAULT_ALBEDO;
    void setAlbedo(const glm::vec3& albedo, bool isSRGB = true);
    virtual glm::vec3 getAlbedo(bool SRGB = true) const { return (SRGB ? ColorUtils::tosRGBVec3(_albedo) : _albedo); }

    static const float DEFAULT_METALLIC;
    void setMetallic(float metallic);
    virtual float getMetallic() const { return _metallic; }

    static const float DEFAULT_ROUGHNESS;
    void setRoughness(float roughness);
    virtual float getRoughness() const { return _roughness; }

    static const float DEFAULT_SCATTERING;
    void setScattering(float scattering);
    virtual float getScattering() const { return _scattering; }

    // The texture map to channel association
    static const int NUM_TEXCOORD_TRANSFORMS { 2 };
    void setTextureMap(MapChannel channel, const TextureMapPointer& textureMap);
    virtual TextureMaps getTextureMaps() const { return _textureMaps; } // FIXME - not thread safe...
    const TextureMapPointer getTextureMap(MapChannel channel) const;

    // Albedo maps cannot have opacity detected until they are loaded
    // This method allows const changing of the key/schemaBuffer without touching the map
    // return true if the opacity changed, flase otherwise
    virtual bool resetOpacityMap() const;

    // conversion from legacy material properties to PBR equivalent
    static float shininessToRoughness(float shininess) { return 1.0f - shininess / 100.0f; }

    void setTextureTransforms(const Transform& transform, MaterialMappingMode mode, bool repeat);

    const std::string& getName() const { return _name; }
    void setName(const std::string& name) { _name = name; }

    const std::string& getModel() const { return _model; }
    void setModel(const std::string& model) { _model = model; }

    virtual glm::mat4 getTexCoordTransform(uint i) const { return _texcoordTransforms[i]; }
    void setTexCoordTransform(uint i, const glm::mat4& mat4) { _texcoordTransforms[i] = mat4; }
    virtual glm::vec2 getLightmapParams() const { return _lightmapParams; }
    virtual glm::vec2 getMaterialParams() const { return _materialParams; }

    virtual bool getDefaultFallthrough() const { return _defaultFallthrough; }
    void setDefaultFallthrough(bool defaultFallthrough) { _defaultFallthrough = defaultFallthrough; }

    enum ExtraFlagBit {
        TEXCOORDTRANSFORM0 = MaterialKey::NUM_FLAGS,
        TEXCOORDTRANSFORM1,
        LIGHTMAP_PARAMS,
        MATERIAL_PARAMS,
        CULL_FACE_MODE,

        NUM_TOTAL_FLAGS
    };
    std::unordered_map<uint, bool> getPropertyFallthroughs() { return _propertyFallthroughs; }
    bool getPropertyFallthrough(uint property) { return _propertyFallthroughs[property]; }
    void setPropertyDoesFallthrough(uint property) { _propertyFallthroughs[property] = true; }

    virtual bool isProcedural() const { return false; }
    virtual bool isEnabled() const { return true; }
    virtual bool isReady() const { return true; }
    virtual QString getProceduralString() const { return QString(); }

    virtual bool isReference() const { return false; }

    static const std::string HIFI_PBR;
    static const std::string HIFI_SHADER_SIMPLE;

protected:
    std::string _name { "" };

private:
    std::string _model { HIFI_PBR };
    mutable MaterialKey _key { 0 };

    // Material properties
    glm::vec3 _emissive { DEFAULT_EMISSIVE };
    float _opacity { DEFAULT_OPACITY };
    glm::vec3 _albedo { DEFAULT_ALBEDO };
    float _roughness { DEFAULT_ROUGHNESS };
    float _metallic { DEFAULT_METALLIC };
    float _scattering { DEFAULT_SCATTERING };
    float _opacityCutoff { DEFAULT_OPACITY_CUTOFF };
    std::array<glm::mat4, NUM_TEXCOORD_TRANSFORMS> _texcoordTransforms;
    glm::vec2 _lightmapParams { 0.0, 1.0 };
    glm::vec2 _materialParams { 0.0, 1.0 };
    MaterialKey::CullFaceMode _cullFaceMode { DEFAULT_CULL_FACE_MODE };
    TextureMaps _textureMaps;

    bool _defaultFallthrough { false };
    std::unordered_map<uint, bool> _propertyFallthroughs { NUM_TOTAL_FLAGS };

    mutable std::recursive_mutex _textureMapsMutex;
};
typedef std::shared_ptr<Material> MaterialPointer;

class MaterialLayer {
public:
    MaterialLayer(MaterialPointer material, quint16 priority) : material(material), priority(priority) {}

    MaterialPointer material { nullptr };
    quint16 priority { 0 };
};

class MaterialLayerCompare {
public:
    bool operator() (MaterialLayer left, MaterialLayer right) {
        return left.priority < right.priority;
    }
};
typedef std::priority_queue<MaterialLayer, std::vector<MaterialLayer>, MaterialLayerCompare> MaterialLayerQueue;

class MultiMaterial : public MaterialLayerQueue {
public:
    MultiMaterial();

    void push(const MaterialLayer& value) {
        MaterialLayerQueue::push(value);
        _hasCalculatedTextureInfo = false;
        _needsUpdate = true;
    }

    bool remove(const MaterialPointer& value) {
        auto it = c.begin();
        while (it != c.end()) {
            if (it->material == value) {
                break;
            }
            it++;
        }
        if (it != c.end()) {
            c.erase(it);
            std::make_heap(c.begin(), c.end(), comp);
            _hasCalculatedTextureInfo = false;
            _needsUpdate = true;
            return true;
        } else {
            return false;
        }
    }

    // Schema to access the attribute values of the material
    class Schema {
    public:
        glm::vec3 _emissive { Material::DEFAULT_EMISSIVE }; // No Emissive
        float _opacity { Material::DEFAULT_OPACITY }; // Opacity = 1 => Not Transparent

        glm::vec3 _albedo { Material::DEFAULT_ALBEDO }; // Grey albedo => isAlbedo
        float _roughness { Material::DEFAULT_ROUGHNESS }; // Roughness = 1 => Not Glossy

        float _metallic { Material::DEFAULT_METALLIC }; // Not Metallic
        float _scattering { Material::DEFAULT_SCATTERING }; // Scattering info
        float _opacityCutoff { Material::DEFAULT_OPACITY_CUTOFF }; // Opacity cutoff applyed when using opacityMap as Mask 
        uint32_t _key { 0 }; // a copy of the materialKey

        // Texture Coord Transform Array
        glm::mat4 _texcoordTransforms[Material::NUM_TEXCOORD_TRANSFORMS];

        glm::vec2 _lightmapParams { 0.0, 1.0 };

        // x: material mode (0 for UV, 1 for PROJECTED)
        // y: 1 for texture repeat, 0 for discard outside of 0 - 1
        glm::vec2 _materialParams { 0.0, 1.0 };

        Schema() {
            for (auto& transform : _texcoordTransforms) {
                transform = glm::mat4();
            }
        }
    };

    gpu::BufferView& getSchemaBuffer() { return _schemaBuffer; }
    graphics::MaterialKey getMaterialKey() const { return graphics::MaterialKey(_schemaBuffer.get<graphics::MultiMaterial::Schema>()._key); }
    const gpu::TextureTablePointer& getTextureTable() const { return _textureTable; }

    void setCullFaceMode(graphics::MaterialKey::CullFaceMode cullFaceMode) { _cullFaceMode = cullFaceMode; }
    graphics::MaterialKey::CullFaceMode getCullFaceMode() const { return _cullFaceMode; }

    void setNeedsUpdate(bool needsUpdate) { _needsUpdate = needsUpdate; }
    void setTexturesLoading(bool value) { _texturesLoading = value; }
    void setInitialized() { _initialized = true; }

    bool shouldUpdate() const { return !_initialized || _needsUpdate || _texturesLoading || anyReferenceMaterialsOrTexturesChanged(); }

    int getTextureCount() const { calculateMaterialInfo(); return _textureCount; }
    size_t getTextureSize()  const { calculateMaterialInfo(); return _textureSize; }
    bool hasTextureInfo() const { return _hasCalculatedTextureInfo; }

    void resetReferenceTexturesAndMaterials();
    void addReferenceTexture(const std::function<gpu::TexturePointer()>& textureOperator);
    void addReferenceMaterial(const std::function<graphics::MaterialPointer()>& materialOperator);

private:
    gpu::BufferView _schemaBuffer;
    graphics::MaterialKey::CullFaceMode _cullFaceMode { graphics::Material::DEFAULT_CULL_FACE_MODE };
    gpu::TextureTablePointer _textureTable { std::make_shared<gpu::TextureTable>() };
    bool _needsUpdate { false };
    bool _texturesLoading { false };
    bool _initialized { false };

    mutable size_t _textureSize { 0 };
    mutable int _textureCount { 0 };
    mutable bool _hasCalculatedTextureInfo { false };
    void calculateMaterialInfo() const;

    bool anyReferenceMaterialsOrTexturesChanged() const;

    std::vector<std::pair<std::function<gpu::TexturePointer()>, gpu::TexturePointer>> _referenceTextures;
    std::vector<std::pair<std::function<graphics::MaterialPointer()>, graphics::MaterialPointer>> _referenceMaterials;
};

};

#endif
