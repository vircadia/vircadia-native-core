//
//  BuildDracoMeshTask.cpp
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/02/20.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BuildDracoMeshTask.h"

// Fix build warnings due to draco headers not casting size_t
#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4267 )
#endif
// gcc and clang
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif


#ifndef Q_OS_ANDROID
#include <draco/compression/encode.h>
#include <draco/mesh/triangle_soup_mesh_builder.h>
#endif

#ifdef _WIN32
#pragma warning( pop )
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "ModelBakerLogging.h"
#include "ModelMath.h"

#ifndef Q_OS_ANDROID

void reindexMaterials(const std::vector<uint32_t>& originalMaterialIndices, std::vector<uint32_t>& materials, std::vector<uint16_t>& materialIndices) {
    materialIndices.resize(originalMaterialIndices.size());
    for (size_t i = 0; i < originalMaterialIndices.size(); ++i) {
        uint32_t material = originalMaterialIndices[i];
        auto foundMaterial = std::find(materials.cbegin(), materials.cend(), material);
        if (foundMaterial == materials.cend()) {
            materials.push_back(material);
            materialIndices[i] = (uint16_t)(materials.size() - 1);
        } else {
            materialIndices[i] = (uint16_t)(foundMaterial - materials.cbegin());
        }
    }
}

void createMaterialLists(const std::vector<hfm::Shape>& shapes, const std::vector<hfm::Mesh>& meshes, const std::vector<hfm::Material>& hfmMaterials, std::vector<std::vector<hifi::ByteArray>>& materialIndexLists, std::vector<std::vector<uint16_t>>& partMaterialIndicesPerMesh) {
    std::vector<std::vector<uint32_t>> materialsPerMesh;
    for (const auto& mesh : meshes) {
        materialsPerMesh.emplace_back(mesh.parts.size(), hfm::UNDEFINED_KEY);
    }
    for (const auto& shape : shapes) {
        materialsPerMesh[shape.mesh][shape.meshPart] = shape.material;
    }

    materialIndexLists.resize(materialsPerMesh.size());
    partMaterialIndicesPerMesh.resize(materialsPerMesh.size());
    for (size_t i = 0; i < materialsPerMesh.size(); ++i) {
        const std::vector<uint32_t>& materials = materialsPerMesh[i];
        std::vector<uint32_t> uniqueMaterials;

        reindexMaterials(materials, uniqueMaterials, partMaterialIndicesPerMesh[i]);

        materialIndexLists[i].reserve(uniqueMaterials.size());
        for (const uint32_t material : uniqueMaterials) {
            const auto& hfmMaterial = hfmMaterials[material];
            materialIndexLists[i].push_back(QVariant(hfmMaterial.materialID).toByteArray());
        }
    }
}

std::tuple<std::unique_ptr<draco::Mesh>, bool> createDracoMesh(const hfm::Mesh& mesh, const std::vector<glm::vec3>& normals, const std::vector<glm::vec3>& tangents, const std::vector<uint16_t>& partMaterialIndices) {
    Q_ASSERT(normals.size() == 0 || (int)normals.size() == mesh.vertices.size());
    Q_ASSERT(mesh.colors.size() == 0 || mesh.colors.size() == mesh.vertices.size());
    Q_ASSERT(mesh.texCoords.size() == 0 || mesh.texCoords.size() == mesh.vertices.size());

    int64_t numTriangles{ 0 };
    for (auto& part : mesh.parts) {
        int extraQuadTriangleIndices = part.quadTrianglesIndices.size() % 3;
        int extraTriangleIndices = part.triangleIndices.size() % 3;
        if (extraQuadTriangleIndices != 0 || extraTriangleIndices != 0) {
            qCWarning(model_baker) << "Found a mesh part with indices not divisible by three. Some indices will be discarded during Draco mesh creation.";
        }
        numTriangles += (part.quadTrianglesIndices.size() - extraQuadTriangleIndices) / 3;
        numTriangles += (part.triangleIndices.size() - extraTriangleIndices) / 3;
    }

    if (numTriangles == 0) {
        return std::make_tuple(std::unique_ptr<draco::Mesh>(), false);
    }

    draco::TriangleSoupMeshBuilder meshBuilder;

    meshBuilder.Start(numTriangles);

    bool hasNormals{ normals.size() > 0 };
    bool hasColors{ mesh.colors.size() > 0 };
    bool hasTexCoords{ mesh.texCoords.size() > 0 };
    bool hasTexCoords1{ mesh.texCoords1.size() > 0 };
    bool hasPerFaceMaterials{ mesh.parts.size() > 1 };
    bool needsOriginalIndices{ (!mesh.clusterIndices.empty() || !mesh.blendshapes.empty()) && mesh.originalIndices.size() > 0 };

    int normalsAttributeID { -1 };
    int colorsAttributeID { -1 };
    int texCoordsAttributeID { -1 };
    int texCoords1AttributeID { -1 };
    int faceMaterialAttributeID { -1 };
    int originalIndexAttributeID { -1 };

    const int positionAttributeID = meshBuilder.AddAttribute(draco::GeometryAttribute::POSITION,
        3, draco::DT_FLOAT32);
    if (needsOriginalIndices) {
        originalIndexAttributeID = meshBuilder.AddAttribute(
            (draco::GeometryAttribute::Type)DRACO_ATTRIBUTE_ORIGINAL_INDEX,
            1, draco::DT_INT32);
    }

    if (hasNormals) {
        normalsAttributeID = meshBuilder.AddAttribute(draco::GeometryAttribute::NORMAL,
            3, draco::DT_FLOAT32);
    }
    if (hasColors) {
        colorsAttributeID = meshBuilder.AddAttribute(draco::GeometryAttribute::COLOR,
            3, draco::DT_FLOAT32);
    }
    if (hasTexCoords) {
        texCoordsAttributeID = meshBuilder.AddAttribute(draco::GeometryAttribute::TEX_COORD,
            2, draco::DT_FLOAT32);
    }
    if (hasTexCoords1) {
        texCoords1AttributeID = meshBuilder.AddAttribute(
            (draco::GeometryAttribute::Type)DRACO_ATTRIBUTE_TEX_COORD_1,
            2, draco::DT_FLOAT32);
    }
    if (hasPerFaceMaterials) {
        faceMaterialAttributeID = meshBuilder.AddAttribute(
            (draco::GeometryAttribute::Type)DRACO_ATTRIBUTE_MATERIAL_ID,
            1, draco::DT_UINT16);
    }

    auto partIndex = 0;
    draco::FaceIndex face;

    for (auto& part : mesh.parts) {
        uint16_t materialID = partMaterialIndices[partIndex];

        auto addFace = [&](const QVector<int>& indices, int index, draco::FaceIndex face) {
            int32_t idx0 = indices[index];
            int32_t idx1 = indices[index + 1];
            int32_t idx2 = indices[index + 2];

            if (hasPerFaceMaterials) {
                meshBuilder.SetPerFaceAttributeValueForFace(faceMaterialAttributeID, face, &materialID);
            }

            meshBuilder.SetAttributeValuesForFace(positionAttributeID, face,
                &mesh.vertices[idx0], &mesh.vertices[idx1],
                &mesh.vertices[idx2]);

            if (needsOriginalIndices) {
                meshBuilder.SetAttributeValuesForFace(originalIndexAttributeID, face,
                    &mesh.originalIndices[idx0],
                    &mesh.originalIndices[idx1],
                    &mesh.originalIndices[idx2]);
            }
            if (hasNormals) {
                meshBuilder.SetAttributeValuesForFace(normalsAttributeID, face,
                    &normals[idx0], &normals[idx1],
                    &normals[idx2]);
            }
            if (hasColors) {
                meshBuilder.SetAttributeValuesForFace(colorsAttributeID, face,
                    &mesh.colors[idx0], &mesh.colors[idx1],
                    &mesh.colors[idx2]);
            }
            if (hasTexCoords) {
                meshBuilder.SetAttributeValuesForFace(texCoordsAttributeID, face,
                    &mesh.texCoords[idx0], &mesh.texCoords[idx1],
                    &mesh.texCoords[idx2]);
            }
            if (hasTexCoords1) {
                meshBuilder.SetAttributeValuesForFace(texCoords1AttributeID, face,
                    &mesh.texCoords1[idx0], &mesh.texCoords1[idx1],
                    &mesh.texCoords1[idx2]);
            }
        };

        for (int i = 0; (i + 2) < part.quadTrianglesIndices.size(); i += 3) {
            addFace(part.quadTrianglesIndices, i, face++);
        }

        for (int i = 0; (i + 2) < part.triangleIndices.size(); i += 3) {
            addFace(part.triangleIndices, i, face++);
        }

        partIndex++;
    }

    auto dracoMesh = meshBuilder.Finalize();

    if (!dracoMesh) {
        qCWarning(model_baker) << "Failed to finalize the baking of a draco Geometry node";
        return std::make_tuple(std::unique_ptr<draco::Mesh>(), true);
    }

    // we need to modify unique attribute IDs for custom attributes
    // so the attributes are easily retrievable on the other side
    if (hasPerFaceMaterials) {
        dracoMesh->attribute(faceMaterialAttributeID)->set_unique_id(DRACO_ATTRIBUTE_MATERIAL_ID);
    }

    if (hasTexCoords1) {
        dracoMesh->attribute(texCoords1AttributeID)->set_unique_id(DRACO_ATTRIBUTE_TEX_COORD_1);
    }

    if (needsOriginalIndices) {
        dracoMesh->attribute(originalIndexAttributeID)->set_unique_id(DRACO_ATTRIBUTE_ORIGINAL_INDEX);
    }
    
    return std::make_tuple(std::move(dracoMesh), false);
}
#endif // not Q_OS_ANDROID

void BuildDracoMeshTask::configure(const Config& config) {
    _encodeSpeed = config.encodeSpeed;
    _decodeSpeed = config.decodeSpeed;
}

void BuildDracoMeshTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
#ifdef Q_OS_ANDROID
    qCWarning(model_baker) << "BuildDracoMesh is disabled on Android. Output meshes will be empty.";
#else
    const auto& shapes = input.get0();
    const auto& meshes = input.get1();
    const auto& materials = input.get2();
    const auto& normalsPerMesh = input.get3();
    const auto& tangentsPerMesh = input.get4();
    auto& dracoBytesPerMesh = output.edit0();
    auto& dracoErrorsPerMesh = output.edit1();

    auto& materialLists = output.edit2();
    std::vector<std::vector<uint16_t>> partMaterialIndicesPerMesh;
    createMaterialLists(shapes, meshes, materials, materialLists, partMaterialIndicesPerMesh);

    dracoBytesPerMesh.reserve(meshes.size());
    // vector<bool> is an exception to the std::vector conventions as it is a bit field
    // So a bool reference to an element doesn't work
    dracoErrorsPerMesh.resize(meshes.size());
    for (size_t i = 0; i < meshes.size(); i++) {
        const auto& mesh = meshes[i];
        const auto& normals = baker::safeGet(normalsPerMesh, i);
        const auto& tangents = baker::safeGet(tangentsPerMesh, i);
        dracoBytesPerMesh.emplace_back();
        auto& dracoBytes = dracoBytesPerMesh.back();
        const auto& partMaterialIndices = partMaterialIndicesPerMesh[i];

        bool dracoError;
        std::unique_ptr<draco::Mesh> dracoMesh;
        std::tie(dracoMesh, dracoError) = createDracoMesh(mesh, normals, tangents, partMaterialIndices);
        dracoErrorsPerMesh[i] = dracoError;

        if (dracoMesh) {
            draco::Encoder encoder;

            encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, 14);
            encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, 12);
            encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, 10);
            encoder.SetSpeedOptions(_encodeSpeed, _decodeSpeed);

            draco::EncoderBuffer buffer;
            encoder.EncodeMeshToBuffer(*dracoMesh, &buffer);

            dracoBytes = hifi::ByteArray(buffer.data(), (int)buffer.size());
        }
    }
#endif // not Q_OS_ANDROID
}
