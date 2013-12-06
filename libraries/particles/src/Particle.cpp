//
//  Particle.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#include <SharedUtil.h> // usecTimestampNow()

#include "Particle.h"

uint32_t Particle::_nextID = 0;


Particle::Particle(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity) {
    init(position, radius, color, velocity);
}

Particle::Particle() {
    rgbColor noColor = { 0, 0, 0 };
    init(glm::vec3(0,0,0), 0, noColor, glm::vec3(0,0,0));
}

Particle::~Particle() {
}

void Particle::init(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity) {
    _id = _nextID;
    _nextID++;
    _lastUpdated = usecTimestampNow();

    _position = position;
    _radius = radius;
    memcpy(_color, color, sizeof(_color));
    _velocity = velocity;
}


bool Particle::appendParticleData(OctreePacketData* packetData) const {

    bool success = packetData->appendValue(getID());

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
    return success;
}

int Particle::expectedBytes() {
    int expectedBytes = sizeof(uint32_t) + sizeof(uint64_t) + sizeof(float) + 
                        sizeof(glm::vec3) + sizeof(rgbColor) + sizeof(glm::vec3);
    return expectedBytes;
}

int Particle::readParticleDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {
    int bytesRead = 0;
    if (bytesLeftToRead >= expectedBytes()) {
        const unsigned char* dataAt = data;

        // id
        memcpy(&_id, dataAt, sizeof(_id));
        dataAt += sizeof(_id);

        // lastupdated
        memcpy(&_lastUpdated, dataAt, sizeof(_lastUpdated));
        dataAt += sizeof(_lastUpdated);

        // radius
        memcpy(&_radius, dataAt, sizeof(_radius));
        dataAt += sizeof(_radius);

        // position
        memcpy(&_position, dataAt, sizeof(_position));
        dataAt += sizeof(_position);

        // color
        memcpy(_color, dataAt, sizeof(_color));
        dataAt += sizeof(_color);

        // velocity
        memcpy(&_velocity, dataAt, sizeof(_velocity));
        dataAt += sizeof(_velocity);
    
        bytesRead = expectedBytes();
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
    
    return newParticle;
}


bool Particle::encodeParticleEditMessageDetails(PACKET_TYPE command, int count, const ParticleDetail* details, 
        unsigned char* bufferOut, int sizeIn, int& sizeOut) {

    bool success = true; // assume the best
    unsigned char* copyAt = bufferOut;
    sizeOut = 0;

    for (int i = 0; i < count && success; i++) {
        // get the coded voxel
        unsigned char* octcode = pointToOctalCode(details[i].position.x, details[i].position.y, details[i].position.z, details[i].radius);

        int octets = numberOfThreeBitSectionsInCode(octcode);
        int lengthOfOctcode = bytesRequiredForCodeLength(octets);
        int lenfthOfEditData = lengthOfOctcode + expectedBytes();
        
        // make sure we have room to copy this voxel
        if (sizeOut + lenfthOfEditData > sizeIn) {
            success = false;
        } else {
            // add it to our message
            memcpy(copyAt, octcode, lengthOfOctcode);
            copyAt += lengthOfOctcode;
            sizeOut += lengthOfOctcode;
            
            // Now add our edit content details...
            
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
        }
        // cleanup
        delete[] octcode;
    }

    return success;
}

