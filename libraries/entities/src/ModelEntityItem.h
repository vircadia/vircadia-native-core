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

#include "EntityItem.h" 

class ModelEntityItem : public EntityItem {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ModelEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties() const;
    virtual void setProperties(const EntityItemProperties& properties, bool forceCopy = false);

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const;


    virtual int readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData);

    virtual void update(const quint64& now);
    virtual SimuationState getSimulationState() const;


    // TODO: Move these to subclasses, or other appropriate abstraction
    // getters/setters applicable to models and particles

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }
    bool hasModel() const { return !_modelURL.isEmpty(); }
    const QString& getModelURL() const { return _modelURL; }
    bool hasAnimation() const { return !_animationURL.isEmpty(); }
    const QString& getAnimationURL() const { return _animationURL; }
    QVector<SittingPoint> getSittingPoints() const { return _sittingPoints; }

    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
            _color[RED_INDEX] = value.red;
            _color[GREEN_INDEX] = value.green;
            _color[BLUE_INDEX] = value.blue;
    }
    
    // model related properties
    void setModelURL(const QString& url) { _modelURL = url; }
    void setAnimationURL(const QString& url) { _animationURL = url; }
    void setAnimationFrameIndex(float value) { _animationFrameIndex = value; }
    void setAnimationIsPlaying(bool value) { _animationIsPlaying = value; }
    void setAnimationFPS(float value) { _animationFPS = value; }
    void setSittingPoints(QVector<SittingPoint> sittingPoints) { _sittingPoints = sittingPoints; }
    
    void mapJoints(const QStringList& modelJointNames);
    QVector<glm::quat> getAnimationFrame();
    bool jointsMapped() const { return _jointMappingCompleted; }
    
    bool getAnimationIsPlaying() const { return _animationIsPlaying; }
    float getAnimationFrameIndex() const { return _animationFrameIndex; }
    float getAnimationFPS() const { return _animationFPS; }
    
    static void cleanupLoadedAnimations();
    
protected:

    /// For reading models from pre V3 bitstreams
    int oldVersionReadEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    bool isAnimatingSomething() const;

    rgbColor _color;
    QString _modelURL;
    QVector<SittingPoint> _sittingPoints;

    quint64 _lastAnimated;
    QString _animationURL;
    float _animationFrameIndex; // we keep this as a float and round to int only when we need the exact index
    bool _animationIsPlaying;
    float _animationFPS;
    bool _jointMappingCompleted;
    QVector<int> _jointMapping;
    
    static Animation* getAnimation(const QString& url);
    static QMap<QString, AnimationPointer> _loadedAnimations;
    static AnimationCache _animationCache;

};

#endif // hifi_ModelEntityItem_h
