//
//  Particle.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#include <QtCore/QObject>

#include <Octree.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // usecTimestampNow()
#include <VoxelsScriptingInterface.h>

// This is not ideal, but adding script-engine as a linked library, will cause a circular reference
// I'm open to other potential solutions. Could we change cmake to allow libraries to reference each others
// headers, but not link to each other, this is essentially what this construct is doing, but would be
// better to add includes to the include path, but not link
#include "../../script-engine/src/ScriptEngine.h"

#include "ParticlesScriptingInterface.h"
#include "Particle.h"
#include "ParticleTree.h"

uint32_t Particle::_nextID = 0;
VoxelEditPacketSender* Particle::_voxelEditSender = NULL;
ParticleEditPacketSender* Particle::_particleEditSender = NULL;

// for locally created particles
std::map<uint32_t,uint32_t> Particle::_tokenIDsToIDs;
uint32_t Particle::_nextCreatorTokenID = 0;

uint32_t Particle::getIDfromCreatorTokenID(uint32_t creatorTokenID) {
    if (_tokenIDsToIDs.find(creatorTokenID) != _tokenIDsToIDs.end()) {
        return _tokenIDsToIDs[creatorTokenID];
    }
    return UNKNOWN_PARTICLE_ID;
}

uint32_t Particle::getNextCreatorTokenID() {
    uint32_t creatorTokenID = _nextCreatorTokenID;
    _nextCreatorTokenID++;
    return creatorTokenID;
}

void Particle::handleAddParticleResponse(unsigned char* packetData , int packetLength) {
    unsigned char* dataAt = packetData;
    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    dataAt += numBytesPacketHeader;

    uint32_t creatorTokenID;
    memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
    dataAt += sizeof(creatorTokenID);

    uint32_t particleID;
    memcpy(&particleID, dataAt, sizeof(particleID));
    dataAt += sizeof(particleID);

    //qDebug() << "handleAddParticleResponse()... particleID=" << particleID << " creatorTokenID=" << creatorTokenID;

    // add our token to id mapping
    _tokenIDsToIDs[creatorTokenID] = particleID;
}



Particle::Particle(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity, glm::vec3 gravity,
                    float damping, float lifetime, bool inHand, QString updateScript, uint32_t id) {

    init(position, radius, color, velocity, gravity, damping, lifetime, inHand, updateScript, id);
}

Particle::Particle() {
    rgbColor noColor = { 0, 0, 0 };
    init(glm::vec3(0,0,0), 0, noColor, glm::vec3(0,0,0),
            DEFAULT_GRAVITY, DEFAULT_DAMPING, DEFAULT_LIFETIME, NOT_IN_HAND, DEFAULT_SCRIPT, NEW_PARTICLE);
}

Particle::~Particle() {
}

void Particle::init(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity, glm::vec3 gravity,
                    float damping, float lifetime, bool inHand, QString updateScript, uint32_t id) {
    if (id == NEW_PARTICLE) {
        _id = _nextID;
        _nextID++;
        //qDebug() << "Particle::init()... assigning new id... _id=" << _id;
    } else {
        _id = id;
        //qDebug() << "Particle::init()... assigning id from init... _id=" << _id;
    }
    uint64_t now = usecTimestampNow();
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
}

void Particle::setMass(float value) {
    if (value > 0.0f) {
        _mass = value;
    }
}

bool Particle::appendParticleData(OctreePacketData* packetData) const {

    bool success = packetData->appendValue(getID());

    //printf("Particle::appendParticleData()... getID()=%d\n", getID());

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
    return success;
}

int Particle::expectedBytes() {
    int expectedBytes = sizeof(uint32_t) // id
                + sizeof(float) // age
                + sizeof(uint64_t) // last updated
                + sizeof(uint64_t) // lasted edited
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

int Particle::expectedEditMessageBytes() {
    int expectedBytes = sizeof(uint32_t) // id
                + sizeof(uint64_t) // lasted edited
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

int Particle::readParticleDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {
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

        //printf("Particle::readParticleDataFromBuffer()... "); debugDump();
    }
    return bytesRead;
}


Particle Particle::fromEditPacket(unsigned char* data, int length, int& processedBytes, ParticleTree* tree) {

    //qDebug() << "Particle::fromEditPacket() length=" << length;

    Particle newParticle; // id and _lastUpdated will get set here...
    unsigned char* dataAt = data;
    processedBytes = 0;

    // the first part of the data is our octcode...
    int octets = numberOfThreeBitSectionsInCode(data);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);

    //qDebug() << "Particle::fromEditPacket() lengthOfOctcode=" << lengthOfOctcode;
    //printOctalCode(data);

    // we don't actually do anything with this octcode...
    dataAt += lengthOfOctcode;
    processedBytes += lengthOfOctcode;

    // id
    uint32_t editID;
    memcpy(&editID, dataAt, sizeof(editID));
    dataAt += sizeof(editID);
    processedBytes += sizeof(editID);

    //qDebug() << "editID:" << editID;

    bool isNewParticle = (editID == NEW_PARTICLE);

    // special case for handling "new" particles
    if (isNewParticle) {
        //qDebug() << "editID == NEW_PARTICLE";
        
        // If this is a NEW_PARTICLE, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id
        uint32_t creatorTokenID;
        memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
        dataAt += sizeof(creatorTokenID);
        processedBytes += sizeof(creatorTokenID);

        //qDebug() << "creatorTokenID:" << creatorTokenID;

        newParticle.setCreatorTokenID(creatorTokenID);
        newParticle._newlyCreated = true;

        newParticle.setAge(0); // this guy is new!

    } else {
        // look up the existing particle
        const Particle* existingParticle = tree->findParticleByID(editID, true);

        // copy existing properties before over-writing with new properties
        if (existingParticle) {
            newParticle = *existingParticle;
            //qDebug() << "newParticle = *existingParticle... calling debugDump()...";
            //existingParticle->debugDump();
        }

        newParticle._id = editID;
        newParticle._newlyCreated = false;


    }

    // lastEdited
    memcpy(&newParticle._lastEdited, dataAt, sizeof(newParticle._lastEdited));
    dataAt += sizeof(newParticle._lastEdited);
    processedBytes += sizeof(newParticle._lastEdited);

    // All of the remaining items are optional, and may or may not be included based on their included values in the
    // properties included bits
    uint16_t packetContainsBits = 0;
    if (!isNewParticle) {
        memcpy(&packetContainsBits, dataAt, sizeof(packetContainsBits));
        dataAt += sizeof(packetContainsBits);
        processedBytes += sizeof(packetContainsBits);
        //qDebug() << "packetContainsBits:" << packetContainsBits;
    }


    // radius
    if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_RADIUS) == PACKET_CONTAINS_RADIUS)) {
        memcpy(&newParticle._radius, dataAt, sizeof(newParticle._radius));
        dataAt += sizeof(newParticle._radius);
        processedBytes += sizeof(newParticle._radius);
    }

    // position
    if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_POSITION) == PACKET_CONTAINS_POSITION)) {
        memcpy(&newParticle._position, dataAt, sizeof(newParticle._position));
        dataAt += sizeof(newParticle._position);
        processedBytes += sizeof(newParticle._position);
    }

    // color
    if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_COLOR) == PACKET_CONTAINS_COLOR)) {
        memcpy(newParticle._color, dataAt, sizeof(newParticle._color));
        dataAt += sizeof(newParticle._color);
        processedBytes += sizeof(newParticle._color);
    }

    // velocity
    if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_VELOCITY) == PACKET_CONTAINS_VELOCITY)) {
        memcpy(&newParticle._velocity, dataAt, sizeof(newParticle._velocity));
        dataAt += sizeof(newParticle._velocity);
        processedBytes += sizeof(newParticle._velocity);
    }

    // gravity
    if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_GRAVITY) == PACKET_CONTAINS_GRAVITY)) {
        memcpy(&newParticle._gravity, dataAt, sizeof(newParticle._gravity));
        dataAt += sizeof(newParticle._gravity);
        processedBytes += sizeof(newParticle._gravity);
    }

    // damping
    if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_DAMPING) == PACKET_CONTAINS_DAMPING)) {
        memcpy(&newParticle._damping, dataAt, sizeof(newParticle._damping));
        dataAt += sizeof(newParticle._damping);
        processedBytes += sizeof(newParticle._damping);
    }

    // lifetime
    if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_LIFETIME) == PACKET_CONTAINS_LIFETIME)) {
        memcpy(&newParticle._lifetime, dataAt, sizeof(newParticle._lifetime));
        dataAt += sizeof(newParticle._lifetime);
        processedBytes += sizeof(newParticle._lifetime);
    }

    // TODO: make inHand and shouldDie into single bits
    // inHand
    if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_INHAND) == PACKET_CONTAINS_INHAND)) {
        memcpy(&newParticle._inHand, dataAt, sizeof(newParticle._inHand));
        dataAt += sizeof(newParticle._inHand);
        processedBytes += sizeof(newParticle._inHand);
    }

    // shouldDie
    if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_SHOULDDIE) == PACKET_CONTAINS_SHOULDDIE)) {
        memcpy(&newParticle._shouldDie, dataAt, sizeof(newParticle._shouldDie));
        dataAt += sizeof(newParticle._shouldDie);
        processedBytes += sizeof(newParticle._shouldDie);
    }

    // script
    if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_SCRIPT) == PACKET_CONTAINS_SCRIPT)) {
        uint16_t scriptLength;
        memcpy(&scriptLength, dataAt, sizeof(scriptLength));
        dataAt += sizeof(scriptLength);
        processedBytes += sizeof(scriptLength);
        QString tempString((const char*)dataAt);
        newParticle._script = tempString;
        dataAt += scriptLength;
        processedBytes += scriptLength;
    }

    const bool wantDebugging = false;
    if (wantDebugging) {
        qDebug("Particle::fromEditPacket()...");
        qDebug() << "   Particle id in packet:" << editID;
        //qDebug() << "    position: " << newParticle._position;
        newParticle.debugDump();
    }

    return newParticle;
}

void Particle::debugDump() const {
    printf("Particle id  :%u\n", _id);
    printf(" age:%f\n", getAge());
    printf(" edited ago:%f\n", getEditedAgo());
    printf(" should die:%s\n", debug::valueOf(getShouldDie()));
    printf(" position:%f,%f,%f\n", _position.x, _position.y, _position.z);
    printf(" radius:%f\n", getRadius());
    printf(" velocity:%f,%f,%f\n", _velocity.x, _velocity.y, _velocity.z);
    printf(" gravity:%f,%f,%f\n", _gravity.x, _gravity.y, _gravity.z);
    printf(" color:%d,%d,%d\n", _color[0], _color[1], _color[2]);
}

bool Particle::encodeParticleEditMessageDetails(PACKET_TYPE command, ParticleID id, const ParticleProperties& properties,
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
    int lenfthOfEditData = lengthOfOctcode + expectedEditMessageBytes();

    // make sure we have room to copy this particle
    if (sizeOut + lenfthOfEditData > sizeIn) {
        success = false;
    } else {
        // add it to our message
        memcpy(copyAt, octcode, lengthOfOctcode);
        copyAt += lengthOfOctcode;
        sizeOut += lengthOfOctcode;

        // Now add our edit content details...
        bool isNewParticle = (id.id == NEW_PARTICLE);

        // id
        memcpy(copyAt, &id.id, sizeof(id.id));
        copyAt += sizeof(id.id);
        sizeOut += sizeof(id.id);

        // special case for handling "new" particles
        if (isNewParticle) {
            // If this is a NEW_PARTICLE, then we assume that there's an additional uint32_t creatorToken, that
            // we want to send back to the creator as an map to the actual id
            memcpy(copyAt, &id.creatorTokenID, sizeof(id.creatorTokenID));
            copyAt += sizeof(id.creatorTokenID);
            sizeOut += sizeof(id.creatorTokenID);
        }

        // lastEdited
        uint64_t lastEdited = properties.getLastEdited();
        memcpy(copyAt, &lastEdited, sizeof(lastEdited));
        copyAt += sizeof(lastEdited);
        sizeOut += sizeof(lastEdited);

        // For new particles, all remaining items are mandatory, for an edited particle, All of the remaining items are
        // optional, and may or may not be included based on their included values in the properties included bits
        uint16_t packetContainsBits = properties.getChangedBits();
        if (!isNewParticle) {
            memcpy(copyAt, &packetContainsBits, sizeof(packetContainsBits));
            copyAt += sizeof(packetContainsBits);
            sizeOut += sizeof(packetContainsBits);
        }

        // radius
        if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_RADIUS) == PACKET_CONTAINS_RADIUS)) {
            float radius = properties.getRadius() / (float) TREE_SCALE;
            memcpy(copyAt, &radius, sizeof(radius));
            copyAt += sizeof(radius);
            sizeOut += sizeof(radius);
        }

        // position
        if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_POSITION) == PACKET_CONTAINS_POSITION)) {
            glm::vec3 position = properties.getPosition() / (float)TREE_SCALE;
            memcpy(copyAt, &position, sizeof(position));
            copyAt += sizeof(position);
            sizeOut += sizeof(position);
        }

        // color
        if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_COLOR) == PACKET_CONTAINS_COLOR)) {
            rgbColor color = { properties.getColor().red, properties.getColor().green, properties.getColor().blue };
            memcpy(copyAt, color, sizeof(color));
            copyAt += sizeof(color);
            sizeOut += sizeof(color);
        }

        // velocity
        if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_VELOCITY) == PACKET_CONTAINS_VELOCITY)) {
            glm::vec3 velocity = properties.getVelocity() / (float)TREE_SCALE;
            memcpy(copyAt, &velocity, sizeof(velocity));
            copyAt += sizeof(velocity);
            sizeOut += sizeof(velocity);
        }

        // gravity
        if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_GRAVITY) == PACKET_CONTAINS_GRAVITY)) {
            glm::vec3 gravity = properties.getGravity() / (float)TREE_SCALE;
            memcpy(copyAt, &gravity, sizeof(gravity));
            copyAt += sizeof(gravity);
            sizeOut += sizeof(gravity);
        }

        // damping
        if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_DAMPING) == PACKET_CONTAINS_DAMPING)) {
            float damping = properties.getDamping();
            memcpy(copyAt, &damping, sizeof(damping));
            copyAt += sizeof(damping);
            sizeOut += sizeof(damping);
        }

        // lifetime
        if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_LIFETIME) == PACKET_CONTAINS_LIFETIME)) {
            float lifetime = properties.getLifetime();
            memcpy(copyAt, &lifetime, sizeof(lifetime));
            copyAt += sizeof(lifetime);
            sizeOut += sizeof(lifetime);
        }

        // inHand
        if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_INHAND) == PACKET_CONTAINS_INHAND)) {
            bool inHand = properties.getInHand();
            memcpy(copyAt, &inHand, sizeof(inHand));
            copyAt += sizeof(inHand);
            sizeOut += sizeof(inHand);
        }

        // shoulDie
        if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_SHOULDDIE) == PACKET_CONTAINS_SHOULDDIE)) {
            bool shouldDie = properties.getShouldDie();
            memcpy(copyAt, &shouldDie, sizeof(shouldDie));
            copyAt += sizeof(shouldDie);
            sizeOut += sizeof(shouldDie);
        }

        // script
        if (isNewParticle || ((packetContainsBits & PACKET_CONTAINS_SCRIPT) == PACKET_CONTAINS_SCRIPT)) {
            uint16_t scriptLength = properties.getScript().size() + 1;
            memcpy(copyAt, &scriptLength, sizeof(scriptLength));
            copyAt += sizeof(scriptLength);
            sizeOut += sizeof(scriptLength);
            memcpy(copyAt, qPrintable(properties.getScript()), scriptLength);
            copyAt += scriptLength;
            sizeOut += scriptLength;
        }

        bool wantDebugging = false;
        if (wantDebugging) {
            printf("encodeParticleEditMessageDetails()....\n");
            printf("Particle id  :%u\n", id.id);
            printf(" nextID:%u\n", _nextID);
        }
    }

    // cleanup
    delete[] octcode;
    
    //qDebug() << "encoding... sizeOut:" << sizeOut;

    return success;
}

// adjust any internal timestamps to fix clock skew for this server
void Particle::adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew) {
    unsigned char* dataAt = codeColorBuffer;
    int octets = numberOfThreeBitSectionsInCode(dataAt);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);
    dataAt += lengthOfOctcode;

    // id
    uint32_t id;
    memcpy(&id, dataAt, sizeof(id));
    dataAt += sizeof(id);
    // special case for handling "new" particles
    if (id == NEW_PARTICLE) {
        // If this is a NEW_PARTICLE, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id
        dataAt += sizeof(uint32_t);
    }

    // lastEdited
    uint64_t lastEditedInLocalTime;
    memcpy(&lastEditedInLocalTime, dataAt, sizeof(lastEditedInLocalTime));
    uint64_t lastEditedInServerTime = lastEditedInLocalTime + clockSkew;
    memcpy(dataAt, &lastEditedInServerTime, sizeof(lastEditedInServerTime));
    const bool wantDebug = false;
    if (wantDebug) {
        qDebug("Particle::adjustEditPacketForClockSkew()...");
        qDebug() << "     lastEditedInLocalTime: " << lastEditedInLocalTime;
        qDebug() << "                 clockSkew: " << clockSkew;
        qDebug() << "    lastEditedInServerTime: " << lastEditedInServerTime;
    }
}

// MIN_VALID_SPEED is obtained by computing speed gained at one gravity during the shortest expected frame period
const float MIN_EXPECTED_FRAME_PERIOD = 0.005f;  // 1/200th of a second
const float MIN_VALID_SPEED = 9.8 * MIN_EXPECTED_FRAME_PERIOD / (float)(TREE_SCALE);

void Particle::update(const uint64_t& now) {
    float timeElapsed = (float)(now - _lastUpdated) / (float)(USECS_PER_SECOND);
    _lastUpdated = now;

    // calculate our default shouldDie state... then allow script to change it if it wants...
    float speed = glm::length(_velocity);
    bool isStopped = (speed < MIN_VALID_SPEED);
    const uint64_t REALLY_OLD = 30 * USECS_PER_SECOND; // 30 seconds
    bool isReallyOld = ((now - _created) > REALLY_OLD);
    bool isInHand = getInHand();
    bool shouldDie = getShouldDie() || (!isInHand && isStopped && isReallyOld);
    setShouldDie(shouldDie);

    runUpdateScript(); // allow the javascript to alter our state

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
        //printf("applying damping to Particle timeElapsed=%f\n",timeElapsed);
    }
}

void Particle::runUpdateScript() {
    if (!_script.isEmpty()) {
        ScriptEngine engine(_script); // no menu or controller interface...

        if (_voxelEditSender) {
            engine.getVoxelsScriptingInterface()->setPacketSender(_voxelEditSender);
        }
        if (_particleEditSender) {
            engine.getParticlesScriptingInterface()->setPacketSender(_particleEditSender);
        }

        // Add the Particle object
        ParticleScriptObject particleScriptable(this);
        engine.registerGlobalObject("Particle", &particleScriptable);

        // init and evaluate the script, but return so we can emit the collision
        engine.evaluate();

        particleScriptable.emitUpdate();

        // it seems like we may need to send out particle edits if the state of our particle was changed.

        if (_voxelEditSender) {
            _voxelEditSender->releaseQueuedMessages();
        }
        if (_particleEditSender) {
            _particleEditSender->releaseQueuedMessages();
        }
    }
}

void Particle::collisionWithParticle(Particle* other) {
    if (!_script.isEmpty()) {
        ScriptEngine engine(_script); // no menu or controller interface...

        if (_voxelEditSender) {
            engine.getVoxelsScriptingInterface()->setPacketSender(_voxelEditSender);
        }
        if (_particleEditSender) {
            engine.getParticlesScriptingInterface()->setPacketSender(_particleEditSender);
        }

        // Add the Particle object
        ParticleScriptObject particleScriptable(this);
        engine.registerGlobalObject("Particle", &particleScriptable);

        // init and evaluate the script, but return so we can emit the collision
        engine.evaluate();

        ParticleScriptObject otherParticleScriptable(other);
        particleScriptable.emitCollisionWithParticle(&otherParticleScriptable);

        // it seems like we may need to send out particle edits if the state of our particle was changed.

        if (_voxelEditSender) {
            _voxelEditSender->releaseQueuedMessages();
        }
        if (_particleEditSender) {
            _particleEditSender->releaseQueuedMessages();
        }
    }
}

void Particle::collisionWithVoxel(VoxelDetail* voxelDetails) {
    if (!_script.isEmpty()) {

        ScriptEngine engine(_script); // no menu or controller interface...

        // setup the packet senders and jurisdiction listeners of the script engine's scripting interfaces so
        // we can use the same ones as our context.
        if (_voxelEditSender) {
            engine.getVoxelsScriptingInterface()->setPacketSender(_voxelEditSender);
        }
        if (_particleEditSender) {
            engine.getParticlesScriptingInterface()->setPacketSender(_particleEditSender);
        }

        // Add the Particle object
        ParticleScriptObject particleScriptable(this);
        engine.registerGlobalObject("Particle", &particleScriptable);

        // init and evaluate the script, but return so we can emit the collision
        engine.evaluate();

        VoxelDetailScriptObject voxelDetailsScriptable(voxelDetails);
        particleScriptable.emitCollisionWithVoxel(&voxelDetailsScriptable);

        // it seems like we may need to send out particle edits if the state of our particle was changed.

        if (_voxelEditSender) {
            _voxelEditSender->releaseQueuedMessages();
        }
        if (_particleEditSender) {
            _particleEditSender->releaseQueuedMessages();
        }
    }
}



void Particle::setAge(float age) {
    uint64_t ageInUsecs = age * USECS_PER_SECOND;
    _created = usecTimestampNow() - ageInUsecs;
}

void Particle::copyChangedProperties(const Particle& other) {
    float age = getAge();
    *this = other;
    setAge(age);
}

ParticleProperties Particle::getProperties() const {
    ParticleProperties properties;
    properties.copyFromParticle(*this);
    return properties;
}

void Particle::setProperties(const ParticleProperties& properties) {
    properties.copyToParticle(*this);
}

ParticleProperties::ParticleProperties() :
    _position(0),
    _color(),
    _radius(DEFAULT_RADIUS),
    _velocity(0),
    _gravity(DEFAULT_GRAVITY),
    _damping(DEFAULT_DAMPING),
    _lifetime(DEFAULT_LIFETIME),
    _script(""),
    _inHand(false),
    _shouldDie(false),

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
    _defaultSettings(true)
{
}


uint16_t ParticleProperties::getChangedBits() const {
    uint16_t changedBits = 0;
    if (_radiusChanged) {
        changedBits += PACKET_CONTAINS_RADIUS;
    }

    if (_positionChanged) {
        changedBits += PACKET_CONTAINS_POSITION;
    }

    if (_colorChanged) {
        changedBits += PACKET_CONTAINS_COLOR;
    }

    if (_velocityChanged) {
        changedBits += PACKET_CONTAINS_VELOCITY;
    }

    if (_gravityChanged) {
        changedBits += PACKET_CONTAINS_GRAVITY;
    }

    if (_dampingChanged) {
        changedBits += PACKET_CONTAINS_DAMPING;
    }

    if (_lifetimeChanged) {
        changedBits += PACKET_CONTAINS_LIFETIME;
    }

    if (_inHandChanged) {
        changedBits += PACKET_CONTAINS_INHAND;
    }

    if (_scriptChanged) {
        changedBits += PACKET_CONTAINS_SCRIPT;
    }

    // how do we want to handle this?
    if (_shouldDieChanged) {
        changedBits += PACKET_CONTAINS_SHOULDDIE;
    }

    return changedBits;
}


QScriptValue ParticleProperties::copyToScriptValue(QScriptEngine* engine) const {
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

    return properties;
}

void ParticleProperties::copyFromScriptValue(const QScriptValue &object) {

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

    _lastEdited = usecTimestampNow();
}

void ParticleProperties::copyToParticle(Particle& particle) const {
    if (_positionChanged) {
        particle.setPosition(_position / (float) TREE_SCALE);
    }

    if (_colorChanged) {
        particle.setColor(_color);
    }

    if (_radiusChanged) {
        particle.setRadius(_radius / (float) TREE_SCALE);
    }

    if (_velocityChanged) {
        particle.setVelocity(_velocity / (float) TREE_SCALE);
    }

    if (_gravityChanged) {
        particle.setGravity(_gravity / (float) TREE_SCALE);
    }

    if (_dampingChanged) {
        particle.setDamping(_damping);
    }

    if (_lifetimeChanged) {
        particle.setLifetime(_lifetime);
    }

    if (_scriptChanged) {
        particle.setScript(_script);
    }

    if (_inHandChanged) {
        particle.setInHand(_inHand);
    }

    if (_shouldDieChanged) {
        particle.setShouldDie(_shouldDie);
    }
}

void ParticleProperties::copyFromParticle(const Particle& particle) {
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
    _defaultSettings = false;
}

QScriptValue ParticlePropertiesToScriptValue(QScriptEngine* engine, const ParticleProperties& properties) {
    return properties.copyToScriptValue(engine);
}

void ParticlePropertiesFromScriptValue(const QScriptValue &object, ParticleProperties& properties) {
    properties.copyFromScriptValue(object);
}


QScriptValue ParticleIDtoScriptValue(QScriptEngine* engine, const ParticleID& id) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("id", id.id);
    obj.setProperty("creatorTokenID", id.creatorTokenID);
    obj.setProperty("isKnownID", id.isKnownID);
    return obj;
}

void ParticleIDfromScriptValue(const QScriptValue &object, ParticleID& id) {
    id.id = object.property("id").toVariant().toUInt();
    id.creatorTokenID = object.property("creatorTokenID").toVariant().toUInt();
    id.isKnownID = object.property("isKnownID").toVariant().toBool();
}


