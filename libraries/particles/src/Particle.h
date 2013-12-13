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

const uint32_t NEW_PARTICLE = 0xFFFFFFFF;
const uint32_t UNKNOWN_TOKEN = 0xFFFFFFFF;

class ParticleDetail {
public:
    uint32_t id;
    uint64_t lastUpdated;
    glm::vec3 position;
    float radius;
    rgbColor color;
    glm::vec3 velocity;
    glm::vec3 gravity;
    float damping;
    QString updateScript;
    uint32_t creatorTokenID;
};

const float DEFAULT_DAMPING = 0.99f;
const glm::vec3 DEFAULT_GRAVITY(0, (-9.8f / TREE_SCALE), 0);
const QString DEFAULT_SCRIPT("");

class Particle  {
    
public:
    Particle();
    Particle(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity, 
            float damping = DEFAULT_DAMPING, glm::vec3 gravity = DEFAULT_GRAVITY, QString updateScript = DEFAULT_SCRIPT, 
            uint32_t id = NEW_PARTICLE);
    
    /// creates an NEW particle from an PACKET_TYPE_PARTICLE_ADD_OR_EDIT edit data buffer
    static Particle fromEditPacket(unsigned char* data, int length, int& processedBytes); 
    
    virtual ~Particle();
    virtual void init(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity, 
                            float damping, glm::vec3 gravity, QString updateScript, uint32_t id);

    const glm::vec3& getPosition() const { return _position; }
    const rgbColor& getColor() const { return _color; }
    xColor getColor() { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }
    float getRadius() const { return _radius; }
    const glm::vec3& getVelocity() const { return _velocity; }
    const glm::vec3& getGravity() const { return _gravity; }
    float getDamping() const { return _damping; }
    uint64_t getLastUpdated() const { return _lastUpdated; }
    uint32_t getID() const { return _id; }
    bool getShouldDie() const { return _shouldDie; }
    QString getUpdateScript() const { return _updateScript; }
    uint32_t getCreatorTokenID() const { return _creatorTokenID; }
    bool isNewlyCreated() const { return _newlyCreated; }

    void setPosition(const glm::vec3& value) { _position = value; }
    void setVelocity(const glm::vec3& value) { _velocity = value; }
    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
            _color[RED_INDEX] = value.red; 
            _color[GREEN_INDEX] = value.green; 
            _color[BLUE_INDEX] = value.blue; 
    }
    void setRadius(float value) { _radius = value; }
    void setGravity(const glm::vec3& value) { _gravity = value; }
    void setDamping(float value) { _damping = value; }
    void setShouldDie(bool shouldDie) { _shouldDie = shouldDie; }
    void setUpdateScript(QString updateScript) { _updateScript = updateScript; }
    void setCreatorTokenID(uint32_t creatorTokenID) { _creatorTokenID = creatorTokenID; }

    bool appendParticleData(OctreePacketData* packetData) const;
    int readParticleDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    static int expectedBytes();

    static bool encodeParticleEditMessageDetails(PACKET_TYPE command, int count, const ParticleDetail* details, 
                        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    void update();

    void debugDump() const;
protected:
    void runScript();
    static QScriptValue vec3toScriptValue(QScriptEngine *engine, const glm::vec3 &vec3);
    static void vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3);
    static QScriptValue xColorToScriptValue(QScriptEngine *engine, const xColor& color);
    static void xColorFromScriptValue(const QScriptValue &object, xColor& color);
    
    glm::vec3 _position;
    rgbColor _color;
    float _radius;
    glm::vec3 _velocity;
    uint64_t _lastUpdated;
    uint32_t _id;
    static uint32_t _nextID;
    bool _shouldDie;
    glm::vec3 _gravity;
    float _damping;
    QString _updateScript;

    uint32_t _creatorTokenID;
    bool _newlyCreated;
};

class ParticleScriptObject  : public QObject {
    Q_OBJECT
public:
    ParticleScriptObject(Particle* particle) { _particle = particle; }

public slots:
    glm::vec3 getPosition() const { return _particle->getPosition(); }
    glm::vec3 getVelocity() const { return _particle->getVelocity(); }
    xColor getColor() const { return _particle->getColor(); }
    glm::vec3 getGravity() const { return _particle->getGravity(); }
    float getDamping() const { return _particle->getDamping(); }
    float getRadius() const { return _particle->getRadius(); }
    bool setShouldDie() { return _particle->getShouldDie(); }

    void setPosition(glm::vec3 value) { _particle->setPosition(value); }
    void setVelocity(glm::vec3 value) { _particle->setVelocity(value); }
    void setGravity(glm::vec3 value) { _particle->setGravity(value); }
    void setDamping(float value) { _particle->setDamping(value); }
    void setColor(xColor value) { _particle->setColor(value); }
    void setRadius(float value) { _particle->setRadius(value); }
    void setShouldDie(bool value) { _particle->setShouldDie(value); }

private:
    Particle* _particle;
};


#endif /* defined(__hifi__Particle__) */
