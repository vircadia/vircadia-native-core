//
//  ModelMath.cpp
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/01/18.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelMath.h"

#include <LogHandler.h>
#include "ModelBakerLogging.h"

namespace baker {
    template<class T>
    const T& checkedAt(const QVector<T>& vector, int i) {
        if (i < 0 || i >= vector.size()) {
            throw std::out_of_range("baker::checked_at (ModelMath.cpp): index " + std::to_string(i) + " is out of range");
        }
        return vector[i];
    }

    template<class T>
    const T& checkedAt(const std::vector<T>& vector, int i) {
        if (i < 0 || i >= vector.size()) {
            throw std::out_of_range("baker::checked_at (ModelMath.cpp): index " + std::to_string(i) + " is out of range");
        }
        return vector[i];
    }

    template<class T>
    T& checkedAt(std::vector<T>& vector, int i) {
        if (i < 0 || i >= vector.size()) {
            throw std::out_of_range("baker::checked_at (ModelMath.cpp): index " + std::to_string(i) + " is out of range");
        }
        return vector[i];
    }

    void setTangent(const HFMMesh& mesh, const IndexAccessor& vertexAccessor, int firstIndex, int secondIndex) {
        glm::vec3 vertex[2];
        glm::vec2 texCoords[2];
        glm::vec3 normal;
        glm::vec3* tangent = vertexAccessor(firstIndex, secondIndex, vertex, texCoords, normal);
        if (tangent) {
            glm::vec3 bitangent = glm::cross(normal, vertex[1] - vertex[0]);
            if (glm::length(bitangent) < EPSILON) {
                return;
            }
            glm::vec2 texCoordDelta = texCoords[1] - texCoords[0];
            glm::vec3 normalizedNormal = glm::normalize(normal);
            *tangent += glm::cross(glm::angleAxis(-atan2f(-texCoordDelta.t, texCoordDelta.s), normalizedNormal) *
                glm::normalize(bitangent), normalizedNormal);
        }
    }

    void calculateNormals(const hfm::Mesh& mesh, NormalAccessor normalAccessor, VertexSetter vertexSetter) {
        static int repeatMessageID = LogHandler::getInstance().newRepeatedMessageID();
        for (const HFMMeshPart& part : mesh.parts) {
            for (int i = 0; i < part.quadIndices.size(); i += 4) {
                glm::vec3* n0 = normalAccessor(part.quadIndices[i]);
                glm::vec3* n1 = normalAccessor(part.quadIndices[i + 1]);
                glm::vec3* n2 = normalAccessor(part.quadIndices[i + 2]);
                glm::vec3* n3 = normalAccessor(part.quadIndices[i + 3]);
                if (!n0 || !n1 || !n2 || !n3) {
                    // Quad is not in the mesh (can occur with blendshape meshes, which are a subset of the hfm Mesh vertices)
                    continue;
                }
                glm::vec3 vertices[3]; // Assume all vertices in this quad are in the same plane, so only the first three are needed to calculate the normal
                vertexSetter(part.quadIndices[i], vertices[0]);
                vertexSetter(part.quadIndices[i + 1], vertices[1]);
                vertexSetter(part.quadIndices[i + 2], vertices[2]);
                *n0 = *n1 = *n2 = *n3 = glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]);
            }
            // <= size - 3 in order to prevent overflowing triangleIndices when (i % 3) != 0
            // This is most likely evidence of a further problem in extractMesh()
            for (int i = 0; i <= part.triangleIndices.size() - 3; i += 3) {
                glm::vec3* n0 = normalAccessor(part.triangleIndices[i]);
                glm::vec3* n1 = normalAccessor(part.triangleIndices[i + 1]);
                glm::vec3* n2 = normalAccessor(part.triangleIndices[i + 2]);
                if (!n0 || !n1 || !n2) {
                    // Tri is not in the mesh (can occur with blendshape meshes, which are a subset of the hfm Mesh vertices)
                    continue;
                }
                glm::vec3 vertices[3];
                vertexSetter(part.triangleIndices[i], vertices[0]);
                vertexSetter(part.triangleIndices[i + 1], vertices[1]);
                vertexSetter(part.triangleIndices[i + 2], vertices[2]);
                *n0 = *n1 = *n2 = glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]);
            }
            if ((part.triangleIndices.size() % 3) != 0) {
                HIFI_FCDEBUG_ID(model_baker(), repeatMessageID, "Error in baker::calculateNormals: part.triangleIndices.size() is not divisible by three");
            }
        }
    }

    void calculateTangents(const hfm::Mesh& mesh, IndexAccessor accessor) {
        static int repeatMessageID = LogHandler::getInstance().newRepeatedMessageID();
        for (const HFMMeshPart& part : mesh.parts) {
            for (int i = 0; i < part.quadIndices.size(); i += 4) {
                setTangent(mesh, accessor, part.quadIndices.at(i), part.quadIndices.at(i + 1));
                setTangent(mesh, accessor, part.quadIndices.at(i + 1), part.quadIndices.at(i + 2));
                setTangent(mesh, accessor, part.quadIndices.at(i + 2), part.quadIndices.at(i + 3));
                setTangent(mesh, accessor, part.quadIndices.at(i + 3), part.quadIndices.at(i));
            }
            // <= size - 3 in order to prevent overflowing triangleIndices when (i % 3) != 0
            // This is most likely evidence of a further problem in extractMesh()
            for (int i = 0; i <= part.triangleIndices.size() - 3; i += 3) {
                setTangent(mesh, accessor, part.triangleIndices.at(i), part.triangleIndices.at(i + 1));
                setTangent(mesh, accessor, part.triangleIndices.at(i + 1), part.triangleIndices.at(i + 2));
                setTangent(mesh, accessor, part.triangleIndices.at(i + 2), part.triangleIndices.at(i));
            }
            if ((part.triangleIndices.size() % 3) != 0) {
                HIFI_FCDEBUG_ID(model_baker(), repeatMessageID, "Error in baker::calculateTangents: part.triangleIndices.size() is not divisible by three");
            }
        }
    }
}

