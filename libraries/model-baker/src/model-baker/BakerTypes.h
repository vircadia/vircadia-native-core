//
//  BakerTypes.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2018/12/10.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BakerTypes_h
#define hifi_BakerTypes_h

#include <QUrl>
#include <hfm/HFM.h>

namespace baker {
    using MeshIndices = std::vector<int>;
    using IndicesPerMesh = std::vector<std::vector<int>>;
    using VerticesPerMesh = std::vector<std::vector<glm::vec3>>;
    using MeshNormals = std::vector<glm::vec3>;
    using NormalsPerMesh = std::vector<std::vector<glm::vec3>>;
    using MeshTangents = std::vector<glm::vec3>;
    using TangentsPerMesh = std::vector<std::vector<glm::vec3>>;

    using Blendshapes = std::vector<hfm::Blendshape>;
    using BlendshapesPerMesh = std::vector<std::vector<hfm::Blendshape>>;
    using BlendshapeVertices = std::vector<glm::vec3>;
    using BlendshapeNormals = std::vector<glm::vec3>;
    using BlendshapeIndices = std::vector<int>;
    using VerticesPerBlendshape = std::vector<std::vector<glm::vec3>>;
    using NormalsPerBlendshape = std::vector<std::vector<glm::vec3>>;
    using IndicesPerBlendshape = std::vector<std::vector<int>>;
    using BlendshapeTangents = std::vector<glm::vec3>;
    using TangentsPerBlendshape = std::vector<std::vector<glm::vec3>>;

    using MeshIndicesToModelNames = QHash<int, QString>;

    class ReweightedDeformers {
    public:
        std::vector<uint16_t> indices;
        std::vector<uint16_t> weights;
        uint16_t weightsPerVertex { 0 };
        bool trimmedToMatch { false };
    };
};

#endif // hifi_BakerTypes_h
