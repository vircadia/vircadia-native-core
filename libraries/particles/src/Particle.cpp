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

printf("Particle::appendParticleData()... getID()=%d\n", getID());

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


void Particle::update() {
    uint64_t now = usecTimestampNow();
    uint64_t elapsed = now - _lastUpdated;
    uint64_t USECS_PER_SECOND = 1000 * 1000;
    uint64_t USECS_PER_FIVE_SECONDS = 5 * USECS_PER_SECOND;
    uint64_t USECS_PER_TEN_SECONDS = 10 * USECS_PER_SECOND;
    uint64_t USECS_PER_THIRTY_SECONDS = 30 * USECS_PER_SECOND;
    uint64_t USECS_PER_MINUTE = 60 * USECS_PER_SECOND;
    uint64_t USECS_PER_HOUR = 60 * USECS_PER_MINUTE;
    float timeElapsed = (float)((float)elapsed/(float)USECS_PER_SECOND);
    //printf("elapsed=%llu timeElapsed=%f\n", elapsed, timeElapsed);

    glm::vec3 position = getPosition() * (float)TREE_SCALE;
    //printf("OLD position=%f, %f, %f\n", position.x, position.y, position.z);
    //printf("velocity=%f, %f, %f\n", getVelocity().x, getVelocity().y, getVelocity().z);
    
    _position += _velocity * timeElapsed;
    
    float gravity = -9.8f / TREE_SCALE;

    glm::vec3 velocity = getVelocity() * (float)TREE_SCALE;
    //printf("OLD velocity=%f, %f, %f\n", velocity.x, velocity.y, velocity.z);

    // handle bounces off the ground...
    if (_position.y <= 0) {
        _velocity = _velocity * glm::vec3(1,-1,1);
        
        velocity = getVelocity() * (float)TREE_SCALE;
        //printf("NEW from ground BOUNCE velocity=%f, %f, %f\n", velocity.x, velocity.y, velocity.z);
        
        _position.y = 0;
    }

    // handle gravity....
    _velocity += glm::vec3(0,gravity,0) * timeElapsed;
    velocity = getVelocity() * (float)TREE_SCALE;
    //printf("NEW from gravity.... velocity=%f, %f, %f\n", velocity.x, velocity.y, velocity.z);

    // handle air resistance....
    glm::vec3 airResistance = _velocity * glm::vec3(-0.25,-0.25,-0.25);
    _velocity += airResistance * timeElapsed;

    velocity = getVelocity() * (float)TREE_SCALE;
    //printf("NEW from air resistance.... velocity=%f, %f, %f\n", velocity.x, velocity.y, velocity.z);

    position = getPosition() * (float)TREE_SCALE;
    //printf("NEW position=%f, %f, %f\n", position.x, position.y, position.z);

    runScript(); // test this...

    position = getPosition() * (float)TREE_SCALE;
    //printf("AFTER SCRIPT NEW position=%f, %f, %f\n", position.x, position.y, position.z);

    _lastUpdated = now;
}

Q_DECLARE_METATYPE(glm::vec3);
Q_DECLARE_METATYPE(xColor);

QScriptValue Particle::vec3toScriptValue(QScriptEngine *engine, const glm::vec3 &vec3) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", vec3.x);
    obj.setProperty("y", vec3.y);
    obj.setProperty("z", vec3.z);
    return obj;
}

void Particle::vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3) {
    vec3.x = object.property("x").toVariant().toFloat();
    vec3.y = object.property("y").toVariant().toFloat();
    vec3.z = object.property("z").toVariant().toFloat();
}

QScriptValue Particle::xColorToScriptValue(QScriptEngine *engine, const xColor& color) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("red", color.red);
    obj.setProperty("green", color.green);
    obj.setProperty("blue", color.blue);
    return obj;
}

void Particle::xColorFromScriptValue(const QScriptValue &object, xColor& color) {
    color.red = object.property("red").toVariant().toInt();
    color.green = object.property("green").toVariant().toInt();
    color.blue = object.property("blue").toVariant().toInt();
}

void Particle::runScript() {
    QString scriptContents(""
        //"print(\"hello world\\n\");\n"
        //"var position = Particle.getPosition();\n"
        //"print(\"position.x=\" + position.x + \"\\n\");\n"
        //"position.x = position.x * 0.9;\n"
        //"Particle.setPosition(position);\n"
        //"print(\"position.x=\" + position.x + \"\\n\");\n"
        //"var color = Particle.getColor();\n"
        //"color.red = color.red - 1;\n"
        //"if (color.red < 10) { color.red = 255; }\n"
        //"Particle.setColor(color);\n"
        //"print(\"position=\" + position.x + \",\" +  position.y + \",\" +  position.z + \"\\n\");\n"
    );
    QScriptEngine engine;
    
    // register meta-type for glm::vec3 and rgbColor conversions
    qScriptRegisterMetaType(&engine, vec3toScriptValue, vec3FromScriptValue);
    qScriptRegisterMetaType(&engine, xColorToScriptValue, xColorFromScriptValue);
    
    ParticleScriptObject particleScriptable(this);
    QScriptValue particleValue = engine.newQObject(&particleScriptable);
    engine.globalObject().setProperty("Particle", particleValue);
    
    //QScriptValue voxelScripterValue =  engine.newQObject(&_voxelScriptingInterface);
    //engine.globalObject().setProperty("Voxels", voxelScripterValue);

    //QScriptValue particleScripterValue =  engine.newQObject(&_particleScriptingInterface);
    //engine.globalObject().setProperty("Particles", particleScripterValue);
    
    QScriptValue treeScaleValue = engine.newVariant(QVariant(TREE_SCALE));
    engine.globalObject().setProperty("TREE_SCALE", TREE_SCALE);
    
    //qDebug() << "Downloaded script:" << scriptContents << "\n";
    QScriptValue result = engine.evaluate(scriptContents);
    //qDebug() << "Evaluated script.\n";
    
    if (engine.hasUncaughtException()) {
        int line = engine.uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at line" << line << ":" << result.toString() << "\n";
    }
}
