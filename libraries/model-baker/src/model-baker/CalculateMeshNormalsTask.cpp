//
//  CalculateMeshNormalsTask.cpp
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/01/22.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CalculateMeshNormalsTask.h"

#include "ModelMath.h"

void CalculateMeshNormalsTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& meshes = input;
    auto& normalsPerMeshOut = output;

    normalsPerMeshOut.reserve(meshes.size());
    for (int i = 0; i < (int)meshes.size(); i++) {
        const auto& mesh = meshes[i];
        normalsPerMeshOut.emplace_back();
        auto& normalsOut = normalsPerMeshOut[normalsPerMeshOut.size()-1];
        // Only calculate normals if this mesh doesn't already have them
        if (!mesh.normals.empty()) {
            normalsOut = std::vector<glm::vec3>(mesh.normals.begin(), mesh.normals.end());
        } else {
            normalsOut.resize(mesh.vertices.size());
            baker::calculateNormals(mesh,
                [&normalsOut](int normalIndex) /* NormalAccessor */ {
                    return &normalsOut[normalIndex];
                },
                [&mesh](int vertexIndex, glm::vec3& outVertex) /* VertexSetter */ {
                    outVertex = baker::safeGet(mesh.vertices, vertexIndex);
                }
            );
        }
    }
}
