//
//  ModelItem.cpp
//  libraries/particles/src
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
//VoxelEditPacketSender* ModelItem::_voxelEditSender = NULL;
//ModelItemEditPacketSender* ModelItem::_particleEditSender = NULL;

// for locally created particles
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

    uint32_t particleID;
    memcpy(&particleID, dataAt, sizeof(particleID));
    dataAt += sizeof(particleID);

    // add our token to id mapping
    _tokenIDsToIDs[creatorTokenID] = particleID;
}

ModelItem::ModelItem() {
    rgbColor noColor = { 0, 0, 0 };
    init(glm::vec3(0,0,0), 0, noColor, glm::vec3(0,0,0),
            MODEL_DEFAULT_GRAVITY, MODEL_DEFAULT_DAMPING, MODEL_DEFAULT_LIFETIME, MODEL_NOT_IN_HAND, MODEL_DEFAULT_SCRIPT, NEW_MODEL);
}

ModelItem::ModelItem(const ModelItemID& particleID, const ModelItemProperties& properties) {
    _id = particleID.id;
    _creatorTokenID = particleID.creatorTokenID;

    // init values with defaults before calling setProperties
    uint64_t now = usecTimestampNow();
    _lastEdited = now;
    _lastUpdated = now;
    _created = now; // will get updated as appropriate in setAge()

    _position = glm::vec3(0,0,0);
    _radius = 0;
    _mass = 1.0f;
    rgbColor noColor = { 0, 0, 0 };
    memcpy(_color, noColor, sizeof(_color));
    _velocity = glm::vec3(0,0,0);
    _damping = MODEL_DEFAULT_DAMPING;
    _lifetime = MODEL_DEFAULT_LIFETIME;
    _gravity = MODEL_DEFAULT_GRAVITY;
    _script = MODEL_DEFAULT_SCRIPT;
    _inHand = MODEL_NOT_IN_HAND;
    _shouldDie = false;
    _modelURL = MODEL_DEFAULT_MODEL_URL;
    _modelTranslation = MODEL_DEFAULT_MODEL_TRANSLATION;
    _modelRotation = MODEL_DEFAULT_MODEL_ROTATION;
    _modelScale = MODEL_DEFAULT_MODEL_SCALE;
    
    setProperties(properties);
}


ModelItem::~ModelItem() {
}

void ModelItem::init(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity, glm::vec3 gravity,
                    float damping, float lifetime, bool inHand, QString updateScript, uint32_t id) {
    if (id == NEW_MODEL) {
        _id = _nextID;
        _nextID++;
    } else {
        _id = id;
    }
    quint64 now = usecTimestampNow();
    _lastEdited = now;
    _lastUpdated = now;
    _created = now; // will get updated as appropriate in setAge()

    _position = position;
    _radius = radius;
    _mass = 1.0f;
    memcpy(_color, color, sizeof(_color));
    _velocity = velocity;
    _damping = damping;
    _lifetime = lifetime;
    _gravity = gravity;
    _script = updateScript;
    _inHand = inHand;
    _shouldDie = false;
    _modelURL = MODEL_DEFAULT_MODEL_URL;
    _modelTranslation = MODEL_DEFAULT_MODEL_TRANSLATION;
    _modelRotation = MODEL_DEFAULT_MODEL_ROTATION;
    _modelScale = MODEL_DEFAULT_MODEL_SCALE;
}

void ModelItem::setMass(float value) {
    if (value > 0.0f) {
        _mass = value;
    }
}

bool ModelItem::appendModelData(OctreePacketData* packetData) const {

    bool success = packetData->appendValue(getID());

    //qDebug("ModelItem::appendModelData()... getID()=%d", getID());

    if (success) {
        success = packetData->appendValue(getAge());
    }
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
        success = packetData->appendValue(getVelocity());
    }
    if (success) {
        success = packetData->appendValue(getGravity());
    }
    if (success) {
        success = packetData->appendValue(getDamping());
    }
    if (success) {
        success = packetData->appendValue(getLifetime());
    }
    if (success) {
        success = packetData->appendValue(getInHand());
    }
    if (success) {
        success = packetData->appendValue(getShouldDie());
    }
    if (success) {
        uint16_t scriptLength = _script.size() + 1; // include NULL
        success = packetData->appendValue(scriptLength);
        if (success) {
            success = packetData->appendRawData((const unsigned char*)qPrintable(_script), scriptLength);
        }
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
                + sizeof(rgbColor) // color
                + sizeof(glm::vec3) // velocity
                + sizeof(glm::vec3) // gravity
                + sizeof(float) // damping
                + sizeof(float) // lifetime
                + sizeof(bool); // inhand
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

        // age
        float age;
        memcpy(&age, dataAt, sizeof(age));
        dataAt += sizeof(age);
        bytesRead += sizeof(age);
        setAge(age);

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

        // velocity
        memcpy(&_velocity, dataAt, sizeof(_velocity));
        dataAt += sizeof(_velocity);
        bytesRead += sizeof(_velocity);

        // gravity
        memcpy(&_gravity, dataAt, sizeof(_gravity));
        dataAt += sizeof(_gravity);
        bytesRead += sizeof(_gravity);

        // damping
        memcpy(&_damping, dataAt, sizeof(_damping));
        dataAt += sizeof(_damping);
        bytesRead += sizeof(_damping);

        // lifetime
        memcpy(&_lifetime, dataAt, sizeof(_lifetime));
        dataAt += sizeof(_lifetime);
        bytesRead += sizeof(_lifetime);

        // inHand
        memcpy(&_inHand, dataAt, sizeof(_inHand));
        dataAt += sizeof(_inHand);
        bytesRead += sizeof(_inHand);

        // shouldDie
        memcpy(&_shouldDie, dataAt, sizeof(_shouldDie));
        dataAt += sizeof(_shouldDie);
        bytesRead += sizeof(_shouldDie);

        // script
        uint16_t scriptLength;
        memcpy(&scriptLength, dataAt, sizeof(scriptLength));
        dataAt += sizeof(scriptLength);
        bytesRead += sizeof(scriptLength);
        QString tempString((const char*)dataAt);
        _script = tempString;
        dataAt += scriptLength;
        bytesRead += scriptLength;

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

    // special case for handling "new" particles
    if (isNewModelItem) {
        // If this is a NEW_MODEL, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id
        uint32_t creatorTokenID;
        memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
        dataAt += sizeof(creatorTokenID);
        processedBytes += sizeof(creatorTokenID);

        newModelItem.setCreatorTokenID(creatorTokenID);
        newModelItem._newlyCreated = true;
        newModelItem.setAge(0); // this guy is new!

    } else {
        // look up the existing particle
        const ModelItem* existingModelItem = tree->findModelByID(editID, true);

        // copy existing properties before over-writing with new properties
        if (existingModelItem) {
            newModelItem = *existingModelItem;
        } else {
            // the user attempted to edit a particle that doesn't exist
            qDebug() << "user attempted to edit a particle that doesn't exist...";
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

    // velocity
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_VELOCITY) == MODEL_PACKET_CONTAINS_VELOCITY)) {
        memcpy(&newModelItem._velocity, dataAt, sizeof(newModelItem._velocity));
        dataAt += sizeof(newModelItem._velocity);
        processedBytes += sizeof(newModelItem._velocity);
    }

    // gravity
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_GRAVITY) == MODEL_PACKET_CONTAINS_GRAVITY)) {
        memcpy(&newModelItem._gravity, dataAt, sizeof(newModelItem._gravity));
        dataAt += sizeof(newModelItem._gravity);
        processedBytes += sizeof(newModelItem._gravity);
    }

    // damping
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_DAMPING) == MODEL_PACKET_CONTAINS_DAMPING)) {
        memcpy(&newModelItem._damping, dataAt, sizeof(newModelItem._damping));
        dataAt += sizeof(newModelItem._damping);
        processedBytes += sizeof(newModelItem._damping);
    }

    // lifetime
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_LIFETIME) == MODEL_PACKET_CONTAINS_LIFETIME)) {
        memcpy(&newModelItem._lifetime, dataAt, sizeof(newModelItem._lifetime));
        dataAt += sizeof(newModelItem._lifetime);
        processedBytes += sizeof(newModelItem._lifetime);
    }

    // TODO: make inHand and shouldDie into single bits
    // inHand
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_INHAND) == MODEL_PACKET_CONTAINS_INHAND)) {
        memcpy(&newModelItem._inHand, dataAt, sizeof(newModelItem._inHand));
        dataAt += sizeof(newModelItem._inHand);
        processedBytes += sizeof(newModelItem._inHand);
    }

    // shouldDie
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_SHOULDDIE) == MODEL_PACKET_CONTAINS_SHOULDDIE)) {
        memcpy(&newModelItem._shouldDie, dataAt, sizeof(newModelItem._shouldDie));
        dataAt += sizeof(newModelItem._shouldDie);
        processedBytes += sizeof(newModelItem._shouldDie);
    }

    // script
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_SCRIPT) == MODEL_PACKET_CONTAINS_SCRIPT)) {
        uint16_t scriptLength;
        memcpy(&scriptLength, dataAt, sizeof(scriptLength));
        dataAt += sizeof(scriptLength);
        processedBytes += sizeof(scriptLength);
        QString tempString((const char*)dataAt);
        newModelItem._script = tempString;
        dataAt += scriptLength;
        processedBytes += scriptLength;
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
    qDebug(" age:%f", getAge());
    qDebug(" edited ago:%f", getEditedAgo());
    qDebug(" should die:%s", debug::valueOf(getShouldDie()));
    qDebug(" position:%f,%f,%f", _position.x, _position.y, _position.z);
    qDebug(" radius:%f", getRadius());
    qDebug(" velocity:%f,%f,%f", _velocity.x, _velocity.y, _velocity.z);
    qDebug(" gravity:%f,%f,%f", _gravity.x, _gravity.y, _gravity.z);
    qDebug(" color:%d,%d,%d", _color[0], _color[1], _color[2]);
}

bool ModelItem::encodeModelEditMessageDetails(PacketType command, ModelItemID id, const ModelItemProperties& properties,
        unsigned char* bufferOut, int sizeIn, int& sizeOut) {

    bool success = true; // assume the best
    unsigned char* copyAt = bufferOut;
    sizeOut = 0;

    // get the octal code for the particle

    // this could be a problem if the caller doesn't include position....
    glm::vec3 rootPosition(0);
    float rootScale = 0.5f;
    unsigned char* octcode = pointToOctalCode(rootPosition.x, rootPosition.y, rootPosition.z, rootScale);

    // TODO: Consider this old code... including the correct octree for where the particle will go matters for 
    // particle servers with different jurisdictions, but for now, we'll send everything to the root, since the 
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

    // special case for handling "new" particles
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
    
    // For new particles, all remaining items are mandatory, for an edited particle, All of the remaining items are
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

    // velocity
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_VELOCITY) == MODEL_PACKET_CONTAINS_VELOCITY)) {
        glm::vec3 velocity = properties.getVelocity() / (float)TREE_SCALE;
        memcpy(copyAt, &velocity, sizeof(velocity));
        copyAt += sizeof(velocity);
        sizeOut += sizeof(velocity);
    }

    // gravity
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_GRAVITY) == MODEL_PACKET_CONTAINS_GRAVITY)) {
        glm::vec3 gravity = properties.getGravity() / (float)TREE_SCALE;
        memcpy(copyAt, &gravity, sizeof(gravity));
        copyAt += sizeof(gravity);
        sizeOut += sizeof(gravity);
    }

    // damping
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_DAMPING) == MODEL_PACKET_CONTAINS_DAMPING)) {
        float damping = properties.getDamping();
        memcpy(copyAt, &damping, sizeof(damping));
        copyAt += sizeof(damping);
        sizeOut += sizeof(damping);
    }

    // lifetime
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_LIFETIME) == MODEL_PACKET_CONTAINS_LIFETIME)) {
        float lifetime = properties.getLifetime();
        memcpy(copyAt, &lifetime, sizeof(lifetime));
        copyAt += sizeof(lifetime);
        sizeOut += sizeof(lifetime);
    }

    // inHand
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_INHAND) == MODEL_PACKET_CONTAINS_INHAND)) {
        bool inHand = properties.getInHand();
        memcpy(copyAt, &inHand, sizeof(inHand));
        copyAt += sizeof(inHand);
        sizeOut += sizeof(inHand);
    }

    // shoulDie
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_SHOULDDIE) == MODEL_PACKET_CONTAINS_SHOULDDIE)) {
        bool shouldDie = properties.getShouldDie();
        memcpy(copyAt, &shouldDie, sizeof(shouldDie));
        copyAt += sizeof(shouldDie);
        sizeOut += sizeof(shouldDie);
    }

    // script
    if (isNewModelItem || ((packetContainsBits & MODEL_PACKET_CONTAINS_SCRIPT) == MODEL_PACKET_CONTAINS_SCRIPT)) {
        uint16_t scriptLength = properties.getScript().size() + 1;
        memcpy(copyAt, &scriptLength, sizeof(scriptLength));
        copyAt += sizeof(scriptLength);
        sizeOut += sizeof(scriptLength);
        memcpy(copyAt, qPrintable(properties.getScript()), scriptLength);
        copyAt += scriptLength;
        sizeOut += scriptLength;
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
    // special case for handling "new" particles
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

// HALTING_* params are determined using expected acceleration of gravity over some timescale.  
// This is a HACK for particles that bounce in a 1.0 gravitational field and should eventually be made more universal.
const float HALTING_MODEL_PERIOD = 0.0167f;  // ~1/60th of a second
const float HALTING_MODEL_SPEED = 9.8 * HALTING_MODEL_PERIOD / (float)(TREE_SCALE);

void ModelItem::applyHardCollision(const CollisionInfo& collisionInfo) {
    //
    //  Update the particle in response to a hard collision.  Position will be reset exactly
    //  to outside the colliding surface.  Velocity will be modified according to elasticity.
    //
    //  if elasticity = 0.0, collision is inelastic (vel normal to collision is lost)
    //  if elasticity = 1.0, collision is 100% elastic.
    //
    glm::vec3 position = getPosition();
    glm::vec3 velocity = getVelocity();

    const float EPSILON = 0.0f;
    glm::vec3 relativeVelocity = collisionInfo._addedVelocity - velocity;
    float velocityDotPenetration = glm::dot(relativeVelocity, collisionInfo._penetration);
    if (velocityDotPenetration < EPSILON) {
        // particle is moving into collision surface
        //
        // TODO: do something smarter here by comparing the mass of the particle vs that of the other thing 
        // (other's mass could be stored in the Collision Info).  The smaller mass should surrender more 
        // position offset and should slave more to the other's velocity in the static-friction case.
        position -= collisionInfo._penetration;

        if (glm::length(relativeVelocity) < HALTING_MODEL_SPEED) {
            // static friction kicks in and particle moves with colliding object
            velocity = collisionInfo._addedVelocity;
        } else {
            glm::vec3 direction = glm::normalize(collisionInfo._penetration);
            velocity += glm::dot(relativeVelocity, direction) * (1.0f + collisionInfo._elasticity) * direction;    // dynamic reflection
            velocity += glm::clamp(collisionInfo._damping, 0.0f, 1.0f) * (relativeVelocity - glm::dot(relativeVelocity, direction) * direction);   // dynamic friction
        }
    }

    // change the local particle too...
    setPosition(position);
    setVelocity(velocity);
}

void ModelItem::update(const quint64& now) {
    float timeElapsed = (float)(now - _lastUpdated) / (float)(USECS_PER_SECOND);
    _lastUpdated = now;

    // calculate our default shouldDie state... then allow script to change it if it wants...
    bool isInHand = getInHand();
    bool shouldDie = (getAge() > getLifetime()) || getShouldDie();
    setShouldDie(shouldDie);

    //executeUpdateScripts(); // allow the javascript to alter our state

    // If the ball is in hand, it doesn't move or have gravity effect it
    if (!isInHand) {
        _position += _velocity * timeElapsed;

        // handle bounces off the ground...
        if (_position.y <= 0) {
            _velocity = _velocity * glm::vec3(1,-1,1);
            _position.y = 0;
        }

        // handle gravity....
        _velocity += _gravity * timeElapsed;

        // handle damping
        glm::vec3 dampingResistance = _velocity * _damping;
        _velocity -= dampingResistance * timeElapsed;
        //qDebug("applying damping to ModelItem timeElapsed=%f",timeElapsed);
    }
}

void ModelItem::setAge(float age) {
    quint64 ageInUsecs = age * USECS_PER_SECOND;
    _created = usecTimestampNow() - ageInUsecs;
}

void ModelItem::copyChangedProperties(const ModelItem& other) {
    float age = getAge();
    *this = other;
    setAge(age);
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
    _velocity(0),
    _gravity(MODEL_DEFAULT_GRAVITY),
    _damping(MODEL_DEFAULT_DAMPING),
    _lifetime(MODEL_DEFAULT_LIFETIME),
    _script(""),
    _inHand(false),
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
    _velocityChanged(false),
    _gravityChanged(false),
    _dampingChanged(false),
    _lifetimeChanged(false),
    _scriptChanged(false),
    _inHandChanged(false),
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

    if (_velocityChanged) {
        changedBits += MODEL_PACKET_CONTAINS_VELOCITY;
    }

    if (_gravityChanged) {
        changedBits += MODEL_PACKET_CONTAINS_GRAVITY;
    }

    if (_dampingChanged) {
        changedBits += MODEL_PACKET_CONTAINS_DAMPING;
    }

    if (_lifetimeChanged) {
        changedBits += MODEL_PACKET_CONTAINS_LIFETIME;
    }

    if (_inHandChanged) {
        changedBits += MODEL_PACKET_CONTAINS_INHAND;
    }

    if (_scriptChanged) {
        changedBits += MODEL_PACKET_CONTAINS_SCRIPT;
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

    QScriptValue velocity = vec3toScriptValue(engine, _velocity);
    properties.setProperty("velocity", velocity);

    QScriptValue gravity = vec3toScriptValue(engine, _gravity);
    properties.setProperty("gravity", gravity);

    properties.setProperty("damping", _damping);
    properties.setProperty("lifetime", _lifetime);
    properties.setProperty("script", _script);
    properties.setProperty("inHand", _inHand);
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

    QScriptValue velocity = object.property("velocity");
    if (velocity.isValid()) {
        QScriptValue x = velocity.property("x");
        QScriptValue y = velocity.property("y");
        QScriptValue z = velocity.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newVelocity;
            newVelocity.x = x.toVariant().toFloat();
            newVelocity.y = y.toVariant().toFloat();
            newVelocity.z = z.toVariant().toFloat();
            if (_defaultSettings || newVelocity != _velocity) {
                _velocity = newVelocity;
                _velocityChanged = true;
            }
        }
    }

    QScriptValue gravity = object.property("gravity");
    if (gravity.isValid()) {
        QScriptValue x = gravity.property("x");
        QScriptValue y = gravity.property("y");
        QScriptValue z = gravity.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newGravity;
            newGravity.x = x.toVariant().toFloat();
            newGravity.y = y.toVariant().toFloat();
            newGravity.z = z.toVariant().toFloat();
            if (_defaultSettings || newGravity != _gravity) {
                _gravity = newGravity;
                _gravityChanged = true;
            }
        }
    }

    QScriptValue damping = object.property("damping");
    if (damping.isValid()) {
        float newDamping;
        newDamping = damping.toVariant().toFloat();
        if (_defaultSettings || newDamping != _damping) {
            _damping = newDamping;
            _dampingChanged = true;
        }
    }

    QScriptValue lifetime = object.property("lifetime");
    if (lifetime.isValid()) {
        float newLifetime;
        newLifetime = lifetime.toVariant().toFloat();
        if (_defaultSettings || newLifetime != _lifetime) {
            _lifetime = newLifetime;
            _lifetimeChanged = true;
        }
    }

    QScriptValue script = object.property("script");
    if (script.isValid()) {
        QString newScript;
        newScript = script.toVariant().toString();
        if (_defaultSettings || newScript != _script) {
            _script = newScript;
            _scriptChanged = true;
        }
    }

    QScriptValue inHand = object.property("inHand");
    if (inHand.isValid()) {
        bool newInHand;
        newInHand = inHand.toVariant().toBool();
        if (_defaultSettings || newInHand != _inHand) {
            _inHand = newInHand;
            _inHandChanged = true;
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

void ModelItemProperties::copyToModelItem(ModelItem& particle) const {
    bool somethingChanged = false;
    if (_positionChanged) {
        particle.setPosition(_position / (float) TREE_SCALE);
        somethingChanged = true;
    }

    if (_colorChanged) {
        particle.setColor(_color);
        somethingChanged = true;
    }

    if (_radiusChanged) {
        particle.setRadius(_radius / (float) TREE_SCALE);
        somethingChanged = true;
    }

    if (_velocityChanged) {
        particle.setVelocity(_velocity / (float) TREE_SCALE);
        somethingChanged = true;
    }

    if (_gravityChanged) {
        particle.setGravity(_gravity / (float) TREE_SCALE);
        somethingChanged = true;
    }

    if (_dampingChanged) {
        particle.setDamping(_damping);
        somethingChanged = true;
    }

    if (_lifetimeChanged) {
        particle.setLifetime(_lifetime);
        somethingChanged = true;
    }

    if (_scriptChanged) {
        particle.setScript(_script);
        somethingChanged = true;
    }

    if (_inHandChanged) {
        particle.setInHand(_inHand);
        somethingChanged = true;
    }

    if (_shouldDieChanged) {
        particle.setShouldDie(_shouldDie);
        somethingChanged = true;
    }

    if (_modelURLChanged) {
        particle.setModelURL(_modelURL);
        somethingChanged = true;
    }

    if (_modelScaleChanged) {
        particle.setModelScale(_modelScale);
        somethingChanged = true;
    }
    
    if (_modelTranslationChanged) {
        particle.setModelTranslation(_modelTranslation);
        somethingChanged = true;
    }
    
    if (_modelRotationChanged) {
        particle.setModelRotation(_modelRotation);
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
        particle.setLastEdited(_lastEdited);
    }
}

void ModelItemProperties::copyFromModelItem(const ModelItem& particle) {
    _position = particle.getPosition() * (float) TREE_SCALE;
    _color = particle.getXColor();
    _radius = particle.getRadius() * (float) TREE_SCALE;
    _velocity = particle.getVelocity() * (float) TREE_SCALE;
    _gravity = particle.getGravity() * (float) TREE_SCALE;
    _damping = particle.getDamping();
    _lifetime = particle.getLifetime();
    _script = particle.getScript();
    _inHand = particle.getInHand();
    _shouldDie = particle.getShouldDie();
    _modelURL = particle.getModelURL();
    _modelScale = particle.getModelScale();
    _modelTranslation = particle.getModelTranslation();
    _modelRotation = particle.getModelRotation();

    _id = particle.getID();
    _idSet = true;

    _positionChanged = false;
    _colorChanged = false;
    _radiusChanged = false;
    _velocityChanged = false;
    _gravityChanged = false;
    _dampingChanged = false;
    _lifetimeChanged = false;
    _scriptChanged = false;
    _inHandChanged = false;
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


