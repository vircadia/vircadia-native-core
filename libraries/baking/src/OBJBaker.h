//
//  OBJBaker.h
//  libraries/baking/src
//
//  Created by Utkarsh Gautam on 9/29/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OBJBaker_h
#define hifi_OBJBaker_h

#include "Baker.h"
#include "TextureBaker.h"
#include "ModelBaker.h"

#include "ModelBakingLoggingCategory.h"

using TextureBakerThreadGetter = std::function<QThread*()>;

using NodeID = qlonglong;

class OBJBaker : public ModelBaker {
    Q_OBJECT
public:
    using ModelBaker::ModelBaker;

public slots:
    virtual void bake() override;

signals:
    void OBJLoaded();

private slots:
    void bakeOBJ();
    void handleOBJNetworkReply();

private:
    void loadOBJ();
    void createFBXNodeTree(FBXNode& rootNode, FBXGeometry& geometry);
    void setMaterialNodeProperties(FBXNode& materialNode, QString material, FBXGeometry& geometry);
    NodeID nextNodeID() { return _nodeID++; }

    NodeID _nodeID { 0 };
    NodeID _geometryID;
    NodeID _modelID;
    std::vector<NodeID> _materialIDs;
    NodeID _textureID;
    std::vector<std::pair<NodeID, int>> _mapTextureMaterial;
    FBXNode _objectNode;
};
#endif // hifi_OBJBaker_h
