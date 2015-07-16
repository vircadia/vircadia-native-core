//
//  ModelEntityItem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelEntityItem_h
#define hifi_ModelEntityItem_h

#include <AnimationLoop.h>

#include "EntityItem.h" 

class ModelEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ModelEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties() const;
    virtual bool setProperties(const EntityItemProperties& properties);

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
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
    virtual void debugDump() const;

    void updateShapeType(ShapeType type);
    virtual ShapeType getShapeType() const;

    // TODO: Move these to subclasses, or other appropriate abstraction
    // getters/setters applicable to models and particles

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }
    bool hasModel() const { return !_modelURL.isEmpty(); }
    virtual bool hasCompoundShapeURL() const { return !_compoundShapeURL.isEmpty(); }

    static const QString DEFAULT_MODEL_URL;
    const QString& getModelURL() const { return _modelURL; }

    static const QString DEFAULT_COMPOUND_SHAPE_URL;
    const QString& getCompoundShapeURL() const { return _compoundShapeURL; }

    bool hasAnimation() const { return !_animationURL.isEmpty(); }
    static const QString DEFAULT_ANIMATION_URL;
    const QString& getAnimationURL() const { return _animationURL; }

    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
            _color[RED_INDEX] = value.red;
            _color[GREEN_INDEX] = value.green;
            _color[BLUE_INDEX] = value.blue;
    }
    
    // model related properties
    void setModelURL(const QString& url) { _modelURL = url; }
    virtual void setCompoundShapeURL(const QString& url);
    void setAnimationURL(const QString& url);
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
    
    void mapJoints(const QStringList& modelJointNames);
    QVector<glm::quat> getAnimationFrame();
    bool jointsMapped() const { return _jointMappingCompleted; }
    
    bool getAnimationIsPlaying() const { return _animationLoop.isRunning(); }
    float getAnimationFrameIndex() const { return _animationLoop.getFrameIndex(); }
    float getAnimationFPS() const { return _animationLoop.getFPS(); }
    QString getAnimationSettings() const;

    static const QString DEFAULT_TEXTURES;
    const QString& getTextures() const { return _textures; }
    void setTextures(const QString& textures) { _textures = textures; }

    virtual bool shouldBePhysical() const;
    
    static void cleanupLoadedAnimations();
    
protected:

    bool isAnimatingSomething() const;

    rgbColor _color;
    QString _modelURL;
    QString _compoundShapeURL;

    quint64 _lastAnimated;
    QString _animationURL;
    AnimationLoop _animationLoop;
    QString _animationSettings;
    QString _textures;
    ShapeType _shapeType = SHAPE_TYPE_NONE;

    // used on client side
    bool _jointMappingCompleted;
    QVector<int> _jointMapping;

    static AnimationPointer getAnimation(const QString& url);
    static QMap<QString, AnimationPointer> _loadedAnimations;
    static AnimationCache _animationCache;

};

#endif // hifi_ModelEntityItem_h
