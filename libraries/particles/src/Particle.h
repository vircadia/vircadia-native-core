//
//  Particle.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__Particle__
#define __hifi__Particle__

#include <glm/glm.hpp>
#include <stdint.h>

#include <SharedUtil.h>
#include <OctreePacketData.h>

class Particle  {
public:
    Particle();
    Particle(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity);
    
    /// creates an NEW particle from an PACKET_TYPE_PARTICLE_ADD edit data buffer
    static Particle fromEditPacket(unsigned char* data, int length, int& processedBytes); 
    
    virtual ~Particle();
    virtual void init(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity);

    const glm::vec3& getPosition() const { return _position; }
    const rgbColor& getColor() const { return _color; }
    float getRadius() const { return _radius; }
    const glm::vec3& getVelocity() const { return _velocity; }
    uint64_t getLastUpdated() const { return _lastUpdated; }
    uint32_t getID() const { return _id; }

    bool appendParticleData(OctreePacketData* packetData) const;
    int readParticleDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    static int expectedBytes();
protected:
    glm::vec3 _position;
    rgbColor _color;
    float _radius;
    glm::vec3 _velocity;
    uint64_t _lastUpdated;
    uint32_t _id;
    static uint32_t _nextID;
};

#endif /* defined(__hifi__Particle__) */