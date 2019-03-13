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
#include "TextureBaker.h"

#ifdef HIFI_DUMP_FBX
#include "FBXToJSON.h"
#endif

FBXBaker::FBXBaker(const QUrl& inputModelURL, TextureBakerThreadGetter inputTextureThreadGetter,
        const QString& bakedOutputDirectory, const QString& originalOutputDirectory, bool hasBeenBaked) :
        ModelBaker(inputModelURL, inputTextureThreadGetter, bakedOutputDirectory, originalOutputDirectory, hasBeenBaked) {
    if (hasBeenBaked) {
        // Look for the original model file one directory higher. Perhaps this is an oven output directory.
        QUrl originalRelativePath = QUrl("../original/" + inputModelURL.fileName().replace(BAKED_FBX_EXTENSION, FBX_EXTENSION));
        QUrl newInputModelURL = inputModelURL.adjusted(QUrl::RemoveFilename).resolved(originalRelativePath);
        _modelURL = newInputModelURL;
    }
}

void FBXBaker::bakeProcessedSource(const hfm::Model::Pointer& hfmModel, const std::vector<hifi::ByteArray>& dracoMeshes, const std::vector<std::vector<hifi::ByteArray>>& dracoMaterialLists) {
    _hfmModel = hfmModel;
    
#ifdef HIFI_DUMP_FBX
    {
        FBXToJSON fbxToJSON;
        fbxToJSON << _rootNode;
        QFileInfo modelFile(_originalModelFilePath);
        QString outFilename(_bakedOutputDir + "/" + modelFile.completeBaseName() + "_FBX.json");
        QFile jsonFile(outFilename);
        if (jsonFile.open(QIODevice::WriteOnly)) {
            jsonFile.write(fbxToJSON.str().c_str(), fbxToJSON.str().length());
            jsonFile.close();
        }
    }
#endif

    if (shouldStop()) {
        return;
    }

    // enumerate the models and textures found in the scene and start a bake for them
    rewriteAndBakeSceneTextures();

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

void FBXBaker::rewriteAndBakeSceneModels(const QVector<hfm::Mesh>& meshes, const std::vector<hifi::ByteArray>& dracoMeshes, const std::vector<std::vector<hifi::ByteArray>>& dracoMaterialLists) {
    std::vector<int> meshIndexToRuntimeOrder;
    auto meshCount = (int)meshes.size();
    meshIndexToRuntimeOrder.resize(meshCount);
    for (int i = 0; i < meshCount; i++) {
        meshIndexToRuntimeOrder[meshes[i].meshIndex] = i;
    }
    
    // The meshIndex represents the order in which the meshes are loaded from the FBX file
    // We replicate this order by iterating over the meshes in the same way that FBXSerializer does
    int meshIndex = 0;
    for (FBXNode& rootChild : _rootNode.children) {
        if (rootChild.name == "Objects") {
            for (FBXNode& object : rootChild.children) {
                if (object.name == "Geometry") {
                    if (object.properties.at(2) == "Mesh") {
                        int meshNum = meshIndexToRuntimeOrder[meshIndex];
                        replaceMeshNodeWithDraco(object, dracoMeshes[meshNum], dracoMaterialLists[meshNum]);
                        meshIndex++;
                    }
                } else if (object.name == "Model") {
                    for (FBXNode& modelChild : object.children) {
                        bool properties = false;
                        hifi::ByteArray propertyName;
                        int index;
                        if (modelChild.name == "Properties60") {
                            properties = true;
                            propertyName = "Property";
                            index = 3;

                        } else if (modelChild.name == "Properties70") {
                            properties = true;
                            propertyName = "P";
                            index = 4;
                        }

                        if (properties) {
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
                            replaceMeshNodeWithDraco(object, dracoMeshes[meshNum], dracoMaterialLists[meshNum]);
                            meshIndex++;
                        }
                    }
                }

                if (hasErrors()) {
                    return;
                }
            }
        }
    }
}

void FBXBaker::rewriteAndBakeSceneTextures() {
    using namespace image::TextureUsage;
    QHash<QString, image::TextureUsage::Type> textureTypes;

    // enumerate the materials in the extracted geometry so we can determine the texture type for each texture ID
    for (const auto& material : _hfmModel->materials) {
        if (material.normalTexture.isBumpmap) {
            textureTypes[material.normalTexture.id] = BUMP_TEXTURE;
        } else {
            textureTypes[material.normalTexture.id] = NORMAL_TEXTURE;
        }

        textureTypes[material.albedoTexture.id] = ALBEDO_TEXTURE;
        textureTypes[material.glossTexture.id] = GLOSS_TEXTURE;
        textureTypes[material.roughnessTexture.id] = ROUGHNESS_TEXTURE;
        textureTypes[material.specularTexture.id] = SPECULAR_TEXTURE;
        textureTypes[material.metallicTexture.id] = METALLIC_TEXTURE;
        textureTypes[material.emissiveTexture.id] = EMISSIVE_TEXTURE;
        textureTypes[material.occlusionTexture.id] = OCCLUSION_TEXTURE;
        textureTypes[material.lightmapTexture.id] = LIGHTMAP_TEXTURE;
    }

    // enumerate the children of the root node
    for (FBXNode& rootChild : _rootNode.children) {

        if (rootChild.name == "Objects") {

            // enumerate the objects
            auto object = rootChild.children.begin();
            while (object != rootChild.children.end()) {
                if (object->name == "Texture") {

                    // double check that we didn't get an abort while baking the last texture
                    if (shouldStop()) {
                        return;
                    }

                    // enumerate the texture children
                    for (FBXNode& textureChild : object->children) {

                        if (textureChild.name == "RelativeFilename") {
                            QString hfmTextureFileName { textureChild.properties.at(0).toString() };
                            
                            // grab the ID for this texture so we can figure out the
                            // texture type from the loaded materials
                            auto textureID { object->properties[0].toString() };
                            auto textureType = textureTypes[textureID];

                            // Compress the texture information and return the new filename to be added into the FBX scene
                            auto bakedTextureFile = compressTexture(hfmTextureFileName, textureType);

                            // If no errors or warnings have occurred during texture compression add the filename to the FBX scene
                            if (!bakedTextureFile.isNull()) {
                                textureChild.properties[0] = bakedTextureFile;
                            } else {
                                // if bake fails - return, if there were errors and continue, if there were warnings.
                                if (hasErrors()) {
                                    return;
                                } else if (hasWarnings()) {
                                    continue;
                                }
                            }
                        }
                    }

                    ++object;

                } else if (object->name == "Video") {
                    // this is an embedded texture, we need to remove it from the FBX
                    object = rootChild.children.erase(object);
                } else {
                    ++object;
                }
            }
        }
    }
}
