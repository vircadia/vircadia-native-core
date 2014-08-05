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
    uint32_t getID() const { return _id; }
    void setID(uint32_t id) { _id = id; }
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
    void setLastEdited(quint64 lastEdited) { _lastEdited = lastEdited; }
    float getEditedAgo() const /// Elapsed seconds since this entity was last edited
        { return (float)(usecTimestampNow() - _lastEdited) / (float)USECS_PER_SECOND; }

    virtual OctreeElement::AppendState appendEntityData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                                EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData) const;

    static EntityItemID readEntityItemIDFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                    ReadBitstreamToTreeParams& args);
    virtual int readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);


    static int expectedBytes();

    static bool encodeEntityEditMessageDetails(PacketType command, EntityItemID id, const EntityItemProperties& details,
                        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static void adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew);
    void update(const quint64& now);
    void debugDump() const;

    // similar to assignment/copy, but it handles keeping lifetime accurate
    void copyChangedProperties(const EntityItem& other);


    // attributes applicable to all entity types
    EntityTypes::EntityType_t getType() const { return _type; }
    const glm::vec3& getPosition() const { return _position; } /// get position in domain scale units (0.0 - 1.0)
    void setPosition(const glm::vec3& value) { _position = value; } /// set position in domain scale units (0.0 - 1.0)

    float getRadius() const { return _radius; } /// get radius in domain scale units (0.0 - 1.0)
    void setRadius(float value) { _radius = value; } /// set radius in domain scale units (0.0 - 1.0)

    const glm::quat& getRotation() const { return _rotation; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; }

    bool getShouldBeDeleted() const { return _shouldBeDeleted; }
    void setShouldBeDeleted(bool shouldBeDeleted) { _shouldBeDeleted = shouldBeDeleted; }
    
    // position, size, and bounds related helpers
    float getSize() const { return _radius * 2.0f; } /// get maximum dimension in domain scale units (0.0 - 1.0)
    glm::vec3 getMinimumPoint() const { return _position - glm::vec3(_radius, _radius, _radius); }
    glm::vec3 getMaximumPoint() const { return _position + glm::vec3(_radius, _radius, _radius); }
    AACube getAACube() const { return AACube(getMinimumPoint(), getSize()); } /// AACube in domain scale units (0.0 - 1.0)

    static void cleanupLoadedAnimations();

protected:
    virtual void initFromEntityItemID(const EntityItemID& entityItemID); // maybe useful to allow subclasses to init

    quint32 _id;
    EntityTypes::EntityType_t _type;
    uint32_t _creatorTokenID;
    bool _newlyCreated;
    quint64 _lastUpdated;
    quint64 _lastEdited;

    glm::vec3 _position;
    float _radius;
    glm::quat _rotation;
    bool _shouldBeDeleted;
};

class SphereEntityItem : public EntityItem {
public:
    SphereEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Sphere; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

class PlaneEntityItem : public EntityItem {
public:
    PlaneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Plane; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

class CylinderEntityItem : public EntityItem {
public:
    CylinderEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Cylinder; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

class PyramidEntityItem : public EntityItem {
public:
    PyramidEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) { _type = EntityTypes::Pyramid; }

    virtual void somePureVirtualFunction() { }; // allow this class to be constructed
};

#endif // hifi_EntityItem_h
