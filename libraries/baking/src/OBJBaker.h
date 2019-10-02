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
#include "ModelBaker.h"
#include "ModelBakingLoggingCategory.h"

using NodeID = qlonglong;

class OBJBaker : public ModelBaker {
    Q_OBJECT
public:
    using ModelBaker::ModelBaker;

protected:
    virtual void bakeProcessedSource(const hfm::Model::Pointer& hfmModel, const std::vector<hifi::ByteArray>& dracoMeshes, const std::vector<std::vector<hifi::ByteArray>>& dracoMaterialLists) override;

private:
    void createFBXNodeTree(FBXNode& rootNode, const hfm::Model::Pointer& hfmModel, const hifi::ByteArray& dracoMesh);
    void setMaterialNodeProperties(FBXNode& materialNode, QString material, const hfm::Model::Pointer& hfmModel);
    NodeID nextNodeID() { return _nodeID++; }

    NodeID _nodeID { 0 };
    std::vector<NodeID> _materialIDs;
};
#endif // hifi_OBJBaker_h
