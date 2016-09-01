//
//  Light.h
//  libraries/model/src/model
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

namespace model {
typedef gpu::BufferView UniformBufferView;
typedef gpu::TextureView TextureView;
typedef glm::vec3 Vec3;
typedef glm::vec4 Vec4;
typedef glm::quat Quat;

class Light {
public:
    enum Type {
        SUN = 0,
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
    Type getType() const { return Type((int) getSchema()._control.x); }

    void setPosition(const Vec3& position);
    const Vec3& getPosition() const { return _transform.getTranslation(); }

    void setDirection(const Vec3& direction);
    const Vec3& getDirection() const;

    void setOrientation(const Quat& orientation);
    const glm::quat& getOrientation() const { return _transform.getRotation(); }

    const Color& getColor() const { return getSchema()._color; }
    void setColor(const Color& color);

    float getIntensity() const { return getSchema()._intensity; }
    void setIntensity(float intensity);

    bool isRanged() const { return (getType() == POINT) || (getType() == SPOT ); }
 
    // FalloffRradius is the physical radius of the light sphere through which energy shines,
    // expressed in meters. It is used only to calculate the falloff curve of the light.
    // Actual rendered lights will all have surface radii approaching 0.
    void setFalloffRadius(float radius);
    float getFalloffRadius() const { return getSchema()._attenuation.x; }

    // Maximum radius is the cutoff radius of the light energy, expressed in meters.
    // It is used to bound light entities, and *will not* affect the falloff curve of the light.
    // Setting it low will result in a noticeable cutoff.
    void setMaximumRadius(float radius);
    float getMaximumRadius() const { return getSchema()._attenuation.y; }

    // Spot properties
    bool isSpot() const { return getType() == SPOT; }
    void setSpotAngle(float angle);
    float getSpotAngle() const { return getSchema()._spot.z; }
    glm::vec2 getSpotAngleCosSin() const { return glm::vec2(getSchema()._spot.x, getSchema()._spot.y); }
    void setSpotExponent(float exponent);
    float getSpotExponent() const { return getSchema()._spot.w; }

    // For editing purpose, show the light volume contour.
    // Set to non 0 to show it, the value is used as the intensity of the contour color
    void setShowContour(float show);
    float getShowContour() const { return getSchema()._control.z; }

    // If the light has an ambient (Indirect) component, then the Ambientintensity can be used to control its contribution to the lighting
    void setAmbientIntensity(float intensity);
    float getAmbientIntensity() const { return getSchema()._ambientIntensity; }

    // Spherical Harmonics storing the Ambient lighting approximation used for the Sun typed light
    void setAmbientSphere(const gpu::SphericalHarmonics& sphere);
    const gpu::SphericalHarmonics& getAmbientSphere() const { return getSchema()._ambientSphere; }
    void setAmbientSpherePreset(gpu::SphericalHarmonics::Preset preset);

    void setAmbientMap(gpu::TexturePointer ambientMap);
    gpu::TexturePointer getAmbientMap() const { return _ambientMap; }

    void setAmbientMapNumMips(uint16_t numMips);
    uint16_t getAmbientMapNumMips() const { return (uint16_t) getSchema()._ambientMapNumMips; }

    // Schema to access the attribute values of the light
    class Schema {
    public:
        Vec4 _position{0.0f, 0.0f, 0.0f, 1.0f}; 
        Vec3 _direction{0.0f, 0.0f, -1.0f};
        float _ambientIntensity{0.0f};
        Color _color{1.0f};
        float _intensity{1.0f};
        Vec4 _attenuation{0.1f, 1.0f, 0.0f, 0.0f};
        Vec4 _spot{0.0f, 0.0f, 0.0f, 0.0f};
        Vec4 _shadow{0.0f};


        float _ambientMapNumMips{ 0.0f };
        Vec3 _control{ 0.0f, 0.0f, 0.0f };

        Vec4 _volumeGeometry { 1.f };

        gpu::SphericalHarmonics _ambientSphere;
    };

    const UniformBufferView& getSchemaBuffer() const { return _schemaBuffer; }

protected:

    Flags _flags{ 0 };
    UniformBufferView _schemaBuffer;
    Transform _transform;

    gpu::TexturePointer _ambientMap;

    const Schema& getSchema() const { return _schemaBuffer.get<Schema>(); }
    Schema& editSchema() { return _schemaBuffer.edit<Schema>(); }

    void updateLightRadius();
    void updateVolumeGeometry();

};
typedef std::shared_ptr< Light > LightPointer;

};

#endif
