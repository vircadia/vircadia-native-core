//
//  HFMModelMath.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/10/04.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_hfm_ModelMath_h
#define hifi_hfm_ModelMath_h

#include "HFM.h"

namespace hfm {

void forEachIndex(const hfm::MeshPart& meshPart, std::function<void(uint32_t)> func);

void initializeExtents(Extents& extents);

// This can't be moved to model-baker until
void calculateExtentsForShape(hfm::Shape& shape, const std::vector<hfm::Mesh>& meshes, const std::vector<hfm::Joint> joints);

void calculateExtentsForModel(Extents& modelExtents, const std::vector<hfm::Shape>& shapes);

class ReweightedDeformers {
public:
    std::vector<uint16_t> indices;
    std::vector<uint16_t> weights;
    uint16_t weightsPerVertex { 0 };
    bool trimmedToMatch { false };
};

const uint16_t DEFAULT_SKINNING_WEIGHTS_PER_VERTEX = 4;

ReweightedDeformers getReweightedDeformers(const size_t numMeshVertices, const std::vector<hfm::SkinCluster> skinClusters, const uint16_t weightsPerVertex = DEFAULT_SKINNING_WEIGHTS_PER_VERTEX);
};

#endif // #define hifi_hfm_ModelMath_h
