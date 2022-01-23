//
//  CalculateBlendshapeTangentsTask.cpp
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/01/08.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CalculateBlendshapeTangentsTask.h"

#include <set>

#include "ModelMath.h"

void CalculateBlendshapeTangentsTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& normalsPerBlendshapePerMesh = input.get0();
    const auto& blendshapesPerMesh = input.get1();
    const auto& meshes = input.get2();
    auto& tangentsPerBlendshapePerMeshOut = output;

    tangentsPerBlendshapePerMeshOut.reserve(normalsPerBlendshapePerMesh.size());
    for (size_t i = 0; i < blendshapesPerMesh.size(); i++) {
        const auto& normalsPerBlendshape = baker::safeGet(normalsPerBlendshapePerMesh, i);
        const auto& blendshapes = blendshapesPerMesh[i];
        const auto& mesh = meshes[i];
        tangentsPerBlendshapePerMeshOut.emplace_back();
        auto& tangentsPerBlendshapeOut = tangentsPerBlendshapePerMeshOut[tangentsPerBlendshapePerMeshOut.size()-1];

        for (size_t j = 0; j < blendshapes.size(); j++) {
            const auto& blendshape = blendshapes[j];
            const auto& tangentsIn = blendshape.tangents;
            const auto& normals = baker::safeGet(normalsPerBlendshape, j);
            tangentsPerBlendshapeOut.emplace_back();
            auto& tangentsOut = tangentsPerBlendshapeOut[tangentsPerBlendshapeOut.size()-1];

            // Check if we already have tangents
            if (!tangentsIn.empty()) {
                tangentsOut = std::vector<glm::vec3>(tangentsIn.begin(), tangentsIn.end());
                continue;
            }

            // Check if we can calculate tangents (we need normals and texcoords to calculate the tangents)
            if (normals.empty() || normals.size() != (size_t)mesh.texCoords.size()) {
                continue;
            }
            tangentsOut.resize(normals.size());

            // Create lookup to get index in blend shape from vertex index in mesh
            std::vector<int> reverseIndices;
            reverseIndices.resize(mesh.vertices.size());
            std::iota(reverseIndices.begin(), reverseIndices.end(), 0);
            for (int indexInBlendShape = 0; indexInBlendShape < blendshape.indices.size(); ++indexInBlendShape) {
                auto indexInMesh = blendshape.indices[indexInBlendShape];
                reverseIndices[indexInMesh] = indexInBlendShape;
            }

            baker::calculateTangents(mesh,
                [&mesh, &blendshape, &normals, &tangentsOut, &reverseIndices](int firstIndex, int secondIndex, glm::vec3* outVertices, glm::vec2* outTexCoords, glm::vec3& outNormal) {
                const auto index1 = reverseIndices[firstIndex];
                const auto index2 = reverseIndices[secondIndex];

                if (index1 < blendshape.vertices.size()) {
                    outVertices[0] = blendshape.vertices[index1];
                    outTexCoords[0] = mesh.texCoords[index1];
                    outTexCoords[1] = mesh.texCoords[index2];
                    if (index2 < blendshape.vertices.size()) {
                        outVertices[1] = blendshape.vertices[index2];
                    } else {
                        // Index isn't in the blend shape so return vertex from mesh
                        outVertices[1] = mesh.vertices[secondIndex];
                    }
                    outNormal = normals[index1];
                    return &tangentsOut[index1];
                } else {
                    // Index isn't in blend shape so return nullptr
                    return (glm::vec3*)nullptr;
                }
            });
        }
    }
}
