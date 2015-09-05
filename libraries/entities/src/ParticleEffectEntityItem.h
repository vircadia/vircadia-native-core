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
    virtual EntityItemProperties getProperties() const;
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

    void setAlpha(float alpha) { _alpha = alpha; }
    float getAlpha() const { return _alpha; }

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

    static const glm::vec3 DEFAULT_EMIT_VELOCITY;
    void setEmitVelocity(const glm::vec3& emitVelocity);
    const glm::vec3& getEmitVelocity() const { return _emitVelocity; }
    
    static const glm::vec3 DEFAULT_VELOCITY_SPREAD;
    void setVelocitySpread(const glm::vec3& velocitySpread);
    const glm::vec3& getVelocitySpread() const { return _velocitySpread; }

    static const glm::vec3 DEFAULT_EMIT_ACCELERATION;
    void setEmitAcceleration(const glm::vec3& emitAcceleration);
    const glm::vec3& getEmitAcceleration() const { return _emitAcceleration; }
    
    static const glm::vec3 DEFAULT_ACCELERATION_SPREAD;
    void setAccelerationSpread(const glm::vec3& accelerationSpread);
    const glm::vec3& getAccelerationSpread() const { return _accelerationSpread; }

    static const float DEFAULT_PARTICLE_RADIUS;
    void setParticleRadius(float particleRadius) { _particleRadius = particleRadius; }
    float getParticleRadius() const { return _particleRadius; }

    static const float RADIUS_UNINITIALIZED;

    static const float DEFAULT_RADIUS_START;
    void setRadiusStart(float radiusStart) { _radiusStart = radiusStart; }
    float getRadiusStart() const { return _radiusStart == RADIUS_UNINITIALIZED ? _particleRadius : _radiusStart; }

    static const float DEFAULT_RADIUS_FINISH;
    void setRadiusFinish(float radiusFinish) { _radiusFinish = radiusFinish; }
    float getRadiusFinish() const { return _radiusFinish == RADIUS_UNINITIALIZED ? _particleRadius : _radiusFinish; }

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
    void updateRadius(quint32 index);
    void extendBounds(const glm::vec3& point);
    void integrateParticle(quint32 index, float deltaTime);
    quint32 getLivingParticleCount() const;

    // the properties of this entity
    rgbColor _color;
    float _alpha;
    float _alphaSpread;
    quint32 _maxParticles;
    float _lifespan;
    float _emitRate;
    glm::vec3 _emitVelocity;
    glm::vec3 _velocitySpread;
    glm::vec3 _emitAcceleration;
    glm::vec3 _accelerationSpread;
    float _particleRadius;
    float _radiusStart;
    float _radiusFinish;
    float _radiusSpread;
    quint64 _lastAnimated;
    AnimationLoop _animationLoop;
    QString _animationSettings;
    QString _textures;
    bool _texturesChangedFlag;
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
    QVector<float> _particleAlphas;

    float _timeUntilNextEmit;

    // particle arrays are a ring buffer, use these indices
    // to keep track of the living particles.
    quint32 _particleHeadIndex;
    quint32 _particleTailIndex;

    // bounding volume
    glm::vec3 _particleMaxBound;
    glm::vec3 _particleMinBound;

private:
    float cubicInterpolate(float y0, float y1, float y2, float y3, float u);
};

#endif // hifi_ParticleEffectEntityItem_h
