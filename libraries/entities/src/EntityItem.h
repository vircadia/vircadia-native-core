//
//  EntityItem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityItem_h
#define hifi_EntityItem_h

#include <stdint.h>

#include <glm/glm.hpp>

#include <AnimationCache.h> // for Animation, AnimationCache, and AnimationPointer classes
#include <Octree.h> // for EncodeBitstreamParams class
#include <OctreeElement.h> // for OctreeElement::AppendState
#include <OctreePacketData.h>

#include "EntityItemID.h" 
#include "EntityItemProperties.h" 
#include "EntityTypes.h"

class EntityTreeElementExtraEncodeData;

/// EntityItem class - this is the actual model item class.
class EntityItem  {

public:
    EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    
    virtual ~EntityItem();
    
    virtual void somePureVirtualFunction() = 0;

    // ID and EntityItemID related methods
    QUuid getID() const { return _id; }
    void setID(const QUuid& id) { _id = id; }
    uint32_t getCreatorTokenID() const { return _creatorTokenID; }
    void setCreatorTokenID(uint32_t creatorTokenID) { _creatorTokenID = creatorTokenID; }
    bool isNewlyCreated() const { return _newlyCreated; }
    bool isKnownID() const { return getID() != UNKNOWN_ENTITY_ID; }
    EntityItemID getEntityItemID() const { return EntityItemID(getID(), getCreatorTokenID(), getID() != UNKNOWN_ENTITY_ID); }

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties() const;
    virtual void setProperties(const EntityItemProperties& properties, bool forceCopy = false);

    quint64 getLastUpdated() const { return _lastUpdated; } /// Last simulated time of this entity universal usecs
    quint64 getLastEdited() const { return _lastEdited; } /// Last edited time of this entity universal usecs
    void setLastEdited(quint64 lastEdited) { _lastEdited = lastEdited; _lastUpdated = lastEdited; }
    float getEditedAgo() const /// Elapsed seconds since this entity was last edited
        { return (float)(usecTimestampNow() - _lastEdited) / (float)USECS_PER_SECOND; }

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;
        
    virtual OctreeElement::AppendState appendEntityData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                                EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData) const;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const { /* do nothing*/ };

    static EntityItemID readEntityItemIDFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                    ReadBitstreamToTreeParams& args);

    virtual int readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) 
                                                { return 0; };

    static int expectedBytes();

    static bool encodeEntityEditMessageDetails(PacketType command, EntityItemID id, const EntityItemProperties& details,
                        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static void adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew);
    virtual void update(const quint64& now);
    
    typedef enum SimuationState_t {
        Static,
        Changing,
        Moving
    } SimuationState;
    
    virtual SimuationState getSimulationState() const;
    void debugDump() const;

    // similar to assignment/copy, but it handles keeping lifetime accurate
    void copyChangedProperties(const EntityItem& other);

    // attributes applicable to all entity types
    EntityTypes::EntityType getType() const { return _type; }
    const glm::vec3& getPosition() const { return _position; } /// get position in domain scale units (0.0 - 1.0)
    void setPosition(const glm::vec3& value) { _position = value; } /// set position in domain scale units (0.0 - 1.0)

    float getRadius() const { return _radius; } /// get radius in domain scale units (0.0 - 1.0)
    void setRadius(float value) { _radius = value; } /// set radius in domain scale units (0.0 - 1.0)

    const glm::quat& getRotation() const { return _rotation; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; }

    bool getShouldBeDeleted() const { return _shouldBeDeleted; }
    void setShouldBeDeleted(bool shouldBeDeleted) { _shouldBeDeleted = shouldBeDeleted; }

    static const float DEFAULT_GLOW_LEVEL;
    float getGlowLevel() const { return _glowLevel; }
    void setGlowLevel(float glowLevel) { _glowLevel = glowLevel; }

    static const float DEFAULT_MASS;
    float getMass() const { return _mass; }
    void setMass(float value) { _mass = value; }

    static const glm::vec3 DEFAULT_VELOCITY;
    static const glm::vec3 NO_VELOCITY;
    static const float EPSILON_VELOCITY_LENGTH;
    const glm::vec3& getVelocity() const { return _velocity; } /// velocity in domain scale units (0.0-1.0) per second
    void setVelocity(const glm::vec3& value) { _velocity = value; } /// velocity in domain scale units (0.0-1.0) per second
    bool hasVelocity() const { return _velocity != NO_VELOCITY; }

    static const glm::vec3 DEFAULT_GRAVITY;
    static const glm::vec3 REGULAR_GRAVITY;
    static const glm::vec3 NO_GRAVITY;
    const glm::vec3& getGravity() const { return _gravity; } /// gravity in domain scale units (0.0-1.0) per second squared
    void setGravity(const glm::vec3& value) { _gravity = value; } /// gravity in domain scale units (0.0-1.0) per second squared
    bool hasGravity() const { return _gravity != NO_GRAVITY; }
    
    // TODO: this should eventually be updated to support resting on collisions with other surfaces
    bool isRestingOnSurface() const;

    static const float DEFAULT_DAMPING;
    float getDamping() const { return _damping; }
    void setDamping(float value) { _damping = value; }

    // lifetime related properties.
    static const float IMMORTAL; /// special lifetime which means the entity lives for ever. default lifetime
    static const float DEFAULT_LIFETIME;
    float getLifetime() const { return _lifetime; } /// get the lifetime in seconds for the entity
    void setLifetime(float value) { _lifetime = value; } /// set the lifetime in seconds for the entity

    /// is this entity immortal, in that it has no lifetime set, and will exist until manually deleted
    bool isImmortal() const { return _lifetime == IMMORTAL; }

    /// is this entity mortal, in that it has a lifetime set, and will automatically be deleted when that lifetime expires
    bool isMortal() const { return _lifetime != IMMORTAL; }
    
    /// age of this entity in seconds
    float getAge() const { return (float)(usecTimestampNow() - _created) / (float)USECS_PER_SECOND; }

    // position, size, and bounds related helpers
    float getSize() const { return _radius * 2.0f; } /// get maximum dimension in domain scale units (0.0 - 1.0)
    glm::vec3 getMinimumPoint() const { return _position - glm::vec3(_radius, _radius, _radius); }
    glm::vec3 getMaximumPoint() const { return _position + glm::vec3(_radius, _radius, _radius); }
    AACube getAACube() const { return AACube(getMinimumPoint(), getSize()); } /// AACube in domain scale units (0.0 - 1.0)

    static const QString DEFAULT_SCRIPT;
    const QString& getScript() const { return _script; }
    void setScript(const QString& value) { _script = value; }

protected:
    virtual void initFromEntityItemID(const EntityItemID& entityItemID); // maybe useful to allow subclasses to init

    EntityTypes::EntityType _type;
    QUuid _id;
    uint32_t _creatorTokenID;
    bool _newlyCreated;
    quint64 _lastUpdated;
    quint64 _lastEdited;
    quint64 _created;

    glm::vec3 _position;
    float _radius;
    glm::quat _rotation;
    bool _shouldBeDeleted;
    float _glowLevel;
    float _mass;
    glm::vec3 _velocity;
    glm::vec3 _gravity;
    float _damping;
    float _lifetime;
    QString _script;
};

class SphereEntityItem : public EntityItem {
public:
    SphereEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Sphere; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed

    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
        return new SphereEntityItem(entityID, properties);
    }
};

class PlaneEntityItem : public EntityItem {
public:
    PlaneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Plane; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed

    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
        return new PlaneEntityItem(entityID, properties);
    }
};

class CylinderEntityItem : public EntityItem {
public:
    CylinderEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Cylinder; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed

    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
        return new CylinderEntityItem(entityID, properties);
    }
};

class PyramidEntityItem : public EntityItem {
public:
    PyramidEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Pyramid; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed

    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
        return new PyramidEntityItem(entityID, properties);
    }
};

#endif // hifi_EntityItem_h
