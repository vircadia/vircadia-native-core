//
//  CalculateBlendshapeNormalsTask.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/01/07.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CalculateBlendshapeNormalsTask.h"

#include "ModelMath.h"

void CalculateBlendshapeNormalsTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& blendshapesPerMesh = input.get0();
    const auto& meshes = input.get1();
    auto& normalsPerBlendshapePerMeshOut = output;

    normalsPerBlendshapePerMeshOut.reserve(blendshapesPerMesh.size());
    for (size_t i = 0; i < blendshapesPerMesh.size(); i++) {
        const auto& mesh = meshes[i];
        const auto& blendshapes = blendshapesPerMesh[i];
        normalsPerBlendshapePerMeshOut.emplace_back();
        auto& normalsPerBlendshapeOut = normalsPerBlendshapePerMeshOut[normalsPerBlendshapePerMeshOut.size()-1];

        normalsPerBlendshapeOut.reserve(blendshapes.size());
        for (size_t j = 0; j < blendshapes.size(); j++) {
            const auto& blendshape = blendshapes[j];
            const auto& normalsIn = blendshape.normals;
            // Check if normals are already defined. Otherwise, calculate them from existing blendshape vertices.
            if (!normalsIn.empty()) {
                normalsPerBlendshapeOut.push_back(std::vector<glm::vec3>(normalsIn.begin(), normalsIn.end()));
            } else {
                // Create lookup to get index in blendshape from vertex index in mesh
                std::vector<int> reverseIndices;
                reverseIndices.resize(mesh.vertices.size());
                std::iota(reverseIndices.begin(), reverseIndices.end(), 0);
                for (int indexInBlendShape = 0; indexInBlendShape < blendshape.indices.size(); ++indexInBlendShape) {
                    auto indexInMesh = blendshape.indices[indexInBlendShape];
                    reverseIndices[indexInMesh] = indexInBlendShape;
                }

                normalsPerBlendshapeOut.emplace_back();
                auto& normals = normalsPerBlendshapeOut[normalsPerBlendshapeOut.size()-1];
                normals.resize(mesh.vertices.size());
                baker::calculateNormals(mesh,
                    [&reverseIndices, &blendshape, &normals](int normalIndex) /* NormalAccessor */ {
                        const auto lookupIndex = reverseIndices[normalIndex];
                        if (lookupIndex < blendshape.vertices.size()) {
                            return &normals[lookupIndex];
                        } else {
                            // Index isn't in the blendshape. Request that the normal not be calculated.
                            return (glm::vec3*)nullptr;
                        }
                    },
                    [&mesh, &reverseIndices, &blendshape](int vertexIndex, glm::vec3& outVertex) /* VertexSetter */ {
                        const auto lookupIndex = reverseIndices[vertexIndex];
                        if (lookupIndex < blendshape.vertices.size()) {
                            outVertex = blendshape.vertices[lookupIndex];
                        } else {
                            // Index isn't in the blendshape, so return vertex from mesh
                            outVertex = baker::safeGet(mesh.vertices, lookupIndex);
                        }
                    });
            }
        }
    }
}
