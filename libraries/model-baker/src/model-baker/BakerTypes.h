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

#include <hfm/HFM.h>

namespace baker {
    using MeshTangents = std::vector<glm::vec3>;
    using TangentsPerMesh = std::vector<std::vector<glm::vec3>>;
    using Blendshapes = std::vector<hfm::Blendshape>;
    using BlendshapesPerMesh = std::vector<std::vector<hfm::Blendshape>>;
    using MeshIndicesToModelNames = QHash<int, QString>;
};

#endif // hifi_BakerTypes_h
