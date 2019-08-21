//
//  Light.h
//  libraries/graphics/src/graphics
//
//  Created by Sam Gateau on 12/10/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_Light_h
#define hifi_model_Light_h

#include <bitset>
#include <map>

#include <glm/glm.hpp>
#include "Transform.h"
#include "gpu/Resource.h"
#include "gpu/Texture.h"

namespace graphics {
typedef gpu::BufferView UniformBufferView;
typedef gpu::TextureView TextureView;
typedef glm::vec3 Vec3;
typedef glm::vec4 Vec4;
typedef glm::quat Quat;


class Light {
public:

    struct LightVolume {
        vec3 position { 0.f };
        float radius { 1.0f };
        vec3 direction { 0.f, 0.f, -1.f };
        float spotCos { -1.f };

        bool isPoint() const { return bool(spotCos < 0.f); }
        bool isSpot() const { return bool(spotCos >= 0.f); }

        vec3 getPosition() const { return position; }
        float getRadius() const { return radius; }
        float getRadiusSquare() const { return radius * radius; }
        vec3 getDirection() const { return direction; }

        float getSpotAngleCos() const { return spotCos; }
        vec2 getSpotAngleCosSin() const { return vec2(spotCos, sqrt(1.f - spotCos * spotCos)); }
    };


    struct LightIrradiance {
        vec3 color { 1.f };
        float intensity { 1.f };
        float falloffRadius { 0.1f };
        float cutoffRadius { 0.1f };
        float falloffSpot { 1.f };
        float spare1;

        vec3 getColor() const { return color; }
        float getIntensity() const { return intensity; }
        vec3 getIrradiance() const { return color * intensity; }
        float getFalloffRadius() const { return falloffRadius; }
        float getCutoffRadius() const { return cutoffRadius; }
        float getFalloffSpot() const { return falloffSpot; }
    };


 
    enum Type {
        AMBIENT = 0,
        SUN,
        POINT,
        SPOT,

        NUM_TYPES,
    };

    typedef Vec3 Color;

    enum FlagBit {
        COLOR_BIT = 0,
        INTENSITY_BIT,
        RANGE_BIT,
        SPOT_BIT,
        TRANSFORM_BIT,

        NUM_FLAGS,
    };
    typedef std::bitset<NUM_FLAGS> Flags;

    Light();
    Light(const Light& light);
    Light& operator= (const Light& light);
    virtual ~Light();

    void setType(Type type);
    Type getType() const { return _type; }

    void setPosition(const Vec3& position);
    const Vec3& getPosition() const { return _transform.getTranslation(); }

    void setDirection(const Vec3& direction);
    const Vec3& getDirection() const;

    void setCastShadows(const bool castShadows);
    bool getCastShadows() const;

    void setShadowsMaxDistance(const float maxDistance);
    float getShadowsMaxDistance() const;

    void setShadowBias(float bias);
    float getShadowBias() const;

    void setOrientation(const Quat& orientation);
    const glm::quat& getOrientation() const { return _transform.getRotation(); }

    const Color& getColor() const { return _lightSchemaBuffer->irradiance.color; }
    void setColor(const Color& color);

    float getIntensity() const { return _lightSchemaBuffer->irradiance.intensity; }
    void setIntensity(float intensity);

    bool isRanged() const { return (getType() == POINT) || (getType() == SPOT); }
 
    // FalloffRradius is the physical radius of the light sphere through which energy shines,
    // expressed in meters. It is used only to calculate the falloff curve of the light.
    // Actual rendered lights will all have surface radii approaching 0.
    void setFalloffRadius(float radius);
    float getFalloffRadius() const { return _lightSchemaBuffer->irradiance.falloffRadius; }

    // Maximum radius is the cutoff radius of the light energy, expressed in meters.
    // It is used to bound light entities, and *will not* affect the falloff curve of the light.
    // Setting it low will result in a noticeable cutoff.
    void setMaximumRadius(float radius);
    float getMaximumRadius() const { return _lightSchemaBuffer->volume.radius; }

    // Spot properties
    bool isSpot() const { return getType() == SPOT; }
    void setSpotAngle(float angle);
    float getSpotAngle() const { return acos(_lightSchemaBuffer->volume.getSpotAngleCos()); }
    glm::vec2 getSpotAngleCosSin() const { return _lightSchemaBuffer->volume.getSpotAngleCosSin(); }
    void setSpotExponent(float exponent);
    float getSpotExponent() const { return _lightSchemaBuffer->irradiance.falloffSpot; }

    // If the light has an ambient (Indirect) component, then the Ambientintensity can be used to control its contribution to the lighting
    void setAmbientIntensity(float intensity);
    float getAmbientIntensity() const { return _ambientSchemaBuffer->intensity; }

    // Spherical Harmonics storing the Ambient lighting approximation used for the Sun typed light
    void setAmbientSphere(const gpu::SphericalHarmonics& sphere);
    const gpu::SphericalHarmonics& getAmbientSphere() const { return _ambientSchemaBuffer->ambientSphere; }
    void setAmbientSpherePreset(gpu::SphericalHarmonics::Preset preset);

    void setAmbientMap(const gpu::TexturePointer& ambientMap);
    gpu::TexturePointer getAmbientMap() const { return _ambientMap; }

    void setAmbientMapNumMips(uint16_t numMips);
    uint16_t getAmbientMapNumMips() const { return (uint16_t) _ambientSchemaBuffer->mapNumMips; }

    void setTransform(const glm::mat4& transform);

    // Light Schema
    class LightSchema {
    public:
        LightVolume volume;
        LightIrradiance irradiance;
    };

    class AmbientSchema {
    public:
        float intensity { 0.0f };
        float mapNumMips { 0.0f };
        float spare1;
        float spare2;

        gpu::SphericalHarmonics ambientSphere;
        glm::mat4 transform;
    };

    using LightSchemaBuffer = gpu::StructBuffer<LightSchema>;
    using AmbientSchemaBuffer = gpu::StructBuffer<AmbientSchema>;

    const LightSchemaBuffer& getLightSchemaBuffer() const { return _lightSchemaBuffer; }
    const AmbientSchemaBuffer& getAmbientSchemaBuffer() const { return _ambientSchemaBuffer; }

protected:

    Flags _flags{ 0 };

    LightSchemaBuffer _lightSchemaBuffer;
    AmbientSchemaBuffer _ambientSchemaBuffer;

    Transform _transform;

    gpu::TexturePointer _ambientMap;

    Type _type { SUN };
    float _spotCos { -1.0f }; // stored here to be able to reset the spot angle when turning the type spot on/off

    float _shadowsMaxDistance { 40.0f };
    float _shadowBias { 0.5f };
    bool _castShadows{ false };

    void updateLightRadius();
};
typedef std::shared_ptr< Light > LightPointer;

};

#endif
