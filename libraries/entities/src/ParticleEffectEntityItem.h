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
                                    EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                 bool& somethingChanged) override;

    virtual void update(const quint64& now) override;
    virtual bool needsToCallUpdate() const override;

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }
    glm::vec3 getColorRGB() const { return  ColorUtils::sRGBToLinearVec3(toGlm(getXColor())); }

    static const xColor DEFAULT_COLOR;
    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
        _color[RED_INDEX] = value.red;
        _color[GREEN_INDEX] = value.green;
        _color[BLUE_INDEX] = value.blue;
    }

    bool _isColorStartInitialized = false;
    void setColorStart(const xColor& colorStart) { _colorStart = colorStart; _isColorStartInitialized = true; }
    xColor getColorStart() const { return _isColorStartInitialized ? _colorStart : getXColor(); }
    glm::vec3 getColorStartRGB() const { return _isColorStartInitialized ? ColorUtils::sRGBToLinearVec3(toGlm(_colorStart)) : getColorRGB(); }

    bool _isColorFinishInitialized = false;
    void setColorFinish(const xColor& colorFinish) { _colorFinish = colorFinish; _isColorFinishInitialized = true; }
    xColor getColorFinish() const { return _isColorFinishInitialized ? _colorFinish : getXColor(); }
    glm::vec3 getColorFinishRGB() const { return _isColorStartInitialized ? ColorUtils::sRGBToLinearVec3(toGlm(_colorFinish)) : getColorRGB(); }

    static const xColor DEFAULT_COLOR_SPREAD;
    void setColorSpread(const xColor& colorSpread) { _colorSpread = colorSpread; }
    xColor getColorSpread() const { return _colorSpread; }
    glm::vec3 getColorSpreadRGB() const { return ColorUtils::sRGBToLinearVec3(toGlm(_colorSpread)); }

    static const float MAXIMUM_ALPHA;
    static const float MINIMUM_ALPHA;

    static const float DEFAULT_ALPHA;
    void setAlpha(float alpha);
    float getAlpha() const { return _alpha; }

    static const float DEFAULT_ALPHA_START;
    bool _isAlphaStartInitialized = false;
    void setAlphaStart(float alphaStart);
    float getAlphaStart() const { return _isAlphaStartInitialized ? _alphaStart : _alpha; }

    static const float DEFAULT_ALPHA_FINISH;
    bool _isAlphaFinishInitialized = false;
    void setAlphaFinish(float alphaFinish);
    float getAlphaFinish() const { return _isAlphaFinishInitialized ? _alphaFinish : _alpha; }

    static const float DEFAULT_ALPHA_SPREAD;
    void setAlphaSpread(float alphaSpread);
    float getAlphaSpread() const { return _alphaSpread; }

    void setShapeType(ShapeType type) override;
    virtual ShapeType getShapeType() const override { return _shapeType; }

    virtual void debugDump() const override;

    bool isEmittingParticles() const; /// emitting enabled, and there are particles alive
    bool getIsEmitting() const { return _isEmitting; }
    void setIsEmitting(bool isEmitting) { _isEmitting = isEmitting; }

    static const quint32 DEFAULT_MAX_PARTICLES;
    static const quint32 MINIMUM_MAX_PARTICLES;
    static const quint32 MAXIMUM_MAX_PARTICLES;
    void setMaxParticles(quint32 maxParticles);
    quint32 getMaxParticles() const { return _maxParticles; }

    static const float DEFAULT_LIFESPAN;
    static const float MINIMUM_LIFESPAN;
    static const float MAXIMUM_LIFESPAN;
    void setLifespan(float lifespan);
    float getLifespan() const { return _lifespan; }

    static const float DEFAULT_EMIT_RATE;
    static const float MINIMUM_EMIT_RATE;
    static const float MAXIMUM_EMIT_RATE;
    void setEmitRate(float emitRate);
    float getEmitRate() const { return _emitRate; }

    static const float DEFAULT_EMIT_SPEED;
    static const float MINIMUM_EMIT_SPEED;
    static const float MAXIMUM_EMIT_SPEED;
    void setEmitSpeed(float emitSpeed);
    float getEmitSpeed() const { return _emitSpeed; }

    static const float DEFAULT_SPEED_SPREAD;
    void setSpeedSpread(float speedSpread);
    float getSpeedSpread() const { return _speedSpread; }

    static const glm::quat DEFAULT_EMIT_ORIENTATION;
    void setEmitOrientation(const glm::quat& emitOrientation);
    const glm::quat& getEmitOrientation() const { return _emitOrientation; }

    static const glm::vec3 DEFAULT_EMIT_DIMENSIONS;
    static const float MINIMUM_EMIT_DIMENSION;
    static const float MAXIMUM_EMIT_DIMENSION;
    void setEmitDimensions(const glm::vec3& emitDimensions);
    const glm::vec3& getEmitDimensions() const { return _emitDimensions; }

    static const float DEFAULT_EMIT_RADIUS_START;
    static const float MINIMUM_EMIT_RADIUS_START;
    static const float MAXIMUM_EMIT_RADIUS_START;
    void setEmitRadiusStart(float emitRadiusStart);
    float getEmitRadiusStart() const { return _emitRadiusStart; }

    static const float MINIMUM_POLAR;
    static const float MAXIMUM_POLAR;

    static const float DEFAULT_POLAR_START;
    void setPolarStart(float polarStart);
    float getPolarStart() const { return _polarStart; }

    static const float DEFAULT_POLAR_FINISH;
    void setPolarFinish(float polarFinish);
    float getPolarFinish() const { return _polarFinish; }

    static const float MINIMUM_AZIMUTH;
    static const float MAXIMUM_AZIMUTH;

    static const float DEFAULT_AZIMUTH_START;
    void setAzimuthStart(float azimuthStart);
    float getAzimuthStart() const { return _azimuthStart; }

    static const float DEFAULT_AZIMUTH_FINISH;
    void setAzimuthFinish(float azimuthFinish);
    float getAzimuthFinish() const { return _azimuthFinish; }

    static const glm::vec3 DEFAULT_EMIT_ACCELERATION;
    static const float MINIMUM_EMIT_ACCELERATION;
    static const float MAXIMUM_EMIT_ACCELERATION;
    void setEmitAcceleration(const glm::vec3& emitAcceleration);
    const glm::vec3& getEmitAcceleration() const { return _emitAcceleration; }
    
    static const glm::vec3 DEFAULT_ACCELERATION_SPREAD;
    static const float MINIMUM_ACCELERATION_SPREAD;
    static const float MAXIMUM_ACCELERATION_SPREAD;
    void setAccelerationSpread(const glm::vec3& accelerationSpread);
    const glm::vec3& getAccelerationSpread() const { return _accelerationSpread; }

    static const float DEFAULT_PARTICLE_RADIUS;
    static const float MINIMUM_PARTICLE_RADIUS;
    static const float MAXIMUM_PARTICLE_RADIUS;
    void setParticleRadius(float particleRadius);
    float getParticleRadius() const { return _particleRadius; }

    static const float DEFAULT_RADIUS_START;
    bool _isRadiusStartInitialized = false;
    void setRadiusStart(float radiusStart);
    float getRadiusStart() const { return _isRadiusStartInitialized ? _radiusStart : _particleRadius; }

    static const float DEFAULT_RADIUS_FINISH;
    bool _isRadiusFinishInitialized = false;
    void setRadiusFinish(float radiusFinish);
    float getRadiusFinish() const { return _isRadiusFinishInitialized ? _radiusFinish : _particleRadius; }

    static const float DEFAULT_RADIUS_SPREAD;
    void setRadiusSpread(float radiusSpread);
    float getRadiusSpread() const { return _radiusSpread; }

    void computeAndUpdateDimensions();

    static const QString DEFAULT_TEXTURES;
    const QString& getTextures() const { return _textures; }
    void setTextures(const QString& textures) {
        if (_textures != textures) {
            _textures = textures;
            _texturesChangedFlag = true;
        }
    }

    static const bool DEFAULT_EMITTER_SHOULD_TRAIL;
    bool getEmitterShouldTrail() const { return _emitterShouldTrail; }
    void setEmitterShouldTrail(bool emitterShouldTrail) {
        _emitterShouldTrail = emitterShouldTrail;
    }

    virtual bool supportsDetailedRayIntersection() const override { return false; }

protected:
    struct Particle;
    using Particles = std::deque<Particle>;

    bool isAnimatingSomething() const;
    
    Particle createParticle(const glm::vec3& position);
    void stepSimulation(float deltaTime);
    void integrateParticle(Particle& particle, float deltaTime);
    
    struct Particle {
        float seed { 0.0f };
        float lifetime { 0.0f };
        glm::vec3 position { Vectors::ZERO };
        glm::vec3 velocity { Vectors::ZERO };
        glm::vec3 acceleration { Vectors::ZERO };
    };
    
    // Particles container
    Particles _particles;
    
    // Particles properties
    rgbColor _color;
    xColor _colorStart = DEFAULT_COLOR;
    xColor _colorFinish = DEFAULT_COLOR;
    xColor _colorSpread = DEFAULT_COLOR_SPREAD;
    float _alpha = DEFAULT_ALPHA;
    float _alphaStart = DEFAULT_ALPHA_START;
    float _alphaFinish = DEFAULT_ALPHA_FINISH;
    float _alphaSpread = DEFAULT_ALPHA_SPREAD;
    float _particleRadius = DEFAULT_PARTICLE_RADIUS;
    float _radiusStart = DEFAULT_RADIUS_START;
    float _radiusFinish = DEFAULT_RADIUS_FINISH;
    float _radiusSpread = DEFAULT_RADIUS_SPREAD;
    float _lifespan = DEFAULT_LIFESPAN;
    
    // Emiter properties
    quint32 _maxParticles = DEFAULT_MAX_PARTICLES;
    
    float _emitRate = DEFAULT_EMIT_RATE;
    float _emitSpeed = DEFAULT_EMIT_SPEED;
    float _speedSpread = DEFAULT_SPEED_SPREAD;
    
    glm::quat _emitOrientation = DEFAULT_EMIT_ORIENTATION;
    glm::vec3 _emitDimensions = DEFAULT_EMIT_DIMENSIONS;
    float _emitRadiusStart = DEFAULT_EMIT_RADIUS_START;
    glm::vec3 _emitAcceleration = DEFAULT_EMIT_ACCELERATION;
    glm::vec3 _accelerationSpread = DEFAULT_ACCELERATION_SPREAD;
    
    float _polarStart = DEFAULT_POLAR_START;
    float _polarFinish = DEFAULT_POLAR_FINISH;
    float _azimuthStart = DEFAULT_AZIMUTH_START;
    float _azimuthFinish = DEFAULT_AZIMUTH_FINISH;
    
    glm::vec3 _previousPosition;
    quint64 _lastSimulated { 0 };
    bool _isEmitting { true };

    QString _textures { DEFAULT_TEXTURES };
    bool _texturesChangedFlag { false };
    ShapeType _shapeType { SHAPE_TYPE_NONE };
    
    float _timeUntilNextEmit { 0.0f };

    
    bool _emitterShouldTrail { DEFAULT_EMITTER_SHOULD_TRAIL };
};

#endif // hifi_ParticleEffectEntityItem_h
