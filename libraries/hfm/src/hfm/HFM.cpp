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
    // even though our caller asked for "unscaled" we need to include any fst scaling, translation, and rotation, which
    // is captured in the offset matrix
    glm::vec3 minimum = glm::vec3(offset * glm::vec4(meshExtents.minimum, 1.0f));
    glm::vec3 maximum = glm::vec3(offset * glm::vec4(meshExtents.maximum, 1.0f));
    Extents scaledExtents = { minimum, maximum };
    return scaledExtents;
}

// TODO: Move to graphics::Mesh when Sam's ready
bool HFMModel::convexHullContains(const glm::vec3& point) const {
    if (!meshExtents.containsPoint(point)) {
        return false;
    }

    auto checkEachPrimitive = [=](HFMMesh& mesh, QVector<int> indices, int primitiveSize) -> bool {
        // Check whether the point is "behind" all the primitives.
        // But first must transform from model-frame into mesh-frame
        glm::vec3 transformedPoint = glm::vec3(glm::inverse(mesh.modelTransform) * glm::vec4(point, 1.0f));
        int verticesSize = mesh.vertices.size();
        for (int j = 0;
            j < indices.size() - 2; // -2 in case the vertices aren't the right size -- we access j + 2 below
            j += primitiveSize) {
            if (indices[j] < verticesSize &&
                indices[j + 1] < verticesSize &&
                indices[j + 2] < verticesSize &&
                !isPointBehindTrianglesPlane(transformedPoint,
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

void HFMModel::computeKdops() {
    const float INV_SQRT_3 = 0.57735026918f;
    ShapeVertices cardinalDirections = {
        Vectors::UNIT_X,
        Vectors::UNIT_Y,
        Vectors::UNIT_Z,
        glm::vec3(INV_SQRT_3,  INV_SQRT_3,  INV_SQRT_3),
        glm::vec3(INV_SQRT_3, -INV_SQRT_3,  INV_SQRT_3),
        glm::vec3(INV_SQRT_3,  INV_SQRT_3, -INV_SQRT_3),
        glm::vec3(INV_SQRT_3, -INV_SQRT_3, -INV_SQRT_3)
    };
    if (joints.size() != (int)shapeVertices.size()) {
        return;
    }
    // now that all joints have been scanned compute a k-Dop bounding volume of mesh
    for (int i = 0; i < joints.size(); ++i) {
        HFMJoint& joint = joints[i];

        // NOTE: points are in joint-frame
        ShapeVertices& points = shapeVertices.at(i);
        glm::quat rotOffset = jointRotationOffsets.contains(i) ? glm::inverse(jointRotationOffsets[i]) : quat();
        if (points.size() > 0) {
            // compute average point
            glm::vec3 avgPoint = glm::vec3(0.0f);
            for (uint32_t j = 0; j < points.size(); ++j) {
                points[j] = rotOffset * points[j];
                avgPoint += points[j];
            }
            avgPoint /= (float)points.size();
            joint.shapeInfo.avgPoint = avgPoint;

            // compute a k-Dop bounding volume
            for (uint32_t j = 0; j < cardinalDirections.size(); ++j) {
                float maxDot = -FLT_MAX;
                float minDot = FLT_MIN;
                for (uint32_t k = 0; k < points.size(); ++k) {
                    float kDot = glm::dot(cardinalDirections[j], points[k] - avgPoint);
                    if (kDot > maxDot) {
                        maxDot = kDot;
                    }
                    if (kDot < minDot) {
                        minDot = kDot;
                    }
                }
                joint.shapeInfo.points.push_back(avgPoint + maxDot * cardinalDirections[j]);
                joint.shapeInfo.dots.push_back(maxDot);
                joint.shapeInfo.points.push_back(avgPoint + minDot * cardinalDirections[j]);
                joint.shapeInfo.dots.push_back(-minDot);
            }
            generateBoundryLinesForDop14(joint.shapeInfo.dots, joint.shapeInfo.avgPoint, joint.shapeInfo.debugLines);
        }
    }
}
