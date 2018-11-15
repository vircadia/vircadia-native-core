//
//  HFM.cpp
//  libraries/hfm/src
//
//  Created by Sabrina Shanman on 2018/11/06.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFM.h"

#include "ModelFormatLogging.h"

void HFMMaterial::getTextureNames(QSet<QString>& textureList) const {
    if (!normalTexture.isNull()) {
        textureList.insert(normalTexture.name);
    }
    if (!albedoTexture.isNull()) {
        textureList.insert(albedoTexture.name);
    }
    if (!opacityTexture.isNull()) {
        textureList.insert(opacityTexture.name);
    }
    if (!glossTexture.isNull()) {
        textureList.insert(glossTexture.name);
    }
    if (!roughnessTexture.isNull()) {
        textureList.insert(roughnessTexture.name);
    }
    if (!specularTexture.isNull()) {
        textureList.insert(specularTexture.name);
    }
    if (!metallicTexture.isNull()) {
        textureList.insert(metallicTexture.name);
    }
    if (!emissiveTexture.isNull()) {
        textureList.insert(emissiveTexture.name);
    }
    if (!occlusionTexture.isNull()) {
        textureList.insert(occlusionTexture.name);
    }
    if (!scatteringTexture.isNull()) {
        textureList.insert(scatteringTexture.name);
    }
    if (!lightmapTexture.isNull()) {
        textureList.insert(lightmapTexture.name);
    }
}

void HFMMaterial::setMaxNumPixelsPerTexture(int maxNumPixels) {
    normalTexture.maxNumPixels = maxNumPixels;
    albedoTexture.maxNumPixels = maxNumPixels;
    opacityTexture.maxNumPixels = maxNumPixels;
    glossTexture.maxNumPixels = maxNumPixels;
    roughnessTexture.maxNumPixels = maxNumPixels;
    specularTexture.maxNumPixels = maxNumPixels;
    metallicTexture.maxNumPixels = maxNumPixels;
    emissiveTexture.maxNumPixels = maxNumPixels;
    occlusionTexture.maxNumPixels = maxNumPixels;
    scatteringTexture.maxNumPixels = maxNumPixels;
    lightmapTexture.maxNumPixels = maxNumPixels;
}

bool HFMMaterial::needTangentSpace() const {
    return !normalTexture.isNull();
}

static void _createBlendShapeTangents(HFMMesh& mesh, bool generateFromTexCoords, HFMBlendshape& blendShape);

void HFMMesh::createBlendShapeTangents(bool generateTangents) {
    for (auto& blendShape : blendshapes) {
        _createBlendShapeTangents(*this, generateTangents, blendShape);
    }
}

using IndexAccessor = std::function<glm::vec3*(const HFMMesh&, int, int, glm::vec3*, glm::vec3&)>;

static void setTangents(const HFMMesh& mesh, const IndexAccessor& vertexAccessor, int firstIndex, int secondIndex,
    const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals, QVector<glm::vec3>& tangents) {
    glm::vec3 vertex[2];
    glm::vec3 normal;
    glm::vec3* tangent = vertexAccessor(mesh, firstIndex, secondIndex, vertex, normal);
    if (tangent) {
        glm::vec3 bitangent = glm::cross(normal, vertex[1] - vertex[0]);
        if (glm::length(bitangent) < EPSILON) {
            return;
        }
        glm::vec2 texCoordDelta = mesh.texCoords.at(secondIndex) - mesh.texCoords.at(firstIndex);
        glm::vec3 normalizedNormal = glm::normalize(normal);
        *tangent += glm::cross(glm::angleAxis(-atan2f(-texCoordDelta.t, texCoordDelta.s), normalizedNormal) *
            glm::normalize(bitangent), normalizedNormal);
    }
}

static void createTangents(const HFMMesh& mesh, bool generateFromTexCoords,
    const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals, QVector<glm::vec3>& tangents,
    IndexAccessor accessor) {
    // if we have a normal map (and texture coordinates), we must compute tangents
    if (generateFromTexCoords && !mesh.texCoords.isEmpty()) {
        tangents.resize(vertices.size());

        foreach(const HFMMeshPart& part, mesh.parts) {
            for (int i = 0; i < part.quadIndices.size(); i += 4) {
                setTangents(mesh, accessor, part.quadIndices.at(i), part.quadIndices.at(i + 1), vertices, normals, tangents);
                setTangents(mesh, accessor, part.quadIndices.at(i + 1), part.quadIndices.at(i + 2), vertices, normals, tangents);
                setTangents(mesh, accessor, part.quadIndices.at(i + 2), part.quadIndices.at(i + 3), vertices, normals, tangents);
                setTangents(mesh, accessor, part.quadIndices.at(i + 3), part.quadIndices.at(i), vertices, normals, tangents);
            }
            // <= size - 3 in order to prevent overflowing triangleIndices when (i % 3) != 0
            // This is most likely evidence of a further problem in extractMesh()
            for (int i = 0; i <= part.triangleIndices.size() - 3; i += 3) {
                setTangents(mesh, accessor, part.triangleIndices.at(i), part.triangleIndices.at(i + 1), vertices, normals, tangents);
                setTangents(mesh, accessor, part.triangleIndices.at(i + 1), part.triangleIndices.at(i + 2), vertices, normals, tangents);
                setTangents(mesh, accessor, part.triangleIndices.at(i + 2), part.triangleIndices.at(i), vertices, normals, tangents);
            }
            if ((part.triangleIndices.size() % 3) != 0) {
                qCDebug(modelformat) << "Error in extractHFMModel part.triangleIndices.size() is not divisible by three ";
            }
        }
    }
}

void HFMMesh::createMeshTangents(bool generateFromTexCoords) {
    HFMMesh& mesh = *this;
    // This is the only workaround I've found to trick the compiler into understanding that mesh.tangents isn't
    // const in the lambda function.
    auto& tangents = mesh.tangents;
    createTangents(mesh, generateFromTexCoords, mesh.vertices, mesh.normals, mesh.tangents, 
        [&](const HFMMesh& mesh, int firstIndex, int secondIndex, glm::vec3* outVertices, glm::vec3& outNormal) {
        outVertices[0] = mesh.vertices[firstIndex];
        outVertices[1] = mesh.vertices[secondIndex];
        outNormal = mesh.normals[firstIndex];
        return &(tangents[firstIndex]);
    });
}

static void _createBlendShapeTangents(HFMMesh& mesh, bool generateFromTexCoords, HFMBlendshape& blendShape) {
    // Create lookup to get index in blend shape from vertex index in mesh
    std::vector<int> reverseIndices;
    reverseIndices.resize(mesh.vertices.size());
    std::iota(reverseIndices.begin(), reverseIndices.end(), 0);

    for (int indexInBlendShape = 0; indexInBlendShape < blendShape.indices.size(); ++indexInBlendShape) {
        auto indexInMesh = blendShape.indices[indexInBlendShape];
        reverseIndices[indexInMesh] = indexInBlendShape;
    }

    createTangents(mesh, generateFromTexCoords, blendShape.vertices, blendShape.normals, blendShape.tangents,
        [&](const HFMMesh& mesh, int firstIndex, int secondIndex, glm::vec3* outVertices, glm::vec3& outNormal) {
        const auto index1 = reverseIndices[firstIndex];
        const auto index2 = reverseIndices[secondIndex];

        if (index1 < blendShape.vertices.size()) {
            outVertices[0] = blendShape.vertices[index1];
            if (index2 < blendShape.vertices.size()) {
                outVertices[1] = blendShape.vertices[index2];
            } else {
                // Index isn't in the blend shape so return vertex from mesh
                outVertices[1] = mesh.vertices[secondIndex];
            }
            outNormal = blendShape.normals[index1];
            return &blendShape.tangents[index1];
        } else {
            // Index isn't in blend shape so return nullptr
            return (glm::vec3*)nullptr;
        }
    });
}

QStringList HFMModel::getJointNames() const {
    QStringList names;
    foreach (const HFMJoint& joint, joints) {
        names.append(joint.name);
    }
    return names;
}

bool HFMModel::hasBlendedMeshes() const {
    if (!meshes.isEmpty()) {
        foreach (const HFMMesh& mesh, meshes) {
            if (!mesh.blendshapes.isEmpty()) {
                return true;
            }
        }
    }
    return false;
}

Extents HFMModel::getUnscaledMeshExtents() const {
    const Extents& extents = meshExtents;

    // even though our caller asked for "unscaled" we need to include any fst scaling, translation, and rotation, which
    // is captured in the offset matrix
    glm::vec3 minimum = glm::vec3(offset * glm::vec4(extents.minimum, 1.0f));
    glm::vec3 maximum = glm::vec3(offset * glm::vec4(extents.maximum, 1.0f));
    Extents scaledExtents = { minimum, maximum };

    return scaledExtents;
}

// TODO: Move to graphics::Mesh when Sam's ready
bool HFMModel::convexHullContains(const glm::vec3& point) const {
    if (!getUnscaledMeshExtents().containsPoint(point)) {
        return false;
    }

    auto checkEachPrimitive = [=](HFMMesh& mesh, QVector<int> indices, int primitiveSize) -> bool {
        // Check whether the point is "behind" all the primitives.
        int verticesSize = mesh.vertices.size();
        for (int j = 0;
            j < indices.size() - 2; // -2 in case the vertices aren't the right size -- we access j + 2 below
            j += primitiveSize) {
            if (indices[j] < verticesSize &&
                indices[j + 1] < verticesSize &&
                indices[j + 2] < verticesSize &&
                !isPointBehindTrianglesPlane(point,
                    mesh.vertices[indices[j]],
                    mesh.vertices[indices[j + 1]],
                    mesh.vertices[indices[j + 2]])) {
                // it's not behind at least one so we bail
                return false;
            }
        }
        return true;
    };

    // Check that the point is contained in at least one convex mesh.
    for (auto mesh : meshes) {
        bool insideMesh = true;

        // To be considered inside a convex mesh,
        // the point needs to be "behind" all the primitives respective planes.
        for (auto part : mesh.parts) {
            // run through all the triangles and quads
            if (!checkEachPrimitive(mesh, part.triangleIndices, 3) ||
                !checkEachPrimitive(mesh, part.quadIndices, 4)) {
                // If not, the point is outside, bail for this mesh
                insideMesh = false;
                continue;
            }
        }
        if (insideMesh) {
            // It's inside this mesh, return true.
            return true;
        }
    }

    // It wasn't in any mesh, return false.
    return false;
}

QString HFMModel::getModelNameOfMesh(int meshIndex) const {
    if (meshIndicesToModelNames.contains(meshIndex)) {
        return meshIndicesToModelNames.value(meshIndex);
    }
    return QString();
}
