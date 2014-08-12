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

#include <OctreeElement.h>
#include <QList>

#include "EntityEditPacketSender.h"
#include "EntityItem.h"
#include "EntityTree.h"

class EntityTree;
class EntityTreeElement;

class EntityTreeUpdateArgs {
public:
    EntityTreeUpdateArgs() :
            _totalElements(0),
            _totalItems(0),
            _movingItems(0)
    { }
    
    QList<EntityItem*> _movingEntities;
    int _totalElements;
    int _totalItems;
    int _movingItems;
};

class EntityTreeElementExtraEncodeData {
public:
    QMap<EntityItemID, EntityPropertyFlags> includedItems;
};

class SendModelsOperationArgs {
public:
    glm::vec3 root;
    EntityEditPacketSender* packetSender;
};


class EntityTreeElement : public OctreeElement {
    friend class EntityTree; // to allow createElement to new us...

    EntityTreeElement(unsigned char* octalCode = NULL);

    virtual OctreeElement* createNewElement(unsigned char* octalCode = NULL);

public:
    virtual ~EntityTreeElement();

    // type safe versions of OctreeElement methods
    EntityTreeElement* getChildAtIndex(int index) { return (EntityTreeElement*)OctreeElement::getChildAtIndex(index); }

    // methods you can and should override to implement your tree functionality

    /// Adds a child to the current element. Override this if there is additional child initialization your class needs.
    virtual EntityTreeElement* addChildAtIndex(int index);

    /// Override this to implement LOD averaging on changes to the tree.
    virtual void calculateAverageFromChildren();

    /// Override this to implement LOD collapsing and identical child pruning on changes to the tree.
    virtual bool collapseChildren();

    /// Should this element be considered to have content in it. This will be used in collision and ray casting methods.
    /// By default we assume that only leaves are actual content, but some octrees may have different semantics.
    virtual bool hasContent() const { return hasEntities(); }

    /// Should this element be considered to have detailed content in it. Specifically should it be rendered.
    /// By default we assume that only leaves have detailed content, but some octrees may have different semantics.
    virtual bool hasDetailedContent() const { return hasEntities(); }

    /// Override this to break up large octree elements when an edit operation is performed on a smaller octree element.
    /// For example, if the octrees represent solid cubes and a delete of a smaller octree element is done then the
    /// meaningful split would be to break the larger cube into smaller cubes of the same color/texture.
    virtual void splitChildren() { }

    /// Override to indicate that this element requires a split before editing lower elements in the octree
    virtual bool requiresSplit() const { return false; }

    /// Override to serialize the state of this element. This is used for persistance and for transmission across the network.
    virtual OctreeElement::AppendState appendElementData(OctreePacketData* packetData, EncodeBitstreamParams& params) const;

    /// Override to deserialize the state of this element. This is used for loading from a persisted file or from reading
    /// from the network.
    virtual int readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);

    /// Override to indicate that the item is currently rendered in the rendering engine. By default we assume that if
    /// the element should be rendered, then your rendering engine is rendering. But some rendering engines my have cases
    /// where an element is not actually rendering all should render elements. If the isRendered() state doesn't match the
    /// shouldRender() state, the tree will remark elements as changed even in cases there the elements have not changed.
    virtual bool isRendered() const { return getShouldRender(); }
    virtual bool deleteApproved() const { return !hasEntities(); }

    virtual bool canRayIntersect() const { return hasEntities(); }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject);

    virtual bool findSpherePenetration(const glm::vec3& center, float radius,
                        glm::vec3& penetration, void** penetratedObject) const;

    const QList<EntityItem*>& getEntities() const { return *_entityItems; }
    QList<EntityItem*>& getEntities() { return *_entityItems; }
    bool hasEntities() const { return _entityItems ? _entityItems->size() > 0 : false; }

    void update(EntityTreeUpdateArgs& args);
    void setTree(EntityTree* tree) { _myTree = tree; }

    bool updateEntity(const EntityItem& entity);
    void addEntityItem(EntityItem* entity);


    void updateEntityItemID(const EntityItemID& creatorTokenEntityID, const EntityItemID& knownIDEntityID);

    const EntityItem* getClosestEntity(glm::vec3 position) const;

    /// finds all entities that touch a sphere
    /// \param position the center of the query sphere
    /// \param radius the radius of the query sphere
    /// \param entities[out] vector of const EntityItem*
    void getEntities(const glm::vec3& position, float radius, QVector<const EntityItem*>& foundEntities) const;

    /// finds all entities that touch a box
    /// \param box the query box
    /// \param entities[out] vector of non-const EntityItem*
    void getEntities(const AACube& box, QVector<EntityItem*>& foundEntities);

    const EntityItem* getEntityWithID(uint32_t id) const;
    const EntityItem* getEntityWithEntityItemID(const EntityItemID& id) const;
    void getEntitiesInside(const AACube& box, QVector<EntityItem*>& foundEntities);

    EntityItem* getEntityWithEntityItemID(const EntityItemID& id);

    bool removeEntityWithID(uint32_t id);
    bool removeEntityWithEntityItemID(const EntityItemID& id);
    bool removeEntityItem(const EntityItem* entity);

    bool containsEntityBounds(const EntityItem* entity) const;
    bool bestFitEntityBounds(const EntityItem* entity) const;

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

protected:
    virtual void init(unsigned char * octalCode);
    EntityTree* _myTree;
    QList<EntityItem*>* _entityItems;
};

#endif // hifi_EntityTreeElement_h
