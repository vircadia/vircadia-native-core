//
//  EntityTree.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityTree_h
#define hifi_EntityTree_h

#include <QSet>
#include <QVector>

#include <Octree.h>
#include <SpatialParentFinder.h>

#include "AddEntityOperator.h"
#include "EntityTreeElement.h"
#include "DeleteEntityOperator.h"
#include "MovingEntitiesOperator.h"

class EntityTree;
using EntityTreePointer = std::shared_ptr<EntityTree>;

class EntitySimulation;

namespace EntityQueryFilterSymbol {
    static const QString NonDefault = "+";
}

class NewlyCreatedEntityHook {
public:
    virtual void entityCreated(const EntityItem& newEntity, const SharedNodePointer& senderNode) = 0;
};

class SendEntitiesOperationArgs {
public:
    glm::vec3 root;
    EntityTree* ourTree;
    EntityTreePointer otherTree;
    QHash<EntityItemID, EntityItemID>* map;
};


class EntityTree : public Octree, public SpatialParentTree {
    Q_OBJECT
public:
    enum FilterType {
        Add,
        Edit,
        Physics,
        Delete
    };
    EntityTree(bool shouldReaverage = false);
    virtual ~EntityTree();

    void createRootElement();


    void setEntityMaxTmpLifetime(float maxTmpEntityLifetime) { _maxTmpEntityLifetime = maxTmpEntityLifetime; }
    void setEntityScriptSourceWhitelist(const QString& entityScriptSourceWhitelist);

    /// Implements our type specific root element factory
    virtual OctreeElementPointer createNewElement(unsigned char* octalCode = NULL) override;

    /// Type safe version of getRoot()
    EntityTreeElementPointer getRoot() {
        if (!_rootElement) {
            createRootElement();
        }
        return std::static_pointer_cast<EntityTreeElement>(_rootElement);
    }

    virtual void eraseAllOctreeElements(bool createNewRoot = true) override;

    virtual void readBitstreamToTree(const unsigned char* bitstream,
            uint64_t bufferSizeBytes, ReadBitstreamToTreeParams& args) override;
    int readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);

    // These methods will allow the OctreeServer to send your tree inbound edit packets of your
    // own definition. Implement these to allow your octree based server to support editing
    virtual PacketType expectedDataPacketType() const override { return PacketType::EntityData; }
    virtual bool handlesEditPacketType(PacketType packetType) const override;
    void fixupTerseEditLogging(EntityItemProperties& properties, QList<QString>& changedProperties);
    virtual int processEditPacketData(ReceivedMessage& message, const unsigned char* editData, int maxLength,
                                      const SharedNodePointer& senderNode) override;
    virtual void processChallengeOwnershipRequestPacket(ReceivedMessage& message, const SharedNodePointer& sourceNode) override;
    virtual void processChallengeOwnershipReplyPacket(ReceivedMessage& message, const SharedNodePointer& sourceNode) override;
    virtual void processChallengeOwnershipPacket(ReceivedMessage& message, const SharedNodePointer& sourceNode) override;

    virtual EntityItemID findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        QVector<EntityItemID> entityIdsToInclude, QVector<EntityItemID> entityIdsToDiscard,
        bool visibleOnly, bool collidableOnly, bool precisionPicking, 
        OctreeElementPointer& element, float& distance,
        BoxFace& face, glm::vec3& surfaceNormal, QVariantMap& extraInfo,
        Octree::lockType lockType = Octree::TryLock, bool* accurateResult = NULL);

    virtual EntityItemID findParabolaIntersection(const PickParabola& parabola,
        QVector<EntityItemID> entityIdsToInclude, QVector<EntityItemID> entityIdsToDiscard,
        bool visibleOnly, bool collidableOnly, bool precisionPicking,
        OctreeElementPointer& element, glm::vec3& intersection, float& distance, float& parabolicDistance,
        BoxFace& face, glm::vec3& surfaceNormal, QVariantMap& extraInfo,
        Octree::lockType lockType = Octree::TryLock, bool* accurateResult = NULL);

    virtual bool rootElementHasData() const override { return true; }

    virtual void releaseSceneEncodeData(OctreeElementExtraEncodeData* extraEncodeData) const override;

    virtual void update() override { update(true); }

    void update(bool simulate);

    // The newer API...
    void postAddEntity(EntityItemPointer entityItem);

    EntityItemPointer addEntity(const EntityItemID& entityID, const EntityItemProperties& properties, bool isClone = false);

    // use this method if you only know the entityID
    bool updateEntity(const EntityItemID& entityID, const EntityItemProperties& properties, const SharedNodePointer& senderNode = SharedNodePointer(nullptr));

    // check if the avatar is a child of this entity, If so set the avatar parentID to null
    void unhookChildAvatar(const EntityItemID entityID);
    void cleanupCloneIDs(const EntityItemID& entityID);
    void deleteEntity(const EntityItemID& entityID, bool force = false, bool ignoreWarnings = true);
    void deleteEntities(QSet<EntityItemID> entityIDs, bool force = false, bool ignoreWarnings = true);

    /// \param position point of query in world-frame (meters)
    /// \param targetRadius radius of query (meters)
    EntityItemPointer findClosestEntity(const glm::vec3& position, float targetRadius);
    EntityItemPointer findEntityByID(const QUuid& id) const;
    EntityItemPointer findEntityByEntityItemID(const EntityItemID& entityID) const;
    virtual SpatiallyNestablePointer findByID(const QUuid& id) const override { return findEntityByID(id); }

    EntityItemID assignEntityID(const EntityItemID& entityItemID); /// Assigns a known ID for a creator token ID


    /// finds all entities that touch a sphere
    /// \param center the center of the sphere in world-frame (meters)
    /// \param radius the radius of the sphere in world-frame (meters)
    /// \param foundEntities[out] vector of EntityItemPointer
    /// \remark Side effect: any initial contents in foundEntities will be lost
    void findEntities(const glm::vec3& center, float radius, QVector<EntityItemPointer>& foundEntities);

    /// finds all entities that touch a cube
    /// \param cube the query cube in world-frame (meters)
    /// \param foundEntities[out] vector of non-EntityItemPointer
    /// \remark Side effect: any initial contents in entities will be lost
    void findEntities(const AACube& cube, QVector<EntityItemPointer>& foundEntities);

    /// finds all entities that touch a box
    /// \param box the query box in world-frame (meters)
    /// \param foundEntities[out] vector of non-EntityItemPointer
    /// \remark Side effect: any initial contents in entities will be lost
    void findEntities(const AABox& box, QVector<EntityItemPointer>& foundEntities);

    /// finds all entities within a frustum
    /// \parameter frustum the query frustum
    /// \param foundEntities[out] vector of EntityItemPointer
    void findEntities(const ViewFrustum& frustum, QVector<EntityItemPointer>& foundEntities);

    /// finds all entities that match scanOperator
    /// \parameter scanOperator function that scans entities that match criteria
    /// \parameter foundEntities[out] vector of EntityItemPointer
    void findEntities(RecurseOctreeOperation& scanOperator, QVector<EntityItemPointer>& foundEntities);

    void addNewlyCreatedHook(NewlyCreatedEntityHook* hook);
    void removeNewlyCreatedHook(NewlyCreatedEntityHook* hook);

    bool hasAnyDeletedEntities() const { 
        QReadLocker locker(&_recentlyDeletedEntitiesLock);
        return _recentlyDeletedEntityItemIDs.size() > 0;
    }

    bool hasEntitiesDeletedSince(quint64 sinceTime);
    static quint64 getAdjustedConsiderSince(quint64 sinceTime);

    QMultiMap<quint64, QUuid> getRecentlyDeletedEntityIDs() const { 
        QReadLocker locker(&_recentlyDeletedEntitiesLock);
        return _recentlyDeletedEntityItemIDs;
    }

    QHash<QString, EntityItemID> getEntityCertificateIDMap() const {
        QReadLocker locker(&_entityCertificateIDMapLock);
        return _entityCertificateIDMap;
    }

    void forgetEntitiesDeletedBefore(quint64 sinceTime);

    int processEraseMessage(ReceivedMessage& message, const SharedNodePointer& sourceNode);
    int processEraseMessageDetails(const QByteArray& buffer, const SharedNodePointer& sourceNode);
    bool shouldEraseEntity(EntityItemID entityID, const SharedNodePointer& sourceNode);


    EntityTreeElementPointer getContainingElement(const EntityItemID& entityItemID)  /*const*/;
    void addEntityMapEntry(EntityItemPointer entity);
    void clearEntityMapEntry(const EntityItemID& id);
    void debugDumpMap();
    virtual void dumpTree() override;
    virtual void pruneTree() override;

    static QByteArray remapActionDataIDs(QByteArray actionData, QHash<EntityItemID, EntityItemID>& map);

    QVector<EntityItemID> sendEntities(EntityEditPacketSender* packetSender, EntityTreePointer localTree,
                                       float x, float y, float z);

    void entityChanged(EntityItemPointer entity);

    void emitEntityScriptChanging(const EntityItemID& entityItemID, bool reload);
    void emitEntityServerScriptChanging(const EntityItemID& entityItemID, bool reload);

    void setSimulation(EntitySimulationPointer simulation);
    EntitySimulationPointer getSimulation() const { return _simulation; }

    bool wantEditLogging() const { return _wantEditLogging; }
    void setWantEditLogging(bool value) { _wantEditLogging = value; }

    bool wantTerseEditLogging() const { return _wantTerseEditLogging; }
    void setWantTerseEditLogging(bool value) { _wantTerseEditLogging = value; }

    virtual bool writeToMap(QVariantMap& entityDescription, OctreeElementPointer element, bool skipDefaultValues,
                            bool skipThoseWithBadParents) override;
    virtual bool readFromMap(QVariantMap& entityDescription) override;

    glm::vec3 getContentsDimensions();
    float getContentsLargestDimension();

    virtual void resetEditStats() override {
        _totalEditMessages = 0;
        _totalUpdates = 0;
        _totalCreates = 0;
        _totalDecodeTime = 0;
        _totalLookupTime = 0;
        _totalUpdateTime = 0;
        _totalCreateTime = 0;
        _totalLoggingTime = 0;
    }

    virtual quint64 getAverageDecodeTime() const override { return _totalEditMessages == 0 ? 0 : _totalDecodeTime / _totalEditMessages; }
    virtual quint64 getAverageLookupTime() const override { return _totalEditMessages == 0 ? 0 : _totalLookupTime / _totalEditMessages; }
    virtual quint64 getAverageUpdateTime() const override { return _totalUpdates == 0 ? 0 : _totalUpdateTime / _totalUpdates; }
    virtual quint64 getAverageCreateTime() const override { return _totalCreates == 0 ? 0 : _totalCreateTime / _totalCreates; }
    virtual quint64 getAverageLoggingTime() const override { return _totalEditMessages == 0 ? 0 : _totalLoggingTime / _totalEditMessages; }
    virtual quint64 getAverageFilterTime() const override { return _totalEditMessages == 0 ? 0 : _totalFilterTime / _totalEditMessages; }

    void trackIncomingEntityLastEdited(quint64 lastEditedTime, int bytesRead);
    quint64 getAverageEditDeltas() const
        { return _totalTrackedEdits == 0 ? 0 : _totalEditDeltas / _totalTrackedEdits; }
    quint64 getAverageEditBytes() const
        { return _totalTrackedEdits == 0 ? 0 : _totalEditBytes / _totalTrackedEdits; }
    quint64 getMaxEditDelta() const { return _maxEditDelta; }
    quint64 getTotalTrackedEdits() const { return _totalTrackedEdits; }

    EntityTreePointer getThisPointer() { return std::static_pointer_cast<EntityTree>(shared_from_this()); }

    bool isDeletedEntity(const QUuid& id) {
        QReadLocker locker(&_deletedEntitiesLock);
        return _deletedEntityItemIDs.contains(id);
    }

    // these are used to call through to EntityItems
    Q_INVOKABLE int getJointIndex(const QUuid& entityID, const QString& name) const;
    Q_INVOKABLE QStringList getJointNames(const QUuid& entityID) const;

    void knowAvatarID(QUuid avatarID) { _avatarIDs += avatarID; }
    void forgetAvatarID(QUuid avatarID) { _avatarIDs -= avatarID; }
    void deleteDescendantsOfAvatar(QUuid avatarID);
    void removeFromChildrenOfAvatars(EntityItemPointer entity);

    void addToNeedsParentFixupList(EntityItemPointer entity);

    void notifyNewCollisionSoundURL(const QString& newCollisionSoundURL, const EntityItemID& entityID);

    static const float DEFAULT_MAX_TMP_ENTITY_LIFETIME;

    QByteArray computeNonce(const QString& certID, const QString ownerKey);
    bool verifyNonce(const QString& certID, const QString& nonce, EntityItemID& id);

    void setMyAvatar(std::shared_ptr<AvatarData> myAvatar) { _myAvatar = myAvatar; }

    void swapStaleProxies(std::vector<int>& proxies) { proxies.swap(_staleProxies); }

    void setIsServerlessMode(bool value) { _serverlessDomain = value; }
    bool isServerlessMode() const { return _serverlessDomain; }

    static void setAddMaterialToEntityOperator(std::function<bool(const QUuid&, graphics::MaterialLayer, const std::string&)> addMaterialToEntityOperator) { _addMaterialToEntityOperator = addMaterialToEntityOperator; }
    static void setRemoveMaterialFromEntityOperator(std::function<bool(const QUuid&, graphics::MaterialPointer, const std::string&)> removeMaterialFromEntityOperator) { _removeMaterialFromEntityOperator = removeMaterialFromEntityOperator; }
    static bool addMaterialToEntity(const QUuid& entityID, graphics::MaterialLayer material, const std::string& parentMaterialName);
    static bool removeMaterialFromEntity(const QUuid& entityID, graphics::MaterialPointer material, const std::string& parentMaterialName);

    static void setAddMaterialToAvatarOperator(std::function<bool(const QUuid&, graphics::MaterialLayer, const std::string&)> addMaterialToAvatarOperator) { _addMaterialToAvatarOperator = addMaterialToAvatarOperator; }
    static void setRemoveMaterialFromAvatarOperator(std::function<bool(const QUuid&, graphics::MaterialPointer, const std::string&)> removeMaterialFromAvatarOperator) { _removeMaterialFromAvatarOperator = removeMaterialFromAvatarOperator; }
    static bool addMaterialToAvatar(const QUuid& avatarID, graphics::MaterialLayer material, const std::string& parentMaterialName);
    static bool removeMaterialFromAvatar(const QUuid& avatarID, graphics::MaterialPointer material, const std::string& parentMaterialName);

    static void setAddMaterialToOverlayOperator(std::function<bool(const QUuid&, graphics::MaterialLayer, const std::string&)> addMaterialToOverlayOperator) { _addMaterialToOverlayOperator = addMaterialToOverlayOperator; }
    static void setRemoveMaterialFromOverlayOperator(std::function<bool(const QUuid&, graphics::MaterialPointer, const std::string&)> removeMaterialFromOverlayOperator) { _removeMaterialFromOverlayOperator = removeMaterialFromOverlayOperator; }
    static bool addMaterialToOverlay(const QUuid& overlayID, graphics::MaterialLayer material, const std::string& parentMaterialName);
    static bool removeMaterialFromOverlay(const QUuid& overlayID, graphics::MaterialPointer material, const std::string& parentMaterialName);

    std::map<QString, QString> getNamedPaths() const { return _namedPaths; }

signals:
    void deletingEntity(const EntityItemID& entityID);
    void deletingEntityPointer(EntityItem* entityID);
    void addingEntity(const EntityItemID& entityID);
    void editingEntityPointer(const EntityItemPointer& entityID);
    void entityScriptChanging(const EntityItemID& entityItemID, const bool reload);
    void entityServerScriptChanging(const EntityItemID& entityItemID, const bool reload);
    void newCollisionSoundURL(const QUrl& url, const EntityItemID& entityID);
    void clearingEntities();
    void killChallengeOwnershipTimeoutTimer(const QString& certID);

protected:

    void processRemovedEntities(const DeleteEntityOperator& theOperator);
    bool updateEntity(EntityItemPointer entity, const EntityItemProperties& properties,
            const SharedNodePointer& senderNode = SharedNodePointer(nullptr));
    static bool findNearPointOperation(const OctreeElementPointer& element, void* extraData);
    static bool findInSphereOperation(const OctreeElementPointer& element, void* extraData);
    static bool findInCubeOperation(const OctreeElementPointer& element, void* extraData);
    static bool findInBoxOperation(const OctreeElementPointer& element, void* extraData);
    static bool findInFrustumOperation(const OctreeElementPointer& element, void* extraData);
    static bool sendEntitiesOperation(const OctreeElementPointer& element, void* extraData);
    static void bumpTimestamp(EntityItemProperties& properties);

    void notifyNewlyCreatedEntity(const EntityItem& newEntity, const SharedNodePointer& senderNode);

    bool isScriptInWhitelist(const QString& scriptURL);

    QReadWriteLock _newlyCreatedHooksLock;
    QVector<NewlyCreatedEntityHook*> _newlyCreatedHooks;

    mutable QReadWriteLock _recentlyDeletedEntitiesLock; /// lock of server side recent deletes
    QMultiMap<quint64, QUuid> _recentlyDeletedEntityItemIDs; /// server side recent deletes

    mutable QReadWriteLock _deletedEntitiesLock; /// lock of client side recent deletes
    QSet<QUuid> _deletedEntityItemIDs; /// client side recent deletes

    void clearDeletedEntities() {
        QWriteLocker locker(&_deletedEntitiesLock);
        _deletedEntityItemIDs.clear();
    }

    void trackDeletedEntity(const QUuid& id) {
        QWriteLocker locker(&_deletedEntitiesLock);
        _deletedEntityItemIDs << id;
    }

    mutable QReadWriteLock _entityMapLock;
    QHash<EntityItemID, EntityItemPointer> _entityMap;

    mutable QReadWriteLock _entityCertificateIDMapLock;
    QHash<QString, EntityItemID> _entityCertificateIDMap;

    mutable QReadWriteLock _certNonceMapLock;
    QHash<QString, QPair<QUuid, QString>> _certNonceMap;

    EntitySimulationPointer _simulation;

    bool _wantEditLogging = false;
    bool _wantTerseEditLogging = false;


    // some performance tracking properties - only used in server trees
    int _totalEditMessages = 0;
    int _totalUpdates = 0;
    int _totalCreates = 0;
    quint64 _totalDecodeTime = 0;
    quint64 _totalLookupTime = 0;
    quint64 _totalUpdateTime = 0;
    quint64 _totalCreateTime = 0;
    quint64 _totalLoggingTime = 0;
    quint64 _totalFilterTime = 0;

    // these performance statistics are only used in the client
    void resetClientEditStats();
    int _totalTrackedEdits = 0;
    quint64 _totalEditBytes = 0;
    quint64 _totalEditDeltas = 0;
    quint64 _maxEditDelta = 0;
    quint64 _treeResetTime = 0;

    void fixupNeedsParentFixups(); // try to hook members of _needsParentFixup to parent instances
    QVector<EntityItemWeakPointer> _needsParentFixup; // entites with a parentID but no (yet) known parent instance
    mutable QReadWriteLock _needsParentFixupLock;

    // we maintain a list of avatarIDs to notice when an entity is a child of one.
    QSet<QUuid> _avatarIDs; // IDs of avatars connected to entity server
    QHash<QUuid, QSet<EntityItemID>> _childrenOfAvatars;  // which entities are children of which avatars

    float _maxTmpEntityLifetime { DEFAULT_MAX_TMP_ENTITY_LIFETIME };

    bool filterProperties(EntityItemPointer& existingEntity, EntityItemProperties& propertiesIn, EntityItemProperties& propertiesOut, bool& wasChanged, FilterType filterType);
    bool _hasEntityEditFilter{ false };
    QStringList _entityScriptSourceWhitelist;

    MovingEntitiesOperator _entityMover;
    QHash<EntityItemID, EntityItemPointer> _entitiesToAdd;

    Q_INVOKABLE void startChallengeOwnershipTimer(const EntityItemID& entityItemID);

private:
    void sendChallengeOwnershipPacket(const QString& certID, const QString& ownerKey, const EntityItemID& entityItemID, const SharedNodePointer& senderNode);
    void sendChallengeOwnershipRequestPacket(const QByteArray& certID, const QByteArray& text, const QByteArray& nodeToChallenge, const SharedNodePointer& senderNode);
    void validatePop(const QString& certID, const EntityItemID& entityItemID, const SharedNodePointer& senderNode);

    std::shared_ptr<AvatarData> _myAvatar{ nullptr };

    static std::function<bool(const QUuid&, graphics::MaterialLayer, const std::string&)> _addMaterialToEntityOperator;
    static std::function<bool(const QUuid&, graphics::MaterialPointer, const std::string&)> _removeMaterialFromEntityOperator;
    static std::function<bool(const QUuid&, graphics::MaterialLayer, const std::string&)> _addMaterialToAvatarOperator;
    static std::function<bool(const QUuid&, graphics::MaterialPointer, const std::string&)> _removeMaterialFromAvatarOperator;
    static std::function<bool(const QUuid&, graphics::MaterialLayer, const std::string&)> _addMaterialToOverlayOperator;
    static std::function<bool(const QUuid&, graphics::MaterialPointer, const std::string&)> _removeMaterialFromOverlayOperator;

    std::vector<int32_t> _staleProxies;

    bool _serverlessDomain { false };

    std::map<QString, QString> _namedPaths;
};

#endif // hifi_EntityTree_h
