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
