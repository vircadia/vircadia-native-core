//
//  HFMModelMath.cpp
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/10/04.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFMModelMath.h"

namespace hfm {

void forEachIndex(const hfm::MeshPart& meshPart, std::function<void(uint32_t)> func) {
    for (int i = 0; i <= meshPart.quadIndices.size() - 4; i += 4) {
        func((uint32_t)meshPart.quadIndices[i]);
        func((uint32_t)meshPart.quadIndices[i+1]);
        func((uint32_t)meshPart.quadIndices[i+2]);
        func((uint32_t)meshPart.quadIndices[i+3]);
    }
    for (int i = 0; i <= meshPart.triangleIndices.size() - 3; i += 3) {
        func((uint32_t)meshPart.triangleIndices[i]);
        func((uint32_t)meshPart.triangleIndices[i+1]);
        func((uint32_t)meshPart.triangleIndices[i+2]);
    }
}

void thickenFlatExtents(Extents& extents) {
    // Add epsilon to extents to compensate for flat plane
    extents.minimum -= glm::vec3(EPSILON, EPSILON, EPSILON);
    extents.maximum += glm::vec3(EPSILON, EPSILON, EPSILON);
}

void calculateExtentsForShape(hfm::Shape& shape, const std::vector<hfm::Mesh>& meshes, const std::vector<hfm::Joint> joints) {
    auto& shapeExtents = shape.transformedExtents;
    shapeExtents.reset();

    const auto& mesh = meshes[shape.mesh];
    const auto& meshPart = mesh.parts[shape.meshPart];

    glm::mat4 globalTransform = joints[shape.transform].globalTransform;
    forEachIndex(meshPart, [&](int32_t idx){
        if (mesh.vertices.size() <= idx) {
            return;
        }
        const glm::vec3& vertex = mesh.vertices[idx];
        const glm::vec3 transformedVertex = glm::vec3(globalTransform * glm::vec4(vertex, 1.0f));
        shapeExtents.addPoint(vertex);
    });

    thickenFlatExtents(shapeExtents);
}

void calculateExtentsForModel(Extents& modelExtents, const std::vector<hfm::Shape>& shapes) {
    modelExtents.reset();

    for (size_t i = 0; i < shapes.size(); ++i) {
        const auto& shape = shapes[i];
        const auto& shapeExtents = shape.transformedExtents;
        modelExtents.addExtents(shapeExtents);
    }
}

};
