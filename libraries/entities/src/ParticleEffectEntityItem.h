//
//  ParticleEffectEntityItem.h
//  libraries/entities/src
//
//  Created by Jason Rickwald on 3/2/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ParticleEffectEntityItem_h
#define hifi_ParticleEffectEntityItem_h

#include <deque>

#include "EntityItem.h"

#include "ColorUtils.h"
#include "PulsePropertyGroup.h"

namespace particle {
    static const float SCRIPT_MAXIMUM_PI = 3.1416f;  // Round up so that reasonable property values work
    static const float UNINITIALIZED = NAN;
    static const u8vec3 DEFAULT_COLOR = ENTITY_ITEM_DEFAULT_COLOR;
    static const vec3 DEFAULT_COLOR_UNINITIALIZED = { UNINITIALIZED, UNINITIALIZED, UNINITIALIZED };
    static const u8vec3 DEFAULT_COLOR_SPREAD = { 0, 0, 0 };
    static const float DEFAULT_ALPHA = ENTITY_ITEM_DEFAULT_ALPHA;
    static const float DEFAULT_ALPHA_SPREAD = 0.0f;
    static const float DEFAULT_ALPHA_START = UNINITIALIZED;
    static const float DEFAULT_ALPHA_FINISH = UNINITIALIZED;
    static const float MINIMUM_ALPHA = 0.0f;
    static const float MAXIMUM_ALPHA = 1.0f;
    static const quint32 DEFAULT_MAX_PARTICLES = 1000;
    static const quint32 MINIMUM_MAX_PARTICLES = 1;
    static const quint32 MAXIMUM_MAX_PARTICLES = 100000;
    static const float DEFAULT_LIFESPAN = 3.0f;
    static const float MINIMUM_LIFESPAN = 0.0f;
    static const float MAXIMUM_LIFESPAN = 86400.0f;  // 1 day
    static const float DEFAULT_EMIT_RATE = 15.0f;
    static const float MINIMUM_EMIT_RATE = 0.0f;
    static const float MAXIMUM_EMIT_RATE = 100000.0f;
    static const float DEFAULT_EMIT_SPEED = 5.0f;
    static const float MINIMUM_EMIT_SPEED = -1000.0f;
    static const float MAXIMUM_EMIT_SPEED = 1000.0f;  // Approx mach 3
    static const float DEFAULT_SPEED_SPREAD = 1.0f;
    static const quat DEFAULT_EMIT_ORIENTATION = glm::angleAxis(-PI_OVER_TWO, Vectors::UNIT_X);  // Vertical
    static const vec3 DEFAULT_EMIT_DIMENSIONS = Vectors::ZERO;  // Emit from point
    static const float MINIMUM_EMIT_DIMENSION = 0.0f;
    static const float MAXIMUM_EMIT_DIMENSION = (float)TREE_SCALE;
    static const float DEFAULT_EMIT_RADIUS_START = 1.0f;  // Emit from surface (when emitDimensions > 0)
    static const float MINIMUM_EMIT_RADIUS_START = 0.0f;
    static const float MAXIMUM_EMIT_RADIUS_START = 1.0f;
    static const float MINIMUM_POLAR = 0.0f;
    static const float MAXIMUM_POLAR = SCRIPT_MAXIMUM_PI;
    static const float DEFAULT_POLAR_START = 0.0f;  // Emit along z-axis
    static const float DEFAULT_POLAR_FINISH = 0.0f; // ""
    static const float MINIMUM_AZIMUTH = -SCRIPT_MAXIMUM_PI;
    static const float MAXIMUM_AZIMUTH = SCRIPT_MAXIMUM_PI;
    static const float DEFAULT_AZIMUTH_START = -PI;  // Emit full circumference (when polarFinish > 0)
    static const float DEFAULT_AZIMUTH_FINISH = PI;  // ""
    static const vec3 DEFAULT_EMIT_ACCELERATION(0.0f, -9.8f, 0.0f);
    static const float MINIMUM_EMIT_ACCELERATION = -100.0f; // ~ 10g
    static const float MAXIMUM_EMIT_ACCELERATION = 100.0f;
    static const vec3 DEFAULT_ACCELERATION_SPREAD(0.0f, 0.0f, 0.0f);
    static const float MINIMUM_ACCELERATION_SPREAD = 0.0f;
    static const float MAXIMUM_ACCELERATION_SPREAD = 100.0f;
    static const float DEFAULT_PARTICLE_RADIUS = 0.025f;
    static const float MINIMUM_PARTICLE_RADIUS = 0.0f;
    static const float MAXIMUM_PARTICLE_RADIUS = (float)TREE_SCALE;
    static const float DEFAULT_RADIUS_SPREAD = 0.0f;
    static const float DEFAULT_RADIUS_START = UNINITIALIZED;
    static const float DEFAULT_RADIUS_FINISH = UNINITIALIZED;
    static const float DEFAULT_PARTICLE_SPIN = 0.0f;
    static const float DEFAULT_SPIN_START = UNINITIALIZED;
    static const float DEFAULT_SPIN_FINISH = UNINITIALIZED;
    static const float DEFAULT_SPIN_SPREAD = 0.0f;
    static const float MINIMUM_PARTICLE_SPIN = -2.0f * SCRIPT_MAXIMUM_PI;
    static const float MAXIMUM_PARTICLE_SPIN = 2.0f * SCRIPT_MAXIMUM_PI;
    static const QString DEFAULT_TEXTURES = "";
    static const bool DEFAULT_EMITTER_SHOULD_TRAIL = false;
    static const bool DEFAULT_ROTATE_WITH_ENTITY = false;
    static const ShapeType DEFAULT_SHAPE_TYPE = ShapeType::SHAPE_TYPE_ELLIPSOID;

    template <typename T>
    struct Range {
        Range() {}
        Range(const Range& other) { *this = other; }
        Range(const T& defaultStart, const T& defaultFinish)
            : start(defaultStart), finish(defaultFinish) {
        }

        Range& operator=(const Range& other) {
            start = other.start;
            finish = other.finish;
            return *this;
        }

        T start;
        T finish;
    };

    template <typename T>
    struct Gradient {
        Gradient() {}
        Gradient(const Gradient& other) { *this = other; }
        Gradient(const T& defaultTarget, const T& defaultSpread)
            : target(defaultTarget), spread(defaultSpread) {}

        Gradient& operator=(const Gradient& other) {
            target = other.target;
            spread = other.spread;
            return *this;
        }

        T target;
        T spread;
    };

    template <typename T>
    struct RangeGradient {
        RangeGradient() {}
        RangeGradient(const RangeGradient& other) { *this = other; }
        RangeGradient(const T& defaultValue, const T& defaultStart, const T& defaultFinish, const T& defaultSpread)
            : range(defaultStart, defaultFinish), gradient(defaultValue, defaultSpread) {
        }

        RangeGradient& operator=(const RangeGradient& other) {
            range = other.range;
            gradient = other.gradient;
            return *this;
        }

        Range<T> range;
        Gradient<T> gradient;
    };

    struct EmitProperties {
        float rate { DEFAULT_EMIT_RATE };
        Gradient<float> speed { DEFAULT_EMIT_SPEED, DEFAULT_SPEED_SPREAD };
        Gradient<vec3> acceleration { DEFAULT_EMIT_ACCELERATION, DEFAULT_ACCELERATION_SPREAD };
        glm::quat orientation { DEFAULT_EMIT_ORIENTATION };
        glm::vec3 dimensions { DEFAULT_EMIT_DIMENSIONS };
        bool shouldTrail { DEFAULT_EMITTER_SHOULD_TRAIL };

        EmitProperties() {};
        EmitProperties(const EmitProperties& other) { *this = other; }
        EmitProperties& operator=(const EmitProperties& other) {
            rate = other.rate;
            speed = other.speed;
            acceleration = other.acceleration;
            orientation = other.orientation;
            dimensions = other.dimensions;
            shouldTrail = other.shouldTrail;
            return *this;
        }
    };

    struct Properties {
        RangeGradient<vec3> color { DEFAULT_COLOR, DEFAULT_COLOR_UNINITIALIZED, DEFAULT_COLOR_UNINITIALIZED, DEFAULT_COLOR_SPREAD };
        RangeGradient<float> alpha { DEFAULT_ALPHA, DEFAULT_ALPHA_START, DEFAULT_ALPHA_FINISH, DEFAULT_ALPHA_SPREAD };
        float radiusStart { DEFAULT_EMIT_RADIUS_START };
        RangeGradient<float> radius { DEFAULT_PARTICLE_RADIUS, DEFAULT_RADIUS_START, DEFAULT_RADIUS_FINISH, DEFAULT_RADIUS_SPREAD };
        RangeGradient<float> spin { DEFAULT_PARTICLE_SPIN, DEFAULT_SPIN_START, DEFAULT_SPIN_FINISH, DEFAULT_SPIN_SPREAD };
        bool rotateWithEntity { DEFAULT_ROTATE_WITH_ENTITY };
        float lifespan { DEFAULT_LIFESPAN };
        uint32_t maxParticles { DEFAULT_MAX_PARTICLES };
        EmitProperties emission;
        Range<float> polar { DEFAULT_POLAR_START, DEFAULT_POLAR_FINISH };
        Range<float> azimuth { DEFAULT_AZIMUTH_START, DEFAULT_AZIMUTH_FINISH };
        QString textures;


        Properties() {};
        Properties(const Properties& other) { *this = other; }
        bool valid() const;
        bool emitting() const;
        uint64_t emitIntervalUsecs() const;
        
        Properties& operator =(const Properties& other) {
            color = other.color;
            alpha = other.alpha;
            radiusStart = other.radiusStart;
            radius = other.radius;
            spin = other.spin;
            rotateWithEntity = other.rotateWithEntity;
            lifespan = other.lifespan;
            maxParticles = other.maxParticles;
            emission = other.emission;
            polar = other.polar;
            azimuth = other.azimuth;
            textures = other.textures;
            return *this;
        }

        vec4 getColorStart() const { return vec4(ColorUtils::sRGBToLinearVec3(toGlm(color.range.start)), alpha.range.start); }
        vec4 getColorMiddle() const { return vec4(ColorUtils::sRGBToLinearVec3(toGlm(color.gradient.target)), alpha.gradient.target); }
        vec4 getColorFinish() const { return vec4(ColorUtils::sRGBToLinearVec3(toGlm(color.range.finish)), alpha.range.finish); }
        vec4 getColorSpread() const { return vec4(ColorUtils::sRGBToLinearVec3(toGlm(color.gradient.spread)), alpha.gradient.spread); }
    };
} // namespace particles


bool operator==(const particle::Properties& a, const particle::Properties& b);
bool operator!=(const particle::Properties& a, const particle::Properties& b);


class ParticleEffectEntityItem : public EntityItem {
public:
    ALLOW_INSTANTIATION // This class can be instantiated

    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ParticleEffectEntityItem(const EntityItemID& entityItemID);

    // methods for getting/setting all properties of this entity
    virtual EntityItemProperties getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const override;
    virtual bool setSubClassProperties(const EntityItemProperties& properties) override;

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                 bool& somethingChanged) override;

    bool shouldBePhysical() const override { return false; }

    virtual void debugDump() const override;

    void setColor(const glm::u8vec3& value);
    glm::u8vec3 getColor() const;

    void setColorStart(const vec3& colorStart);
    vec3 getColorStart() const;

    void setColorFinish(const vec3& colorFinish);
    vec3 getColorFinish() const;

    void setColorSpread(const glm::u8vec3& colorSpread);
    glm::u8vec3 getColorSpread() const;

    void setAlpha(float alpha);
    float getAlpha() const;

    void setAlphaStart(float alphaStart);
    float getAlphaStart() const;

    void setAlphaFinish(float alphaFinish);
    float getAlphaFinish() const;

    void setAlphaSpread(float alphaSpread);
    float getAlphaSpread() const;

    void setShapeType(ShapeType type) override;
    virtual ShapeType getShapeType() const override;

    QString getCompoundShapeURL() const;
    virtual void setCompoundShapeURL(const QString& url);

    bool getIsEmitting() const { return _isEmitting; }
    void setIsEmitting(bool isEmitting);

    void setMaxParticles(quint32 maxParticles);
    quint32 getMaxParticles() const;

    void setLifespan(float lifespan);
    float getLifespan() const;

    void setEmitRate(float emitRate);
    float getEmitRate() const;

    void setEmitSpeed(float emitSpeed);
    float getEmitSpeed() const;

    void setSpeedSpread(float speedSpread);
    float getSpeedSpread() const;

    void setEmitOrientation(const glm::quat& emitOrientation);
    glm::quat getEmitOrientation() const;

    void setEmitDimensions(const glm::vec3& emitDimensions);
    glm::vec3 getEmitDimensions() const;

    void setEmitRadiusStart(float emitRadiusStart);
    float getEmitRadiusStart() const;

    void setPolarStart(float polarStart);
    float getPolarStart() const;

    void setPolarFinish(float polarFinish);
    float getPolarFinish() const;

    void setAzimuthStart(float azimuthStart);
    float getAzimuthStart() const;

    void setAzimuthFinish(float azimuthFinish);
    float getAzimuthFinish() const;

    void setEmitAcceleration(const glm::vec3& emitAcceleration);
    glm::vec3 getEmitAcceleration() const;
    
    void setAccelerationSpread(const glm::vec3& accelerationSpread);
    glm::vec3 getAccelerationSpread() const;

    void setParticleRadius(float particleRadius);
    float getParticleRadius() const;

    void setRadiusStart(float radiusStart);
    float getRadiusStart() const;

    void setRadiusFinish(float radiusFinish);
    float getRadiusFinish() const;

    void setRadiusSpread(float radiusSpread);
    float getRadiusSpread() const;

    void setParticleSpin(float particleSpin);
    float getParticleSpin() const;

    void setSpinStart(float spinStart);
    float getSpinStart() const;

    void setSpinFinish(float spinFinish);
    float getSpinFinish() const;

    void setSpinSpread(float spinSpread);
    float getSpinSpread() const;

    void setRotateWithEntity(bool rotateWithEntity);
    bool getRotateWithEntity() const { return _particleProperties.rotateWithEntity; }

    void computeAndUpdateDimensions();

    void setTextures(const QString& textures);
    QString getTextures() const;

    void setEmitterShouldTrail(bool emitterShouldTrail);
    bool getEmitterShouldTrail() const { return _particleProperties.emission.shouldTrail; }

    virtual bool supportsDetailedIntersection() const override { return false; }

    particle::Properties getParticleProperties() const;
    PulsePropertyGroup getPulseProperties() const;

protected:
    particle::Properties _particleProperties;
    PulsePropertyGroup _pulseProperties;
    bool _isEmitting { true };

    ShapeType _shapeType{ particle::DEFAULT_SHAPE_TYPE };
    QString _compoundShapeURL { "" };
};

#endif // hifi_ParticleEffectEntityItem_h
