//
//  ModelItem.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QObject>

#include <Octree.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // usecTimestampNow()
#include <VoxelsScriptingInterface.h>
#include <VoxelDetail.h>


// This is not ideal, but adding script-engine as a linked library, will cause a circular reference
// I'm open to other potential solutions. Could we change cmake to allow libraries to reference each others
// headers, but not link to each other, this is essentially what this construct is doing, but would be
// better to add includes to the include path, but not link
#include "../../script-engine/src/ScriptEngine.h"

#include "ModelsScriptingInterface.h"
#include "ModelItem.h"
#include "ModelTree.h"

uint32_t ModelItem::_nextID = 0;

// for locally created models
std::map<uint32_t,uint32_t> ModelItem::_tokenIDsToIDs;
uint32_t ModelItem::_nextCreatorTokenID = 0;

uint32_t ModelItem::getIDfromCreatorTokenID(uint32_t creatorTokenID) {
    if (_tokenIDsToIDs.find(creatorTokenID) != _tokenIDsToIDs.end()) {
        return _tokenIDsToIDs[creatorTokenID];
    }
    return UNKNOWN_MODEL_ID;
}

uint32_t ModelItem::getNextCreatorTokenID() {
    uint32_t creatorTokenID = _nextCreatorTokenID;
    _nextCreatorTokenID++;
    return creatorTokenID;
}

void ModelItem::handleAddModelResponse(const QByteArray& packet) {
    const unsigned char* dataAt = reinterpret_cast<const unsigned char*>(packet.data());
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    dataAt += numBytesPacketHeader;

    uint32_t creatorTokenID;
    memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
    dataAt += sizeof(creatorTokenID);

    uint32_t modelItemID;
    memcpy(&modelItemID, dataAt, sizeof(modelItemID));
    dataAt += sizeof(modelItemID);

    // add our token to id mapping
    _tokenIDsToIDs[creatorTokenID] = modelItemID;
}

ModelItem::ModelItem() {
    rgbColor noColor = { 0, 0, 0 };
    init(glm::vec3(0,0,0), 0, noColor, NEW_MODEL);
}

ModelItem::ModelItem(const ModelItemID& modelItemID, const ModelItemProperties& properties) {
    _id = modelItemID.id;
    _creatorTokenID = modelItemID.creatorTokenID;

    // init values with defaults before calling setProperties
    uint64_t now = usecTimestampNow();
    _lastEdited = now;
    _lastUpdated = now;

    _position = glm::vec3(0,0,0);
    _radius = 0;
    rgbColor noColor = { 0, 0, 0 };
    memcpy(_color, noColor, sizeof(_color));
    _shouldDie = false;
    _modelURL = MODEL_DEFAULT_MODEL_URL;
    _modelTranslation = MODEL_DEFAULT_MODEL_TRANSLATION;
    _modelRotation = MODEL_DEFAULT_MODEL_ROTATION;
    _modelScale = MODEL_DEFAULT_MODEL_SCALE;
    
    setProperties(properties);
}


ModelItem::~ModelItem() {
}

void ModelItem::init(glm::vec3 position, float radius, rgbColor color, uint32_t id) {
    if (id == NEW_MODEL) {
        _id = _nextID;
        _nextID++;
    } else {
        _id = id;
    }
    quint64 now = usecTimestampNow();
    _lastEdited = now;
    _lastUpdated = now;

    _position = position;
    _radius = radius;
    memcpy(_color, color, sizeof(_color));
    _shouldDie = false;
    _modelURL = MODEL_DEFAULT_MODEL_URL;
    _modelTranslation = MODEL_DEFAULT_MODEL_TRANSLATION;
    _modelRotation = MODEL_DEFAULT_MODEL_ROTATION;
    _modelScale = MODEL_DEFAULT_MODEL_SCALE;
}

bool ModelItem::appendModelData(OctreePacketData* packetData) const {

    bool success = packetData->appendValue(getID());

    //qDebug("ModelItem::appendModelData()... getID()=%d", getID());

    if (success) {
        success = packetData->appendValue(getLastUpdated());
    }
    if (success) {
        success = packetData->appendValue(getLastEdited());
    }
    if (success) {
        success = packetData->appendValue(getRadius());
    }
    if (success) {
        success = packetData->appendPosition(getPosition());
    }
    if (success) {
        success = packetData->appendColor(getColor());
    }
    if (success) {
        success = packetData->appendValue(getShouldDie());
    }

    // modelURL
    if (success) {
        uint16_t modelURLLength = _modelURL.size() + 1; // include NULL
        success = packetData->appendValue(modelURLLength);
        if (success) {
            success = packetData->appendRawData((const unsigned char*)qPrintable(_modelURL), modelURLLength);
        }
    }

    // modelScale
    if (success) {
        success = packetData->appendValue(getModelScale());
    }

    // modelTranslation
    if (success) {
        success = packetData->appendValue(getModelTranslation());
    }
    // modelRotation
    if (success) {
        success = packetData->appendValue(getModelRotation());
    }
    return success;
}

int ModelItem::expectedBytes() {
    int expectedBytes = sizeof(uint32_t) // id
                + sizeof(float) // age
                + sizeof(quint64) // last updated
                + sizeof(quint64) // lasted edited
                + sizeof(float) // radius
                + sizeof(glm::vec3) // position
                + sizeof(rgbColor); // color
                // potentially more...
    return expectedBytes;
}

int ModelItem::readModelDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {
    int bytesRead = 0;
    if (bytesLeftToRead >= expectedBytes()) {
        int clockSkew = args.sourceNode ? args.sourceNode->getClockSkewUsec() : 0;

        const unsigned char* dataAt = data;

        // id
        memcpy(&_id, dataAt, sizeof(_id));
        dataAt += sizeof(_id);
        bytesRead += sizeof(_id);

        // _lastUpdated
        memcpy(&_lastUpdated, dataAt, sizeof(_lastUpdated));
        dataAt += sizeof(_lastUpdated);
        bytesRead += sizeof(_lastUpdated);
        _lastUpdated -= clockSkew;

        // _lastEdited
        memcpy(&_lastEdited, dataAt, sizeof(_lastEdited));
        dataAt += sizeof(_lastEdited);
        bytesRead += sizeof(_lastEdited);
        _lastEdited -= clockSkew;

        // radius
        memcpy(&_radius, dataAt, sizeof(_radius));
        dataAt += sizeof(_radius);
        bytesRead += sizeof(_radius);

        // position
        memcpy(&_position, dataAt, sizeof(_position));
        dataAt += sizeof(_position);
        bytesRead += sizeof(_position);

        // color
        memcpy(_color, dataAt, sizeof(_color));
        dataAt += sizeof(_color);
        bytesRead += sizeof(_color);

        // shouldDie
        memcpy(&_shouldDie, dataAt, sizeof(_shouldDie));
        dataAt += sizeof(_shouldDie);
        bytesRead += sizeof(_shouldDie);

        // modelURL
        uint16_t modelURLLength;
        memcpy(&modelURLLength, dataAt, sizeof(modelURLLength));
        dataAt += sizeof(modelURLLength);
        bytesRead += sizeof(modelURLLength);
        QString modelURLString((const char*)dataAt);
        _modelURL = modelURLString;
        dataAt += modelURLLength;
        bytesRead += modelURLLength;

        // modelScale
        memcpy(&_modelScale, dataAt, sizeof(_modelScale));
        dataAt += sizeof(_modelScale);
        bytesRead += sizeof(_modelScale);

        // modelTranslation
        memcpy(&_modelTranslation, dataAt, sizeof(_modelTranslation));
        dataAt += sizeof(_modelTranslation);
        bytesRead += sizeof(_modelTranslation);

        // modelRotation
        int bytes = unpackOrientationQuatFromBytes(dataAt, _modelRotation);
        dataAt += bytes;
        bytesRead += bytes;

        //printf("ModelItem::readModelDataFromBuffer()... "); debugDump();
    }
    return bytesRead;
}

ModelItem ModelItem::fromEditPacket(const unsigned char* data, int length, int& processedBytes, ModelTree* tree, bool& valid) {

    ModelItem newModelItem; // id and _lastUpdated will get set here...
    const unsigned char* dataAt = data;
    processedBytes = 0;

    // the first part of the data is our octcode...
    int octets = numberOfThreeBitSectionsInCode(data);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);

    // we don't actually do anything with this octcode...
    dataAt += lengthOfOctcode;
    processedBytes += lengthOfOctcode;

    // id
    uint32_t editID;
    memcpy(&editID, dataAt, sizeof(editID));
    dataAt += sizeof(editID);
    processedBytes += sizeof(editID);

    bool isNewModelItem = (editID == NEW_MODEL);

    // special case for handling "new" modelItems
    if (isNewModelItem) {
        // If this is a NEW_MODEL, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id
        uint32_t creatorTokenID;
        memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
        dataAt += sizeof(creatorTokenID);
        processedBytes += sizeof(creatorTokenID);

        newModelItem.setCreatorTokenID(creatorTokenID);
        newModelItem._newlyCreated = true;

    } else {
        // look up the existing modelItem
        const ModelItem* existingModelItem = tree->findModelByID(editID, true);

        // copy existing properties before over-writing with new properties
        if (existingModelItem) {
            newModelItem = *existingModelItem;
        } else {
            // the user attempted to edit a modelItem that doesn't exist
            qDebug() << "user attempted to edit a modelItem that doesn't exist...";
            valid = false;
            return newModelItem;
        }
        newModelItem._id = editID;
        newModelItem._newlyCreated = false;
    }
    
    // if we got this far, then our result will be valid
    valid = true;
    

    // lastEdited
    memcpy(&newModelItem._lastEdited, dataAt, sizeof(newModelItem._lastEdited));
    dataAt += sizeof(newModelItem._lastEdited);
    processedBytes += sizeof(newModelItem._lastEdited);

    // All of the remaining items are optional, and may or may not be included based on their included values in the
    // properties included bits
    uint16_t packetContainsBits = 0;
    if (!isNewModelItem) {
        memcpy(&packetContainsBits, dataAt, sizeof(packetContainsBits));
        dataAt += sizeof(packetContainsBits);
        processedBytes += sizeof(packetContainsBits);
    }


    // radius
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_RADIUS) == MODEL_PACKET_CONTAINS_RADIUS)) {
        memcpy(&newModelItem._radius, dataAt, sizeof(newModelItem._radius));
        dataAt += sizeof(newModelItem._radius);
        processedBytes += sizeof(newModelItem._radius);
    }

    // position
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_POSITION) == MODEL_PACKET_CONTAINS_POSITION)) {
        memcpy(&newModelItem._position, dataAt, sizeof(newModelItem._position));
        dataAt += sizeof(newModelItem._position);
        processedBytes += sizeof(newModelItem._position);
    }

    // color
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_COLOR) == MODEL_PACKET_CONTAINS_COLOR)) {
        memcpy(newModelItem._color, dataAt, sizeof(newModelItem._color));
        dataAt += sizeof(newModelItem._color);
        processedBytes += sizeof(newModelItem._color);
    }

    // shouldDie
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_SHOULDDIE) == MODEL_PACKET_CONTAINS_SHOULDDIE)) {
        memcpy(&newModelItem._shouldDie, dataAt, sizeof(newModelItem._shouldDie));
        dataAt += sizeof(newModelItem._shouldDie);
        processedBytes += sizeof(newModelItem._shouldDie);
    }

    // modelURL
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_MODEL_URL) == MODEL_PACKET_CONTAINS_MODEL_URL)) {
        uint16_t modelURLLength;
        memcpy(&modelURLLength, dataAt, sizeof(modelURLLength));
        dataAt += sizeof(modelURLLength);
        processedBytes += sizeof(modelURLLength);
        QString tempString((const char*)dataAt);
        newModelItem._modelURL = tempString;
        dataAt += modelURLLength;
        processedBytes += modelURLLength;
    }

    // modelScale
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_MODEL_SCALE) == MODEL_PACKET_CONTAINS_MODEL_SCALE)) {
        memcpy(&newModelItem._modelScale, dataAt, sizeof(newModelItem._modelScale));
        dataAt += sizeof(newModelItem._modelScale);
        processedBytes += sizeof(newModelItem._modelScale);
    }

    // modelTranslation
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_MODEL_TRANSLATION) == MODEL_PACKET_CONTAINS_MODEL_TRANSLATION)) {
        memcpy(&newModelItem._modelTranslation, dataAt, sizeof(newModelItem._modelTranslation));
        dataAt += sizeof(newModelItem._modelTranslation);
        processedBytes += sizeof(newModelItem._modelTranslation);
    }

    // modelRotation
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_MODEL_ROTATION) == MODEL_PACKET_CONTAINS_MODEL_ROTATION)) {
        int bytes = unpackOrientationQuatFromBytes(dataAt, newModelItem._modelRotation);
        dataAt += bytes;
        processedBytes += bytes;
    }

    const bool wantDebugging = false;
    if (wantDebugging) {
        qDebug("ModelItem::fromEditPacket()...");
        qDebug() << "   ModelItem id in packet:" << editID;
        newModelItem.debugDump();
    }

    return newModelItem;
}

void ModelItem::debugDump() const {
    qDebug("ModelItem id  :%u", _id);
    qDebug(" edited ago:%f", getEditedAgo());
    qDebug(" should die:%s", debug::valueOf(getShouldDie()));
    qDebug(" position:%f,%f,%f", _position.x, _position.y, _position.z);
    qDebug(" radius:%f", getRadius());
    qDebug(" color:%d,%d,%d", _color[0], _color[1], _color[2]);
}

bool ModelItem::encodeModelEditMessageDetails(PacketType command, ModelItemID id, const ModelItemProperties& properties,
        unsigned char* bufferOut, int sizeIn, int& sizeOut) {

    bool success = true; // assume the best
    unsigned char* copyAt = bufferOut;
    sizeOut = 0;

    // get the octal code for the modelItem

    // this could be a problem if the caller doesn't include position....
    glm::vec3 rootPosition(0);
    float rootScale = 0.5f;
    unsigned char* octcode = pointToOctalCode(rootPosition.x, rootPosition.y, rootPosition.z, rootScale);

    // TODO: Consider this old code... including the correct octree for where the modelItem will go matters for 
    // modelItem servers with different jurisdictions, but for now, we'll send everything to the root, since the 
    // tree does the right thing...
    //
    //unsigned char* octcode = pointToOctalCode(details[i].position.x, details[i].position.y,
    //                                          details[i].position.z, details[i].radius);

    int octets = numberOfThreeBitSectionsInCode(octcode);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);

    // add it to our message
    memcpy(copyAt, octcode, lengthOfOctcode);
    copyAt += lengthOfOctcode;
    sizeOut += lengthOfOctcode;

    // Now add our edit content details...
    bool isNewModelItem = (id.id == NEW_MODEL);

    // id
    memcpy(copyAt, &id.id, sizeof(id.id));
    copyAt += sizeof(id.id);
    sizeOut += sizeof(id.id);

    // special case for handling "new" modelItems
    if (isNewModelItem) {
        // If this is a NEW_MODEL, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id
        memcpy(copyAt, &id.creatorTokenID, sizeof(id.creatorTokenID));
        copyAt += sizeof(id.creatorTokenID);
        sizeOut += sizeof(id.creatorTokenID);
    }
    
    // lastEdited
    quint64 lastEdited = properties.getLastEdited();
    memcpy(copyAt, &lastEdited, sizeof(lastEdited));
    copyAt += sizeof(lastEdited);
    sizeOut += sizeof(lastEdited);
    
    // For new modelItems, all remaining items are mandatory, for an edited modelItem, All of the remaining items are
    // optional, and may or may not be included based on their included values in the properties included bits
    uint16_t packetContainsBits = properties.getChangedBits();
    if (!isNewModelItem) {
        memcpy(copyAt, &packetContainsBits, sizeof(packetContainsBits));
        copyAt += sizeof(packetContainsBits);
        sizeOut += sizeof(packetContainsBits);
    }

    // radius
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_RADIUS) == MODEL_PACKET_CONTAINS_RADIUS)) {
        float radius = properties.getRadius() / (float) TREE_SCALE;
        memcpy(copyAt, &radius, sizeof(radius));
        copyAt += sizeof(radius);
        sizeOut += sizeof(radius);
    }

    // position
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_POSITION) == MODEL_PACKET_CONTAINS_POSITION)) {
        glm::vec3 position = properties.getPosition() / (float)TREE_SCALE;
        memcpy(copyAt, &position, sizeof(position));
        copyAt += sizeof(position);
        sizeOut += sizeof(position);
    }

    // color
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_COLOR) == MODEL_PACKET_CONTAINS_COLOR)) {
        rgbColor color = { properties.getColor().red, properties.getColor().green, properties.getColor().blue };
        memcpy(copyAt, color, sizeof(color));
        copyAt += sizeof(color);
        sizeOut += sizeof(color);
    }

    // shoulDie
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_SHOULDDIE) == MODEL_PACKET_CONTAINS_SHOULDDIE)) {
        bool shouldDie = properties.getShouldDie();
        memcpy(copyAt, &shouldDie, sizeof(shouldDie));
        copyAt += sizeof(shouldDie);
        sizeOut += sizeof(shouldDie);
    }

    // modelURL
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_MODEL_URL) == MODEL_PACKET_CONTAINS_MODEL_URL)) {
        uint16_t urlLength = properties.getModelURL().size() + 1;
        memcpy(copyAt, &urlLength, sizeof(urlLength));
        copyAt += sizeof(urlLength);
        sizeOut += sizeof(urlLength);
        memcpy(copyAt, qPrintable(properties.getModelURL()), urlLength);
        copyAt += urlLength;
        sizeOut += urlLength;
    }

    // modelScale
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_MODEL_SCALE) == MODEL_PACKET_CONTAINS_MODEL_SCALE)) {
        float modelScale = properties.getModelScale();
        memcpy(copyAt, &modelScale, sizeof(modelScale));
        copyAt += sizeof(modelScale);
        sizeOut += sizeof(modelScale);
    }

    // modelTranslation
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_MODEL_TRANSLATION) == MODEL_PACKET_CONTAINS_MODEL_TRANSLATION)) {
        glm::vec3 modelTranslation = properties.getModelTranslation(); // should this be relative to TREE_SCALE??
        memcpy(copyAt, &modelTranslation, sizeof(modelTranslation));
        copyAt += sizeof(modelTranslation);
        sizeOut += sizeof(modelTranslation);
    }

    // modelRotation
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_MODEL_ROTATION) == MODEL_PACKET_CONTAINS_MODEL_ROTATION)) {
        int bytes = packOrientationQuatToBytes(copyAt, properties.getModelRotation());
        copyAt += bytes;
        sizeOut += bytes;
    }

    bool wantDebugging = false;
    if (wantDebugging) {
        qDebug("encodeModelItemEditMessageDetails()....");
        qDebug("ModelItem id  :%u", id.id);
        qDebug(" nextID:%u", _nextID);
    }

    // cleanup
    delete[] octcode;
    
    return success;
}

// adjust any internal timestamps to fix clock skew for this server
void ModelItem::adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew) {
    unsigned char* dataAt = codeColorBuffer;
    int octets = numberOfThreeBitSectionsInCode(dataAt);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);
    dataAt += lengthOfOctcode;

    // id
    uint32_t id;
    memcpy(&id, dataAt, sizeof(id));
    dataAt += sizeof(id);
    // special case for handling "new" modelItems
    if (id == NEW_MODEL) {
        // If this is a NEW_MODEL, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id
        dataAt += sizeof(uint32_t);
    }

    // lastEdited
    quint64 lastEditedInLocalTime;
    memcpy(&lastEditedInLocalTime, dataAt, sizeof(lastEditedInLocalTime));
    quint64 lastEditedInServerTime = lastEditedInLocalTime + clockSkew;
    memcpy(dataAt, &lastEditedInServerTime, sizeof(lastEditedInServerTime));
    const bool wantDebug = false;
    if (wantDebug) {
        qDebug("ModelItem::adjustEditPacketForClockSkew()...");
        qDebug() << "     lastEditedInLocalTime: " << lastEditedInLocalTime;
        qDebug() << "                 clockSkew: " << clockSkew;
        qDebug() << "    lastEditedInServerTime: " << lastEditedInServerTime;
    }
}

void ModelItem::update(const quint64& now) {
    _lastUpdated = now;
    setShouldDie(getShouldDie());
}

void ModelItem::copyChangedProperties(const ModelItem& other) {
    *this = other;
}

ModelItemProperties ModelItem::getProperties() const {
    ModelItemProperties properties;
    properties.copyFromModelItem(*this);
    return properties;
}

void ModelItem::setProperties(const ModelItemProperties& properties) {
    properties.copyToModelItem(*this);
}

ModelItemProperties::ModelItemProperties() :
    _position(0),
    _color(),
    _radius(MODEL_DEFAULT_RADIUS),
    _shouldDie(false),
    _modelURL(""),
    _modelScale(MODEL_DEFAULT_MODEL_SCALE),
    _modelTranslation(MODEL_DEFAULT_MODEL_TRANSLATION),
    _modelRotation(MODEL_DEFAULT_MODEL_ROTATION),

    _id(UNKNOWN_MODEL_ID),
    _idSet(false),
    _lastEdited(usecTimestampNow()),

    _positionChanged(false),
    _colorChanged(false),
    _radiusChanged(false),
    _shouldDieChanged(false),
    _modelURLChanged(false),
    _modelScaleChanged(false),
    _modelTranslationChanged(false),
    _modelRotationChanged(false),
    _defaultSettings(true)
{
}


uint16_t ModelItemProperties::getChangedBits() const {
    uint16_t changedBits = 0;
    if (_radiusChanged) {
        changedBits += MODEL_PACKET_CONTAINS_RADIUS;
    }

    if (_positionChanged) {
        changedBits += MODEL_PACKET_CONTAINS_POSITION;
    }

    if (_colorChanged) {
        changedBits += MODEL_PACKET_CONTAINS_COLOR;
    }

    if (_shouldDieChanged) {
        changedBits += MODEL_PACKET_CONTAINS_SHOULDDIE;
    }

    if (_modelURLChanged) {
        changedBits += MODEL_PACKET_CONTAINS_MODEL_URL;
    }

    if (_modelScaleChanged) {
        changedBits += MODEL_PACKET_CONTAINS_MODEL_SCALE;
    }

    if (_modelTranslationChanged) {
        changedBits += MODEL_PACKET_CONTAINS_MODEL_TRANSLATION;
    }

    if (_modelRotationChanged) {
        changedBits += MODEL_PACKET_CONTAINS_MODEL_ROTATION;
    }

    return changedBits;
}


QScriptValue ModelItemProperties::copyToScriptValue(QScriptEngine* engine) const {
    QScriptValue properties = engine->newObject();

    QScriptValue position = vec3toScriptValue(engine, _position);
    properties.setProperty("position", position);

    QScriptValue color = xColorToScriptValue(engine, _color);
    properties.setProperty("color", color);

    properties.setProperty("radius", _radius);

    properties.setProperty("shouldDie", _shouldDie);

    properties.setProperty("modelURL", _modelURL);

    properties.setProperty("modelScale", _modelScale);

    QScriptValue modelTranslation = vec3toScriptValue(engine, _modelTranslation);
    properties.setProperty("modelTranslation", modelTranslation);

    QScriptValue modelRotation = quatToScriptValue(engine, _modelRotation);
    properties.setProperty("modelRotation", modelRotation);


    if (_idSet) {
        properties.setProperty("id", _id);
        properties.setProperty("isKnownID", (_id != UNKNOWN_MODEL_ID));
    }

    return properties;
}

void ModelItemProperties::copyFromScriptValue(const QScriptValue &object) {

    QScriptValue position = object.property("position");
    if (position.isValid()) {
        QScriptValue x = position.property("x");
        QScriptValue y = position.property("y");
        QScriptValue z = position.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newPosition;
            newPosition.x = x.toVariant().toFloat();
            newPosition.y = y.toVariant().toFloat();
            newPosition.z = z.toVariant().toFloat();
            if (_defaultSettings || newPosition != _position) {
                _position = newPosition;
                _positionChanged = true;
            }
        }
    }

    QScriptValue color = object.property("color");
    if (color.isValid()) {
        QScriptValue red = color.property("red");
        QScriptValue green = color.property("green");
        QScriptValue blue = color.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            xColor newColor;
            newColor.red = red.toVariant().toInt();
            newColor.green = green.toVariant().toInt();
            newColor.blue = blue.toVariant().toInt();
            if (_defaultSettings || (newColor.red != _color.red ||
                newColor.green != _color.green ||
                newColor.blue != _color.blue)) {
                _color = newColor;
                _colorChanged = true;
            }
        }
    }

    QScriptValue radius = object.property("radius");
    if (radius.isValid()) {
        float newRadius;
        newRadius = radius.toVariant().toFloat();
        if (_defaultSettings || newRadius != _radius) {
            _radius = newRadius;
            _radiusChanged = true;
        }
    }

    QScriptValue shouldDie = object.property("shouldDie");
    if (shouldDie.isValid()) {
        bool newShouldDie;
        newShouldDie = shouldDie.toVariant().toBool();
        if (_defaultSettings || newShouldDie != _shouldDie) {
            _shouldDie = newShouldDie;
            _shouldDieChanged = true;
        }
    }

    QScriptValue modelURL = object.property("modelURL");
    if (modelURL.isValid()) {
        QString newModelURL;
        newModelURL = modelURL.toVariant().toString();
        if (_defaultSettings || newModelURL != _modelURL) {
            _modelURL = newModelURL;
            _modelURLChanged = true;
        }
    }

    QScriptValue modelScale = object.property("modelScale");
    if (modelScale.isValid()) {
        float newModelScale;
        newModelScale = modelScale.toVariant().toFloat();
        if (_defaultSettings || newModelScale != _modelScale) {
            _modelScale = newModelScale;
            _modelScaleChanged = true;
        }
    }

    QScriptValue modelTranslation = object.property("modelTranslation");
    if (modelTranslation.isValid()) {
        QScriptValue x = modelTranslation.property("x");
        QScriptValue y = modelTranslation.property("y");
        QScriptValue z = modelTranslation.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newModelTranslation;
            newModelTranslation.x = x.toVariant().toFloat();
            newModelTranslation.y = y.toVariant().toFloat();
            newModelTranslation.z = z.toVariant().toFloat();
            if (_defaultSettings || newModelTranslation != _modelTranslation) {
                _modelTranslation = newModelTranslation;
                _modelTranslationChanged = true;
            }
        }
    }

    
    QScriptValue modelRotation = object.property("modelRotation");
    if (modelRotation.isValid()) {
        QScriptValue x = modelRotation.property("x");
        QScriptValue y = modelRotation.property("y");
        QScriptValue z = modelRotation.property("z");
        QScriptValue w = modelRotation.property("w");
        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            glm::quat newModelRotation;
            newModelRotation.x = x.toVariant().toFloat();
            newModelRotation.y = y.toVariant().toFloat();
            newModelRotation.z = z.toVariant().toFloat();
            newModelRotation.w = w.toVariant().toFloat();
            if (_defaultSettings || newModelRotation != _modelRotation) {
                _modelRotation = newModelRotation;
                _modelRotationChanged = true;
            }
        }
    }

    _lastEdited = usecTimestampNow();
}

void ModelItemProperties::copyToModelItem(ModelItem& modelItem) const {
    bool somethingChanged = false;
    if (_positionChanged) {
        modelItem.setPosition(_position / (float) TREE_SCALE);
        somethingChanged = true;
    }

    if (_colorChanged) {
        modelItem.setColor(_color);
        somethingChanged = true;
    }

    if (_radiusChanged) {
        modelItem.setRadius(_radius / (float) TREE_SCALE);
        somethingChanged = true;
    }

    if (_shouldDieChanged) {
        modelItem.setShouldDie(_shouldDie);
        somethingChanged = true;
    }

    if (_modelURLChanged) {
        modelItem.setModelURL(_modelURL);
        somethingChanged = true;
    }

    if (_modelScaleChanged) {
        modelItem.setModelScale(_modelScale);
        somethingChanged = true;
    }
    
    if (_modelTranslationChanged) {
        modelItem.setModelTranslation(_modelTranslation);
        somethingChanged = true;
    }
    
    if (_modelRotationChanged) {
        modelItem.setModelRotation(_modelRotation);
        somethingChanged = true;
    }
    
    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - _lastEdited;
            qDebug() << "ModelItemProperties::copyToModelItem() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " _lastEdited=" << _lastEdited;
        }
        modelItem.setLastEdited(_lastEdited);
    }
}

void ModelItemProperties::copyFromModelItem(const ModelItem& modelItem) {
    _position = modelItem.getPosition() * (float) TREE_SCALE;
    _color = modelItem.getXColor();
    _radius = modelItem.getRadius() * (float) TREE_SCALE;
    _shouldDie = modelItem.getShouldDie();
    _modelURL = modelItem.getModelURL();
    _modelScale = modelItem.getModelScale();
    _modelTranslation = modelItem.getModelTranslation();
    _modelRotation = modelItem.getModelRotation();

    _id = modelItem.getID();
    _idSet = true;

    _positionChanged = false;
    _colorChanged = false;
    _radiusChanged = false;
    
    _shouldDieChanged = false;
    _modelURLChanged = false;
    _modelScaleChanged = false;
    _modelTranslationChanged = false;
    _modelRotationChanged = false;
    _defaultSettings = false;
}

QScriptValue ModelItemPropertiesToScriptValue(QScriptEngine* engine, const ModelItemProperties& properties) {
    return properties.copyToScriptValue(engine);
}

void ModelItemPropertiesFromScriptValue(const QScriptValue &object, ModelItemProperties& properties) {
    properties.copyFromScriptValue(object);
}


QScriptValue ModelItemIDtoScriptValue(QScriptEngine* engine, const ModelItemID& id) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("id", id.id);
    obj.setProperty("creatorTokenID", id.creatorTokenID);
    obj.setProperty("isKnownID", id.isKnownID);
    return obj;
}

void ModelItemIDfromScriptValue(const QScriptValue &object, ModelItemID& id) {
    id.id = object.property("id").toVariant().toUInt();
    id.creatorTokenID = object.property("creatorTokenID").toVariant().toUInt();
    id.isKnownID = object.property("isKnownID").toVariant().toBool();
}



