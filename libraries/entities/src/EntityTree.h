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

#include "EntityTreeElement.h"
#include "DeleteEntityOperator.h"

class Model;
class EntitySimulation;

class NewlyCreatedEntityHook {
public:
    virtual void entityCreated(const EntityItem& newEntity, const SharedNodePointer& senderNode) = 0;
};

class EntityItemFBXService {
public:
    virtual const FBXGeometry* getGeometryForEntity(EntityItemPointer entityItem) = 0;
    virtual const Model* getModelForEntityItem(EntityItemPointer entityItem) = 0;
    virtual const FBXGeometry* getCollisionGeometryForEntity(EntityItemPointer entityItem) = 0;
};


class SendEntitiesOperationArgs {
public:
    glm::vec3 root;
    EntityTree* localTree;
    EntityEditPacketSender* packetSender;
    QVector<EntityItemID>* newEntityIDs;
};


class EntityTree : public Octree {
    Q_OBJECT
public:
    EntityTree(bool shouldReaverage = false);
    virtual ~EntityTree();

    /// Implements our type specific root element factory
    virtual EntityTreeElement* createNewElement(unsigned char * octalCode = NULL);

    /// Type safe version of getRoot()
    EntityTreeElement* getRoot() { return static_cast<EntityTreeElement*>(_rootElement); }

    virtual void eraseAllOctreeElements(bool createNewRoot = true);

    // These methods will allow the OctreeServer to send your tree inbound edit packets of your
    // own definition. Implement these to allow your octree based server to support editing
    virtual bool getWantSVOfileVersions() const { return true; }
    virtual PacketType expectedDataPacketType() const { return PacketType::EntityData; }
    virtual bool canProcessVersion(PacketVersion thisVersion) const
                    { return thisVersion >= VERSION_ENTITIES_USE_METERS_AND_RADIANS; }
    virtual bool handlesEditPacketType(PacketType packetType) const;
    virtual int processEditPacketData(NLPacket& packet, const unsigned char* editData, int maxLength,
                                      const SharedNodePointer& senderNode);

    virtual bool rootElementHasData() const { return true; }

    // the root at least needs to store the number of entities in the packet/buffer
    virtual int minimumRequiredRootDataBytes() const { return sizeof(uint16_t); }
    virtual bool suppressEmptySubtrees() const { return false; }
    virtual void releaseSceneEncodeData(OctreeElementExtraEncodeData* extraEncodeData) const;
    virtual bool mustIncludeAllChildData() const { return false; }

    virtual bool versionHasSVOfileBreaks(PacketVersion thisVersion) const
                    { return thisVersion >= VERSION_ENTITIES_HAS_FILE_BREAKS; }

    virtual void update();

    // The newer API...
    void postAddEntity(EntityItemPointer entityItem);

    EntityItemPointer addEntity(const EntityItemID& entityID, const EntityItemProperties& properties);

    // use this method if you only know the entityID
    bool updateEntity(const EntityItemID& entityID, const EntityItemProperties& properties, const SharedNodePointer& senderNode = SharedNodePointer(nullptr));

    // use this method if you have a pointer to the entity (avoid an extra entity lookup)
    bool updateEntity(EntityItemPointer entity, const EntityItemProperties& properties, const SharedNodePointer& senderNode = SharedNodePointer(nullptr));

    void deleteEntity(const EntityItemID& entityID, bool force = false, bool ignoreWarnings = false);
    void deleteEntities(QSet<EntityItemID> entityIDs, bool force = false, bool ignoreWarnings = false);

    /// \param position point of query in world-frame (meters)
    /// \param targetRadius radius of query (meters)
    EntityItemPointer findClosestEntity(glm::vec3 position, float targetRadius);
    EntityItemPointer findEntityByID(const QUuid& id);
    EntityItemPointer findEntityByEntityItemID(const EntityItemID& entityID);

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

    void addNewlyCreatedHook(NewlyCreatedEntityHook* hook);
    void removeNewlyCreatedHook(NewlyCreatedEntityHook* hook);

    bool hasAnyDeletedEntities() const { return _recentlyDeletedEntityItemIDs.size() > 0; }
    bool hasEntitiesDeletedSince(quint64 sinceTime);
    std::unique_ptr<NLPacket> encodeEntitiesDeletedSince(OCTREE_PACKET_SEQUENCE sequenceNumber, quint64& sinceTime,
                                                         bool& hasMore);
    void forgetEntitiesDeletedBefore(quint64 sinceTime);

    int processEraseMessage(NLPacket& packet, const SharedNodePointer& sourceNode);
    int processEraseMessageDetails(const QByteArray& buffer, const SharedNodePointer& sourceNode);

    EntityItemFBXService* getFBXService() const { return _fbxService; }
    void setFBXService(EntityItemFBXService* service) { _fbxService = service; }
    const FBXGeometry* getGeometryForEntity(EntityItemPointer entityItem) {
        return _fbxService ? _fbxService->getGeometryForEntity(entityItem) : NULL;
    }
    const Model* getModelForEntityItem(EntityItemPointer entityItem) {
        return _fbxService ? _fbxService->getModelForEntityItem(entityItem) : NULL;
    }

    EntityTreeElement* getContainingElement(const EntityItemID& entityItemID)  /*const*/;
    void setContainingElement(const EntityItemID& entityItemID, EntityTreeElement* element);
    void debugDumpMap();
    virtual void dumpTree();
    virtual void pruneTree();

    QVector<EntityItemID> sendEntities(EntityEditPacketSender* packetSender, EntityTree* localTree, float x, float y, float z);

    void entityChanged(EntityItemPointer entity);

    void emitEntityScriptChanging(const EntityItemID& entityItemID, const bool reload);

    void setSimulation(EntitySimulation* simulation);
    EntitySimulation* getSimulation() const { return _simulation; }

    bool wantEditLogging() const { return _wantEditLogging; }
    void setWantEditLogging(bool value) { _wantEditLogging = value; }

    bool writeToMap(QVariantMap& entityDescription, OctreeElement* element, bool skipDefaultValues);
    bool readFromMap(QVariantMap& entityDescription);

    float getContentsLargestDimension();

    virtual void resetEditStats() {
        _totalEditMessages = 0;
        _totalUpdates = 0;
        _totalCreates = 0;
        _totalDecodeTime = 0;
        _totalLookupTime = 0;
        _totalUpdateTime = 0;
        _totalCreateTime = 0;
        _totalLoggingTime = 0;
    }

    virtual quint64 getAverageDecodeTime() const { return _totalEditMessages == 0 ? 0 : _totalDecodeTime / _totalEditMessages; }
    virtual quint64 getAverageLookupTime() const { return _totalEditMessages == 0 ? 0 : _totalLookupTime / _totalEditMessages; }
    virtual quint64 getAverageUpdateTime() const { return _totalUpdates == 0 ? 0 : _totalUpdateTime / _totalUpdates; }
    virtual quint64 getAverageCreateTime() const { return _totalCreates == 0 ? 0 : _totalCreateTime / _totalCreates; }
    virtual quint64 getAverageLoggingTime() const { return _totalEditMessages == 0 ? 0 : _totalLoggingTime / _totalEditMessages; }

    void trackIncomingEntityLastEdited(quint64 lastEditedTime, int bytesRead);
    quint64 getAverageEditDeltas() const
        { return _totalTrackedEdits == 0 ? 0 : _totalEditDeltas / _totalTrackedEdits; }
    quint64 getAverageEditBytes() const
        { return _totalTrackedEdits == 0 ? 0 : _totalEditBytes / _totalTrackedEdits; }
    quint64 getMaxEditDelta() const { return _maxEditDelta; }
    quint64 getTotalTrackedEdits() const { return _totalTrackedEdits; }

signals:
    void deletingEntity(const EntityItemID& entityID);
    void addingEntity(const EntityItemID& entityID);
    void entityScriptChanging(const EntityItemID& entityItemID, const bool reload);
    void newCollisionSoundURL(const QUrl& url);
    void clearingEntities();

private:

    void processRemovedEntities(const DeleteEntityOperator& theOperator);
    bool updateEntityWithElement(EntityItemPointer entity, const EntityItemProperties& properties,
                                 EntityTreeElement* containingElement,
                                 const SharedNodePointer& senderNode = SharedNodePointer(nullptr));
    static bool findNearPointOperation(OctreeElement* element, void* extraData);
    static bool findInSphereOperation(OctreeElement* element, void* extraData);
    static bool findInCubeOperation(OctreeElement* element, void* extraData);
    static bool findInBoxOperation(OctreeElement* element, void* extraData);
    static bool sendEntitiesOperation(OctreeElement* element, void* extraData);

    void notifyNewlyCreatedEntity(const EntityItem& newEntity, const SharedNodePointer& senderNode);

    QReadWriteLock _newlyCreatedHooksLock;
    QVector<NewlyCreatedEntityHook*> _newlyCreatedHooks;

    QReadWriteLock _recentlyDeletedEntitiesLock;
    QMultiMap<quint64, QUuid> _recentlyDeletedEntityItemIDs;
    EntityItemFBXService* _fbxService;

    QHash<EntityItemID, EntityTreeElement*> _entityToElementMap;

    EntitySimulation* _simulation;

    bool _wantEditLogging = false;
    void maybeNotifyNewCollisionSoundURL(const QString& oldCollisionSoundURL, const QString& newCollisionSoundURL);


    // some performance tracking properties - only used in server trees
    int _totalEditMessages = 0;
    int _totalUpdates = 0;
    int _totalCreates = 0;
    quint64 _totalDecodeTime = 0;
    quint64 _totalLookupTime = 0;
    quint64 _totalUpdateTime = 0;
    quint64 _totalCreateTime = 0;
    quint64 _totalLoggingTime = 0;

    // these performance statistics are only used in the client
    void resetClientEditStats();
    int _totalTrackedEdits = 0;
    quint64 _totalEditBytes = 0;
    quint64 _totalEditDeltas = 0;
    quint64 _maxEditDelta = 0;
    quint64 _treeResetTime = 0;
};

#endif // hifi_EntityTree_h
