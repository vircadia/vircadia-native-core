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

#include <AACubeShape.h>
#include <AnimationCache.h> // for Animation, AnimationCache, and AnimationPointer classes
#include <CollisionInfo.h>
#include <Octree.h> // for EncodeBitstreamParams class
#include <OctreeElement.h> // for OctreeElement::AppendState
#include <OctreePacketData.h>
#include <VoxelDetail.h>

#include "EntityItemID.h" 
#include "EntityItemProperties.h" 
#include "EntityTypes.h"

class EntityTree;
class EntityTreeElement;
class EntityTreeElementExtraEncodeData;

#define DONT_ALLOW_INSTANTIATION virtual void pureVirtualFunctionPlaceHolder() = 0;
#define ALLOW_INSTANTIATION virtual void pureVirtualFunctionPlaceHolder() { };

class EntityMotionState;

/// EntityItem class this is the base class for all entity types. It handles the basic properties and functionality available
/// to all other entity types. In particular: postion, size, rotation, age, lifetime, velocity, gravity. You can not instantiate
/// one directly, instead you must only construct one of it's derived classes with additional features.
class EntityItem  {

public:
    enum EntityUpdateFlags {
        // flags for things that need to be relayed to physics engine
        UPDATE_POSITION = 0x0001,
        UPDATE_VELOCITY = 0x0002,
        UPDATE_GRAVITY = 0x0004,
        UPDATE_MASS = 0x0008,
        UPDATE_COLLISION_GROUP = 0x0010,
        UPDATE_MOTION_TYPE = 0x0020,
        UPDATE_SHAPE = 0x0040,
        //...
        // add new flags here in the middle
        //...
        // non-physics stuff
        UPDATE_SCRIPT = 0x2000,
        UPDATE_LIFETIME = 0x4000,
        UPDATE_APPEARANCE = 0x8000
    };

    DONT_ALLOW_INSTANTIATION // This class can not be instantiated directly
    
    EntityItem(const EntityItemID& entityItemID);
    EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    virtual ~EntityItem();

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
    
    /// returns true if something changed
    virtual bool setProperties(const EntityItemProperties& properties, bool forceCopy = false);

    /// Override this in your derived class if you'd like to be informed when something about the state of the entity
    /// has changed. This will be called with properties change or when new data is loaded from a stream
    virtual void somethingChangedNotification() { }

    quint64 getLastUpdated() const { return _lastUpdated; } /// Last simulated time of this entity universal usecs

     /// Last edited time of this entity universal usecs
    quint64 getLastEdited() const { return _lastEdited; }
    void setLastEdited(quint64 lastEdited) 
        { _lastEdited = _lastUpdated = lastEdited; _changedOnServer = glm::max(lastEdited, _changedOnServer); }
    float getEditedAgo() const /// Elapsed seconds since this entity was last edited
        { return (float)(usecTimestampNow() - getLastEdited()) / (float)USECS_PER_SECOND; }

    void markAsChangedOnServer() {  _changedOnServer = usecTimestampNow();  }
    quint64 getLastChangedOnServer() const { return _changedOnServer; }

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;
        
    virtual OctreeElement::AppendState appendEntityData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                                EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData) const;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
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
                                                { return 0; }

    virtual void render(RenderArgs* args) { } // by default entity items don't know how to render

    static int expectedBytes();

    static bool encodeEntityEditMessageDetails(PacketType command, EntityItemID id, const EntityItemProperties& details,
                        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static void adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, size_t length, int clockSkew);
    virtual void update(const quint64& now);
    
    typedef enum SimulationState_t {
        Static,
        Mortal,
        Moving
    } SimulationState;
    
    // computes the SimulationState that the entity SHOULD be in.  
    // Use getSimulationState() to find the state under which it is currently categorized.
    virtual SimulationState computeSimulationState() const; 

    virtual void debugDump() const;

    // attributes applicable to all entity types
    EntityTypes::EntityType getType() const { return _type; }
    const glm::vec3& getPosition() const { return _position; } /// get position in domain scale units (0.0 - 1.0)
    glm::vec3 getPositionInMeters() const { return _position * (float) TREE_SCALE; } /// get position in meters
    
    /// set position in domain scale units (0.0 - 1.0)
    void setPosition(const glm::vec3& value) { _position = value; recalculateCollisionShape(); }
    void setPositionInMeters(const glm::vec3& value) /// set position in meter units (0.0 - TREE_SCALE)
            { setPosition(glm::clamp(value / (float) TREE_SCALE, 0.0f, 1.0f)); }

    glm::vec3 getCenter() const; /// calculates center of the entity in domain scale units (0.0 - 1.0)
    glm::vec3 getCenterInMeters() const { return getCenter() * (float) TREE_SCALE; }

    static const glm::vec3 DEFAULT_DIMENSIONS;
    const glm::vec3& getDimensions() const { return _dimensions; } /// get dimensions in domain scale units (0.0 - 1.0)
    glm::vec3 getDimensionsInMeters() const { return _dimensions * (float) TREE_SCALE; } /// get dimensions in meters
    float getDistanceToBottomOfEntity() const; /// get the distance from the position of the entity to its "bottom" in y axis
    float getLargestDimension() const { return glm::length(_dimensions); } /// get the largest possible dimension

    /// set dimensions in domain scale units (0.0 - 1.0) this will also reset radius appropriately
    void setDimensions(const glm::vec3& value) { _dimensions = value; recalculateCollisionShape(); }

    /// set dimensions in meter units (0.0 - TREE_SCALE) this will also reset radius appropriately
    void setDimensionsInMeters(const glm::vec3& value) { setDimensions(value / (float) TREE_SCALE); }

    static const glm::quat DEFAULT_ROTATION;
    const glm::quat& getRotation() const { return _rotation; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; recalculateCollisionShape(); }

    static const float DEFAULT_GLOW_LEVEL;
    float getGlowLevel() const { return _glowLevel; }
    void setGlowLevel(float glowLevel) { _glowLevel = glowLevel; }

    static const float DEFAULT_LOCAL_RENDER_ALPHA;
    float getLocalRenderAlpha() const { return _localRenderAlpha; }
    void setLocalRenderAlpha(float localRenderAlpha) { _localRenderAlpha = localRenderAlpha; }

    static const float DEFAULT_MASS;
    float getMass() const { return _mass; }
    void setMass(float value) { _mass = value; }

    static const glm::vec3 DEFAULT_VELOCITY;
    static const glm::vec3 NO_VELOCITY;
    static const float EPSILON_VELOCITY_LENGTH;
    const glm::vec3& getVelocity() const { return _velocity; } /// velocity in domain scale units (0.0-1.0) per second
    glm::vec3 getVelocityInMeters() const { return _velocity * (float) TREE_SCALE; } /// get velocity in meters
    void setVelocity(const glm::vec3& value) { _velocity = value; } /// velocity in domain scale units (0.0-1.0) per second
    void setVelocityInMeters(const glm::vec3& value) { _velocity = value / (float) TREE_SCALE; } /// velocity in meters
    bool hasVelocity() const { return _velocity != NO_VELOCITY; }

    static const glm::vec3 DEFAULT_GRAVITY;
    static const glm::vec3 REGULAR_GRAVITY;
    static const glm::vec3 NO_GRAVITY;
    const glm::vec3& getGravity() const { return _gravity; } /// gravity in domain scale units (0.0-1.0) per second squared
    glm::vec3 getGravityInMeters() const { return _gravity * (float) TREE_SCALE; } /// get gravity in meters
    void setGravity(const glm::vec3& value) { _gravity = value; } /// gravity in domain scale units (0.0-1.0) per second squared
    void setGravityInMeters(const glm::vec3& value) { _gravity = value / (float) TREE_SCALE; } /// gravity in meters
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
    bool lifetimeHasExpired() const;

    // position, size, and bounds related helpers
    float getSize() const; /// get maximum dimension in domain scale units (0.0 - 1.0)
    AACube getMaximumAACube() const;
    AACube getMinimumAACube() const;
    AABox getAABox() const; /// axis aligned bounding box in domain scale units (0.0 - 1.0)

    static const QString DEFAULT_SCRIPT;
    const QString& getScript() const { return _script; }
    void setScript(const QString& value) { _script = value; }

    static const glm::vec3 DEFAULT_REGISTRATION_POINT;
    const glm::vec3& getRegistrationPoint() const { return _registrationPoint; } /// registration point as ratio of entity

    /// registration point as ratio of entity
    void setRegistrationPoint(const glm::vec3& value) 
            { _registrationPoint = glm::clamp(value, 0.0f, 1.0f); recalculateCollisionShape(); }

    static const glm::vec3 NO_ANGULAR_VELOCITY;
    static const glm::vec3 DEFAULT_ANGULAR_VELOCITY;
    const glm::vec3& getAngularVelocity() const { return _angularVelocity; }
    void setAngularVelocity(const glm::vec3& value) { _angularVelocity = value; }
    bool hasAngularVelocity() const { return _angularVelocity != NO_ANGULAR_VELOCITY; }

    static const float DEFAULT_ANGULAR_DAMPING;
    float getAngularDamping() const { return _angularDamping; }
    void setAngularDamping(float value) { _angularDamping = value; }

    static const bool DEFAULT_VISIBLE;
    bool getVisible() const { return _visible; }
    void setVisible(bool value) { _visible = value; }
    bool isVisible() const { return _visible; }
    bool isInvisible() const { return !_visible; }

    static const bool DEFAULT_IGNORE_FOR_COLLISIONS;
    bool getIgnoreForCollisions() const { return _ignoreForCollisions; }
    void setIgnoreForCollisions(bool value) { _ignoreForCollisions = value; }

    static const bool DEFAULT_COLLISIONS_WILL_MOVE;
    bool getCollisionsWillMove() const { return _collisionsWillMove; }
    void setCollisionsWillMove(bool value) { _collisionsWillMove = value; }

    static const bool DEFAULT_LOCKED;
    bool getLocked() const { return _locked; }
    void setLocked(bool value) { _locked = value; }
    
    static const QString DEFAULT_USER_DATA;
    const QString& getUserData() const { return _userData; }
    void setUserData(const QString& value) { _userData = value; }
    
    // TODO: We need to get rid of these users of getRadius()... 
    float getRadius() const;
    
    void applyHardCollision(const CollisionInfo& collisionInfo);
    virtual const Shape& getCollisionShapeInMeters() const { return _collisionShape; }
    virtual bool contains(const glm::vec3& point) const { return getAABox().contains(point); }

    // updateFoo() methods to be used when changes need to be accumulated in the _updateFlags
    void updatePosition(const glm::vec3& value);
    void updatePositionInMeters(const glm::vec3& value);
    void updateDimensions(const glm::vec3& value);
    void updateDimensionsInMeters(const glm::vec3& value);
    void updateRotation(const glm::quat& rotation);
    void updateMass(float value);
    void updateVelocity(const glm::vec3& value);
    void updateVelocityInMeters(const glm::vec3& value);
    void updateGravity(const glm::vec3& value);
    void updateGravityInMeters(const glm::vec3& value);
    void updateAngularVelocity(const glm::vec3& value);
    void updateIgnoreForCollisions(bool value);
    void updateCollisionsWillMove(bool value);
    void updateLifetime(float value);
    void updateScript(const QString& value);

    uint32_t getUpdateFlags() const { return _updateFlags; }
    void clearUpdateFlags() { _updateFlags = 0; }

    void* getPhysicsInfo() const { return _physicsInfo; }
    SimulationState getSimulationState() const { return _simulationState; }
    
    void setSimulationState(SimulationState state) { _simulationState = state; }
protected:

    virtual void initFromEntityItemID(const EntityItemID& entityItemID); // maybe useful to allow subclasses to init
    virtual void recalculateCollisionShape();

    EntityTypes::EntityType _type;
    QUuid _id;
    uint32_t _creatorTokenID;
    bool _newlyCreated;
    quint64 _lastUpdated;
    quint64 _lastEdited; // this is the last official local or remote edit time
    quint64 _lastEditedFromRemote; // this is the last time we received and edit from the server
    quint64 _lastEditedFromRemoteInRemoteTime; // time in server time space the last time we received and edit from the server
    quint64 _created;
    quint64 _changedOnServer;

    glm::vec3 _position;
    glm::vec3 _dimensions;
    glm::quat _rotation;
    float _glowLevel;
    float _localRenderAlpha;
    float _mass;
    glm::vec3 _velocity;
    glm::vec3 _gravity;
    float _damping;
    float _lifetime;
    QString _script;
    glm::vec3 _registrationPoint;
    glm::vec3 _angularVelocity;
    float _angularDamping;
    bool _visible;
    bool _ignoreForCollisions;
    bool _collisionsWillMove;
    bool _locked;
    QString _userData;
    
    // NOTE: Radius support is obsolete, but these private helper functions are available for this class to 
    //       parse old data streams
    
    /// set radius in domain scale units (0.0 - 1.0) this will also reset dimensions to be equal for each axis
    void setRadius(float value); 

    AACubeShape _collisionShape;
    void* _physicsInfo;
    SimulationState _simulationState;   // only set by EntityTree

    // UpdateFlags are set whenever a property changes that requires the change to be communicated to other
    // data structures.  It is the responsibility of the EntityTree to relay changes entity and clear flags.
    uint32_t _updateFlags;
};



#endif // hifi_EntityItem_h
