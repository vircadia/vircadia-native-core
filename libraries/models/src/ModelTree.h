//
//  ModelTree.h
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelTree_h
#define hifi_ModelTree_h

#include <Octree.h>
#include "ModelTreeElement.h"

class NewlyCreatedModelHook {
public:
    virtual void modelCreated(const ModelItem& newModel, const SharedNodePointer& senderNode) = 0;
};

class ModelItemFBXService {
public:
    virtual const FBXGeometry* getGeometryForModel(const ModelItem& modelItem) = 0;
};

class ModelTree : public Octree {
    Q_OBJECT
public:
    ModelTree(bool shouldReaverage = false);

    /// Implements our type specific root element factory
    virtual ModelTreeElement* createNewElement(unsigned char * octalCode = NULL);

    /// Type safe version of getRoot()
    ModelTreeElement* getRoot() { return static_cast<ModelTreeElement*>(_rootElement); }

    ModelTreeElement* getModelAt(float x, float y, float z, float s) const;

    // These methods will allow the OctreeServer to send your tree inbound edit packets of your
    // own definition. Implement these to allow your octree based server to support editing
    virtual bool getWantSVOfileVersions() const { return true; }
    virtual PacketType expectedDataPacketType() const { return PacketTypeModelData; }
    virtual bool canProcessVersion(PacketVersion thisVersion) const { return true; } // we support all versions
    virtual bool handlesEditPacketType(PacketType packetType) const;
    virtual int processEditPacketData(PacketType packetType, const unsigned char* packetData, int packetLength,
                    const unsigned char* editData, int maxLength, const SharedNodePointer& senderNode);

    virtual bool rootElementHasData() const { return true; }
    virtual void update();

    void storeModel(const ModelItem& model, const SharedNodePointer& senderNode = SharedNodePointer());
    void updateModel(const ModelItemID& modelID, const ModelItemProperties& properties);
    void addModel(const ModelItemID& modelID, const ModelItemProperties& properties);
    void deleteModel(const ModelItemID& modelID);
    const ModelItem* findClosestModel(glm::vec3 position, float targetRadius);
    const ModelItem* findModelByID(uint32_t id, bool alreadyLocked = false);

    /// finds all models that touch a sphere
    /// \param center the center of the sphere
    /// \param radius the radius of the sphere
    /// \param foundModels[out] vector of const ModelItem*
    /// \remark Side effect: any initial contents in foundModels will be lost
    void findModels(const glm::vec3& center, float radius, QVector<const ModelItem*>& foundModels);
    void findModelsInCube(const AACube& cube, QVector<ModelItem*>& foundModels);

    /// finds all models that touch a cube
    /// \param cube the query cube
    /// \param foundModels[out] vector of non-const ModelItem*
    /// \remark Side effect: any initial contents in models will be lost
    void findModelsForUpdate(const AACube& cube, QVector<ModelItem*>& foundModels);

    void addNewlyCreatedHook(NewlyCreatedModelHook* hook);
    void removeNewlyCreatedHook(NewlyCreatedModelHook* hook);

    bool hasAnyDeletedModels() const { return _recentlyDeletedModelItemIDs.size() > 0; }
    bool hasModelsDeletedSince(quint64 sinceTime);
    bool encodeModelsDeletedSince(OCTREE_PACKET_SEQUENCE sequenceNumber, quint64& sinceTime, unsigned char* packetData, size_t maxLength, size_t& outputLength);
    void forgetModelsDeletedBefore(quint64 sinceTime);

    void processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);
    void handleAddModelResponse(const QByteArray& packet);
    
    void setFBXService(ModelItemFBXService* service) { _fbxService = service; }
    const FBXGeometry* getGeometryForModel(const ModelItem& modelItem) {
        return _fbxService ? _fbxService->getGeometryForModel(modelItem) : NULL;

    }
    void sendModels(ModelEditPacketSender* packetSender, float x, float y, float z);

private:

    static bool sendModelsOperation(OctreeElement* element, void* extraData);
    static bool updateOperation(OctreeElement* element, void* extraData);
    static bool findInCubeOperation(OctreeElement* element, void* extraData);
    static bool findAndUpdateOperation(OctreeElement* element, void* extraData);
    static bool findAndUpdateWithIDandPropertiesOperation(OctreeElement* element, void* extraData);
    static bool findNearPointOperation(OctreeElement* element, void* extraData);
    static bool findInSphereOperation(OctreeElement* element, void* extraData);
    static bool pruneOperation(OctreeElement* element, void* extraData);
    static bool findByIDOperation(OctreeElement* element, void* extraData);
    static bool findAndDeleteOperation(OctreeElement* element, void* extraData);
    static bool findAndUpdateModelItemIDOperation(OctreeElement* element, void* extraData);
    static bool findInCubeForUpdateOperation(OctreeElement* element, void* extraData);

    void notifyNewlyCreatedModel(const ModelItem& newModel, const SharedNodePointer& senderNode);

    QReadWriteLock _newlyCreatedHooksLock;
    std::vector<NewlyCreatedModelHook*> _newlyCreatedHooks;


    QReadWriteLock _recentlyDeletedModelsLock;
    QMultiMap<quint64, uint32_t> _recentlyDeletedModelItemIDs;
    ModelItemFBXService* _fbxService;
};

#endif // hifi_ModelTree_h
