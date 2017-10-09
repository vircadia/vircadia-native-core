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

namespace particle {
    static const float SCRIPT_MAXIMUM_PI = 3.1416f;  // Round up so that reasonable property values work
    static const xColor DEFAULT_COLOR = { 255, 255, 255 };
    static const xColor DEFAULT_COLOR_SPREAD = { 0, 0, 0 };
    static const float DEFAULT_ALPHA = 1.0f;
    static const float DEFAULT_ALPHA_SPREAD = 0.0f;
    static const float DEFAULT_ALPHA_START = DEFAULT_ALPHA;
    static const float DEFAULT_ALPHA_FINISH = DEFAULT_ALPHA;
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
    static const float MINIMUM_EMIT_SPEED = 0.0f;
    static const float MAXIMUM_EMIT_SPEED = 1000.0f;  // Approx mach 3
    static const float DEFAULT_SPEED_SPREAD = 1.0f;
    static const glm::quat DEFAULT_EMIT_ORIENTATION = glm::angleAxis(-PI_OVER_TWO, Vectors::UNIT_X);  // Vertical
    static const glm::vec3 DEFAULT_EMIT_DIMENSIONS = Vectors::ZERO;  // Emit from point
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
    static const glm::vec3 DEFAULT_EMIT_ACCELERATION(0.0f, -9.8f, 0.0f);
    static const float MINIMUM_EMIT_ACCELERATION = -100.0f; // ~ 10g
    static const float MAXIMUM_EMIT_ACCELERATION = 100.0f;
    static const glm::vec3 DEFAULT_ACCELERATION_SPREAD(0.0f, 0.0f, 0.0f);
    static const float MINIMUM_ACCELERATION_SPREAD = 0.0f;
    static const float MAXIMUM_ACCELERATION_SPREAD = 100.0f;
    static const float DEFAULT_PARTICLE_RADIUS = 0.025f;
    static const float MINIMUM_PARTICLE_RADIUS = 0.0f;
    static const float MAXIMUM_PARTICLE_RADIUS = (float)TREE_SCALE;
    static const float DEFAULT_RADIUS_SPREAD = 0.0f;
    static const float DEFAULT_RADIUS_START = DEFAULT_PARTICLE_RADIUS;
    static const float DEFAULT_RADIUS_FINISH = DEFAULT_PARTICLE_RADIUS;
    static const QString DEFAULT_TEXTURES = "";
    static const bool DEFAULT_EMITTER_SHOULD_TRAIL = false;

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
        RangeGradient<xColor> color{ DEFAULT_COLOR, DEFAULT_COLOR, DEFAULT_COLOR, DEFAULT_COLOR_SPREAD };
        RangeGradient<float> alpha{ DEFAULT_ALPHA, DEFAULT_ALPHA_START, DEFAULT_ALPHA_FINISH, DEFAULT_ALPHA_SPREAD };
        float radiusStart{ DEFAULT_EMIT_RADIUS_START };
        RangeGradient<float> radius{ DEFAULT_PARTICLE_RADIUS, DEFAULT_RADIUS_START, DEFAULT_RADIUS_FINISH, DEFAULT_RADIUS_SPREAD };
        float lifespan{ DEFAULT_LIFESPAN };
        uint32_t maxParticles{ DEFAULT_MAX_PARTICLES };
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
            radius = other.radius;
            lifespan = other.lifespan;
            maxParticles = other.maxParticles;
            emission = other.emission;
            polar = other.polar;
            azimuth = other.azimuth;
            textures = other.textures;
            return *this;
        }

        glm::vec4 getColorStart() const { return glm::vec4(ColorUtils::sRGBToLinearVec3(toGlm(color.range.start)), alpha.range.start); }
        glm::vec4 getColorMiddle() const { return glm::vec4(ColorUtils::sRGBToLinearVec3(toGlm(color.gradient.target)), alpha.gradient.target); }
        glm::vec4 getColorFinish() const { return glm::vec4(ColorUtils::sRGBToLinearVec3(toGlm(color.range.finish)), alpha.range.finish); }
        glm::vec4 getColorSpread() const { return glm::vec4(ColorUtils::sRGBToLinearVec3(toGlm(color.gradient.spread)), alpha.gradient.spread); }
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
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;

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

    const rgbColor& getColor() const { return _particleColorHack; }
    xColor getXColor() const { return _particleProperties.color.gradient.target; }
    glm::vec3 getColorRGB() const { return  ColorUtils::sRGBToLinearVec3(toGlm(getXColor())); }

    void setColor(const rgbColor& value);
    void setColor(const xColor& value);

    bool _isColorStartInitialized = false;
    void setColorStart(const xColor& colorStart) { _particleProperties.color.range.start = colorStart; _isColorStartInitialized = true; }
    xColor getColorStart() const { return _isColorStartInitialized ? _particleProperties.color.range.start : getXColor(); }
    glm::vec3 getColorStartRGB() const { return _isColorStartInitialized ? ColorUtils::sRGBToLinearVec3(toGlm(_particleProperties.color.range.start)) : getColorRGB(); }

    bool _isColorFinishInitialized = false;
    void setColorFinish(const xColor& colorFinish) { _particleProperties.color.range.finish = colorFinish; _isColorFinishInitialized = true; }
    xColor getColorFinish() const { return _isColorFinishInitialized ? _particleProperties.color.range.finish : getXColor(); }
    glm::vec3 getColorFinishRGB() const { return _isColorFinishInitialized ? ColorUtils::sRGBToLinearVec3(toGlm(_particleProperties.color.range.finish)) : getColorRGB(); }

    void setColorSpread(const xColor& colorSpread) { _particleProperties.color.gradient.spread = colorSpread; }
    xColor getColorSpread() const { return _particleProperties.color.gradient.spread; }
    glm::vec3 getColorSpreadRGB() const { return ColorUtils::sRGBToLinearVec3(toGlm(_particleProperties.color.gradient.spread)); }

    void setAlpha(float alpha);
    float getAlpha() const { return _particleProperties.alpha.gradient.target; }

    bool _isAlphaStartInitialized = false;
    void setAlphaStart(float alphaStart);
    float getAlphaStart() const { return _isAlphaStartInitialized ? _particleProperties.alpha.range.start : _particleProperties.alpha.gradient.target; }

    bool _isAlphaFinishInitialized = false;
    void setAlphaFinish(float alphaFinish);
    float getAlphaFinish() const { return _isAlphaFinishInitialized ? _particleProperties.alpha.range.finish : _particleProperties.alpha.gradient.target; }

    void setAlphaSpread(float alphaSpread);
    float getAlphaSpread() const { return _particleProperties.alpha.gradient.spread; }

    void setShapeType(ShapeType type) override;
    virtual ShapeType getShapeType() const override { return _shapeType; }

    virtual void debugDump() const override;

    bool getIsEmitting() const { return _isEmitting; }
    void setIsEmitting(bool isEmitting) { _isEmitting = isEmitting; }

    void setMaxParticles(quint32 maxParticles);
    quint32 getMaxParticles() const { return _particleProperties.maxParticles; }

    void setLifespan(float lifespan);
    float getLifespan() const { return _particleProperties.lifespan; }

    void setEmitRate(float emitRate);
    float getEmitRate() const { return _particleProperties.emission.rate; }

    void setEmitSpeed(float emitSpeed);
    float getEmitSpeed() const { return _particleProperties.emission.speed.target; }

    void setSpeedSpread(float speedSpread);
    float getSpeedSpread() const { return _particleProperties.emission.speed.spread; }

    void setEmitOrientation(const glm::quat& emitOrientation);
    const glm::quat& getEmitOrientation() const { return _particleProperties.emission.orientation; }

    void setEmitDimensions(const glm::vec3& emitDimensions);
    const glm::vec3& getEmitDimensions() const { return _particleProperties.emission.dimensions; }

    void setEmitRadiusStart(float emitRadiusStart);
    float getEmitRadiusStart() const { return _particleProperties.radiusStart; }

    void setPolarStart(float polarStart);
    float getPolarStart() const { return _particleProperties.polar.start; }

    void setPolarFinish(float polarFinish);
    float getPolarFinish() const { return _particleProperties.polar.finish; }

    void setAzimuthStart(float azimuthStart);
    float getAzimuthStart() const { return _particleProperties.azimuth.start; }

    void setAzimuthFinish(float azimuthFinish);
    float getAzimuthFinish() const { return _particleProperties.azimuth.finish; }

    void setEmitAcceleration(const glm::vec3& emitAcceleration);
    const glm::vec3& getEmitAcceleration() const { return _particleProperties.emission.acceleration.target; }
    
    void setAccelerationSpread(const glm::vec3& accelerationSpread);
    const glm::vec3& getAccelerationSpread() const { return _particleProperties.emission.acceleration.spread; }

    void setParticleRadius(float particleRadius);
    float getParticleRadius() const { return _particleProperties.radius.gradient.target; }

    bool _isRadiusStartInitialized = false;
    void setRadiusStart(float radiusStart);
    float getRadiusStart() const { return _isRadiusStartInitialized ? _particleProperties.radius.range.start : _particleProperties.radius.gradient.target; }

    bool _isRadiusFinishInitialized = false;
    void setRadiusFinish(float radiusFinish);
    float getRadiusFinish() const { return _isRadiusFinishInitialized ? _particleProperties.radius.range.finish : _particleProperties.radius.gradient.target; }

    void setRadiusSpread(float radiusSpread);
    float getRadiusSpread() const { return _particleProperties.radius.gradient.spread; }

    void computeAndUpdateDimensions();

    QString getTextures() const;
    void setTextures(const QString& textures);

    bool getEmitterShouldTrail() const { return _particleProperties.emission.shouldTrail; }
    void setEmitterShouldTrail(bool emitterShouldTrail) { _particleProperties.emission.shouldTrail = emitterShouldTrail; }

    virtual bool supportsDetailedRayIntersection() const override { return false; }

    particle::Properties getParticleProperties() const; 

protected:
    rgbColor _particleColorHack;
    particle::Properties _particleProperties;
    bool _isEmitting { true };

    ShapeType _shapeType { SHAPE_TYPE_NONE };
};

#endif // hifi_ParticleEffectEntityItem_h
