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

#include <memory>
#include <stdint.h>

#include <glm/glm.hpp>

#include <AnimationCache.h> // for Animation, AnimationCache, and AnimationPointer classes
#include <CollisionInfo.h>
#include <Octree.h> // for EncodeBitstreamParams class
#include <OctreeElement.h> // for OctreeElement::AppendState
#include <OctreePacketData.h>
#include <ShapeInfo.h>
#include <Transform.h>

#include "EntityItemID.h"
#include "EntityItemProperties.h"
#include "EntityItemPropertiesDefaults.h"
#include "EntityTypes.h"
#include "SimulationOwner.h"

class EntitySimulation;
class EntityTreeElement;
class EntityTreeElementExtraEncodeData;

class EntityActionInterface;
typedef std::shared_ptr<EntityActionInterface> EntityActionPointer;


namespace render {
    class Scene;
    class PendingChanges;
}

// these thesholds determine what updates will be ignored (client and server)
const float IGNORE_POSITION_DELTA = 0.0001f;
const float IGNORE_DIMENSIONS_DELTA = 0.0005f;
const float IGNORE_ALIGNMENT_DOT = 0.99997f;
const float IGNORE_LINEAR_VELOCITY_DELTA = 0.001f;
const float IGNORE_DAMPING_DELTA = 0.001f;
const float IGNORE_GRAVITY_DELTA = 0.001f;
const float IGNORE_ANGULAR_VELOCITY_DELTA = 0.0002f;

// these thresholds determine what updates will activate the physical object
const float ACTIVATION_POSITION_DELTA = 0.005f;
const float ACTIVATION_DIMENSIONS_DELTA = 0.005f;
const float ACTIVATION_ALIGNMENT_DOT = 0.99990f;
const float ACTIVATION_LINEAR_VELOCITY_DELTA = 0.01f;
const float ACTIVATION_GRAVITY_DELTA = 0.1f;
const float ACTIVATION_ANGULAR_VELOCITY_DELTA = 0.03f;

#define DONT_ALLOW_INSTANTIATION virtual void pureVirtualFunctionPlaceHolder() = 0;
#define ALLOW_INSTANTIATION virtual void pureVirtualFunctionPlaceHolder() { };

#define debugTime(T, N) qPrintable(QString("%1 [ %2 ago]").arg(T, 16, 10).arg(formatUsecTime(N - T), 15))
#define debugTimeOnly(T) qPrintable(QString("%1").arg(T, 16, 10))
#define debugTreeVector(V) V << "[" << V << " in meters ]"

#if DEBUG
  #define assertLocked() assert(isLocked())
#else
  #define assertLocked()
#endif

#if DEBUG
  #define assertWriteLocked() assert(isWriteLocked())
#else
  #define assertWriteLocked()
#endif

#if DEBUG
  #define assertUnlocked() assert(isUnlocked())
#else
  #define assertUnlocked()
#endif

/// EntityItem class this is the base class for all entity types. It handles the basic properties and functionality available
/// to all other entity types. In particular: postion, size, rotation, age, lifetime, velocity, gravity. You can not instantiate
/// one directly, instead you must only construct one of it's derived classes with additional features.
class EntityItem : public std::enable_shared_from_this<EntityItem> {
    // These two classes manage lists of EntityItem pointers and must be able to cleanup pointers when an EntityItem is deleted.
    // To make the cleanup robust each EntityItem has backpointers to its manager classes (which are only ever set/cleared by
    // the managers themselves, hence they are fiends) whose NULL status can be used to determine which managers still need to
    // do cleanup.
    friend class EntityTreeElement;
    friend class EntitySimulation;
public:
    enum EntityDirtyFlags {
        DIRTY_POSITION = 0x0001,
        DIRTY_ROTATION = 0x0002,
        DIRTY_LINEAR_VELOCITY = 0x0004,
        DIRTY_ANGULAR_VELOCITY = 0x0008,
        DIRTY_MASS = 0x0010,
        DIRTY_COLLISION_GROUP = 0x0020,
        DIRTY_MOTION_TYPE = 0x0040,
        DIRTY_SHAPE = 0x0080,
        DIRTY_LIFETIME = 0x0100,
        DIRTY_UPDATEABLE = 0x0200,
        DIRTY_MATERIAL = 0x00400,
        DIRTY_PHYSICS_ACTIVATION = 0x0800, // should activate object in physics engine
        DIRTY_SIMULATOR_OWNERSHIP = 0x1000, // should claim simulator ownership
        DIRTY_SIMULATOR_ID = 0x2000, // the simulatorID has changed
        DIRTY_TRANSFORM = DIRTY_POSITION | DIRTY_ROTATION,
        DIRTY_VELOCITIES = DIRTY_LINEAR_VELOCITY | DIRTY_ANGULAR_VELOCITY
    };

    DONT_ALLOW_INSTANTIATION // This class can not be instantiated directly

    EntityItem(const EntityItemID& entityItemID);
    virtual ~EntityItem();

    // ID and EntityItemID related methods
    const QUuid& getID() const { return _id; }
    void setID(const QUuid& id) { _id = id; }
    EntityItemID getEntityItemID() const { return EntityItemID(_id); }

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties() const;

    /// returns true if something changed
    virtual bool setProperties(const EntityItemProperties& properties);

    /// Override this in your derived class if you'd like to be informed when something about the state of the entity
    /// has changed. This will be called with properties change or when new data is loaded from a stream
    virtual void somethingChangedNotification() { }

    void recordCreationTime();    // set _created to 'now'
    quint64 getLastSimulated() const { return _lastSimulated; } /// Last simulated time of this entity universal usecs
    void setLastSimulated(quint64 now) { _lastSimulated = now; }

     /// Last edited time of this entity universal usecs
    quint64 getLastEdited() const { return _lastEdited; }
    void setLastEdited(quint64 lastEdited)
        { _lastEdited = _lastUpdated = lastEdited; _changedOnServer = glm::max(lastEdited, _changedOnServer); }
    float getEditedAgo() const /// Elapsed seconds since this entity was last edited
        { return (float)(usecTimestampNow() - getLastEdited()) / (float)USECS_PER_SECOND; }

    /// Last time we sent out an edit packet for this entity
    quint64 getLastBroadcast() const { return _lastBroadcast; }
    void setLastBroadcast(quint64 lastBroadcast) { _lastBroadcast = lastBroadcast; }

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

    virtual bool addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene,
                            render::PendingChanges& pendingChanges) { return false; } // by default entity items don't add to scene
    virtual void removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene,
                                render::PendingChanges& pendingChanges) { } // by default entity items don't add to scene
    virtual void render(RenderArgs* args) { } // by default entity items don't know how to render

    static int expectedBytes();

    static void adjustEditPacketForClockSkew(QByteArray& buffer, int clockSkew);

    // perform update
    virtual void update(const quint64& now) { _lastUpdated = now; }
    quint64 getLastUpdated() const { return _lastUpdated; }

    // perform linear extrapolation for SimpleEntitySimulation
    void simulate(const quint64& now);
    void simulateKinematicMotion(float timeElapsed, bool setFlags=true);

    virtual bool needsToCallUpdate() const { return false; }

    virtual void debugDump() const;

    virtual bool supportsDetailedRayIntersection() const { return false; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                         void** intersectedObject, bool precisionPicking) const { return true; }

    // attributes applicable to all entity types
    EntityTypes::EntityType getType() const { return _type; }

    inline glm::vec3 getCenterPosition() const { return getTransformToCenter().getTranslation(); }
    void setCenterPosition(const glm::vec3& position);

    const Transform getTransformToCenter() const;
    void setTranformToCenter(const Transform& transform);

    inline const Transform& getTransform() const { return _transform; }
    inline void setTransform(const Transform& transform) { _transform = transform; }

    /// Position in meters (0.0 - TREE_SCALE)
    inline const glm::vec3& getPosition() const { return _transform.getTranslation(); }
    inline void setPosition(const glm::vec3& value) { _transform.setTranslation(value); }

    inline const glm::quat& getRotation() const { return _transform.getRotation(); }
    inline void setRotation(const glm::quat& rotation) { _transform.setRotation(rotation); }

    // Hyperlink related getters and setters
    QString getHref() const { return _href; }
    void setHref(QString value) { _href = value; }

    QString getDescription() const { return _description; }
    void setDescription(QString value) { _description = value; }

    /// Dimensions in meters (0.0 - TREE_SCALE)
    inline const glm::vec3& getDimensions() const { return _transform.getScale(); }
    virtual void setDimensions(const glm::vec3& value);

    float getGlowLevel() const { return _glowLevel; }
    void setGlowLevel(float glowLevel) { _glowLevel = glowLevel; }

    float getLocalRenderAlpha() const { return _localRenderAlpha; }
    void setLocalRenderAlpha(float localRenderAlpha) { _localRenderAlpha = localRenderAlpha; }

    void setDensity(float density);
    float computeMass() const;
    void setMass(float mass);

    float getDensity() const { return _density; }

    const glm::vec3& getVelocity() const { return _velocity; } /// get velocity in meters
    void setVelocity(const glm::vec3& value) { _velocity = value; } /// velocity in meters
    bool hasVelocity() const { return _velocity != ENTITY_ITEM_ZERO_VEC3; }

    const glm::vec3& getGravity() const { return _gravity; } /// get gravity in meters
    void setGravity(const glm::vec3& value) { _gravity = value; } /// gravity in meters
    bool hasGravity() const { return _gravity != ENTITY_ITEM_ZERO_VEC3; }

    const glm::vec3& getAcceleration() const { return _acceleration; } /// get acceleration in meters/second/second
    void setAcceleration(const glm::vec3& value) { _acceleration = value; } /// acceleration in meters/second/second
    bool hasAcceleration() const { return _acceleration != ENTITY_ITEM_ZERO_VEC3; }

    float getDamping() const { return _damping; }
    void setDamping(float value) { _damping = value; }

    float getRestitution() const { return _restitution; }
    void setRestitution(float value);

    float getFriction() const { return _friction; }
    void setFriction(float value);

    // lifetime related properties.
    float getLifetime() const { return _lifetime; } /// get the lifetime in seconds for the entity
    void setLifetime(float value) { _lifetime = value; } /// set the lifetime in seconds for the entity

    quint64 getCreated() const { return _created; } /// get the created-time in useconds for the entity
    void setCreated(quint64 value) { _created = value; } /// set the created-time in useconds for the entity

    /// is this entity immortal, in that it has no lifetime set, and will exist until manually deleted
    bool isImmortal() const { return _lifetime == ENTITY_ITEM_IMMORTAL_LIFETIME; }

    /// is this entity mortal, in that it has a lifetime set, and will automatically be deleted when that lifetime expires
    bool isMortal() const { return _lifetime != ENTITY_ITEM_IMMORTAL_LIFETIME; }

    /// age of this entity in seconds
    float getAge() const { return (float)(usecTimestampNow() - _created) / (float)USECS_PER_SECOND; }
    bool lifetimeHasExpired() const;
    quint64 getExpiry() const;

    // position, size, and bounds related helpers
    AACube getMaximumAACube() const;
    AACube getMinimumAACube() const;
    AABox getAABox() const; /// axis aligned bounding box in world-frame (meters)

    const QString& getScript() const { return _script; }
    void setScript(const QString& value) { _script = value; }

    quint64 getScriptTimestamp() const { return _scriptTimestamp; }
    void setScriptTimestamp(const quint64 value) { _scriptTimestamp = value; }

    const QString& getCollisionSoundURL() const { return _collisionSoundURL; }
    void setCollisionSoundURL(const QString& value) { _collisionSoundURL = value; }

    const glm::vec3& getRegistrationPoint() const { return _registrationPoint; } /// registration point as ratio of entity

    /// registration point as ratio of entity
    void setRegistrationPoint(const glm::vec3& value)
            { _registrationPoint = glm::clamp(value, 0.0f, 1.0f); }

    const glm::vec3& getAngularVelocity() const { return _angularVelocity; }
    void setAngularVelocity(const glm::vec3& value) { _angularVelocity = value; }
    bool hasAngularVelocity() const { return _angularVelocity != ENTITY_ITEM_ZERO_VEC3; }

    float getAngularDamping() const { return _angularDamping; }
    void setAngularDamping(float value) { _angularDamping = value; }

    QString getName() const { return _name; }
    void setName(const QString& value) { _name = value; }

    bool getVisible() const { return _visible; }
    void setVisible(bool value) { _visible = value; }
    bool isVisible() const { return _visible; }
    bool isInvisible() const { return !_visible; }

    bool getIgnoreForCollisions() const { return _ignoreForCollisions; }
    void setIgnoreForCollisions(bool value) { _ignoreForCollisions = value; }

    bool getCollisionsWillMove() const { return _collisionsWillMove; }
    void setCollisionsWillMove(bool value) { _collisionsWillMove = value; }

    virtual bool shouldBePhysical() const { return !_ignoreForCollisions; }

    bool getLocked() const { return _locked; }
    void setLocked(bool value) { _locked = value; }

    const QString& getUserData() const { return _userData; }
    void setUserData(const QString& value) { _userData = value; }

    const SimulationOwner& getSimulationOwner() const { return _simulationOwner; }
    void setSimulationOwner(const QUuid& id, quint8 priority);
    void setSimulationOwner(const SimulationOwner& owner);
    void promoteSimulationPriority(quint8 priority);

    quint8 getSimulationPriority() const { return _simulationOwner.getPriority(); }
    QUuid getSimulatorID() const { return _simulationOwner.getID(); }
    void updateSimulatorID(const QUuid& value);
    void clearSimulationOwnership();

    const QString& getMarketplaceID() const { return _marketplaceID; }
    void setMarketplaceID(const QString& value) { _marketplaceID = value; }

    // TODO: get rid of users of getRadius()...
    float getRadius() const;

    virtual bool contains(const glm::vec3& point) const;

    virtual bool isReadyToComputeShape() { return true; }
    virtual void computeShapeInfo(ShapeInfo& info);
    virtual float getVolumeEstimate() const { return getDimensions().x * getDimensions().y * getDimensions().z; }

    /// return preferred shape type (actual physical shape may differ)
    virtual ShapeType getShapeType() const { return SHAPE_TYPE_NONE; }

    // updateFoo() methods to be used when changes need to be accumulated in the _dirtyFlags
    void updatePosition(const glm::vec3& value);
    void updateDimensions(const glm::vec3& value);
    void updateRotation(const glm::quat& rotation);
    void updateDensity(float value);
    void updateMass(float value);
    void updateVelocity(const glm::vec3& value);
    void updateDamping(float value);
    void updateRestitution(float value);
    void updateFriction(float value);
    void updateGravity(const glm::vec3& value);
    void updateAngularVelocity(const glm::vec3& value);
    void updateAngularDamping(float value);
    void updateIgnoreForCollisions(bool value);
    void updateCollisionsWillMove(bool value);
    void updateLifetime(float value);
    void updateCreated(uint64_t value);
    virtual void updateShapeType(ShapeType type) { /* do nothing */ }

    uint32_t getDirtyFlags() const { return _dirtyFlags; }
    void clearDirtyFlags(uint32_t mask = 0xffffffff) { _dirtyFlags &= ~mask; }

    bool isMoving() const;

    void* getPhysicsInfo() const { return _physicsInfo; }

    void setPhysicsInfo(void* data) { _physicsInfo = data; }
    EntityTreeElement* getElement() const { return _element; }

    static void setSendPhysicsUpdates(bool value) { _sendPhysicsUpdates = value; }
    static bool getSendPhysicsUpdates() { return _sendPhysicsUpdates; }

    glm::mat4 getEntityToWorldMatrix() const;
    glm::mat4 getWorldToEntityMatrix() const;
    glm::vec3 worldToEntity(const glm::vec3& point) const;
    glm::vec3 entityToWorld(const glm::vec3& point) const;

    quint64 getLastEditedFromRemote() { return _lastEditedFromRemote; }

    void getAllTerseUpdateProperties(EntityItemProperties& properties) const;

    void flagForOwnership() { _dirtyFlags |= DIRTY_SIMULATOR_OWNERSHIP; }

    bool addAction(EntitySimulation* simulation, EntityActionPointer action);
    bool updateAction(EntitySimulation* simulation, const QUuid& actionID, const QVariantMap& arguments);
    bool removeAction(EntitySimulation* simulation, const QUuid& actionID);
    bool clearActions(EntitySimulation* simulation);
    void setActionData(QByteArray actionData);
    const QByteArray getActionData() const;
    bool hasActions() { return !_objectActions.empty(); }
    QList<QUuid> getActionIDs() { return _objectActions.keys(); }
    QVariantMap getActionArguments(const QUuid& actionID) const;
    void deserializeActions();
    void setActionDataDirty(bool value) const { _actionDataDirty = value; }

protected:

    const QByteArray getActionDataInternal() const;
    void setActionDataInternal(QByteArray actionData);

    static bool _sendPhysicsUpdates;
    EntityTypes::EntityType _type;
    QUuid _id;
    quint64 _lastSimulated; // last time this entity called simulate(), this includes velocity, angular velocity,
                            // and physics changes
    quint64 _lastUpdated; // last time this entity called update(), this includes animations and non-physics changes
    quint64 _lastEdited; // last official local or remote edit time
    quint64 _lastBroadcast; // the last time we sent an edit packet about this entity

    quint64 _lastEditedFromRemote; // last time we received and edit from the server
    quint64 _lastEditedFromRemoteInRemoteTime; // last time we received an edit from the server (in server-time-frame)
    quint64 _created;
    quint64 _changedOnServer;

    Transform _transform;
    float _glowLevel;
    float _localRenderAlpha;
    float _density = ENTITY_ITEM_DEFAULT_DENSITY; // kg/m^3
    // NOTE: _volumeMultiplier is used to allow some mass properties code exist in the EntityItem base class
    // rather than in all of the derived classes.  If we ever collapse these classes to one we could do it a
    // different way.
    float _volumeMultiplier = 1.0f;
    glm::vec3 _velocity;
    glm::vec3 _gravity;
    glm::vec3 _acceleration;
    float _damping;
    float _restitution;
    float _friction;
    float _lifetime;
    QString _script;
    quint64 _scriptTimestamp;
    QString _collisionSoundURL;
    glm::vec3 _registrationPoint;
    glm::vec3 _angularVelocity;
    float _angularDamping;
    bool _visible;
    bool _ignoreForCollisions;
    bool _collisionsWillMove;
    bool _locked;
    QString _userData;
    SimulationOwner _simulationOwner;
    QString _marketplaceID;
    QString _name;
    QString _href; //Hyperlink href
    QString _description; //Hyperlink description

    // NOTE: Damping is applied like this:  v *= pow(1 - damping, dt)
    //
    // Hence the damping coefficient must range from 0 (no damping) to 1 (immediate stop).
    // Each damping value relates to a corresponding exponential decay timescale as follows:
    //
    // timescale = -1 / ln(1 - damping)
    //
    // damping = 1 - exp(-1 / timescale)
    //

    // NOTE: Radius support is obsolete, but these private helper functions are available for this class to
    //       parse old data streams

    /// set radius in domain scale units (0.0 - 1.0) this will also reset dimensions to be equal for each axis
    void setRadius(float value);

    // DirtyFlags are set whenever a property changes that the EntitySimulation needs to know about.
    uint32_t _dirtyFlags;   // things that have changed from EXTERNAL changes (via script or packet) but NOT from simulation

    // these backpointers are only ever set/cleared by friends:
    EntityTreeElement* _element = nullptr; // set by EntityTreeElement
    void* _physicsInfo = nullptr; // set by EntitySimulation
    bool _simulated; // set by EntitySimulation

    bool addActionInternal(EntitySimulation* simulation, EntityActionPointer action);
    bool removeActionInternal(const QUuid& actionID, EntitySimulation* simulation = nullptr);
    void deserializeActionsInternal();
    QByteArray serializeActions(bool& success) const;
    QHash<QUuid, EntityActionPointer> _objectActions;

    static int _maxActionsDataSize;
    mutable QByteArray _allActionsDataCache;
    // when an entity-server starts up, EntityItem::setActionData is called before the entity-tree is
    // ready.  This means we can't find our EntityItemPointer or add the action to the simulation.  These
    // are used to keep track of and work around this situation.
    void checkWaitingToRemove(EntitySimulation* simulation = nullptr);
    mutable QSet<QUuid> _actionsToRemove;
    mutable bool _actionDataDirty = false;

    mutable QReadWriteLock _lock;
    void lockForRead() const;
    bool tryLockForRead() const;
    void lockForWrite() const;
    bool tryLockForWrite() const;
    void unlock() const;
    bool isLocked() const;
    bool isWriteLocked() const;
    bool isUnlocked() const;
};

#endif // hifi_EntityItem_h
