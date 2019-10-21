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

#include <LogHandler.h>
#include "ModelFormatLogging.h"

#include <unordered_map>

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

    glm::mat4 transform = joints[shape.joint].transform;
    forEachIndex(meshPart, [&](int32_t idx){
        if (mesh.vertices.size() <= idx) {
            return;
        }
        const glm::vec3& vertex = mesh.vertices[idx];
        const glm::vec3 transformedVertex = glm::vec3(transform * glm::vec4(vertex, 1.0f));
        shapeExtents.addPoint(transformedVertex);
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

ReweightedDeformers getReweightedDeformers(const size_t numMeshVertices, const std::vector<hfm::SkinCluster> skinClusters, const uint16_t weightsPerVertex) {
    ReweightedDeformers reweightedDeformers;
    if (skinClusters.size() == 0) {
        return reweightedDeformers;
    }

    size_t numClusterIndices = numMeshVertices * weightsPerVertex;
    reweightedDeformers.indices.resize(numClusterIndices, (uint16_t)(skinClusters.size() - 1));
    reweightedDeformers.weights.resize(numClusterIndices, 0);
    reweightedDeformers.weightsPerVertex = weightsPerVertex;

    std::vector<float> weightAccumulators;
    weightAccumulators.resize(numClusterIndices, 0.0f);
    for (uint16_t i = 0; i < (uint16_t)skinClusters.size(); ++i) {
        const hfm::SkinCluster& skinCluster = skinClusters[i];

        if (skinCluster.indices.size() != skinCluster.weights.size()) {
            reweightedDeformers.trimmedToMatch = true;
        }
        size_t numIndicesOrWeights = std::min(skinCluster.indices.size(), skinCluster.weights.size());
        for (size_t j = 0; j < numIndicesOrWeights; ++j) {
            uint32_t index = skinCluster.indices[j];
            float weight = skinCluster.weights[j];

            // look for an unused slot in the weights vector
            uint32_t weightIndex = index * weightsPerVertex;
            uint32_t lowestIndex = -1;
            float lowestWeight = FLT_MAX;
            uint16_t k = 0;
            for (; k < weightsPerVertex; k++) {
                if (weightAccumulators[weightIndex + k] == 0.0f) {
                    reweightedDeformers.indices[weightIndex + k] = i;
                    weightAccumulators[weightIndex + k] = weight;
                    break;
                }
                if (weightAccumulators[weightIndex + k] < lowestWeight) {
                    lowestIndex = k;
                    lowestWeight = weightAccumulators[weightIndex + k];
                }
            }
            if (k == weightsPerVertex && weight > lowestWeight) {
                // no space for an additional weight; we must replace the lowest
                weightAccumulators[weightIndex + lowestIndex] = weight;
                reweightedDeformers.indices[weightIndex + lowestIndex] = i;
            }
        }
    }

    // now that we've accumulated the most relevant weights for each vertex
    // normalize and compress to 16-bits
    for (size_t i = 0; i < numMeshVertices; ++i) {
        size_t j = i * weightsPerVertex;

        // normalize weights into uint16_t
        float totalWeight = 0.0f;
        for (size_t k = j; k < j + weightsPerVertex; ++k) {
            totalWeight += weightAccumulators[k];
        }

        const float ALMOST_HALF = 0.499f;
        if (totalWeight > 0.0f) {
            float weightScalingFactor = (float)(UINT16_MAX) / totalWeight;
            for (size_t k = j; k < j + weightsPerVertex; ++k) {
                reweightedDeformers.weights[k] = (uint16_t)(weightScalingFactor * weightAccumulators[k] + ALMOST_HALF);
            }
        } else {
            reweightedDeformers.weights[j] = (uint16_t)((float)(UINT16_MAX) + ALMOST_HALF);
        }
    }

    return reweightedDeformers;
}


MeshIndexedTrianglesPos generateMeshIndexedTrianglePos(const std::vector<glm::vec3>& srcVertices, const std::vector<HFMMeshPart> srcParts) {

    MeshIndexedTrianglesPos dest;
    dest.vertices.resize(srcVertices.size());

    std::vector<uint32_t> remap(srcVertices.size());
    {
        std::unordered_map<glm::vec3, uint32_t> uniqueVertices;
        int vi = 0;
        int vu = 0;
        for (const auto& v : srcVertices) {
            auto foundIndex = uniqueVertices.find(v);
            if (foundIndex != uniqueVertices.end()) {
                remap[vi] = foundIndex->second;
            } else {
                uniqueVertices[v] = vu;
                remap[vi] = vu;
                dest.vertices[vu] = v;
                vu++;
            }
            ++vi;
        }
        if (uniqueVertices.size() < srcVertices.size()) {
            dest.vertices.resize(uniqueVertices.size());
            dest.vertices.shrink_to_fit();

        }
    }

    auto newIndicesCount = 0;
    for (const auto& part : srcParts) {
        newIndicesCount += part.triangleIndices.size() + part.quadTrianglesIndices.size();
    }

    {
        dest.indices.resize(newIndicesCount);
        int i = 0;
        for (const auto& part : srcParts) {
            for (const auto& qti : part.quadTrianglesIndices) {
                dest.indices[i] = remap[qti];
                ++i;
            }
            for (const auto& ti : part.quadTrianglesIndices) {
                dest.indices[i] = remap[ti];
                ++i;
            }

           // dest.indices.insert(dest.indices.end(), part.quadTrianglesIndices.cbegin(), part.quadTrianglesIndices.cend());
           // dest.indices.insert(dest.indices.end(), part.triangleIndices.cbegin(), part.triangleIndices.cend());
        }
    }

    return dest;
}

};
