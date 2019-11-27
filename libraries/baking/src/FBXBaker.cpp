//
//  FBXBaker.cpp
//  tools/baking/src
//
//  Created by Stephen Birarda on 3/30/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FBXBaker.h"

#include <cmath> // need this include so we don't get an error looking for std::isnan

#include <QtConcurrent>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>

#include <mutex>

#include <NetworkAccessManager.h>
#include <SharedUtil.h>

#include <PathUtils.h>

#include <FBXSerializer.h>
#include <FBXWriter.h>

#include "ModelBakingLoggingCategory.h"

FBXBaker::FBXBaker(const QUrl& inputModelURL, const QString& bakedOutputDirectory, const QString& originalOutputDirectory, bool hasBeenBaked) :
        ModelBaker(inputModelURL, bakedOutputDirectory, originalOutputDirectory, hasBeenBaked) {
    if (hasBeenBaked) {
        // Look for the original model file one directory higher. Perhaps this is an oven output directory.
        QUrl originalRelativePath = QUrl("../original/" + inputModelURL.fileName().replace(BAKED_FBX_EXTENSION, FBX_EXTENSION));
        QUrl newInputModelURL = inputModelURL.adjusted(QUrl::RemoveFilename).resolved(originalRelativePath);
        _modelURL = newInputModelURL;
    }
}

void FBXBaker::bakeProcessedSource(const hfm::Model::Pointer& hfmModel, const std::vector<hifi::ByteArray>& dracoMeshes, const std::vector<std::vector<hifi::ByteArray>>& dracoMaterialLists) {
    if (shouldStop()) {
        return;
    }

    rewriteAndBakeSceneModels(hfmModel->meshes, dracoMeshes, dracoMaterialLists);
}

void FBXBaker::replaceMeshNodeWithDraco(FBXNode& meshNode, const QByteArray& dracoMeshBytes, const std::vector<hifi::ByteArray>& dracoMaterialList) {
    // Compress mesh information and store in dracoMeshNode
    FBXNode dracoMeshNode;
    bool success = buildDracoMeshNode(dracoMeshNode, dracoMeshBytes, dracoMaterialList);

    if (!success) {
        return;
    } else {
        meshNode.children.push_back(dracoMeshNode);

        static const std::vector<QString> nodeNamesToDelete {
            // Node data that is packed into the draco mesh
            "Vertices",
            "PolygonVertexIndex",
            "LayerElementNormal",
            "LayerElementColor",
            "LayerElementUV",
            "LayerElementMaterial",
            "LayerElementTexture",

            // Node data that we don't support
            "Edges",
            "LayerElementTangent",
            "LayerElementBinormal",
            "LayerElementSmoothing"
        };
        auto& children = meshNode.children;
        auto it = children.begin();
        while (it != children.end()) {
            auto begin = nodeNamesToDelete.begin();
            auto end = nodeNamesToDelete.end();
            if (find(begin, end, it->name) != end) {
                it = children.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void FBXBaker::rewriteAndBakeSceneModels(const std::vector<hfm::Mesh>& meshes, const std::vector<hifi::ByteArray>& dracoMeshes, const std::vector<std::vector<hifi::ByteArray>>& dracoMaterialLists) {
    std::vector<int> meshIndexToRuntimeOrder;
    auto meshCount = (uint32_t)meshes.size();
    meshIndexToRuntimeOrder.resize(meshCount);
    for (uint32_t i = 0; i < meshCount; i++) {
        meshIndexToRuntimeOrder[meshes[i].meshIndex] = i;
    }
    
    // The meshIndex represents the order in which the meshes are loaded from the FBX file
    // We replicate this order by iterating over the meshes in the same way that FBXSerializer does
    int meshIndex = 0;
    for (FBXNode& rootChild : _rootNode.children) {
        if (rootChild.name == "Objects") {
            auto object = rootChild.children.begin();
            while (object != rootChild.children.end()) {
                if (object->name == "Geometry") {
                    if (object->properties.at(2) == "Mesh") {
                        int meshNum = meshIndexToRuntimeOrder[meshIndex];
                        replaceMeshNodeWithDraco(*object, dracoMeshes[meshNum], dracoMaterialLists[meshNum]);
                        meshIndex++;
                    }
                    object++;
                } else if (object->name == "Model") {
                    for (FBXNode& modelChild : object->children) {
                        if (modelChild.name == "Properties60" || modelChild.name == "Properties70") {
                            // This is a properties node
                            // Remove the geometric transform because that has been applied directly to the vertices in FBXSerializer
                            static const QVariant GEOMETRIC_TRANSLATION = hifi::ByteArray("GeometricTranslation");
                            static const QVariant GEOMETRIC_ROTATION = hifi::ByteArray("GeometricRotation");
                            static const QVariant GEOMETRIC_SCALING = hifi::ByteArray("GeometricScaling");
                            for (int i = 0; i < modelChild.children.size(); i++) {
                                const auto& prop = modelChild.children[i];
                                const auto& propertyName = prop.properties.at(0);
                                if (propertyName == GEOMETRIC_TRANSLATION ||
                                        propertyName == GEOMETRIC_ROTATION ||
                                        propertyName == GEOMETRIC_SCALING) {
                                    modelChild.children.removeAt(i);
                                    --i;
                                }
                            }
                        } else if (modelChild.name == "Vertices") {
                            // This model is also a mesh
                            int meshNum = meshIndexToRuntimeOrder[meshIndex];
                            replaceMeshNodeWithDraco(*object, dracoMeshes[meshNum], dracoMaterialLists[meshNum]);
                            meshIndex++;
                        }
                    }
                    object++;
                } else if (object->name == "Texture" || object->name == "Video") {
                    // this is an embedded texture, we need to remove it from the FBX
                    object = rootChild.children.erase(object);
                } else if (object->name == "Material") {
                    for (FBXNode& materialChild : object->children) {
                        if (materialChild.name == "Properties60" || materialChild.name == "Properties70") {
                            // This is a properties node
                            // Remove the material texture scale because that is now included in the material JSON
                            // Texture nodes are removed, so their texture scale is effectively gone already
                            static const QVariant MAYA_UV_SCALE = hifi::ByteArray("Maya|uv_scale");
                            static const QVariant MAYA_UV_OFFSET = hifi::ByteArray("Maya|uv_offset");
                            for (int i = 0; i < materialChild.children.size(); i++) {
                                const auto& prop = materialChild.children[i];
                                const auto& propertyName = prop.properties.at(0);
                                if (propertyName == MAYA_UV_SCALE ||
                                    propertyName == MAYA_UV_OFFSET) {
                                    materialChild.children.removeAt(i);
                                    --i;
                                }
                            }
                        }
                    }

                    object++;
                } else {
                    object++;
                }

                if (hasErrors()) {
                    return;
                }
            }
        }
    }
}