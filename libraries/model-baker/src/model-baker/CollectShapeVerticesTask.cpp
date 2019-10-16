//
//  CollectShapeVerticesTask.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/09/27.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CollectShapeVerticesTask.h"

#include <glm/gtx/transform.hpp>

// Used to track and avoid duplicate shape vertices, as multiple shapes can have the same mesh and dynamicTransform
class VertexSource {
public:
    uint32_t mesh;
    uint32_t dynamicTransform;

    bool operator==(const VertexSource& other) const {
        return mesh == other.mesh &&
            dynamicTransform == other.dynamicTransform;
    }
};

void CollectShapeVerticesTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& meshes = input.get0();
    const auto& shapes = input.get1();
    const auto& joints = input.get2();
    const auto& dynamicTransforms = input.get3();
    const auto& reweightedDeformers = input.get4();
    auto& shapeVerticesPerJoint = output;

    shapeVerticesPerJoint.resize(joints.size());
    std::vector<std::vector<VertexSource>> vertexSourcesPerJoint;
    vertexSourcesPerJoint.resize(joints.size());
    for (size_t i = 0; i < shapes.size(); ++i) {
        const auto& shape = shapes[i];
        const uint32_t dynamicTransformKey = shape.dynamicTransform;
        if (dynamicTransformKey == hfm::UNDEFINED_KEY) {
            continue;
        }

        VertexSource vertexSource;
        vertexSource.mesh = shape.mesh;
        vertexSource.dynamicTransform = dynamicTransformKey;

        const auto& dynamicTransform = dynamicTransforms[dynamicTransformKey];
        for (size_t j = 0; j < dynamicTransform.clusters.size(); ++j) {
            const auto& cluster = dynamicTransform.clusters[j];
            const uint32_t jointIndex = cluster.jointIndex;

            auto& vertexSources = vertexSourcesPerJoint[jointIndex];
            if (std::find(vertexSources.cbegin(), vertexSources.cend(), vertexSource) == vertexSources.cend()) {
                vertexSources.push_back(vertexSource);
                auto& shapeVertices = shapeVerticesPerJoint[jointIndex];

                const auto& mesh = meshes[shape.mesh];
                const auto& vertices = mesh.vertices;
                const auto& reweightedDeformer = reweightedDeformers[shape.mesh];
                const glm::mat4 meshToJoint = cluster.inverseBindMatrix;

                const uint16_t weightsPerVertex = reweightedDeformer.weightsPerVertex;
                if (weightsPerVertex == 0) {
                    for (int vertexIndex = 0; vertexIndex < (int)vertices.size(); ++vertexIndex) {
                        const glm::mat4 vertexTransform = meshToJoint * glm::translate(vertices[vertexIndex]);
                        shapeVertices.push_back(extractTranslation(vertexTransform));
                    }
                } else {
                    for (int vertexIndex = 0; vertexIndex < (int)vertices.size(); ++vertexIndex) {
                        for (uint16_t weightIndex = 0; weightIndex < weightsPerVertex; ++weightIndex) {
                            const size_t index = vertexIndex*4 + weightIndex;
                            const uint16_t clusterIndex = reweightedDeformer.indices[index];
                            const uint16_t clusterWeight = reweightedDeformer.weights[index];
                            // Remember vertices associated with this joint with at least 1/4 weight
                            const uint16_t EXPANSION_WEIGHT_THRESHOLD = std::numeric_limits<uint16_t>::max() / 4;
                            if (clusterIndex != j || clusterWeight < EXPANSION_WEIGHT_THRESHOLD) {
                                continue;
                            }
                        
                            const glm::mat4 vertexTransform = meshToJoint * glm::translate(vertices[vertexIndex]);
                            shapeVertices.push_back(extractTranslation(vertexTransform));
                        }
                    }
                }
            }
        }
    }
}
