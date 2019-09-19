//
//  BuildGraphicsMeshTask.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2018/12/06.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BuildGraphicsMeshTask.h"

#include <glm/gtc/packing.hpp>

#include <LogHandler.h>
#include "ModelBakerLogging.h"
#include "ModelMath.h"

using vec2h = glm::tvec2<glm::detail::hdata>;

glm::vec3 normalizeDirForPacking(const glm::vec3& dir) {
    auto maxCoord = glm::max(fabsf(dir.x), glm::max(fabsf(dir.y), fabsf(dir.z)));
    if (maxCoord > 1e-6f) {
        return dir / maxCoord;
    }
    return dir;
}

class ReweightedDeformers {
public:
    std::vector<uint16_t> indices;
    std::vector<uint16_t> weights;
    bool trimmedToMatch { false };
};

ReweightedDeformers getReweightedDeformers(size_t numMeshVertices, const hfm::DynamicTransform* dynamicTransform, const std::vector<const hfm::Deformer*> deformers, const uint16_t weightsPerVertex) {
    size_t numClusterIndices = numMeshVertices * weightsPerVertex;
    ReweightedDeformers reweightedDeformers;
    // TODO: Consider having a rootCluster property in the DynamicTransform rather than appending the root to the end of the cluster list.
    reweightedDeformers.indices.resize(numClusterIndices, deformers.size() - 1);
    reweightedDeformers.weights.resize(numClusterIndices, 0);

    std::vector<float> weightAccumulators;
    weightAccumulators.resize(numClusterIndices, 0.0f);
    for (size_t i = 0; i < deformers.size(); ++i) {
        const hfm::Deformer& deformer = *deformers[i];
        const hfm::Cluster& cluster = dynamicTransform->clusters[i];

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

void buildGraphicsMesh(const hfm::Mesh& hfmMesh, graphics::MeshPointer& graphicsMeshPointer, const baker::MeshNormals& meshNormals, const baker::MeshTangents& meshTangentsIn, const hfm::DynamicTransform* dynamicTransform, const std::vector<const hfm::Deformer*> meshDeformers) {
    auto graphicsMesh = std::make_shared<graphics::Mesh>();

    // Fill tangents with a dummy value to force tangents to be present if there are normals
    baker::MeshTangents meshTangents;
    if (!meshTangentsIn.empty()) {
        meshTangents = meshTangentsIn;
    } else {
        meshTangents.reserve(meshNormals.size());
        std::fill_n(std::back_inserter(meshTangents), meshNormals.size(), Vectors::UNIT_X);
    }

    unsigned int totalSourceIndices = 0;
    foreach(const HFMMeshPart& part, hfmMesh.parts) {
        totalSourceIndices += (part.quadTrianglesIndices.size() + part.triangleIndices.size());
    }

    static int repeatMessageID = LogHandler::getInstance().newRepeatedMessageID();

    if (!totalSourceIndices) {
        HIFI_FCDEBUG_ID(model_baker(), repeatMessageID, "BuildGraphicsMeshTask failed -- no indices");
        return;
    }

    if (hfmMesh.vertices.size() == 0) {
        HIFI_FCDEBUG_ID(model_baker(), repeatMessageID, "BuildGraphicsMeshTask failed -- no vertices");
        return;
    }

    int numVerts = hfmMesh.vertices.size();

    // evaluate all attribute elements and data sizes

    // Position is a vec3
    const auto positionElement = gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ); 
    const int positionsSize = numVerts * positionElement.getSize();

    // Normal and tangent are always there together packed in normalized xyz32bits word (times 2)
    const auto normalElement = HFM_NORMAL_ELEMENT;
    const int normalsSize = (int)meshNormals.size() * normalElement.getSize();
    const int tangentsSize = (int)meshTangents.size() * normalElement.getSize();
    // If there are normals then there should be tangents
    assert(normalsSize <= tangentsSize);
    if (tangentsSize > normalsSize) {
        HIFI_FCDEBUG_ID(model_baker(), repeatMessageID, "BuildGraphicsMeshTask -- Unexpected tangents in mesh");
    }
    const auto normalsAndTangentsSize = normalsSize + tangentsSize;

    // Color attrib
    const auto colorElement = HFM_COLOR_ELEMENT;
    const int colorsSize = hfmMesh.colors.size() * colorElement.getSize();

    // Texture coordinates are stored in 2 half floats
    const auto texCoordsElement = gpu::Element(gpu::VEC2, gpu::HALF, gpu::UV);
    const int texCoordsSize = hfmMesh.texCoords.size() * texCoordsElement.getSize();
    const int texCoords1Size = hfmMesh.texCoords1.size() * texCoordsElement.getSize();

    // Support for 4 skinning clusters:
    // 4 Indices are uint8 ideally, uint16 if more than 256.
    const auto clusterIndiceElement = ((meshDeformers.size() < (size_t)UINT8_MAX) ? gpu::Element(gpu::VEC4, gpu::UINT8, gpu::XYZW) : gpu::Element(gpu::VEC4, gpu::UINT16, gpu::XYZW));
    // 4 Weights are normalized 16bits
    const auto clusterWeightElement = gpu::Element(gpu::VEC4, gpu::NUINT16, gpu::XYZW);

    // Calculate a more condensed view of all the deformer weights
    const uint16_t NUM_CLUSTERS_PER_VERT = 4;
    ReweightedDeformers reweightedDeformers = getReweightedDeformers(hfmMesh.vertices.size(), dynamicTransform, meshDeformers, NUM_CLUSTERS_PER_VERT);
    // Cluster indices and weights must be the same sizes
    if (reweightedDeformers.trimmedToMatch) {
        HIFI_FCDEBUG_ID(model_baker(), repeatMessageID, "BuildGraphicsMeshTask -- The number of indices and weights for a deformer had different sizes and have been trimmed to match");
    }
    // Record cluster sizes
    const int numVertClusters = reweightedDeformers.indices.size() / NUM_CLUSTERS_PER_VERT;
    const int clusterIndicesSize = numVertClusters * clusterIndiceElement.getSize();
    const int clusterWeightsSize = numVertClusters * clusterWeightElement.getSize();

    // Decide on where to put what seequencially in a big buffer:
    const int positionsOffset = 0;
    const int normalsAndTangentsOffset = positionsOffset + positionsSize;
    const int colorsOffset = normalsAndTangentsOffset + normalsAndTangentsSize;
    const int texCoordsOffset = colorsOffset + colorsSize;
    const int texCoords1Offset = texCoordsOffset + texCoordsSize;
    const int clusterIndicesOffset = texCoords1Offset + texCoords1Size;
    const int clusterWeightsOffset = clusterIndicesOffset + clusterIndicesSize;
    const int totalVertsSize = clusterWeightsOffset + clusterWeightsSize;

    // Copy all vertex data in a single buffer
    auto vertBuffer = std::make_shared<gpu::Buffer>();
    vertBuffer->resize(totalVertsSize);

    // First positions
    vertBuffer->setSubData(positionsOffset, positionsSize, (const gpu::Byte*) hfmMesh.vertices.data());

    // Interleave normals and tangents
    if (normalsSize > 0) {
        std::vector<NormalType> normalsAndTangents;

        normalsAndTangents.reserve(meshNormals.size() + (int)meshTangents.size());
        auto normalIt = meshNormals.cbegin();
        auto tangentIt = meshTangents.cbegin();
        for (;
            normalIt != meshNormals.cend();
            ++normalIt, ++tangentIt) {
#if HFM_PACK_NORMALS
            const auto normal = normalizeDirForPacking(*normalIt);
            const auto tangent = normalizeDirForPacking(*tangentIt);
            const auto packedNormal = glm_packSnorm3x10_1x2(glm::vec4(normal, 0.0f));
            const auto packedTangent = glm_packSnorm3x10_1x2(glm::vec4(tangent, 0.0f));
#else
            const auto packedNormal = *normalIt;
            const auto packedTangent = *tangentIt;
#endif
            normalsAndTangents.push_back(packedNormal);
            normalsAndTangents.push_back(packedTangent);
        }
        vertBuffer->setSubData(normalsAndTangentsOffset, normalsAndTangentsSize, (const gpu::Byte*) normalsAndTangents.data());
    }

    // Pack colors
    if (colorsSize > 0) {
#if HFM_PACK_COLORS
        std::vector<ColorType> colors;

        colors.reserve(hfmMesh.colors.size());
        for (const auto& color : hfmMesh.colors) {
            colors.push_back(glm::packUnorm4x8(glm::vec4(color, 1.0f)));
        }
        vertBuffer->setSubData(colorsOffset, colorsSize, (const gpu::Byte*) colors.data());
#else
        vertBuffer->setSubData(colorsOffset, colorsSize, (const gpu::Byte*) hfmMesh.colors.constData());
#endif
    }

    // Pack Texcoords 0 and 1 (if exists)
    if (texCoordsSize > 0) {
        QVector<vec2h> texCoordData;
        texCoordData.reserve(hfmMesh.texCoords.size());
        for (auto& texCoordVec2f : hfmMesh.texCoords) {
            vec2h texCoordVec2h;

            texCoordVec2h.x = glm::detail::toFloat16(texCoordVec2f.x);
            texCoordVec2h.y = glm::detail::toFloat16(texCoordVec2f.y);
            texCoordData.push_back(texCoordVec2h);
        }
        vertBuffer->setSubData(texCoordsOffset, texCoordsSize, (const gpu::Byte*) texCoordData.constData());
    }
    if (texCoords1Size > 0) {
        QVector<vec2h> texCoordData;
        texCoordData.reserve(hfmMesh.texCoords1.size());
        for (auto& texCoordVec2f : hfmMesh.texCoords1) {
            vec2h texCoordVec2h;

            texCoordVec2h.x = glm::detail::toFloat16(texCoordVec2f.x);
            texCoordVec2h.y = glm::detail::toFloat16(texCoordVec2f.y);
            texCoordData.push_back(texCoordVec2h);
        }
        vertBuffer->setSubData(texCoords1Offset, texCoords1Size, (const gpu::Byte*) texCoordData.constData());
    }

    // Clusters data
    if (clusterIndicesSize > 0) {
        if (meshDeformers.size() < UINT8_MAX) {
            // yay! we can fit the clusterIndices within 8-bits
            int32_t numIndices = reweightedDeformers.indices.size();
            std::vector<uint8_t> packedDeformerIndices;
            packedDeformerIndices.resize(numIndices);
            for (int32_t i = 0; i < numIndices; ++i) {
                assert(hfmMesh.clusterIndices[i] <= UINT8_MAX);
                packedDeformerIndices[i] = (uint8_t)(reweightedDeformers.indices[i]);
            }
            vertBuffer->setSubData(clusterIndicesOffset, clusterIndicesSize, (const gpu::Byte*) packedDeformerIndices.data());
        } else {
            vertBuffer->setSubData(clusterIndicesOffset, clusterIndicesSize, (const gpu::Byte*) reweightedDeformers.indices.data());
        }
    }
    if (clusterWeightsSize > 0) {
        vertBuffer->setSubData(clusterWeightsOffset, clusterWeightsSize, (const gpu::Byte*) reweightedDeformers.weights.data());
    }


    // Now we decide on how to interleave the attributes and provide the vertices among bufers:
    // Aka the Vertex format and the vertexBufferStream
    auto vertexFormat = std::make_shared<gpu::Stream::Format>();
    auto vertexBufferStream = std::make_shared<gpu::BufferStream>();

    gpu::BufferPointer attribBuffer;
    int totalAttribBufferSize = totalVertsSize;
    gpu::uint8 posChannel = 0;
    gpu::uint8 tangentChannel = posChannel;
    gpu::uint8 attribChannel = posChannel;
    bool interleavePositions = true;
    bool interleaveNormalsTangents = true;

    // Define the vertex format, compute the offset for each attributes as we append them to the vertex format
    gpu::Offset bufOffset = 0;
    if (positionsSize) {
        vertexFormat->setAttribute(gpu::Stream::POSITION, posChannel, positionElement, bufOffset);
        bufOffset += positionElement.getSize();
        if (!interleavePositions) {
            bufOffset = 0;
        }
    }
    if (normalsSize) {
        vertexFormat->setAttribute(gpu::Stream::NORMAL, tangentChannel, normalElement, bufOffset);
        bufOffset += normalElement.getSize();
        vertexFormat->setAttribute(gpu::Stream::TANGENT, tangentChannel, normalElement, bufOffset);
        bufOffset += normalElement.getSize();
        if (!interleaveNormalsTangents) {
            bufOffset = 0;
        }
    }

    // Pack normal and Tangent with the rest of atributes
    if (colorsSize) {
        vertexFormat->setAttribute(gpu::Stream::COLOR, attribChannel, colorElement, bufOffset);
        bufOffset += colorElement.getSize();
    }
    if (texCoordsSize) {
        vertexFormat->setAttribute(gpu::Stream::TEXCOORD, attribChannel, texCoordsElement, bufOffset);
        bufOffset += texCoordsElement.getSize();
    }
    if (texCoords1Size) {
        vertexFormat->setAttribute(gpu::Stream::TEXCOORD1, attribChannel, texCoordsElement, bufOffset);
        bufOffset += texCoordsElement.getSize();
    } else if (texCoordsSize) {
        vertexFormat->setAttribute(gpu::Stream::TEXCOORD1, attribChannel, texCoordsElement, bufOffset - texCoordsElement.getSize());
    }
    if (clusterIndicesSize) {
        vertexFormat->setAttribute(gpu::Stream::SKIN_CLUSTER_INDEX, attribChannel, clusterIndiceElement, bufOffset);
        bufOffset += clusterIndiceElement.getSize();
    }
    if (clusterWeightsSize) {
        vertexFormat->setAttribute(gpu::Stream::SKIN_CLUSTER_WEIGHT, attribChannel, clusterWeightElement, bufOffset);
        bufOffset += clusterWeightElement.getSize();
    }

    // Finally, allocate and fill the attribBuffer interleaving the attributes as needed:
    {
        auto vPositionOffset = 0;
        auto vPositionSize = (interleavePositions ? positionsSize / numVerts : 0);

        auto vNormalsAndTangentsOffset = vPositionOffset + vPositionSize;
        auto vNormalsAndTangentsSize = (interleaveNormalsTangents ? normalsAndTangentsSize / numVerts : 0);

        auto vColorOffset = vNormalsAndTangentsOffset + vNormalsAndTangentsSize;
        auto vColorSize = colorsSize / numVerts;

        auto vTexcoord0Offset = vColorOffset + vColorSize;
        auto vTexcoord0Size = texCoordsSize / numVerts;

        auto vTexcoord1Offset = vTexcoord0Offset + vTexcoord0Size;
        auto vTexcoord1Size = texCoords1Size / numVerts;

        auto vClusterIndiceOffset = vTexcoord1Offset + vTexcoord1Size;
        auto vClusterIndiceSize = clusterIndicesSize / numVerts;

        auto vClusterWeightOffset = vClusterIndiceOffset + vClusterIndiceSize;
        auto vClusterWeightSize = clusterWeightsSize / numVerts;

        auto vStride = vClusterWeightOffset + vClusterWeightSize;

        std::vector<gpu::Byte> dest;
        dest.resize(totalAttribBufferSize);
        auto vDest = dest.data();

        auto source = vertBuffer->getData();

        for (int i = 0; i < numVerts; i++) {

            if (vPositionSize) memcpy(vDest + vPositionOffset, source + positionsOffset + i * vPositionSize, vPositionSize);
            if (vNormalsAndTangentsSize) memcpy(vDest + vNormalsAndTangentsOffset, source + normalsAndTangentsOffset + i * vNormalsAndTangentsSize, vNormalsAndTangentsSize);
            if (vColorSize) memcpy(vDest + vColorOffset, source + colorsOffset + i * vColorSize, vColorSize);
            if (vTexcoord0Size) memcpy(vDest + vTexcoord0Offset, source + texCoordsOffset + i * vTexcoord0Size, vTexcoord0Size);
            if (vTexcoord1Size) memcpy(vDest + vTexcoord1Offset, source + texCoords1Offset + i * vTexcoord1Size, vTexcoord1Size);
            if (vClusterIndiceSize) memcpy(vDest + vClusterIndiceOffset, source + clusterIndicesOffset + i * vClusterIndiceSize, vClusterIndiceSize);
            if (vClusterWeightSize) memcpy(vDest + vClusterWeightOffset, source + clusterWeightsOffset + i * vClusterWeightSize, vClusterWeightSize);

            vDest += vStride;
        }

        auto attribBuffer = std::make_shared<gpu::Buffer>();
        attribBuffer->setData(totalAttribBufferSize, dest.data());
        vertexBufferStream->addBuffer(attribBuffer, 0, vStride);
    }

    // Mesh vertex format and vertex stream is ready
    graphicsMesh->setVertexFormatAndStream(vertexFormat, vertexBufferStream);

    // Index and Part Buffers
    unsigned int totalIndices = 0;
    foreach(const HFMMeshPart& part, hfmMesh.parts) {
        totalIndices += (part.quadTrianglesIndices.size() + part.triangleIndices.size());
    }

    if (!totalIndices) {
        HIFI_FCDEBUG_ID(model_baker(), repeatMessageID, "BuildGraphicsMeshTask failed -- no indices");
        return;
    }

    auto indexBuffer = std::make_shared<gpu::Buffer>();
    indexBuffer->resize(totalIndices * sizeof(int));

    int indexNum = 0;
    int offset = 0;

    std::vector< graphics::Mesh::Part > parts;
    if (hfmMesh.parts.size() > 1) {
        indexNum = 0;
    }
    foreach(const HFMMeshPart& part, hfmMesh.parts) {
        graphics::Mesh::Part modelPart(indexNum, 0, 0, graphics::Mesh::TRIANGLES);

        if (part.quadTrianglesIndices.size()) {
            indexBuffer->setSubData(offset,
                part.quadTrianglesIndices.size() * sizeof(int),
                (gpu::Byte*) part.quadTrianglesIndices.constData());
            offset += part.quadTrianglesIndices.size() * sizeof(int);
            indexNum += part.quadTrianglesIndices.size();
            modelPart._numIndices += part.quadTrianglesIndices.size();
        }

        if (part.triangleIndices.size()) {
            indexBuffer->setSubData(offset,
                part.triangleIndices.size() * sizeof(int),
                (gpu::Byte*) part.triangleIndices.constData());
            offset += part.triangleIndices.size() * sizeof(int);
            indexNum += part.triangleIndices.size();
            modelPart._numIndices += part.triangleIndices.size();
        }

        parts.push_back(modelPart);
    }

    gpu::BufferView indexBufferView(indexBuffer, gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::XYZ));
    graphicsMesh->setIndexBuffer(indexBufferView);

    if (parts.size()) {
        auto pb = std::make_shared<gpu::Buffer>();
        pb->setData(parts.size() * sizeof(graphics::Mesh::Part), (const gpu::Byte*) parts.data());
        gpu::BufferView pbv(pb, gpu::Element(gpu::VEC4, gpu::UINT32, gpu::XYZW));
        graphicsMesh->setPartBuffer(pbv);
    } else {
        HIFI_FCDEBUG_ID(model_baker(), repeatMessageID, "BuildGraphicsMeshTask failed -- no parts");
        return;
    }

    graphicsMesh->evalPartBound(0);

    graphicsMeshPointer = graphicsMesh;
}

void BuildGraphicsMeshTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& meshes = input.get0();
    const auto& url = input.get1();
    const auto& meshIndicesToModelNames = input.get2();
    const auto& normalsPerMesh = input.get3();
    const auto& tangentsPerMesh = input.get4();
    const auto& shapes = input.get5();
    const auto& dynamicTransforms = input.get6();
    const auto& deformers = input.get7();

    // Currently, there is only (at most) one dynamicTransform per mesh
    // An undefined shape.dynamicTransform has the value hfm::Shape::UNDEFINED_KEY
    std::vector<uint32_t> dynamicTransformPerMesh;
    dynamicTransformPerMesh.resize(meshes.size(), hfm::Shape::UNDEFINED_KEY);
    for (const auto& shape : shapes) {
        uint32_t dynamicTransformIndex = shape.dynamicTransform;
        dynamicTransformPerMesh[shape.mesh] = dynamicTransformIndex;
    }

    auto& graphicsMeshes = output;

    int n = (int)meshes.size();
    for (int i = 0; i < n; i++) {
        graphicsMeshes.emplace_back();
        auto& graphicsMesh = graphicsMeshes[i];

        auto dynamicTransformIndex = dynamicTransformPerMesh[i];
        const hfm::DynamicTransform* dynamicTransform = nullptr;
        std::vector<const hfm::Deformer*> meshDeformers;
        if (dynamicTransformIndex != hfm::Shape::UNDEFINED_KEY) {
            dynamicTransform = &dynamicTransforms[dynamicTransformIndex];
            for (const auto& deformerIndex : dynamicTransform->deformers) {
                const auto& deformer = deformers[deformerIndex];
                meshDeformers.push_back(&deformer);
            }
        }

        // Try to create the graphics::Mesh
        buildGraphicsMesh(meshes[i], graphicsMesh, baker::safeGet(normalsPerMesh, i), baker::safeGet(tangentsPerMesh, i), dynamicTransform, meshDeformers);

        // Choose a name for the mesh
        if (graphicsMesh) {
            graphicsMesh->displayName = url.toString().toStdString() + "#/mesh/" + std::to_string(i);
            if (meshIndicesToModelNames.find(i) != meshIndicesToModelNames.cend()) {
                graphicsMesh->modelName = meshIndicesToModelNames[i].toStdString();
            }
        }
    }
}
