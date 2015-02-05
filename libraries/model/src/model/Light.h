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

    void setType(Type type) { editSchema()._control.x = float(type); }
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
 
    void setMaximumRadius(float radius);
    float getMaximumRadius() const { return getSchema()._attenuation.w; }

    // Spot properties
    bool isSpot() const { return getType() == SPOT; }
    void setSpotAngle(float angle);
    float getSpotAngle() const { return getSchema()._spot.z; }
    void setSpotExponent(float exponent);
    float getSpotExponent() const { return getSchema()._spot.w; }

    // Schema to access the attribute values of the light
    class Schema {
    public:
        Vec4 _position; 
        Vec3 _direction;
        float _spare0;
        Color _color;
        float _intensity;
        Vec4 _attenuation;
        Vec4 _spot;
        Vec4 _shadow;

        Vec4 _control;

        Schema() :
            _position(0.0f, 0.0f, 0.0f, 1.0f), 
            _direction(0.0f, 0.0f, -1.0f),
            _spare0(0.f), 
            _color(1.0f),
            _intensity(1.0f),
            _attenuation(1.0f, 1.0f, 1.0f, 1.0f),
            _spot(0.0f, 0.0f, 0.0f, 3.0f),
            _control(0.0f)
            {}
    };

    const UniformBufferView& getSchemaBuffer() const { return _schemaBuffer; }

protected:

    Flags _flags;
    UniformBufferView _schemaBuffer;
    Transform _transform;

    const Schema& getSchema() const { return _schemaBuffer.get<Schema>(); }
    Schema& editSchema() { return _schemaBuffer.edit<Schema>(); }
};
typedef QSharedPointer< Light > LightPointer;

};

#endif
