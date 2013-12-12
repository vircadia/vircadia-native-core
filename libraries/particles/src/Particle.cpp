//
//  Particle.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>

#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // usecTimestampNow()

#include "Particle.h"

uint32_t Particle::_nextID = 0;


Particle::Particle(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity, 
                    float damping, glm::vec3 gravity, QString updateScript, uint32_t id) {
                    
    init(position, radius, color, velocity, damping, gravity, updateScript, id);
}

Particle::Particle() {
    rgbColor noColor = { 0, 0, 0 };
    init(glm::vec3(0,0,0), 0, noColor, glm::vec3(0,0,0), DEFAULT_DAMPING, DEFAULT_GRAVITY, DEFAULT_SCRIPT, NEW_PARTICLE);
}

Particle::~Particle() {
}

void Particle::init(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity, 
                        float damping, glm::vec3 gravity, QString updateScript, uint32_t id) {
    if (id == NEW_PARTICLE) {
        _id = _nextID;
        _nextID++;
    } else {
        _id = id;
    }
    _lastUpdated = usecTimestampNow();

    _position = position;
    _radius = radius;
    memcpy(_color, color, sizeof(_color));
    _velocity = velocity;
    _damping = damping;
    _gravity = gravity;
    _updateScript = updateScript;
}


bool Particle::appendParticleData(OctreePacketData* packetData) const {

    bool success = packetData->appendValue(getID());

    //printf("Particle::appendParticleData()... getID()=%d\n", getID());

    if (success) {
        success = packetData->appendValue(getLastUpdated());
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
        uint16_t scriptLength = _updateScript.size() + 1; // include NULL
        success = packetData->appendValue(scriptLength);
        if (success) {
            success = packetData->appendRawData((const unsigned char*)qPrintable(_updateScript), scriptLength);
        }
    }
    return success;
}

int Particle::expectedBytes() {
    int expectedBytes = sizeof(uint32_t) + sizeof(uint64_t) + sizeof(float) + 
                        sizeof(glm::vec3) + sizeof(rgbColor) + sizeof(glm::vec3) +
                        sizeof(glm::vec3) + sizeof(float);
    return expectedBytes;
}

int Particle::readParticleDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {
    int bytesRead = 0;
    if (bytesLeftToRead >= expectedBytes()) {
        const unsigned char* dataAt = data;

        // id
        memcpy(&_id, dataAt, sizeof(_id));
        dataAt += sizeof(_id);
        bytesRead += sizeof(_id);

        // lastupdated
        memcpy(&_lastUpdated, dataAt, sizeof(_lastUpdated));
        dataAt += sizeof(_lastUpdated);
        bytesRead += sizeof(_lastUpdated);

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

        // script
        uint16_t scriptLength;
        memcpy(&scriptLength, dataAt, sizeof(scriptLength));
        dataAt += sizeof(scriptLength);
        bytesRead += sizeof(scriptLength);
        QString tempString((const char*)dataAt);
        _updateScript = tempString;
        dataAt += scriptLength;
        bytesRead += scriptLength;
        
        //printf("Particle::readParticleDataFromBuffer()... "); debugDump();
    }
    return bytesRead;
}


Particle Particle::fromEditPacket(unsigned char* data, int length, int& processedBytes) {
    Particle newParticle; // id and lastUpdated will get set here...
    unsigned char* dataAt = data;
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
    
    // special case for handling "new" particles
    if (editID == NEW_PARTICLE) {
        // If this is a NEW_PARTICLE, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id
        uint32_t creatorTokenID;
        memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
        dataAt += sizeof(creatorTokenID);
        processedBytes += sizeof(creatorTokenID);
        newParticle.setCreatorTokenID(creatorTokenID);
        newParticle._newlyCreated = true;
    } else {
        newParticle._id = editID;
        newParticle._newlyCreated = false;
    } 

    // lastUpdated
    memcpy(&newParticle._lastUpdated, dataAt, sizeof(newParticle._lastUpdated));
    dataAt += sizeof(newParticle._lastUpdated);
    processedBytes += sizeof(newParticle._lastUpdated);
    
    // radius
    memcpy(&newParticle._radius, dataAt, sizeof(newParticle._radius));
    dataAt += sizeof(newParticle._radius);
    processedBytes += sizeof(newParticle._radius);

    // position
    memcpy(&newParticle._position, dataAt, sizeof(newParticle._position));
    dataAt += sizeof(newParticle._position);
    processedBytes += sizeof(newParticle._position);

    // color
    memcpy(newParticle._color, dataAt, sizeof(newParticle._color));
    dataAt += sizeof(newParticle._color);
    processedBytes += sizeof(newParticle._color);

    // velocity
    memcpy(&newParticle._velocity, dataAt, sizeof(newParticle._velocity));
    dataAt += sizeof(newParticle._velocity);
    processedBytes += sizeof(newParticle._velocity);
    
    // gravity
    memcpy(&newParticle._gravity, dataAt, sizeof(newParticle._gravity));
    dataAt += sizeof(newParticle._gravity);
    processedBytes += sizeof(newParticle._gravity);
    
    // damping
    memcpy(&newParticle._damping, dataAt, sizeof(newParticle._damping));
    dataAt += sizeof(newParticle._damping);
    processedBytes += sizeof(newParticle._damping);

    // script
    uint16_t scriptLength;
    memcpy(&scriptLength, dataAt, sizeof(scriptLength));
    dataAt += sizeof(scriptLength);
    processedBytes += sizeof(scriptLength);
    QString tempString((const char*)dataAt);
    newParticle._updateScript = tempString;
    dataAt += scriptLength;
    processedBytes += scriptLength;

    const bool wantDebugging = false;
    if (wantDebugging) {
        printf("Particle::fromEditPacket()...\n"); 
        printf("   Particle id in packet:%u\n", editID);
        newParticle.debugDump();
    }
    
    return newParticle;
}

void Particle::debugDump() const {
    printf("Particle id  :%u\n", _id);
    printf(" last updated:%llu\n", _lastUpdated);
    printf(" position:%f,%f,%f\n", _position.x, _position.y, _position.z);
    printf(" velocity:%f,%f,%f\n", _velocity.x, _velocity.y, _velocity.z);
    printf(" gravity:%f,%f,%f\n", _gravity.x, _gravity.y, _gravity.z);
}

bool Particle::encodeParticleEditMessageDetails(PACKET_TYPE command, int count, const ParticleDetail* details, 
        unsigned char* bufferOut, int sizeIn, int& sizeOut) {

    bool success = true; // assume the best
    unsigned char* copyAt = bufferOut;
    sizeOut = 0;

    for (int i = 0; i < count && success; i++) {
        // get the octal code for the particle
        unsigned char* octcode = pointToOctalCode(details[i].position.x, details[i].position.y, 
                                                    details[i].position.z, details[i].radius);

        int octets = numberOfThreeBitSectionsInCode(octcode);
        int lengthOfOctcode = bytesRequiredForCodeLength(octets);
        int lenfthOfEditData = lengthOfOctcode + expectedBytes();
        
        // make sure we have room to copy this particle
        if (sizeOut + lenfthOfEditData > sizeIn) {
            success = false;
        } else {
            // add it to our message
            memcpy(copyAt, octcode, lengthOfOctcode);
            copyAt += lengthOfOctcode;
            sizeOut += lengthOfOctcode;
            
            // Now add our edit content details...

            // id
            memcpy(copyAt, &details[i].id, sizeof(details[i].id));
            copyAt += sizeof(details[i].id);
            sizeOut += sizeof(details[i].id);
            // special case for handling "new" particles
            if (details[i].id == NEW_PARTICLE) {
                // If this is a NEW_PARTICLE, then we assume that there's an additional uint32_t creatorToken, that
                // we want to send back to the creator as an map to the actual id
                memcpy(copyAt, &details[i].creatorTokenID, sizeof(details[i].creatorTokenID));
                copyAt += sizeof(details[i].creatorTokenID);
                sizeOut += sizeof(details[i].creatorTokenID);
            } 

            // lastUpdated
            memcpy(copyAt, &details[i].lastUpdated, sizeof(details[i].lastUpdated));
            copyAt += sizeof(details[i].lastUpdated);
            sizeOut += sizeof(details[i].lastUpdated);
            
            // radius
            memcpy(copyAt, &details[i].radius, sizeof(details[i].radius));
            copyAt += sizeof(details[i].radius);
            sizeOut += sizeof(details[i].radius);

            // position
            memcpy(copyAt, &details[i].position, sizeof(details[i].position));
            copyAt += sizeof(details[i].position);
            sizeOut += sizeof(details[i].position);

            // color
            memcpy(copyAt, details[i].color, sizeof(details[i].color));
            copyAt += sizeof(details[i].color);
            sizeOut += sizeof(details[i].color);

            // velocity
            memcpy(copyAt, &details[i].velocity, sizeof(details[i].velocity));
            copyAt += sizeof(details[i].velocity);
            sizeOut += sizeof(details[i].velocity);

            // gravity
            memcpy(copyAt, &details[i].gravity, sizeof(details[i].gravity));
            copyAt += sizeof(details[i].gravity);
            sizeOut += sizeof(details[i].gravity);

            // damping
            memcpy(copyAt, &details[i].damping, sizeof(details[i].damping));
            copyAt += sizeof(details[i].damping);
            sizeOut += sizeof(details[i].damping);

            // script
            uint16_t scriptLength = details[i].updateScript.size() + 1;
            memcpy(copyAt, &scriptLength, sizeof(scriptLength));
            copyAt += sizeof(scriptLength);
            sizeOut += sizeof(scriptLength);
            memcpy(copyAt, qPrintable(details[i].updateScript), scriptLength);
            copyAt += scriptLength;
            sizeOut += scriptLength;

            bool wantDebugging = false;
            if (wantDebugging) {            
                printf("encodeParticleEditMessageDetails()....\n");
                printf("Particle id  :%u\n", details[i].id);
                printf(" last updated:%llu\n", details[i].lastUpdated);
                printf(" nextID:%u\n", _nextID);
            }
        }
        // cleanup
        delete[] octcode;
    }

    return success;
}


void Particle::update() {
    uint64_t now = usecTimestampNow();
    uint64_t elapsed = now - _lastUpdated;
    uint64_t USECS_PER_SECOND = 1000 * 1000;
    float timeElapsed = (float)((float)elapsed/(float)USECS_PER_SECOND);
    
    // calculate our default shouldDie state... then allow script to change it if it wants...
    float velocityScalar = glm::length(getVelocity());
    const float STILL_MOVING = 0.05 / TREE_SCALE;
    setShouldDie(velocityScalar < STILL_MOVING);

    runScript(); // allow the javascript to alter our state

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

    _lastUpdated = now;
}

void Particle::runScript() {
    if (!_updateScript.isEmpty()) {
        QScriptEngine engine;
    
        // register meta-type for glm::vec3 and rgbColor conversions
        registerMetaTypes(&engine);
    
        ParticleScriptObject particleScriptable(this);
        QScriptValue particleValue = engine.newQObject(&particleScriptable);
        engine.globalObject().setProperty("Particle", particleValue);
        
        QScriptValue treeScaleValue = engine.newVariant(QVariant(TREE_SCALE));
        engine.globalObject().setProperty("TREE_SCALE", TREE_SCALE);
    
        //qDebug() << "Downloaded script:" << _updateScript << "\n";
        QScriptValue result = engine.evaluate(_updateScript);
        //qDebug() << "Evaluated script.\n";
    
        if (engine.hasUncaughtException()) {
            int line = engine.uncaughtExceptionLineNumber();
            qDebug() << "Uncaught exception at line" << line << ":" << result.toString() << "\n";
        }
    }
}
