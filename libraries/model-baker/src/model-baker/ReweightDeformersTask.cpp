//
//  ReweightDeformersTask.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/09/26.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ReweightDeformersTask.h"

baker::ReweightedDeformers getReweightedDeformers(size_t numMeshVertices, const hfm::DynamicTransform* dynamicTransform, const std::vector<const hfm::Deformer*> deformers, const uint16_t weightsPerVertex) {
    size_t numClusterIndices = numMeshVertices * weightsPerVertex;
    baker::ReweightedDeformers reweightedDeformers;
    reweightedDeformers.weightsPerVertex = weightsPerVertex;
    // TODO: Consider having a rootCluster property in the DynamicTransform rather than appending the root to the end of the cluster list.
    reweightedDeformers.indices.resize(numClusterIndices, (uint16_t)(deformers.size() - 1));
    reweightedDeformers.weights.resize(numClusterIndices, 0);

    std::vector<float> weightAccumulators;
    weightAccumulators.resize(numClusterIndices, 0.0f);
    for (uint16_t i = 0; i < (uint16_t)deformers.size(); ++i) {
        const hfm::Deformer& deformer = *deformers[i];

        if (deformer.indices.size() != deformer.weights.size()) {
            reweightedDeformers.trimmedToMatch = true;
        }
        size_t numIndicesOrWeights = std::min(deformer.indices.size(), deformer.weights.size());
        for (size_t j = 0; j < numIndicesOrWeights; ++j) {
            uint32_t index = deformer.indices[j];
            float weight = deformer.weights[j];

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

void ReweightDeformersTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const uint16_t NUM_WEIGHTS_PER_VERTEX { 4 };

    const auto& meshes = input.get0();
    const auto& shapes = input.get1();
    const auto& dynamicTransforms = input.get2();
    const auto& deformers = input.get3();
    auto& reweightedDeformers = output;

    // Currently, there is only (at most) one dynamicTransform per mesh
    // An undefined shape.dynamicTransform has the value hfm::UNDEFINED_KEY
    std::vector<uint32_t> dynamicTransformPerMesh;
    dynamicTransformPerMesh.resize(meshes.size(), hfm::UNDEFINED_KEY);
    for (const auto& shape : shapes) {
        uint32_t dynamicTransformIndex = shape.dynamicTransform;
        dynamicTransformPerMesh[shape.mesh] = dynamicTransformIndex;
    }

    reweightedDeformers.reserve(meshes.size());
    for (size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = meshes[i];
        uint32_t dynamicTransformIndex = dynamicTransformPerMesh[i];

        const hfm::DynamicTransform* dynamicTransform = nullptr;
        std::vector<const hfm::Deformer*> meshDeformers;
        if (dynamicTransformIndex != hfm::UNDEFINED_KEY) {
            dynamicTransform = &dynamicTransforms[dynamicTransformIndex];
            for (const auto& deformerIndex : dynamicTransform->deformers) {
                const auto& deformer = deformers[deformerIndex];
                meshDeformers.push_back(&deformer);
            }
        }

        reweightedDeformers.push_back(getReweightedDeformers((size_t)mesh.vertices.size(), dynamicTransform, meshDeformers, NUM_WEIGHTS_PER_VERTEX));
    }
}
