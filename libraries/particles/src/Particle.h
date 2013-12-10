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

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>

#include <SharedUtil.h>
#include <OctreePacketData.h>

class ParticleDetail {
public:
    glm::vec3 position;
	float radius;
	rgbColor color;
    glm::vec3 velocity;
};


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
    xColor getColor() { return { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; }

    float getRadius() const { return _radius; }
    const glm::vec3& getVelocity() const { return _velocity; }
    uint64_t getLastUpdated() const { return _lastUpdated; }
    uint32_t getID() const { return _id; }

    void setPosition(const glm::vec3& value) { _position = value; }
    void setVelocity(const glm::vec3& value) { _velocity = value; }
    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
            _color[RED_INDEX] = value.red; 
            _color[GREEN_INDEX] = value.green; 
            _color[BLUE_INDEX] = value.blue; 
    }

    bool appendParticleData(OctreePacketData* packetData) const;
    int readParticleDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    static int expectedBytes();

    static bool encodeParticleEditMessageDetails(PACKET_TYPE command, int count, const ParticleDetail* details, 
                        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    void update();
    void runScript();

    static QScriptValue vec3toScriptValue(QScriptEngine *engine, const glm::vec3 &vec3);
    static void vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3);
    static QScriptValue xColorToScriptValue(QScriptEngine *engine, const xColor& color);
    static void xColorFromScriptValue(const QScriptValue &object, xColor& color);
    
protected:
    glm::vec3 _position;
    rgbColor _color;
    float _radius;
    glm::vec3 _velocity;
    uint64_t _lastUpdated;
    uint32_t _id;
    static uint32_t _nextID;
};

class ParticleScriptObject  : public QObject {
    Q_OBJECT
public:
    ParticleScriptObject(Particle* particle) { _particle = particle; }

public slots:
    glm::vec3 getPosition() const { return _particle->getPosition(); }
    glm::vec3 getVelocity() const { return _particle->getVelocity(); }
    xColor getColor() const { return _particle->getColor(); }

    void setPosition(glm::vec3 value) { return _particle->setPosition(value); }
    void setVelocity(glm::vec3 value) { return _particle->setVelocity(value); }
    void setColor(xColor value) { return _particle->setColor(value); }

private:
    Particle* _particle;
};

#endif /* defined(__hifi__Particle__) */