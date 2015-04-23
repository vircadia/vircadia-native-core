//
//  ParticleEffectEntityItem.h
//  libraries/entities/src
//
//  Some starter code for a particle simulation entity, which could ideally be used for a variety of effects.
//  This is some really early and rough stuff here.  It was enough for now to just get it up and running in the interface.
//
//  Todo's and other notes:
//  - The simulation should restart when the AnimationLoop's max frame is reached (or passed), but there doesn't seem
//    to be a good way to set that max frame to something reasonable right now.
//  - There seems to be a bug whereby entities on the edge of screen will just pop off or on.  This is probably due
//    to my lack of understanding of how entities in the octree are picked for rendering.  I am updating the entity
//    dimensions based on the bounds of the sim, but maybe I need to update a dirty flag or something.
//  - This should support some kind of pre-roll of the simulation.
//  - Just to get this out the door, I just did forward Euler integration.  There are better ways.
//  - Gravity always points along the Y axis.  Support an actual gravity vector.
//  - Add the ability to add arbitrary forces to the simulation.
//  - Add controls for spread (which is currently hard-coded) and varying emission strength (not currently implemented).
//  - Add drag.
//  - Add some kind of support for collisions.
//  - For simplicity, I'm currently just rendering each particle as a cross of four axis-aligned quads.  Really, we'd
//    want multiple render modes, including (the most important) textured billboards (always facing camera).  Also, these
//    should support animated textures.
//  - There's no synchronization of the simulation across clients at all.  In fact, it's using rand() under the hood, so
//    there's no gaurantee that different clients will see simulations that look anything like the other.
//  - MORE?
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
    
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

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

    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
        _color[RED_INDEX] = value.red;
        _color[GREEN_INDEX] = value.green;
        _color[BLUE_INDEX] = value.blue;
    }

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
    void setMaxParticles(quint32 maxParticles) { _maxParticles = maxParticles; }
    quint32 getMaxParticles() const { return _maxParticles; }

    static const float DEFAULT_LIFESPAN;
    void setLifespan(float lifespan) { _lifespan = lifespan; }
    float getLifespan() const { return _lifespan; }

    static const float DEFAULT_EMIT_RATE;
    void setEmitRate(float emitRate) { _emitRate = emitRate; }
    float getEmitRate() const { return _emitRate; }

    static const glm::vec3 DEFAULT_EMIT_DIRECTION;
    void setEmitDirection(glm::vec3 emitDirection) { _emitDirection = emitDirection; }
    const glm::vec3& getEmitDirection() const { return _emitDirection; }

    static const float DEFAULT_EMIT_STRENGTH;
    void setEmitStrength(float emitStrength) { _emitStrength = emitStrength; }
    float getEmitStrength() const { return _emitStrength; }

    static const float DEFAULT_LOCAL_GRAVITY;
    void setLocalGravity(float localGravity) { _localGravity = localGravity; }
    float getLocalGravity() const { return _localGravity; }

    static const float DEFAULT_PARTICLE_RADIUS;
    void setParticleRadius(float particleRadius) { _particleRadius = particleRadius; }
    float getParticleRadius() const { return _particleRadius; }

    bool getAnimationIsPlaying() const { return _animationLoop.isRunning(); }
    float getAnimationFrameIndex() const { return _animationLoop.getFrameIndex(); }
    float getAnimationFPS() const { return _animationLoop.getFPS(); }
    QString getAnimationSettings() const;

protected:

    bool isAnimatingSomething() const;
    void stepSimulation(float deltaTime);
    void resetSimulation();

    // the properties of this entity
    rgbColor _color;
    quint32 _maxParticles;
    float _lifespan;
    float _emitRate;
    glm::vec3 _emitDirection;
    float _emitStrength;
    float _localGravity;
    float _particleRadius;
    quint64 _lastAnimated;
    AnimationLoop _animationLoop;
    QString _animationSettings;
    ShapeType _shapeType = SHAPE_TYPE_NONE;

    // all the internals of running the particle sim
    const quint32 XYZ_STRIDE = 3;
    float* _paLife;
    float* _paPosition;
    float* _paVelocity;
    float _partialEmit;
    quint32 _paCount;
    quint32 _paHead;
    float _paXmin;
    float _paXmax;
    float _paYmin;
    float _paYmax;
    float _paZmin;
    float _paZmax;
    unsigned int _randSeed;

};

#endif // hifi_ParticleEffectEntityItem_h

