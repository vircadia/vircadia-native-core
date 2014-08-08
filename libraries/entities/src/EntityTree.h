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

#include <Octree.h>
#include "EntityTreeElement.h"


class Model;

class NewlyCreatedEntityHook {
public:
    virtual void entityCreated(const EntityItem& newEntity, const SharedNodePointer& senderNode) = 0;
};

class EntityItemFBXService {
public:
    virtual const FBXGeometry* getGeometryForEntity(const EntityItem* entityItem) = 0;
    virtual const Model* getModelForEntityItem(const EntityItem* entityItem) = 0;    
};


class SendEntitiesOperationArgs {
public:
    glm::vec3 root;
    EntityEditPacketSender* packetSender;
};


class EntityTree : public Octree {
    Q_OBJECT
public:
    EntityTree(bool shouldReaverage = false);

    /// Implements our type specific root element factory
    virtual EntityTreeElement* createNewElement(unsigned char * octalCode = NULL);

    /// Type safe version of getRoot()
    EntityTreeElement* getRoot() { return static_cast<EntityTreeElement*>(_rootElement); }

    virtual void eraseAllOctreeElements();

    // These methods will allow the OctreeServer to send your tree inbound edit packets of your
    // own definition. Implement these to allow your octree based server to support editing
    virtual bool getWantSVOfileVersions() const { return true; }
    virtual PacketType expectedDataPacketType() const { return PacketTypeEntityData; }
    virtual bool canProcessVersion(PacketVersion thisVersion) const { return true; } // we support all versions
    virtual bool handlesEditPacketType(PacketType packetType) const;
    virtual int processEditPacketData(PacketType packetType, const unsigned char* packetData, int packetLength,
                    const unsigned char* editData, int maxLength, const SharedNodePointer& senderNode);

    virtual bool rootElementHasData() const { return true; }
    virtual void update();

    // The newer API...
    EntityItem* getOrCreateEntityItem(const EntityItemID& entityID, const EntityItemProperties& properties);
    void addEntityItem(EntityItem* entityItem);

    EntityItem* addEntity(const EntityItemID& entityID, const EntityItemProperties& properties);
    bool updateEntity(const EntityItemID& entityID, const EntityItemProperties& properties);
    void deleteEntity(const EntityItemID& entityID);
    void deleteEntities(QSet<EntityItemID> entityIDs);

    const EntityItem* findClosestEntity(glm::vec3 position, float targetRadius);
    EntityItem* findEntityByID(const QUuid& id);
    EntityItem* findEntityByEntityItemID(const EntityItemID& entityID);

    EntityItemID assignEntityID(const EntityItemID& entityItemID); /// Assigns a known ID for a creator token ID


    /// finds all entities that touch a sphere
    /// \param center the center of the sphere
    /// \param radius the radius of the sphere
    /// \param foundEntities[out] vector of const EntityItem*
    /// \remark Side effect: any initial contents in foundEntities will be lost
    void findEntities(const glm::vec3& center, float radius, QVector<const EntityItem*>& foundEntities);

    /// finds all entities that touch a cube
    /// \param cube the query cube
    /// \param foundEntities[out] vector of non-const EntityItem*
    /// \remark Side effect: any initial contents in entities will be lost
    void findEntities(const AACube& cube, QVector<EntityItem*> foundEntities);

    void addNewlyCreatedHook(NewlyCreatedEntityHook* hook);
    void removeNewlyCreatedHook(NewlyCreatedEntityHook* hook);

    bool hasAnyDeletedEntities() const { return _recentlyDeletedEntityItemIDs.size() > 0; }
    bool hasEntitiesDeletedSince(quint64 sinceTime);
    bool encodeEntitiesDeletedSince(OCTREE_PACKET_SEQUENCE sequenceNumber, quint64& sinceTime, 
                                    unsigned char* packetData, size_t maxLength, size_t& outputLength);
    void forgetEntitiesDeletedBefore(quint64 sinceTime);

    void processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);
    void handleAddEntityResponse(const QByteArray& packet);
    
    EntityItemFBXService* getFBXService() const { return _fbxService; }
    void setFBXService(EntityItemFBXService* service) { _fbxService = service; }
    const FBXGeometry* getGeometryForEntity(const EntityItem* entityItem) {
        return _fbxService ? _fbxService->getGeometryForEntity(entityItem) : NULL;
    }
    
    EntityTreeElement* getContainingElement(const EntityItemID& entityItemID)  /*const*/;
    void setContainingElement(const EntityItemID& entityItemID, EntityTreeElement* element);
    void resetContainingElement(const EntityItemID& entityItemID, EntityTreeElement* element);
    void debugDumpMap();
    void dumpTree();

    void rememberDirtyCube(const AACube& cube);
    void rememberEntityToMove(const EntityItem* entity);
    void sendEntities(EntityEditPacketSender* packetSender, float x, float y, float z);

    void changeEntityState(EntityItem* const entity, EntityItem::SimuationState oldState, EntityItem::SimuationState newState);

private:

    static bool updateOperation(OctreeElement* element, void* extraData);
    static bool findAndUpdateOperation(OctreeElement* element, void* extraData);
    static bool findAndUpdateWithIDandPropertiesOperation(OctreeElement* element, void* extraData);
    static bool findNearPointOperation(OctreeElement* element, void* extraData);
    static bool findInSphereOperation(OctreeElement* element, void* extraData);
    static bool pruneOperation(OctreeElement* element, void* extraData);
    static bool findByIDOperation(OctreeElement* element, void* extraData);
    static bool findAndDeleteOperation(OctreeElement* element, void* extraData);
    static bool findAndUpdateEntityItemIDOperation(OctreeElement* element, void* extraData);
    static bool findInCubeOperation(OctreeElement* element, void* extraData);
    static bool sendEntitiesOperation(OctreeElement* element, void* extraData);

    void notifyNewlyCreatedEntity(const EntityItem& newEntity, const SharedNodePointer& senderNode);

    QReadWriteLock _newlyCreatedHooksLock;
    QVector<NewlyCreatedEntityHook*> _newlyCreatedHooks;

    QReadWriteLock _recentlyDeletedEntitiesLock;
    QMultiMap<quint64, uint32_t> _recentlyDeletedEntityItemIDs;
    EntityItemFBXService* _fbxService;

    QHash<EntityItemID, EntityTreeElement*> _entityToElementMap;


    QList<EntityItem*> _movingEntities; // entities that are moving as part of update
    QList<EntityItem*> _changingEntities; // entities that are changing (like animating), but not moving
};

#endif // hifi_EntityTree_h
