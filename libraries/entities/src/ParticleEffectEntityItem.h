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

#include <AnimationLoop.h>
#include "EntityItem.h"

class ParticleEffectEntityItem : public EntityItem {
public:

    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    virtual ~ParticleEffectEntityItem();

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of this entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const;
    virtual bool setProperties(const EntityItemProperties& properties);

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData);

    virtual void update(const quint64& now);
    virtual bool needsToCallUpdate() const;

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }

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

    bool _isColorFinishInitialized = false;
    void setColorFinish(const xColor& colorFinish) { _colorFinish = colorFinish; _isColorFinishInitialized = true; }
    xColor getColorFinish() const { return _isColorFinishInitialized ? _colorFinish : getXColor(); }

    static const xColor DEFAULT_COLOR_SPREAD;
    void setColorSpread(const xColor& colorSpread) { _colorSpread = colorSpread; }
    xColor getColorSpread() const { return _colorSpread; }

    static const float DEFAULT_ALPHA;
    void setAlpha(float alpha) { _alpha = alpha; }
    float getAlpha() const { return _alpha; }

    static const float DEFAULT_ALPHA_START;
    bool _isAlphaStartInitialized = false;
    void setAlphaStart(float alphaStart) { _alphaStart = alphaStart; _isAlphaStartInitialized = true; }
    float getAlphaStart() const { return _isAlphaStartInitialized ? _alphaStart : _alpha; }

    static const float DEFAULT_ALPHA_FINISH;
    bool _isAlphaFinishInitialized = false;
    void setAlphaFinish(float alphaFinish) { _alphaFinish = alphaFinish; _isAlphaFinishInitialized = true; }
    float getAlphaFinish() const { return _isAlphaFinishInitialized ? _alphaFinish : _alpha; }

    static const float DEFAULT_ALPHA_SPREAD;
    void setAlphaSpread(float alphaSpread) { _alphaSpread = alphaSpread; }
    float getAlphaSpread() const { return _alphaSpread; }

    void updateShapeType(ShapeType type);
    virtual ShapeType getShapeType() const { return _shapeType; }

    virtual void debugDump() const;

    static const float DEFAULT_ANIMATION_FRAME_INDEX;
    void setAnimationFrameIndex(float value);
    void setAnimationSettings(const QString& value);

    static const bool DEFAULT_ANIMATION_IS_PLAYING;
    void setAnimationIsPlaying(bool value);

    static const float DEFAULT_ANIMATION_FPS;
    void setAnimationFPS(float value);

    void setAnimationLoop(bool loop) { _animationLoop.setLoop(loop); }
    bool getAnimationLoop() const { return _animationLoop.getLoop(); }

    void setAnimationHold(bool hold) { _animationLoop.setHold(hold); }
    bool getAnimationHold() const { return _animationLoop.getHold(); }

    void setAnimationStartAutomatically(bool startAutomatically) { _animationLoop.setStartAutomatically(startAutomatically); }
    bool getAnimationStartAutomatically() const { return _animationLoop.getStartAutomatically(); }

    void setAnimationFirstFrame(float firstFrame) { _animationLoop.setFirstFrame(firstFrame); }
    float getAnimationFirstFrame() const { return _animationLoop.getFirstFrame(); }

    void setAnimationLastFrame(float lastFrame) { _animationLoop.setLastFrame(lastFrame); }
    float getAnimationLastFrame() const { return _animationLoop.getLastFrame(); }

    static const quint32 DEFAULT_MAX_PARTICLES;
    void setMaxParticles(quint32 maxParticles);
    quint32 getMaxParticles() const { return _maxParticles; }

    static const float DEFAULT_LIFESPAN;
    void setLifespan(float lifespan) { _lifespan = lifespan; }
    float getLifespan() const { return _lifespan; }

    static const float DEFAULT_EMIT_RATE;
    void setEmitRate(float emitRate) { _emitRate = emitRate; }
    float getEmitRate() const { return _emitRate; }

    static const float DEFAULT_EMIT_SPEED;
    void setEmitSpeed(float emitSpeed);
    float getEmitSpeed() const { return _emitSpeed; }

    static const float DEFAULT_SPEED_SPREAD;
    void setSpeedSpread(float speedSpread);
    float getSpeedSpread() const { return _speedSpread; }

    static const glm::quat DEFAULT_EMIT_ORIENTATION;
    void setEmitOrientation(const glm::quat& emitOrientation);
    const glm::quat& getEmitOrientation() const { return _emitOrientation; }

    static const glm::vec3 DEFAULT_EMIT_DIMENSIONS;
    void setEmitDimensions(const glm::vec3& emitDimensions);
    const glm::vec3& getEmitDimensions() const { return _emitDimensions; }

    static const float DEFAULT_EMIT_RADIUS_START;
    void setEmitRadiusStart(float emitRadiusStart) { _emitRadiusStart = emitRadiusStart; }
    float getEmitRadiusStart() const { return _emitRadiusStart; }

    static const float DEFAULT_POLAR_START;
    void setPolarStart(float polarStart) { _polarStart = polarStart; }
    float getPolarStart() const { return _polarStart; }

    static const float DEFAULT_POLAR_FINISH;
    void setPolarFinish(float polarFinish) { _polarFinish = polarFinish; }
    float getPolarFinish() const { return _polarFinish; }

    static const float DEFAULT_AZIMUTH_START;
    void setAzimuthStart(float azimuthStart) { _azimuthStart = azimuthStart; }
    float getAzimuthStart() const { return _azimuthStart; }

    static const float DEFAULT_AZIMUTH_FINISH;
    void setAzimuthFinish(float azimuthFinish) { _azimuthFinish = azimuthFinish; }
    float getAzimuthFinish() const { return _azimuthFinish; }

    static const glm::vec3 DEFAULT_EMIT_ACCELERATION;
    void setEmitAcceleration(const glm::vec3& emitAcceleration);
    const glm::vec3& getEmitAcceleration() const { return _emitAcceleration; }
    
    static const glm::vec3 DEFAULT_ACCELERATION_SPREAD;
    void setAccelerationSpread(const glm::vec3& accelerationSpread);
    const glm::vec3& getAccelerationSpread() const { return _accelerationSpread; }

    static const float DEFAULT_PARTICLE_RADIUS;
    void setParticleRadius(float particleRadius) { _particleRadius = particleRadius; }
    float getParticleRadius() const { return _particleRadius; }

    static const float DEFAULT_RADIUS_START;
    bool _isRadiusStartInitialized = false;
    void setRadiusStart(float radiusStart) { _radiusStart = radiusStart; _isRadiusStartInitialized = true; }
    float getRadiusStart() const { return _isRadiusStartInitialized ? _radiusStart : _particleRadius; }

    static const float DEFAULT_RADIUS_FINISH;
    bool _isRadiusFinishInitialized = false;
    void setRadiusFinish(float radiusFinish) { _radiusFinish = radiusFinish; _isRadiusFinishInitialized = true; }
    float getRadiusFinish() const { return _isRadiusFinishInitialized ? _radiusFinish : _particleRadius; }

    static const float DEFAULT_RADIUS_SPREAD;
    void setRadiusSpread(float radiusSpread) { _radiusSpread = radiusSpread; }
    float getRadiusSpread() const { return _radiusSpread; }

    void computeAndUpdateDimensions();


    bool getAnimationIsPlaying() const { return _animationLoop.isRunning(); }
    float getAnimationFrameIndex() const { return _animationLoop.getFrameIndex(); }
    float getAnimationFPS() const { return _animationLoop.getFPS(); }
    QString getAnimationSettings() const;

    static const QString DEFAULT_TEXTURES;
    const QString& getTextures() const { return _textures; }
    void setTextures(const QString& textures) {
        if (_textures != textures) {
            _textures = textures;
            _texturesChangedFlag = true;
        }
    }

protected:

    bool isAnimatingSomething() const;
    void stepSimulation(float deltaTime);
    void updateRadius(quint32 index, float age);
    void updateColor(quint32 index, float age);
    void updateAlpha(quint32 index, float age);
    void extendBounds(const glm::vec3& point);
    void integrateParticle(quint32 index, float deltaTime);
    quint32 getLivingParticleCount() const;

    // the properties of this entity
    rgbColor _color;
    xColor _colorStart = DEFAULT_COLOR;
    xColor _colorFinish = DEFAULT_COLOR;
    xColor _colorSpread = DEFAULT_COLOR_SPREAD;
    float _alpha = DEFAULT_ALPHA;
    float _alphaStart = DEFAULT_ALPHA_START;
    float _alphaFinish = DEFAULT_ALPHA_FINISH;
    float _alphaSpread = DEFAULT_ALPHA_SPREAD;
    quint32 _maxParticles = DEFAULT_MAX_PARTICLES;
    float _lifespan = DEFAULT_LIFESPAN;
    float _emitRate = DEFAULT_EMIT_RATE;
    float _emitSpeed = DEFAULT_EMIT_SPEED;
    float _speedSpread = DEFAULT_SPEED_SPREAD;
    glm::quat _emitOrientation = DEFAULT_EMIT_ORIENTATION;
    glm::vec3 _emitDimensions = DEFAULT_EMIT_DIMENSIONS;
    float _emitRadiusStart = DEFAULT_EMIT_RADIUS_START;
    float _polarStart = DEFAULT_POLAR_START;
    float _polarFinish = DEFAULT_POLAR_FINISH;
    float _azimuthStart = DEFAULT_AZIMUTH_START;
    float _azimuthFinish = DEFAULT_AZIMUTH_FINISH;
    glm::vec3 _emitAcceleration = DEFAULT_EMIT_ACCELERATION;
    glm::vec3 _accelerationSpread = DEFAULT_ACCELERATION_SPREAD;
    float _particleRadius = DEFAULT_PARTICLE_RADIUS;
    float _radiusStart = DEFAULT_RADIUS_START;
    float _radiusFinish = DEFAULT_RADIUS_FINISH;
    float _radiusSpread = DEFAULT_RADIUS_SPREAD;
    quint64 _lastAnimated;
    AnimationLoop _animationLoop;
    QString _animationSettings;
    QString _textures = DEFAULT_TEXTURES;
    bool _texturesChangedFlag = false;
    ShapeType _shapeType = SHAPE_TYPE_NONE;

    // all the internals of running the particle sim
    QVector<float> _particleLifetimes;
    QVector<glm::vec3> _particlePositions;
    QVector<glm::vec3> _particleVelocities;
    QVector<glm::vec3> _particleAccelerations;
    QVector<float> _particleRadiuses;
    QVector<float> _radiusStarts;
    QVector<float> _radiusMiddles;
    QVector<float> _radiusFinishes;
    QVector<xColor> _particleColors;
    QVector<xColor> _colorStarts;
    QVector<xColor> _colorMiddles;
    QVector<xColor> _colorFinishes;
    QVector<float> _particleAlphas;
    QVector<float> _alphaStarts;
    QVector<float> _alphaMiddles;
    QVector<float> _alphaFinishes;

    float _timeUntilNextEmit = 0.0f;

    // particle arrays are a ring buffer, use these indices
    // to keep track of the living particles.
    quint32 _particleHeadIndex = 0;
    quint32 _particleTailIndex = 0;

    // bounding volume
    glm::vec3 _particleMaxBound;
    glm::vec3 _particleMinBound;
};

#endif // hifi_ParticleEffectEntityItem_h
