//
//  EntityTreeElement.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityTreeElement_h
#define hifi_EntityTreeElement_h

#include <memory>

#include <OctreeElement.h>
#include <QList>

#include "EntityEditPacketSender.h"
#include "EntityItem.h"
#include "EntityTree.h"

typedef QVector<EntityItemPointer> EntityItems;

class EntityTree;
class EntityTreeElement;
typedef std::shared_ptr<EntityTreeElement> EntityTreeElementPointer;

class EntityTreeUpdateArgs {
public:
    EntityTreeUpdateArgs() :
            _totalElements(0),
            _totalItems(0),
            _movingItems(0)
    { }

    QList<EntityItemPointer> _movingEntities;
    int _totalElements;
    int _totalItems;
    int _movingItems;
};

class EntityTreeElementExtraEncodeData {
public:
    EntityTreeElementExtraEncodeData() :
        elementCompleted(false),
        subtreeCompleted(false),
        entities() {
            memset(childCompleted, 0, sizeof(childCompleted));
        }
    bool elementCompleted;
    bool subtreeCompleted;
    bool childCompleted[NUMBER_OF_CHILDREN];
    QMap<EntityItemID, EntityPropertyFlags> entities;
};

inline QDebug operator<<(QDebug debug, const EntityTreeElementExtraEncodeData* data) {
    debug << "{";
    debug << " elementCompleted: " << data->elementCompleted << ", ";
    debug << " subtreeCompleted: " << data->subtreeCompleted << ", ";
    debug << " childCompleted[]: ";
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        debug << " " << i << ":" << data->childCompleted[i] << ", ";
    }
    debug << " entities.size: " << data->entities.size() << "}";
    return debug;
}


class SendModelsOperationArgs {
public:
    glm::vec3 root;
    EntityEditPacketSender* packetSender;
};


class EntityTreeElement : public OctreeElement, ReadWriteLockable {
    friend class EntityTree; // to allow createElement to new us...

    EntityTreeElement(unsigned char* octalCode = NULL);

    virtual OctreeElementPointer createNewElement(unsigned char* octalCode = NULL) override;

public:
    virtual ~EntityTreeElement();

    // type safe versions of OctreeElement methods
    EntityTreeElementPointer getChildAtIndex(int index) const {
        return std::static_pointer_cast<EntityTreeElement>(OctreeElement::getChildAtIndex(index));
    }

    // methods you can and should override to implement your tree functionality

    /// Adds a child to the current element. Override this if there is additional child initialization your class needs.
    virtual OctreeElementPointer addChildAtIndex(int index) override;

    /// Override this to implement LOD averaging on changes to the tree.
    virtual void calculateAverageFromChildren() override;

    /// Override this to implement LOD collapsing and identical child pruning on changes to the tree.
    virtual bool collapseChildren() override;

    /// Should this element be considered to have content in it. This will be used in collision and ray casting methods.
    /// By default we assume that only leaves are actual content, but some octrees may have different semantics.
    virtual bool hasContent() const override { return hasEntities(); }

    /// Should this element be considered to have detailed content in it. Specifically should it be rendered.
    /// By default we assume that only leaves have detailed content, but some octrees may have different semantics.
    virtual bool hasDetailedContent() const override { return hasEntities(); }

    /// Override this to break up large octree elements when an edit operation is performed on a smaller octree element.
    /// For example, if the octrees represent solid cubes and a delete of a smaller octree element is done then the
    /// meaningful split would be to break the larger cube into smaller cubes of the same color/texture.
    virtual void splitChildren() override { }

    /// Override to indicate that this element requires a split before editing lower elements in the octree
    virtual bool requiresSplit() const override { return false; }

    virtual void debugExtraEncodeData(EncodeBitstreamParams& params) const override;
    virtual void initializeExtraEncodeData(EncodeBitstreamParams& params) override;
    virtual bool shouldIncludeChildData(int childIndex, EncodeBitstreamParams& params) const override;
    virtual bool shouldRecurseChildTree(int childIndex, EncodeBitstreamParams& params) const override;
    virtual void updateEncodedData(int childIndex, AppendState childAppendState, EncodeBitstreamParams& params) const override;
    virtual void elementEncodeComplete(EncodeBitstreamParams& params) const override;

    bool alreadyFullyEncoded(EncodeBitstreamParams& params) const;


    /// Override to serialize the state of this element. This is used for persistance and for transmission across the network.
    virtual OctreeElement::AppendState appendElementData(OctreePacketData* packetData,
                                                         EncodeBitstreamParams& params) const override;

    /// Override to deserialize the state of this element. This is used for loading from a persisted file or from reading
    /// from the network.
    virtual int readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                          ReadBitstreamToTreeParams& args) override;

    /// Override to indicate that the item is currently rendered in the rendering engine. By default we assume that if
    /// the element should be rendered, then your rendering engine is rendering. But some rendering engines my have cases
    /// where an element is not actually rendering all should render elements. If the isRendered() state doesn't match the
    /// shouldRender() state, the tree will remark elements as changed even in cases there the elements have not changed.
    virtual bool isRendered() const override { return getShouldRender(); }
    virtual bool deleteApproved() const override { return !hasEntities(); }

    virtual bool canRayIntersect() const override { return hasEntities(); }
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        bool& keepSearching, OctreeElementPointer& node, float& distance,
        BoxFace& face, glm::vec3& surfaceNormal, const QVector<EntityItemID>& entityIdsToInclude,
        const QVector<EntityItemID>& entityIdsToDiscard,
        void** intersectedObject = NULL, bool precisionPicking = false);
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElementPointer& element, float& distance,
                         BoxFace& face, glm::vec3& surfaceNormal, const QVector<EntityItemID>& entityIdsToInclude,
                         const QVector<EntityItemID>& entityIdsToDiscard,
                         void** intersectedObject, bool precisionPicking, float distanceToElementCube);
    virtual bool findSpherePenetration(const glm::vec3& center, float radius,
                        glm::vec3& penetration, void** penetratedObject) const override;


    template <typename F>
    void forEachEntity(F f) const {
        withReadLock([&] {
            foreach(EntityItemPointer entityItem, _entityItems) {
                f(entityItem);
            }
        });
    }

    virtual uint16_t size() const;
    bool hasEntities() const { return size() > 0; }

    void setTree(EntityTreePointer tree) { _myTree = tree; }
    EntityTreePointer getTree() const { return _myTree; }

    bool updateEntity(const EntityItem& entity);
    void addEntityItem(EntityItemPointer entity);

    EntityItemPointer getClosestEntity(glm::vec3 position) const;

    /// finds all entities that touch a sphere
    /// \param position the center of the query sphere
    /// \param radius the radius of the query sphere
    /// \param entities[out] vector of const EntityItemPointer
    void getEntities(const glm::vec3& position, float radius, QVector<EntityItemPointer>& foundEntities) const;

    /// finds all entities that touch a box
    /// \param box the query box
    /// \param entities[out] vector of non-const EntityItemPointer
    void getEntities(const AACube& cube, QVector<EntityItemPointer>& foundEntities);

    /// finds all entities that touch a box
    /// \param box the query box
    /// \param entities[out] vector of non-const EntityItemPointer
    void getEntities(const AABox& box, QVector<EntityItemPointer>& foundEntities);

    /// finds all entities that touch a frustum
    /// \param frustum the query frustum
    /// \param entities[out] vector of non-const EntityItemPointer
    void getEntities(const ViewFrustum& frustum, QVector<EntityItemPointer>& foundEntities);

    EntityItemPointer getEntityWithID(uint32_t id) const;
    EntityItemPointer getEntityWithEntityItemID(const EntityItemID& id) const;
    void getEntitiesInside(const AACube& box, QVector<EntityItemPointer>& foundEntities);

    void cleanupEntities(); /// called by EntityTree on cleanup this will free all entities
    bool removeEntityWithEntityItemID(const EntityItemID& id);
    bool removeEntityItem(EntityItemPointer entity);

    bool containsEntityBounds(EntityItemPointer entity) const;
    bool bestFitEntityBounds(EntityItemPointer entity) const;

    bool containsBounds(const EntityItemProperties& properties) const; // NOTE: property units in meters
    bool bestFitBounds(const EntityItemProperties& properties) const; // NOTE: property units in meters

    bool containsBounds(const AACube& bounds) const; // NOTE: units in tree units
    bool bestFitBounds(const AACube& bounds) const; // NOTE: units in tree units

    bool containsBounds(const AABox& bounds) const; // NOTE: units in tree units
    bool bestFitBounds(const AABox& bounds) const; // NOTE: units in tree units

    bool containsBounds(const glm::vec3& minPoint, const glm::vec3& maxPoint) const; // NOTE: units in tree units
    bool bestFitBounds(const glm::vec3& minPoint, const glm::vec3& maxPoint) const; // NOTE: units in tree units

    void debugDump();

    bool pruneChildren();

    void expandExtentsToContents(Extents& extents);

    EntityTreeElementPointer getThisPointer() {
        return std::static_pointer_cast<EntityTreeElement>(shared_from_this());
    }
    OctreeElementPointer getThisOctreeElementPointer() {
        return std::static_pointer_cast<OctreeElement>(shared_from_this());
    }
    const ConstOctreeElementPointer getConstThisOctreeElementPointer() const {
        return std::static_pointer_cast<const OctreeElement>(shared_from_this());
    }

protected:
    virtual void init(unsigned char * octalCode) override;
    EntityTreePointer _myTree;
    EntityItems _entityItems;
};

#endif // hifi_EntityTreeElement_h
