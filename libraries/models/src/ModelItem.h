//
//  ModelItem.h
//  libraries/models/src
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
const uint16_t MODEL_PACKET_CONTAINS_SHOULDDIE = 512;
const uint16_t MODEL_PACKET_CONTAINS_MODEL_URL = 1024;
const uint16_t MODEL_PACKET_CONTAINS_MODEL_ROTATION = 2048;

const float MODEL_DEFAULT_RADIUS = 0.1f / TREE_SCALE;
const float MINIMUM_MODEL_ELEMENT_SIZE = (1.0f / 100000.0f) / TREE_SCALE; // smallest size container
const QString MODEL_DEFAULT_MODEL_URL("");
const glm::quat MODEL_DEFAULT_MODEL_ROTATION;

/// A collection of properties of a model item used in the scripting API. Translates between the actual properties of a model
/// and a JavaScript style hash/QScriptValue storing a set of properties. Used in scripting to set/get the complete set of
/// model item properties via JavaScript hashes/QScriptValues
/// all units for position, radius, etc are in meter units
class ModelItemProperties {
public:
    ModelItemProperties();

    QScriptValue copyToScriptValue(QScriptEngine* engine) const;
    void copyFromScriptValue(const QScriptValue& object);

    void copyToModelItem(ModelItem& modelItem) const;
    void copyFromModelItem(const ModelItem& modelItem);

    const glm::vec3& getPosition() const { return _position; }
    xColor getColor() const { return _color; }
    float getRadius() const { return _radius; }
    bool getShouldDie() const { return _shouldDie; }
    
    const QString& getModelURL() const { return _modelURL; }
    const glm::quat& getModelRotation() const { return _modelRotation; }

    quint64 getLastEdited() const { return _lastEdited; }
    uint16_t getChangedBits() const;

    /// set position in meter units
    void setPosition(const glm::vec3& value) { _position = value; _positionChanged = true; }
    void setColor(const xColor& value) { _color = value; _colorChanged = true; }
    void setRadius(float value) { _radius = value; _radiusChanged = true; }
    void setShouldDie(bool shouldDie) { _shouldDie = shouldDie; _shouldDieChanged = true;  }

    // model related properties
    void setModelURL(const QString& url) { _modelURL = url; _modelURLChanged = true; }
    void setModelRotation(const glm::quat& rotation) { _modelRotation = rotation; _modelRotationChanged = true; }
    
    /// used by ModelScriptingInterface to return ModelItemProperties for unknown models
    void setIsUnknownID() { _id = UNKNOWN_MODEL_ID; _idSet = true; }
    
    glm::vec3 getMinimumPoint() const { return _position - glm::vec3(_radius, _radius, _radius); }
    glm::vec3 getMaximumPoint() const { return _position + glm::vec3(_radius, _radius, _radius); }

private:
    glm::vec3 _position;
    xColor _color;
    float _radius;
    bool _shouldDie; /// to delete it
    
    QString _modelURL;
    glm::quat _modelRotation;

    uint32_t _id;
    bool _idSet;
    quint64 _lastEdited;

    bool _positionChanged;
    bool _colorChanged;
    bool _radiusChanged;
    bool _shouldDieChanged;

    bool _modelURLChanged;
    bool _modelRotationChanged;
    bool _defaultSettings;
};
Q_DECLARE_METATYPE(ModelItemProperties);
QScriptValue ModelItemPropertiesToScriptValue(QScriptEngine* engine, const ModelItemProperties& properties);
void ModelItemPropertiesFromScriptValue(const QScriptValue &object, ModelItemProperties& properties);


/// Abstract ID for editing model items. Used in ModelItem JS API - When models are created in the JS api, they are given a
/// local creatorTokenID, the actual id for the model is not known until the server responds to the creator with the
/// correct mapping. This class works with the scripting API an allows the developer to edit models they created.
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



/// ModelItem class - this is the actual model item class.
class ModelItem  {

public:
    ModelItem();

    ModelItem(const ModelItemID& modelItemID, const ModelItemProperties& properties);
    
    /// creates an NEW model from an model add or edit message data buffer
    static ModelItem fromEditPacket(const unsigned char* data, int length, int& processedBytes, ModelTree* tree, bool& valid);

    virtual ~ModelItem();
    virtual void init(glm::vec3 position, float radius, rgbColor color, uint32_t id = NEW_MODEL);

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
    AABox getAABox() const { return AABox(getMinimumPoint(), getSize()); }
    
    // model related properties
    bool hasModel() const { return !_modelURL.isEmpty(); }
    const QString& getModelURL() const { return _modelURL; }
    const glm::quat& getModelRotation() const { return _modelRotation; }

    ModelItemID getModelItemID() const { return ModelItemID(getID(), getCreatorTokenID(), getID() != UNKNOWN_MODEL_ID); }
    ModelItemProperties getProperties() const;

    /// The last updated/simulated time of this model from the time perspective of the authoritative server/source
    quint64 getLastUpdated() const { return _lastUpdated; }

    /// The last edited time of this model from the time perspective of the authoritative server/source
    quint64 getLastEdited() const { return _lastEdited; }
    void setLastEdited(quint64 lastEdited) { _lastEdited = lastEdited; }

    /// how long ago was this model edited in seconds
    float getEditedAgo() const { return static_cast<float>(usecTimestampNow() - _lastEdited) / static_cast<float>(USECS_PER_SECOND); }
    uint32_t getID() const { return _id; }
    void setID(uint32_t id) { _id = id; }
    bool getShouldDie() const { return _shouldDie; }
    uint32_t getCreatorTokenID() const { return _creatorTokenID; }
    bool isNewlyCreated() const { return _newlyCreated; }

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

    void setShouldDie(bool shouldDie) { _shouldDie = shouldDie; }
    void setCreatorTokenID(uint32_t creatorTokenID) { _creatorTokenID = creatorTokenID; }
    
    // model related properties
    void setModelURL(const QString& url) { _modelURL = url; }
    void setModelRotation(const glm::quat& rotation) { _modelRotation = rotation; }
    
    void setProperties(const ModelItemProperties& properties);

    bool appendModelData(OctreePacketData* packetData) const;
    int readModelDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    static int expectedBytes();

    static bool encodeModelEditMessageDetails(PacketType command, ModelItemID id, const ModelItemProperties& details,
                        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static void adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew);
    
    void update(const quint64& now);

    void debugDump() const;

    // similar to assignment/copy, but it handles keeping lifetime accurate
    void copyChangedProperties(const ModelItem& other);

    // these methods allow you to create models, and later edit them.
    static uint32_t getIDfromCreatorTokenID(uint32_t creatorTokenID);
    static uint32_t getNextCreatorTokenID();
    static void handleAddModelResponse(const QByteArray& packet);

protected:
    glm::vec3 _position;
    rgbColor _color;
    float _radius;
    uint32_t _id;
    static uint32_t _nextID;
    bool _shouldDie;

    // model related items
    QString _modelURL;
    glm::quat _modelRotation;

    uint32_t _creatorTokenID;
    bool _newlyCreated;

    quint64 _lastUpdated;
    quint64 _lastEdited;

    // used by the static interfaces for creator token ids
    static uint32_t _nextCreatorTokenID;
    static std::map<uint32_t,uint32_t> _tokenIDsToIDs;
};

#endif // hifi_ModelItem_h
