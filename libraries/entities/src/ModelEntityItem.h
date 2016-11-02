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
#include "AnimationPropertyGroup.h"

class ModelEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ModelEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
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
    virtual void debugDump() const override;

    void setShapeType(ShapeType type) override;
    virtual ShapeType getShapeType() const override;


    // TODO: Move these to subclasses, or other appropriate abstraction
    // getters/setters applicable to models and particles

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }
    bool hasModel() const { return !_modelURL.isEmpty(); }
    virtual bool hasCompoundShapeURL() const { return !_compoundShapeURL.isEmpty(); }

    static const QString DEFAULT_MODEL_URL;
    const QString& getModelURL() const { return _modelURL; }
    const QUrl& getParsedModelURL() const { return _parsedModelURL; }

    static const QString DEFAULT_COMPOUND_SHAPE_URL;
    const QString& getCompoundShapeURL() const { return _compoundShapeURL; }

    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
            _color[RED_INDEX] = value.red;
            _color[GREEN_INDEX] = value.green;
            _color[BLUE_INDEX] = value.blue;
    }

    // model related properties
    virtual void setModelURL(const QString& url);
    virtual void setCompoundShapeURL(const QString& url);

    // Animation related items...
    const AnimationPropertyGroup& getAnimationProperties() const { return _animationProperties; }

    bool hasAnimation() const { return !_animationProperties.getURL().isEmpty(); }
    const QString& getAnimationURL() const { return _animationProperties.getURL(); }
    void setAnimationURL(const QString& url);

    void setAnimationCurrentFrame(float value) { _animationLoop.setCurrentFrame(value); }
    void setAnimationIsPlaying(bool value);
    void setAnimationFPS(float value);

    void setAnimationLoop(bool loop) { _animationLoop.setLoop(loop); }
    bool getAnimationLoop() const { return _animationLoop.getLoop(); }

    void setAnimationHold(bool hold) { _animationLoop.setHold(hold); }
    bool getAnimationHold() const { return _animationLoop.getHold(); }

    void setAnimationFirstFrame(float firstFrame) { _animationLoop.setFirstFrame(firstFrame); }
    float getAnimationFirstFrame() const { return _animationLoop.getFirstFrame(); }

    void setAnimationLastFrame(float lastFrame) { _animationLoop.setLastFrame(lastFrame); }
    float getAnimationLastFrame() const { return _animationLoop.getLastFrame(); }

    void mapJoints(const QStringList& modelJointNames);
    bool jointsMapped() const { return _jointMappingURL == getAnimationURL() && _jointMappingCompleted; }

    AnimationPointer getAnimation() const { return _animation; }
    bool getAnimationIsPlaying() const { return _animationLoop.getRunning(); }
    float getAnimationCurrentFrame() const { return _animationLoop.getCurrentFrame(); }
    float getAnimationFPS() const { return _animationLoop.getFPS(); }

    static const QString DEFAULT_TEXTURES;
    const QString getTextures() const;
    void setTextures(const QString& textures);

    virtual bool shouldBePhysical() const override;

    virtual glm::vec3 getJointPosition(int jointIndex) const { return glm::vec3(); }
    virtual glm::quat getJointRotation(int jointIndex) const { return glm::quat(); }

    virtual void setJointRotations(const QVector<glm::quat>& rotations);
    virtual void setJointRotationsSet(const QVector<bool>& rotationsSet);
    virtual void setJointTranslations(const QVector<glm::vec3>& translations);
    virtual void setJointTranslationsSet(const QVector<bool>& translationsSet);
    QVector<glm::quat> getJointRotations() const;
    QVector<bool> getJointRotationsSet() const;
    QVector<glm::vec3> getJointTranslations() const;
    QVector<bool> getJointTranslationsSet() const;

private:
    void setAnimationSettings(const QString& value); // only called for old bitstream format
    ShapeType computeTrueShapeType() const;

protected:
    // these are used:
    // - to bounce joint data from an animation into the model/rig.
    // - to relay changes from scripts to model/rig.
    // - to relay between network and model/rig
    // they aren't currently updated from data in the model/rig, and they don't have a direct effect
    // on what's rendered.
    ReadWriteLockable _jointDataLock;

    bool _jointRotationsExplicitlySet { false }; // were the joints set as a property or just side effect of animations
    QVector<glm::quat> _localJointRotations;
    QVector<bool> _localJointRotationsSet; // ever set?
    QVector<bool> _localJointRotationsDirty; // needs a relay to model/rig?

    bool _jointTranslationsExplicitlySet { false }; // were the joints set as a property or just side effect of animations
    QVector<glm::vec3> _localJointTranslations;
    QVector<bool> _localJointTranslationsSet; // ever set?
    QVector<bool> _localJointTranslationsDirty; // needs a relay to model/rig?
    int _lastKnownCurrentFrame;
    virtual void resizeJointArrays(int newSize = -1);

    bool isAnimatingSomething() const;

    rgbColor _color;
    QString _modelURL;
    QUrl _parsedModelURL;
    QString _compoundShapeURL;

    AnimationPointer _animation;
    AnimationPropertyGroup _animationProperties;
    AnimationLoop _animationLoop;

    mutable QReadWriteLock _texturesLock;
    QString _textures;

    ShapeType _shapeType = SHAPE_TYPE_NONE;

    // used on client side
    bool _jointMappingCompleted;
    QVector<int> _jointMapping; // domain is index into model-joints, range is index into animation-joints
    QString _jointMappingURL;
};

#endif // hifi_ModelEntityItem_h
