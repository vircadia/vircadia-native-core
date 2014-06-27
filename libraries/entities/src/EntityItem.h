//
//  EntityItem.h
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityItem_h
#define hifi_EntityItem_h

#include <glm/glm.hpp>
#include <stdint.h>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>

#include <AnimationCache.h>
#include <CollisionInfo.h>
#include <OctreePacketData.h>
#include <PropertyFlags.h>
#include <SharedUtil.h>

class EntityItem;
class EntityEditPacketSender;
class EntityItemProperties;
class EntitysScriptingInterface;
class EntityTree;
class EntityTreeElementExtraEncodeData;
class ScriptEngine;
class VoxelEditPacketSender;
class VoxelsScriptingInterface;
struct VoxelDetail;

const uint32_t NEW_MODEL = 0xFFFFFFFF;
const uint32_t UNKNOWN_MODEL_TOKEN = 0xFFFFFFFF;
const uint32_t UNKNOWN_MODEL_ID = 0xFFFFFFFF;

const uint16_t MODEL_PACKET_CONTAINS_RADIUS = 1;
const uint16_t MODEL_PACKET_CONTAINS_POSITION = 2;
const uint16_t MODEL_PACKET_CONTAINS_COLOR = 4;
const uint16_t MODEL_PACKET_CONTAINS_SHOULDDIE = 8;
const uint16_t MODEL_PACKET_CONTAINS_MODEL_URL = 16;
const uint16_t MODEL_PACKET_CONTAINS_MODEL_ROTATION = 32;
const uint16_t MODEL_PACKET_CONTAINS_ANIMATION_URL = 64;
const uint16_t MODEL_PACKET_CONTAINS_ANIMATION_PLAYING = 128;
const uint16_t MODEL_PACKET_CONTAINS_ANIMATION_FRAME = 256;
const uint16_t MODEL_PACKET_CONTAINS_ANIMATION_FPS = 512;

const float MODEL_DEFAULT_RADIUS = 0.1f / TREE_SCALE;
const float MINIMUM_MODEL_ELEMENT_SIZE = (1.0f / 100000.0f) / TREE_SCALE; // smallest size container
const QString MODEL_DEFAULT_MODEL_URL("");
const glm::quat MODEL_DEFAULT_MODEL_ROTATION;
const QString MODEL_DEFAULT_ANIMATION_URL("");
const float MODEL_DEFAULT_ANIMATION_FPS = 30.0f;

// PropertyFlags support

class EntityTypes {
public:
    typedef enum EntityType {
        Base,
        Model,
        Particle
    } EntityType_t;

    static const QString& getEntityTypeName(EntityType_t entityType);
    static bool registerEntityTypeName(EntityType_t entityType, const QString& name);
private:
    static QHash<EntityType_t, QString> _typeNameHash;
};

// PropertyFlags support
enum EntityPropertyList {
    PROP_PAGED_PROPERTY,
    PROP_CUSTOM_PROPERTIES_INCLUDED,
    PROP_VISIBLE,
    PROP_POSITION,
    PROP_RADIUS,
    PROP_MODEL_URL,
    PROP_ROTATION,
    PROP_COLOR,
    PROP_SCRIPT,
    PROP_ANIMATION_URL,
    PROP_ANIMATION_FPS,
    PROP_ANIMATION_FRAME_INDEX,
    PROP_ANIMATION_PLAYING,
    PROP_SHOULD_BE_DELETED,
    PROP_LAST_ITEM = PROP_SHOULD_BE_DELETED
};

typedef PropertyFlags<EntityPropertyList> EntityPropertyFlags;


/// A collection of properties of a model item used in the scripting API. Translates between the actual properties of a model
/// and a JavaScript style hash/QScriptValue storing a set of properties. Used in scripting to set/get the complete set of
/// model item properties via JavaScript hashes/QScriptValues
/// all units for position, radius, etc are in meter units
class EntityItemProperties {
public:
    EntityItemProperties();

    QScriptValue copyToScriptValue(QScriptEngine* engine) const;
    void copyFromScriptValue(const QScriptValue& object);

    void copyToEntityItem(EntityItem& modelItem, bool forceCopy = false) const;
    void copyFromEntityItem(const EntityItem& modelItem);

    const glm::vec3& getPosition() const { return _position; }
    xColor getColor() const { return _color; }
    float getRadius() const { return _radius; }
    bool getShouldBeDeleted() const { return _shouldBeDeleted; }
    
    const QString& getModelURL() const { return _modelURL; }
    const glm::quat& getRotation() const { return _rotation; }
    const QString& getAnimationURL() const { return _animationURL; }
    float getAnimationFrameIndex() const { return _animationFrameIndex; }
    bool getAnimationIsPlaying() const { return _animationIsPlaying;  }
    float getAnimationFPS() const { return _animationFPS; }
    float getGlowLevel() const { return _glowLevel; }

    quint64 getLastEdited() const { return _lastEdited; }
    uint16_t getChangedBits() const;

    /// set position in meter units
    void setPosition(const glm::vec3& value) { _position = value; _positionChanged = true; }
    void setColor(const xColor& value) { _color = value; _colorChanged = true; }
    void setRadius(float value) { _radius = value; _radiusChanged = true; }
    void setShouldBeDeleted(bool shouldBeDeleted) { _shouldBeDeleted = shouldBeDeleted; _shouldBeDeletedChanged = true;  }

    // model related properties
    void setModelURL(const QString& url) { _modelURL = url; _modelURLChanged = true; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; _rotationChanged = true; }
    void setAnimationURL(const QString& url) { _animationURL = url; _animationURLChanged = true; }
    void setAnimationFrameIndex(float value) { _animationFrameIndex = value; _animationFrameIndexChanged = true; }
    void setAnimationIsPlaying(bool value) { _animationIsPlaying = value; _animationIsPlayingChanged = true;  }
    void setAnimationFPS(float value) { _animationFPS = value; _animationFPSChanged = true; }
    void setGlowLevel(float value) { _glowLevel = value; _glowLevelChanged = true; }
    
    /// used by EntityScriptingInterface to return EntityItemProperties for unknown models
    void setIsUnknownID() { _id = UNKNOWN_MODEL_ID; _idSet = true; }
    
    glm::vec3 getMinimumPoint() const { return _position - glm::vec3(_radius, _radius, _radius); }
    glm::vec3 getMaximumPoint() const { return _position + glm::vec3(_radius, _radius, _radius); }

    void debugDump() const;

private:
    glm::vec3 _position;
    xColor _color;
    float _radius;
    bool _shouldBeDeleted; /// to delete it
    
    QString _modelURL;
    glm::quat _rotation;
    QString _animationURL;
    bool _animationIsPlaying;
    float _animationFrameIndex;
    float _animationFPS;
    float _glowLevel;

    quint32 _id;
    bool _idSet;
    quint64 _lastEdited;

    bool _positionChanged;
    bool _colorChanged;
    bool _radiusChanged;
    bool _shouldBeDeletedChanged;

    bool _modelURLChanged;
    bool _rotationChanged;
    bool _animationURLChanged;
    bool _animationIsPlayingChanged;
    bool _animationFrameIndexChanged;
    bool _animationFPSChanged;
    bool _glowLevelChanged;
    bool _defaultSettings;
};
Q_DECLARE_METATYPE(EntityItemProperties);
QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties);
void EntityItemPropertiesFromScriptValue(const QScriptValue &object, EntityItemProperties& properties);


/// Abstract ID for editing model items. Used in EntityItem JS API - When models are created in the JS api, they are given a
/// local creatorTokenID, the actual id for the model is not known until the server responds to the creator with the
/// correct mapping. This class works with the scripting API an allows the developer to edit models they created.
class EntityItemID {
public:
    EntityItemID() :
            id(NEW_MODEL), creatorTokenID(UNKNOWN_MODEL_TOKEN), isKnownID(false) { };

    EntityItemID(uint32_t id, uint32_t creatorTokenID, bool isKnownID) :
            id(id), creatorTokenID(creatorTokenID), isKnownID(isKnownID) { };

    EntityItemID(uint32_t id) :
            id(id), creatorTokenID(UNKNOWN_MODEL_TOKEN), isKnownID(true) { };

    uint32_t id;
    uint32_t creatorTokenID;
    bool isKnownID;
};

inline bool operator<(const EntityItemID& a, const EntityItemID& b) {
    return (a.id == b.id) ? (a.creatorTokenID < b.creatorTokenID) : (a.id < b.id);
}

inline bool operator==(const EntityItemID& a, const EntityItemID& b) {
    if (a.id == UNKNOWN_MODEL_ID && b.id == UNKNOWN_MODEL_ID) {
        return a.creatorTokenID == b.creatorTokenID;
    }
    return a.id == b.id;
}

inline uint qHash(const EntityItemID& a, uint seed) {
    qint64 temp;
    if (a.id == UNKNOWN_MODEL_ID) {
        temp = -a.creatorTokenID;
    } else {
        temp = a.id;
    }
    return qHash(temp, seed);
}

inline QDebug operator<<(QDebug debug, const EntityItemID& id) {
    debug << "[ id:" << id.id << ", creatorTokenID:" << id.creatorTokenID << "]";
    return debug;
}

Q_DECLARE_METATYPE(EntityItemID);
Q_DECLARE_METATYPE(QVector<EntityItemID>);
QScriptValue EntityItemIDtoScriptValue(QScriptEngine* engine, const EntityItemID& properties);
void EntityItemIDfromScriptValue(const QScriptValue &object, EntityItemID& properties);



/// EntityItem class - this is the actual model item class.
class EntityItem  {

public:
    EntityItem();
    EntityItem(const EntityItemID& entityItemID);
    EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    
    /// creates an NEW model from an model add or edit message data buffer
    static EntityItem fromEditPacket(const unsigned char* data, int length, int& processedBytes, EntityTree* tree, bool& valid);

    virtual ~EntityItem();

    quint8 getType() const { return _type; }

    /// get position in domain scale units (0.0 - 1.0)
    const glm::vec3& getPosition() const { return _position; }

    glm::vec3 getMinimumPoint() const { return _position - glm::vec3(_radius, _radius, _radius); }
    glm::vec3 getMaximumPoint() const { return _position + glm::vec3(_radius, _radius, _radius); }

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }

    /// get radius in domain scale units (0.0 - 1.0)
    float getRadius() const { return _radius; }

    /// get maximum dimension in domain scale units (0.0 - 1.0)
    float getSize() const { return _radius * 2.0f; }

    /// get maximum dimension in domain scale units (0.0 - 1.0)
    AACube getAACube() const { return AACube(getMinimumPoint(), getSize()); }
    
    // model related properties
    bool hasModel() const { return !_modelURL.isEmpty(); }
    const QString& getModelURL() const { return _modelURL; }
    const glm::quat& getRotation() const { return _rotation; }
    bool hasAnimation() const { return !_animationURL.isEmpty(); }
    const QString& getAnimationURL() const { return _animationURL; }
    float getGlowLevel() const { return _glowLevel; }

    EntityItemID getEntityItemID() const { return EntityItemID(getID(), getCreatorTokenID(), getID() != UNKNOWN_MODEL_ID); }
    EntityItemProperties getProperties() const;

    /// The last updated/simulated time of this model from the time perspective of the authoritative server/source
    quint64 getLastUpdated() const { return _lastUpdated; }

    /// The last edited time of this model from the time perspective of the authoritative server/source
    quint64 getLastEdited() const { return _lastEdited; }
    void setLastEdited(quint64 lastEdited) { _lastEdited = lastEdited; }

    /// how long ago was this model edited in seconds
    float getEditedAgo() const { return static_cast<float>(usecTimestampNow() - _lastEdited) / static_cast<float>(USECS_PER_SECOND); }
    uint32_t getID() const { return _id; }
    void setID(uint32_t id) { _id = id; }
    bool getShouldBeDeleted() const { return _shouldBeDeleted; }
    uint32_t getCreatorTokenID() const { return _creatorTokenID; }
    bool isNewlyCreated() const { return _newlyCreated; }
    bool isKnownID() const { return getID() != UNKNOWN_MODEL_ID; }

    /// set position in domain scale units (0.0 - 1.0)
    void setPosition(const glm::vec3& value) { _position = value; }

    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
            _color[RED_INDEX] = value.red;
            _color[GREEN_INDEX] = value.green;
            _color[BLUE_INDEX] = value.blue;
    }
    /// set radius in domain scale units (0.0 - 1.0)
    void setRadius(float value) { _radius = value; }

    void setShouldBeDeleted(bool shouldBeDeleted) { _shouldBeDeleted = shouldBeDeleted; }
    void setCreatorTokenID(uint32_t creatorTokenID) { _creatorTokenID = creatorTokenID; }
    
    // model related properties
    void setModelURL(const QString& url) { _modelURL = url; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; }
    void setAnimationURL(const QString& url) { _animationURL = url; }
    void setAnimationFrameIndex(float value) { _animationFrameIndex = value; }
    void setAnimationIsPlaying(bool value) { _animationIsPlaying = value; }
    void setAnimationFPS(float value) { _animationFPS = value; }
    void setGlowLevel(float glowLevel) { _glowLevel = glowLevel; }
    
    void setProperties(const EntityItemProperties& properties, bool forceCopy = false);

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

    // these methods allow you to create models, and later edit them.
    static uint32_t getIDfromCreatorTokenID(uint32_t creatorTokenID);
    static uint32_t getNextCreatorTokenID();
    static void handleAddEntityResponse(const QByteArray& packet);

    void mapJoints(const QStringList& modelJointNames);
    QVector<glm::quat> getAnimationFrame();
    bool jointsMapped() const { return _jointMappingCompleted; }
    
    bool getAnimationIsPlaying() const { return _animationIsPlaying; }
    float getAnimationFrameIndex() const { return _animationFrameIndex; }
    float getAnimationFPS() const { return _animationFPS; }
    
    static void cleanupLoadedAnimations();

protected:
    void initFromEntityItemID(const EntityItemID& entityItemID);
    virtual void init(glm::vec3 position, float radius, rgbColor color, uint32_t id = NEW_MODEL);

    glm::vec3 _position;
    rgbColor _color;
    float _radius;
    quint32 _id;
    static quint32 _nextID;
    bool _shouldBeDeleted;
    quint32 _type;

    // model related items
    QString _modelURL;
    glm::quat _rotation;
    
    float _glowLevel;

    uint32_t _creatorTokenID;
    bool _newlyCreated;

    quint64 _lastUpdated;
    quint64 _lastEdited;
    quint64 _lastAnimated;

    QString _animationURL;
    float _animationFrameIndex; // we keep this as a float and round to int only when we need the exact index
    bool _animationIsPlaying;
    float _animationFPS;
    
    bool _jointMappingCompleted;
    QVector<int> _jointMapping;
    

    // used by the static interfaces for creator token ids
    static uint32_t _nextCreatorTokenID;
    static std::map<uint32_t,uint32_t> _tokenIDsToIDs;


    static Animation* getAnimation(const QString& url);
    static QMap<QString, AnimationPointer> _loadedAnimations;
    static AnimationCache _animationCache;

};

#endif // hifi_EntityItem_h
