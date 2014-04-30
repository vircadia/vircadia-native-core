//
//  ModelItem.h
//  libraries/particles/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelItem_h
#define hifi_ModelItem_h

#include <glm/glm.hpp>
#include <stdint.h>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>

#include <CollisionInfo.h>
#include <SharedUtil.h>
#include <OctreePacketData.h>

class ModelItem;
class ModelEditPacketSender;
class ModelItemProperties;
class ModelsScriptingInterface;
class ModelTree;
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
const uint16_t MODEL_PACKET_CONTAINS_VELOCITY = 8;
const uint16_t MODEL_PACKET_CONTAINS_GRAVITY = 16;
const uint16_t MODEL_PACKET_CONTAINS_DAMPING = 32;
const uint16_t MODEL_PACKET_CONTAINS_LIFETIME = 64;
const uint16_t MODEL_PACKET_CONTAINS_INHAND = 128;
const uint16_t MODEL_PACKET_CONTAINS_SCRIPT = 256;
const uint16_t MODEL_PACKET_CONTAINS_SHOULDDIE = 512;
const uint16_t MODEL_PACKET_CONTAINS_MODEL_URL = 1024;
const uint16_t MODEL_PACKET_CONTAINS_MODEL_TRANSLATION = 1024;
const uint16_t MODEL_PACKET_CONTAINS_MODEL_ROTATION = 2048;
const uint16_t MODEL_PACKET_CONTAINS_MODEL_SCALE = 4096;

const float MODEL_DEFAULT_LIFETIME = 10.0f; // particles live for 10 seconds by default
const float MODEL_DEFAULT_DAMPING = 0.99f;
const float MODEL_DEFAULT_RADIUS = 0.1f / TREE_SCALE;
const float MODEL_MINIMUM_PARTICLE_ELEMENT_SIZE = (1.0f / 100000.0f) / TREE_SCALE; // smallest size container
const glm::vec3 MODEL_DEFAULT_GRAVITY(0, (-9.8f / TREE_SCALE), 0);
const QString MODEL_DEFAULT_SCRIPT("");
const QString MODEL_DEFAULT_MODEL_URL("");
const glm::vec3 MODEL_DEFAULT_MODEL_TRANSLATION(0, 0, 0);
const glm::quat MODEL_DEFAULT_MODEL_ROTATION(0, 0, 0, 0);
const float MODEL_DEFAULT_MODEL_SCALE = 1.0f;
const bool MODEL_IN_HAND = true; // it's in a hand
const bool MODEL_NOT_IN_HAND = !MODEL_IN_HAND; // it's not in a hand

/// A collection of properties of a particle used in the scripting API. Translates between the actual properties of a particle
/// and a JavaScript style hash/QScriptValue storing a set of properties. Used in scripting to set/get the complete set of
/// particle properties via JavaScript hashes/QScriptValues
/// all units for position, velocity, gravity, radius, etc are in meter units
class ModelItemProperties {
public:
    ModelItemProperties();

    QScriptValue copyToScriptValue(QScriptEngine* engine) const;
    void copyFromScriptValue(const QScriptValue& object);

    void copyToModelItem(ModelItem& particle) const;
    void copyFromModelItem(const ModelItem& particle);

    const glm::vec3& getPosition() const { return _position; }
    xColor getColor() const { return _color; }
    float getRadius() const { return _radius; }
    const glm::vec3& getVelocity() const { return _velocity; }
    const glm::vec3& getGravity() const { return _gravity; }
    float getDamping() const { return _damping; }
    float getLifetime() const { return _lifetime; }
    const QString& getScript() const { return _script; }
    bool getInHand() const { return _inHand; }
    bool getShouldDie() const { return _shouldDie; }
    const QString& getModelURL() const { return _modelURL; }
    float getModelScale() const { return _modelScale; }
    const glm::vec3& getModelTranslation() const { return _modelTranslation; }
    const glm::quat& getModelRotation() const { return _modelRotation; }

    quint64 getLastEdited() const { return _lastEdited; }
    uint16_t getChangedBits() const;

    /// set position in meter units
    void setPosition(const glm::vec3& value) { _position = value; _positionChanged = true; }

    /// set velocity in meter units
    void setVelocity(const glm::vec3& value) { _velocity = value; _velocityChanged = true;  }
    void setColor(const xColor& value) { _color = value; _colorChanged = true; }
    void setRadius(float value) { _radius = value; _radiusChanged = true; }

    /// set gravity in meter units
    void setGravity(const glm::vec3& value) { _gravity = value; _gravityChanged = true;  }
    void setInHand(bool inHand) { _inHand = inHand; _inHandChanged = true; }
    void setDamping(float value) { _damping = value; _dampingChanged = true;  }
    void setShouldDie(bool shouldDie) { _shouldDie = shouldDie; _shouldDieChanged = true;  }
    void setLifetime(float value) { _lifetime = value; _lifetimeChanged = true;  }
    void setScript(const QString& updateScript) { _script = updateScript; _scriptChanged = true;  }

    // model related properties
    void setModelURL(const QString& url) { _modelURL = url; _modelURLChanged = true; }
    void setModelScale(float scale) { _modelScale = scale; _modelScaleChanged = true; }
    void setModelTranslation(const glm::vec3&  translation) { _modelTranslation = translation; 
                                                              _modelTranslationChanged = true; }
    void setModelRotation(const glm::quat& rotation) { _modelRotation = rotation; _modelRotationChanged = true; }
    
    /// used by ModelScriptingInterface to return ModelItemProperties for unknown particles
    void setIsUnknownID() { _id = UNKNOWN_MODEL_ID; _idSet = true; }

private:
    glm::vec3 _position;
    xColor _color;
    float _radius;
    glm::vec3 _velocity;
    glm::vec3 _gravity;
    float _damping;
    float _lifetime;
    QString _script;
    bool _inHand;
    bool _shouldDie;
    QString _modelURL;
    float _modelScale;
    glm::vec3 _modelTranslation;
    glm::quat _modelRotation;

    uint32_t _id;
    bool _idSet;
    quint64 _lastEdited;

    bool _positionChanged;
    bool _colorChanged;
    bool _radiusChanged;
    bool _velocityChanged;
    bool _gravityChanged;
    bool _dampingChanged;
    bool _lifetimeChanged;
    bool _scriptChanged;
    bool _inHandChanged;
    bool _shouldDieChanged;
    bool _modelURLChanged;
    bool _modelScaleChanged;
    bool _modelTranslationChanged;
    bool _modelRotationChanged;
    bool _defaultSettings;
};
Q_DECLARE_METATYPE(ModelItemProperties);
QScriptValue ModelItemPropertiesToScriptValue(QScriptEngine* engine, const ModelItemProperties& properties);
void ModelItemPropertiesFromScriptValue(const QScriptValue &object, ModelItemProperties& properties);


/// Abstract ID for editing particles. Used in ModelItem JS API - When particles are created in the JS api, they are given a
/// local creatorTokenID, the actual id for the particle is not known until the server responds to the creator with the
/// correct mapping. This class works with the scripting API an allows the developer to edit particles they created.
class ModelItemID {
public:
    ModelItemID() :
            id(NEW_MODEL), creatorTokenID(UNKNOWN_MODEL_TOKEN), isKnownID(false) { };

    ModelItemID(uint32_t id, uint32_t creatorTokenID, bool isKnownID) :
            id(id), creatorTokenID(creatorTokenID), isKnownID(isKnownID) { };

    ModelItemID(uint32_t id) :
            id(id), creatorTokenID(UNKNOWN_MODEL_TOKEN), isKnownID(true) { };

    uint32_t id;
    uint32_t creatorTokenID;
    bool isKnownID;
};

Q_DECLARE_METATYPE(ModelItemID);
Q_DECLARE_METATYPE(QVector<ModelItemID>);
QScriptValue ModelItemIDtoScriptValue(QScriptEngine* engine, const ModelItemID& properties);
void ModelItemIDfromScriptValue(const QScriptValue &object, ModelItemID& properties);



/// ModelItem class - this is the actual particle class.
class ModelItem  {

public:
    ModelItem();

    ModelItem(const ModelItemID& particleID, const ModelItemProperties& properties);
    
    /// creates an NEW particle from an PACKET_TYPE_PARTICLE_ADD_OR_EDIT edit data buffer
    static ModelItem fromEditPacket(const unsigned char* data, int length, int& processedBytes, ModelTree* tree, bool& valid);

    virtual ~ModelItem();
    virtual void init(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity,
            glm::vec3 gravity = MODEL_DEFAULT_GRAVITY, float damping = MODEL_DEFAULT_DAMPING, float lifetime = MODEL_DEFAULT_LIFETIME,
            bool inHand = MODEL_NOT_IN_HAND, QString updateScript = MODEL_DEFAULT_SCRIPT, uint32_t id = NEW_MODEL);

    /// get position in domain scale units (0.0 - 1.0)
    const glm::vec3& getPosition() const { return _position; }

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }

    /// get radius in domain scale units (0.0 - 1.0)
    float getRadius() const { return _radius; }
    float getMass() const { return _mass; }

    /// get velocity in domain scale units (0.0 - 1.0)
    const glm::vec3& getVelocity() const { return _velocity; }

    /// get gravity in domain scale units (0.0 - 1.0)
    const glm::vec3& getGravity() const { return _gravity; }

    bool getInHand() const { return _inHand; }
    float getDamping() const { return _damping; }
    float getLifetime() const { return _lifetime; }
    
    // model related properties
    bool hasModel() const { return !_modelURL.isEmpty(); }
    const QString& getModelURL() const { return _modelURL; }
    float getModelScale() const { return _modelScale; }
    const glm::vec3& getModelTranslation() const { return _modelTranslation; }
    const glm::quat& getModelRotation() const { return _modelRotation; }

    ModelItemID getModelItemID() const { return ModelItemID(getID(), getCreatorTokenID(), getID() != UNKNOWN_MODEL_ID); }
    ModelItemProperties getProperties() const;

    /// The last updated/simulated time of this particle from the time perspective of the authoritative server/source
    quint64 getLastUpdated() const { return _lastUpdated; }

    /// The last edited time of this particle from the time perspective of the authoritative server/source
    quint64 getLastEdited() const { return _lastEdited; }
    void setLastEdited(quint64 lastEdited) { _lastEdited = lastEdited; }

    /// lifetime of the particle in seconds
    float getAge() const { return static_cast<float>(usecTimestampNow() - _created) / static_cast<float>(USECS_PER_SECOND); }
    float getEditedAgo() const { return static_cast<float>(usecTimestampNow() - _lastEdited) / static_cast<float>(USECS_PER_SECOND); }
    uint32_t getID() const { return _id; }
    void setID(uint32_t id) { _id = id; }
    bool getShouldDie() const { return _shouldDie; }
    QString getScript() const { return _script; }
    uint32_t getCreatorTokenID() const { return _creatorTokenID; }
    bool isNewlyCreated() const { return _newlyCreated; }

    /// set position in domain scale units (0.0 - 1.0)
    void setPosition(const glm::vec3& value) { _position = value; }

    /// set velocity in domain scale units (0.0 - 1.0)
    void setVelocity(const glm::vec3& value) { _velocity = value; }
    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
            _color[RED_INDEX] = value.red;
            _color[GREEN_INDEX] = value.green;
            _color[BLUE_INDEX] = value.blue;
    }
    /// set radius in domain scale units (0.0 - 1.0)
    void setRadius(float value) { _radius = value; }
    void setMass(float value);

    /// set gravity in domain scale units (0.0 - 1.0)
    void setGravity(const glm::vec3& value) { _gravity = value; }
    void setInHand(bool inHand) { _inHand = inHand; }
    void setDamping(float value) { _damping = value; }
    void setShouldDie(bool shouldDie) { _shouldDie = shouldDie; }
    void setLifetime(float value) { _lifetime = value; }
    void setScript(QString updateScript) { _script = updateScript; }
    void setCreatorTokenID(uint32_t creatorTokenID) { _creatorTokenID = creatorTokenID; }
    
    // model related properties
    void setModelURL(const QString& url) { _modelURL = url; }
    void setModelScale(float scale) { _modelScale = scale; }
    void setModelTranslation(const glm::vec3&  translation) { _modelTranslation = translation; }
    void setModelRotation(const glm::quat& rotation) { _modelRotation = rotation; }
    
    void setProperties(const ModelItemProperties& properties);

    bool appendModelData(OctreePacketData* packetData) const;
    int readModelDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    static int expectedBytes();

    static bool encodeModelEditMessageDetails(PacketType command, ModelItemID id, const ModelItemProperties& details,
                        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static void adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew);
    
    void applyHardCollision(const CollisionInfo& collisionInfo);

    void update(const quint64& now);

    void debugDump() const;

    // similar to assignment/copy, but it handles keeping lifetime accurate
    void copyChangedProperties(const ModelItem& other);

    static VoxelEditPacketSender* getVoxelEditPacketSender() { return _voxelEditSender; }
    static ModelEditPacketSender* getModelEditPacketSender() { return _particleEditSender; }

    static void setVoxelEditPacketSender(VoxelEditPacketSender* senderInterface)
                    { _voxelEditSender = senderInterface; }

    static void setModelEditPacketSender(ModelEditPacketSender* senderInterface)
                    { _particleEditSender = senderInterface; }


    // these methods allow you to create particles, and later edit them.
    static uint32_t getIDfromCreatorTokenID(uint32_t creatorTokenID);
    static uint32_t getNextCreatorTokenID();
    static void handleAddModelResponse(const QByteArray& packet);

protected:
    static VoxelEditPacketSender* _voxelEditSender;
    static ModelEditPacketSender* _particleEditSender;

    void setAge(float age);

    glm::vec3 _position;
    rgbColor _color;
    float _radius;
    float _mass;
    glm::vec3 _velocity;
    uint32_t _id;
    static uint32_t _nextID;
    bool _shouldDie;
    glm::vec3 _gravity;
    float _damping;
    float _lifetime;
    QString _script;
    bool _inHand;

    // model related items
    QString _modelURL;
    float _modelScale;
    glm::vec3 _modelTranslation;
    glm::quat _modelRotation;

    uint32_t _creatorTokenID;
    bool _newlyCreated;

    quint64 _lastUpdated;
    quint64 _lastEdited;

    // this doesn't go on the wire, we send it as lifetime
    quint64 _created;

    // used by the static interfaces for creator token ids
    static uint32_t _nextCreatorTokenID;
    static std::map<uint32_t,uint32_t> _tokenIDsToIDs;
};

#endif // hifi_ModelItem_h
