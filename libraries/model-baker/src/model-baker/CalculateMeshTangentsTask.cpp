//
//  CalculateMeshTangentsTask.cpp
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/01/22.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CalculateMeshTangentsTask.h"

#include "ModelMath.h"

void CalculateMeshTangentsTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& normalsPerMesh = input.get0();
    const std::vector<hfm::Mesh>& meshes = input.get1();
    const hfm::Model& model = *input.get2();
    auto& tangentsPerMeshOut = output;

    tangentsPerMeshOut.reserve(meshes.size());
    for (int i = 0; i < meshes.size(); i++) {
        const auto& mesh = meshes[i];
        const auto& tangentsIn = mesh.tangents;
        const auto& normals = normalsPerMesh[i];
        const auto& vertices = mesh.vertices;
        tangentsPerMeshOut.emplace_back();
        auto& tangentsOut = tangentsPerMeshOut[tangentsPerMeshOut.size()-1];

        // Check if we already have tangents and therefore do not need to do any calculation
        if (!tangentsIn.empty()) {
            tangentsOut = tangentsIn.toStdVector();
            continue;
        }

        // Check if we have normals, and if not then tangents can't be calculated
        if (normals.empty()) {
            continue;
        }

        // Check if we actually need to calculate the tangents
        bool needTangents = false;
        for (const auto& meshPart : mesh.parts) {
            auto materialIt = model.materials.find(meshPart.materialID);
            if (materialIt != model.materials.end() && (*materialIt).needTangentSpace()) {
                needTangents = true;
                break;
            }
        }
        if (needTangents) {
            continue;
        }

        tangentsOut.resize(normals.size());
        baker::calculateTangents(mesh,
        [&mesh, &normals, &tangentsOut](int firstIndex, int secondIndex, glm::vec3* outVertices, glm::vec2* outTexCoords, glm::vec3& outNormal) {
            outVertices[0] = mesh.vertices[firstIndex];
            outVertices[1] = mesh.vertices[secondIndex];
            outNormal = normals[firstIndex];
            outTexCoords[0] = mesh.texCoords[firstIndex];
            outTexCoords[1] = mesh.texCoords[secondIndex];
            return &(tangentsOut[firstIndex]);
        });
    }
}
