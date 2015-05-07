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

class SphericalHarmonics {
public:
    glm::vec3 L00    ; float spare0;
    glm::vec3 L1m1   ; float spare1;
    glm::vec3 L10    ; float spare2;
    glm::vec3 L11    ; float spare3;
    glm::vec3 L2m2   ; float spare4;
    glm::vec3 L2m1   ; float spare5;
    glm::vec3 L20    ; float spare6;
    glm::vec3 L21    ; float spare7;
    glm::vec3 L22    ; float spare8;

    static const int NUM_COEFFICIENTS = 9;

    enum Preset {
        OLD_TOWN_SQUARE = 0,
        GRACE_CATHEDRAL,
        EUCALYPTUS_GROVE,
        ST_PETERS_BASILICA,
        UFFIZI_GALLERY,
        GALILEOS_TOMB,
        VINE_STREET_KITCHEN,
        BREEZEWAY,
        CAMPUS_SUNSET,
        FUNSTON_BEACH_SUNSET,

        NUM_PRESET,
    };

    void assignPreset(int p) {
        switch (p) {
        case OLD_TOWN_SQUARE: {
                L00    = glm::vec3( 0.871297f, 0.875222f, 0.864470f);
                L1m1   = glm::vec3( 0.175058f, 0.245335f, 0.312891f);
                L10    = glm::vec3( 0.034675f, 0.036107f, 0.037362f);
                L11    = glm::vec3(-0.004629f,-0.029448f,-0.048028f);
                L2m2   = glm::vec3(-0.120535f,-0.121160f,-0.117507f);
                L2m1   = glm::vec3( 0.003242f, 0.003624f, 0.007511f);
                L20    = glm::vec3(-0.028667f,-0.024926f,-0.020998f);
                L21    = glm::vec3(-0.077539f,-0.086325f,-0.091591f);
                L22    = glm::vec3(-0.161784f,-0.191783f,-0.219152f);
            }
            break;
        case GRACE_CATHEDRAL: {
                L00    = glm::vec3( 0.79f,  0.44f,  0.54f);
                L1m1   = glm::vec3( 0.39f,  0.35f,  0.60f);
                L10    = glm::vec3(-0.34f, -0.18f, -0.27f);
                L11    = glm::vec3(-0.29f, -0.06f,  0.01f);
                L2m2   = glm::vec3(-0.11f, -0.05f, -0.12f);
                L2m1   = glm::vec3(-0.26f, -0.22f, -0.47f);
                L20    = glm::vec3(-0.16f, -0.09f, -0.15f);
                L21    = glm::vec3( 0.56f,  0.21f,  0.14f);
                L22    = glm::vec3( 0.21f, -0.05f, -0.30f);
            }
            break;
        case EUCALYPTUS_GROVE: {
                L00    = glm::vec3( 0.38f,  0.43f,  0.45f);
                L1m1   = glm::vec3( 0.29f,  0.36f,  0.41f);
                L10    = glm::vec3( 0.04f,  0.03f,  0.01f);
                L11    = glm::vec3(-0.10f, -0.10f, -0.09f);
                L2m2   = glm::vec3(-0.06f, -0.06f, -0.04f);
                L2m1   = glm::vec3( 0.01f, -0.01f, -0.05f);
                L20    = glm::vec3(-0.09f, -0.13f, -0.15f);
                L21    = glm::vec3(-0.06f, -0.05f, -0.04f);
                L22    = glm::vec3( 0.02f,  0.00f, -0.05f);
            }
            break;
        case ST_PETERS_BASILICA: {
                L00    = glm::vec3( 0.36f,  0.26f,  0.23f);
                L1m1   = glm::vec3( 0.18f,  0.14f,  0.13f);
                L10    = glm::vec3(-0.02f, -0.01f,  0.00f);
                L11    = glm::vec3( 0.03f,  0.02f, -0.00f);
                L2m2   = glm::vec3( 0.02f,  0.01f, -0.00f);
                L2m1   = glm::vec3(-0.05f, -0.03f, -0.01f);
                L20    = glm::vec3(-0.09f, -0.08f, -0.07f);
                L21    = glm::vec3( 0.01f,  0.00f,  0.00f);
                L22    = glm::vec3(-0.08f, -0.03f, -0.00f);
            }
            break;
        case UFFIZI_GALLERY: {
                L00    = glm::vec3( 0.32f,  0.31f,  0.35f);
                L1m1   = glm::vec3( 0.37f,  0.37f,  0.43f);
                L10    = glm::vec3( 0.00f,  0.00f,  0.00f);
                L11    = glm::vec3(-0.01f, -0.01f, -0.01f);
                L2m2   = glm::vec3(-0.02f, -0.02f, -0.03f);
                L2m1   = glm::vec3(-0.01f, -0.01f, -0.01f);
                L20    = glm::vec3(-0.28f, -0.28f, -0.32f);
                L21    = glm::vec3( 0.00f,  0.00f,  0.00f);
                L22    = glm::vec3(-0.24f, -0.24f, -0.28f);
            }
            break;
        case GALILEOS_TOMB: {
                L00    = glm::vec3( 1.04f,  0.76f,  0.71f);
                L1m1   = glm::vec3( 0.44f,  0.34f,  0.34f);
                L10    = glm::vec3(-0.22f, -0.18f, -0.17f);
                L11    = glm::vec3( 0.71f,  0.54f,  0.56f);
                L2m2   = glm::vec3( 0.64f,  0.50f,  0.52f);
                L2m1   = glm::vec3(-0.12f, -0.09f, -0.08f);
                L20    = glm::vec3(-0.37f, -0.28f, -0.32f);
                L21    = glm::vec3(-0.17f, -0.13f, -0.13f);
                L22    = glm::vec3( 0.55f,  0.42f,  0.42f);
            }
            break;
        case VINE_STREET_KITCHEN: {
                L00    = glm::vec3( 0.64f,  0.67f,  0.73f);
                L1m1   = glm::vec3( 0.28f,  0.32f,  0.33f);
                L10    = glm::vec3( 0.42f,  0.60f,  0.77f);
                L11    = glm::vec3(-0.05f, -0.04f, -0.02f);
                L2m2   = glm::vec3(-0.10f, -0.08f, -0.05f);
                L2m1   = glm::vec3( 0.25f,  0.39f,  0.53f);
                L20    = glm::vec3( 0.38f,  0.54f,  0.71f);
                L21    = glm::vec3( 0.06f,  0.01f, -0.02f);
                L22    = glm::vec3(-0.03f, -0.02f, -0.03f);
            }
            break;
        case BREEZEWAY: {
                L00    = glm::vec3( 0.32f,  0.36f,  0.38f);
                L1m1   = glm::vec3( 0.37f,  0.41f,  0.45f);
                L10    = glm::vec3(-0.01f, -0.01f, -0.01f);
                L11    = glm::vec3(-0.10f, -0.12f, -0.12f);
                L2m2   = glm::vec3(-0.13f, -0.15f, -0.17f);
                L2m1   = glm::vec3(-0.01f, -0.02f,  0.02f);
                L20    = glm::vec3(-0.07f, -0.08f, -0.09f);
                L21    = glm::vec3( 0.02f,  0.03f,  0.03f);
                L22    = glm::vec3(-0.29f, -0.32f, -0.36f);
            }
            break;
        case CAMPUS_SUNSET: {
                L00    = glm::vec3( 0.79f,  0.94f,  0.98f);
                L1m1   = glm::vec3( 0.44f,  0.56f,  0.70f);
                L10    = glm::vec3(-0.10f, -0.18f, -0.27f);
                L11    = glm::vec3( 0.45f,  0.38f,  0.20f);
                L2m2   = glm::vec3( 0.18f,  0.14f,  0.05f);
                L2m1   = glm::vec3(-0.14f, -0.22f, -0.31f);
                L20    = glm::vec3(-0.39f, -0.40f, -0.36f);
                L21    = glm::vec3( 0.09f,  0.07f,  0.04f);
                L22    = glm::vec3( 0.67f,  0.67f,  0.52f);
            }
            break;
        case FUNSTON_BEACH_SUNSET: {
                L00    = glm::vec3( 0.68f,  0.69f,  0.70f);
                L1m1   = glm::vec3( 0.32f,  0.37f,  0.44f);
                L10    = glm::vec3(-0.17f, -0.17f, -0.17f);
                L11    = glm::vec3(-0.45f, -0.42f, -0.34f);
                L2m2   = glm::vec3(-0.17f, -0.17f, -0.15f);
                L2m1   = glm::vec3(-0.08f, -0.09f, -0.10f);
                L20    = glm::vec3(-0.03f, -0.02f, -0.01f);
                L21    = glm::vec3( 0.16f,  0.14f,  0.10f);
                L22    = glm::vec3( 0.37f,  0.31f,  0.20f);
            }
            break;
        }
    }
};

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

    // For editing purpose, show the light volume contour.
    // Set to non 0 to show it, the value is used as the intensity of the contour color
    void setShowContour(float show);
    float getShowContour() const { return getSchema()._control.w; }

    // If the light has an ambient (Indirect) component, then the Ambientintensity can be used to control its contribution to the lighting
    void setAmbientIntensity(float intensity);
    float getAmbientIntensity() const { return getSchema()._ambientIntensity; }

    // Spherical Harmonics storing the Ambien lighting approximation used for the Sun typed light
    void setAmbientSphere(const SphericalHarmonics& sphere) { _ambientSphere = sphere; }
    const SphericalHarmonics& getAmbientSphere() const { return _ambientSphere; }
    void setAmbientSpherePreset(SphericalHarmonics::Preset preset) { _ambientSphere.assignPreset(preset); }

    // Schema to access the attribute values of the light
    class Schema {
    public:
        Vec4 _position{0.0f, 0.0f, 0.0f, 1.0f}; 
        Vec3 _direction{0.0f, 0.0f, -1.0f};
        float _ambientIntensity{0.0f};
        Color _color{1.0f};
        float _intensity{1.0f};
        Vec4 _attenuation{1.0f};
        Vec4 _spot{0.0f, 0.0f, 0.0f, 3.0f};
        Vec4 _shadow{0.0f};

        Vec4 _control{0.0f, 0.0f, 0.0f, 0.0f};

        Schema() {}
    };

    const UniformBufferView& getSchemaBuffer() const { return _schemaBuffer; }

protected:

    Flags _flags;
    UniformBufferView _schemaBuffer;
    Transform _transform;
    SphericalHarmonics _ambientSphere;

    const Schema& getSchema() const { return _schemaBuffer.get<Schema>(); }
    Schema& editSchema() { return _schemaBuffer.edit<Schema>(); }
};
typedef std::shared_ptr< Light > LightPointer;

};

#endif
