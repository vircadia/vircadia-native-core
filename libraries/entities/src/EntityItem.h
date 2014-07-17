//
//  EntityItem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityItem_h
#define hifi_EntityItem_h

#define HIDE_SUBCLASS_METHODS 1

#include <stdint.h>

#include <glm/glm.hpp>

#include <AnimationCache.h> // for Animation, AnimationCache, and AnimationPointer classes
#include <Octree.h> // for EncodeBitstreamParams class
#include <OctreeElement.h> // for OctreeElement::AppendState
#include <OctreePacketData.h>

#include "EntityItemID.h" 
#include "EntityItemProperties.h" 
#include "EntityTypes.h" 

class EntityTreeElementExtraEncodeData;

/// EntityItem class - this is the actual model item class.
class EntityItem  {

public:
    EntityItem();
    EntityItem(const EntityItemID& entityItemID);
    EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    
    virtual ~EntityItem();
    
    virtual void somePureVirtualFunction() = 0;

    // ID and EntityItemID related methods
    uint32_t getID() const { return _id; }
    void setID(uint32_t id) { _id = id; }
    uint32_t getCreatorTokenID() const { return _creatorTokenID; }
    void setCreatorTokenID(uint32_t creatorTokenID) { _creatorTokenID = creatorTokenID; }
    bool isNewlyCreated() const { return _newlyCreated; }
    bool isKnownID() const { return getID() != UNKNOWN_ENTITY_ID; }
    EntityItemID getEntityItemID() const { return EntityItemID(getID(), getCreatorTokenID(), getID() != UNKNOWN_ENTITY_ID); }

/*
    // these methods allow you to create models, and later edit them.
    static uint32_t getIDfromCreatorTokenID(uint32_t creatorTokenID);
    static uint32_t getNextCreatorTokenID();
    static void handleAddEntityResponse(const QByteArray& packet);
*/
    // methods for getting/setting all properties of an entity
    EntityItemProperties getProperties() const;
    void setProperties(const EntityItemProperties& properties, bool forceCopy = false);

    quint64 getLastUpdated() const { return _lastUpdated; } /// Last simulated time of this entity universal usecs
    quint64 getLastEdited() const { return _lastEdited; } /// Last edited time of this entity universal usecs
    void setLastEdited(quint64 lastEdited) { _lastEdited = lastEdited; }
    float getEditedAgo() const /// Elapsed seconds since this entity was last edited
        { return (float)(usecTimestampNow() - _lastEdited) / (float)USECS_PER_SECOND; }

    OctreeElement::AppendState appendEntityData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                                EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData) const;

    static EntityItemID readEntityItemIDFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                    ReadBitstreamToTreeParams& args);
    int readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);

    /// For reading models from pre V3 bitstreams
    int oldVersionReadEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    static int expectedBytes();

    static bool encodeEntityEditMessageDetails(PacketType command, EntityItemID id, const EntityItemProperties& details,
                        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static void adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew);
    void update(const quint64& now);
    void debugDump() const;

    // similar to assignment/copy, but it handles keeping lifetime accurate
    void copyChangedProperties(const EntityItem& other);


    // attributes applicable to all entity types
    EntityTypes::EntityType_t getType() const { return _type; }
    const glm::vec3& getPosition() const { return _position; } /// get position in domain scale units (0.0 - 1.0)
    void setPosition(const glm::vec3& value) { _position = value; } /// set position in domain scale units (0.0 - 1.0)

    float getRadius() const { return _radius; } /// get radius in domain scale units (0.0 - 1.0)
    void setRadius(float value) { _radius = value; } /// set radius in domain scale units (0.0 - 1.0)

    const glm::quat& getRotation() const { return _rotation; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; }

    bool getShouldBeDeleted() const { return _shouldBeDeleted; }
    void setShouldBeDeleted(bool shouldBeDeleted) { _shouldBeDeleted = shouldBeDeleted; }
    
    // position, size, and bounds related helpers
    float getSize() const { return _radius * 2.0f; } /// get maximum dimension in domain scale units (0.0 - 1.0)
    glm::vec3 getMinimumPoint() const { return _position - glm::vec3(_radius, _radius, _radius); }
    glm::vec3 getMaximumPoint() const { return _position + glm::vec3(_radius, _radius, _radius); }
    AACube getAACube() const { return AACube(getMinimumPoint(), getSize()); } /// AACube in domain scale units (0.0 - 1.0)

    // TODO: Move these to subclasses, or other appropriate abstraction
    // getters/setters applicable to models and particles
#ifdef HIDE_SUBCLASS_METHODS    
    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }
    bool hasModel() const { return !_modelURL.isEmpty(); }
    const QString& getModelURL() const { return _modelURL; }
    bool hasAnimation() const { return !_animationURL.isEmpty(); }
    const QString& getAnimationURL() const { return _animationURL; }
    float getGlowLevel() const { return _glowLevel; }
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
    void setGlowLevel(float glowLevel) { _glowLevel = glowLevel; }
    void setSittingPoints(QVector<SittingPoint> sittingPoints) { _sittingPoints = sittingPoints; }
    
    void mapJoints(const QStringList& modelJointNames);
    QVector<glm::quat> getAnimationFrame();
    bool jointsMapped() const { return _jointMappingCompleted; }
    
    bool getAnimationIsPlaying() const { return _animationIsPlaying; }
    float getAnimationFrameIndex() const { return _animationFrameIndex; }
    float getAnimationFPS() const { return _animationFPS; }
#endif
    
    static void cleanupLoadedAnimations();

protected:
    void initFromEntityItemID(const EntityItemID& entityItemID);
    virtual void init(glm::vec3 position, float radius, rgbColor color, uint32_t id = NEW_ENTITY);

    /*
    static quint32 _nextID;
    static uint32_t _nextCreatorTokenID; /// used by the static interfaces for creator token ids
    static std::map<uint32_t,uint32_t> _tokenIDsToIDs;
    */

    quint32 _id;
    EntityTypes::EntityType_t _type;
    uint32_t _creatorTokenID;
    bool _newlyCreated;
    quint64 _lastUpdated;
    quint64 _lastEdited;

    glm::vec3 _position;
    float _radius;
    glm::quat _rotation;
    bool _shouldBeDeleted;
    
#ifdef HIDE_SUBCLASS_METHODS
    rgbColor _color;
    QString _modelURL;
    QVector<SittingPoint> _sittingPoints;
    float _glowLevel;

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
#endif

};

// our non-pure virtual subclass for now...
class ModelEntityItem : public EntityItem {
public:
    ModelEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Model; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

class ParticleEntityItem : public EntityItem {
public:
    ParticleEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Particle; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

class BoxEntityItem : public EntityItem {
public:
    BoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Box; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

class SphereEntityItem : public EntityItem {
public:
    SphereEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Sphere; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

class PlaneEntityItem : public EntityItem {
public:
    PlaneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Plane; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

class CylinderEntityItem : public EntityItem {
public:
    CylinderEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Cylinder; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

class PyramidEntityItem : public EntityItem {
public:
    PyramidEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Pyramid; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

#endif // hifi_EntityItem_h
