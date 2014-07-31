//
//  ModelTreeElement.h
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelTreeElement_h
#define hifi_ModelTreeElement_h

#include <OctreeElement.h>
#include <QList>

#include "ModelItem.h"
#include "ModelTree.h"

class ModelTree;
class ModelTreeElement;

class ModelTreeUpdateArgs {
public:
    ModelTreeUpdateArgs() :
            _totalElements(0),
            _totalItems(0),
            _movingItems(0)
    { }
    
    QList<ModelItem> _movingModels;
    int _totalElements;
    int _totalItems;
    int _movingItems;
};

class FindAndUpdateModelItemIDArgs {
public:
    uint32_t modelID;
    uint32_t creatorTokenID;
    bool creatorTokenFound;
    bool viewedModelFound;
    bool isViewing;
};

class SendModelsOperationArgs {
public:
    glm::vec3 root;
    ModelEditPacketSender* packetSender;
};



class ModelTreeElement : public OctreeElement {
    friend class ModelTree; // to allow createElement to new us...

    ModelTreeElement(unsigned char* octalCode = NULL);

    virtual OctreeElement* createNewElement(unsigned char* octalCode = NULL);

public:
    virtual ~ModelTreeElement();

    // type safe versions of OctreeElement methods
    ModelTreeElement* getChildAtIndex(int index) { return (ModelTreeElement*)OctreeElement::getChildAtIndex(index); }

    // methods you can and should override to implement your tree functionality

    /// Adds a child to the current element. Override this if there is additional child initialization your class needs.
    virtual ModelTreeElement* addChildAtIndex(int index);

    /// Override this to implement LOD averaging on changes to the tree.
    virtual void calculateAverageFromChildren();

    /// Override this to implement LOD collapsing and identical child pruning on changes to the tree.
    virtual bool collapseChildren();

    /// Should this element be considered to have content in it. This will be used in collision and ray casting methods.
    /// By default we assume that only leaves are actual content, but some octrees may have different semantics.
    virtual bool hasContent() const { return hasModels(); }

    /// Should this element be considered to have detailed content in it. Specifically should it be rendered.
    /// By default we assume that only leaves have detailed content, but some octrees may have different semantics.
    virtual bool hasDetailedContent() const { return hasModels(); }

    /// Override this to break up large octree elements when an edit operation is performed on a smaller octree element.
    /// For example, if the octrees represent solid cubes and a delete of a smaller octree element is done then the
    /// meaningful split would be to break the larger cube into smaller cubes of the same color/texture.
    virtual void splitChildren() { }

    /// Override to indicate that this element requires a split before editing lower elements in the octree
    virtual bool requiresSplit() const { return false; }

    /// Override to serialize the state of this element. This is used for persistance and for transmission across the network.
    virtual bool appendElementData(OctreePacketData* packetData, EncodeBitstreamParams& params) const;

    /// Override to deserialize the state of this element. This is used for loading from a persisted file or from reading
    /// from the network.
    virtual int readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);

    /// Override to indicate that the item is currently rendered in the rendering engine. By default we assume that if
    /// the element should be rendered, then your rendering engine is rendering. But some rendering engines my have cases
    /// where an element is not actually rendering all should render elements. If the isRendered() state doesn't match the
    /// shouldRender() state, the tree will remark elements as changed even in cases there the elements have not changed.
    virtual bool isRendered() const { return getShouldRender(); }
    virtual bool deleteApproved() const { return !hasModels(); }

    virtual bool canRayIntersect() const { return hasModels(); }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject);

    virtual bool findSpherePenetration(const glm::vec3& center, float radius,
                        glm::vec3& penetration, void** penetratedObject) const;

    const QList<ModelItem>& getModels() const { return *_modelItems; }
    QList<ModelItem>& getModels() { return *_modelItems; }
    bool hasModels() const { return _modelItems ? _modelItems->size() > 0 : false; }

    void update(ModelTreeUpdateArgs& args);
    void setTree(ModelTree* tree) { _myTree = tree; }

    bool updateModel(const ModelItem& model);
    bool updateModel(const ModelItemID& modelID, const ModelItemProperties& properties);
    void updateModelItemID(FindAndUpdateModelItemIDArgs* args);

    const ModelItem* getClosestModel(glm::vec3 position) const;

    /// finds all models that touch a sphere
    /// \param position the center of the query sphere
    /// \param radius the radius of the query sphere
    /// \param models[out] vector of const ModelItem*
    void getModels(const glm::vec3& position, float radius, QVector<const ModelItem*>& foundModels) const;

    /// finds all models that touch a box
    /// \param box the query box
    /// \param models[out] vector of non-const ModelItem*
    void getModelsForUpdate(const AACube& box, QVector<ModelItem*>& foundModels);

    void getModelsInside(const AACube& box, QVector<ModelItem*>& foundModels);

    const ModelItem* getModelWithID(uint32_t id) const;

    bool removeModelWithID(uint32_t id);

    bool containsModelBounds(const ModelItem& model) const;
    bool bestFitModelBounds(const ModelItem& model) const;

protected:
    virtual void init(unsigned char * octalCode);

    void storeModel(const ModelItem& model);

    ModelTree* _myTree;
    QList<ModelItem>* _modelItems;
};

#endif // hifi_ModelTreeElement_h
