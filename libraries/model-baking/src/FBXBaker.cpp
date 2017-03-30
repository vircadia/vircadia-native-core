//
//  FBXBaker.cpp
//  libraries/model-baking/src
//
//  Created by Stephen Birarda on 3/30/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <fbxsdk.h>
#include <fbxsdk/scene/shading/fbxlayeredtexture.h>

#include <QtCore/QFileInfo>

#include "FBXBaker.h"

Q_LOGGING_CATEGORY(model_baking, "hifi.model-baking");

FBXBaker::FBXBaker(std::string fbxPath) :
    _fbxPath(fbxPath)
{
    // create an FBX SDK manager
    _sdkManager = FbxManager::Create();
}

bool FBXBaker::bakeFBX() {

    // load the scene from the FBX file
    if (importScene()) {
        // enumerate the textures found in the scene and bake them
        rewriteAndCollectSceneTextures();
    } else {
        return false;
    }

    return true;
}

bool FBXBaker::importScene() {
    // create an FBX SDK importer
    FbxImporter* importer = FbxImporter::Create(_sdkManager, "");

    // import the FBX file at the given path
    bool importStatus = importer->Initialize(_fbxPath.c_str());

    if (!importStatus) {
        // failed to import the FBX file, print an error and return
        qCDebug(model_baking) << "Failed to import FBX file at" << _fbxPath.c_str() << "- error:" << importer->GetStatus().GetErrorString();

        return false;
    }

    // setup a new scene to hold the imported file
    _scene = FbxScene::Create(_sdkManager, "bakeScene");

    // import the file to the created scene
    importer->Import(_scene);

    // destroy the importer that is no longer needed
    importer->Destroy();

    return true;
}

bool FBXBaker::rewriteAndCollectSceneTextures() {
    // grab the root node from the scene
    FbxNode* rootNode = _scene->GetRootNode();

    if (rootNode) {
        // enumerate the children of the root node
        for (int i = 0; i < rootNode->GetChildCount(); ++i) {
            FbxNode* node = rootNode->GetChild(i);

            // check if this child is a mesh node
            if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
                FbxMesh* mesh = (FbxMesh*) node->GetNodeAttribute();

                // make sure this mesh is valid
                if (mesh->GetNode() != nullptr) {
                    // figure out the number of materials in this mesh
                    int numMaterials = mesh->GetNode()->GetSrcObjectCount<FbxSurfaceMaterial>();

                    // enumerate the materials in this mesh
                    for (int materialIndex = 0; materialIndex < numMaterials; materialIndex++) {
                        // grab this material
                        FbxSurfaceMaterial* material = mesh->GetNode()->GetSrcObject<FbxSurfaceMaterial>(materialIndex);

                        if (material) {
                            // enumerate the textures in this valid material
                            int textureIndex;
                            FBXSDK_FOR_EACH_TEXTURE(textureIndex) {
                                // collect this texture so we know later to bake it
                                FbxProperty property = material->FindProperty(FbxLayerElement::sTextureChannelNames[textureIndex]);
                                if (property.IsValid()) {
                                    rewriteAndCollectChannelTextures(property);
                                }
                            }

                        }
                    }

                }
            }
        }
    }

    return true;
}

bool FBXBaker::rewriteAndCollectChannelTextures(FbxProperty& property) {
    if (property.IsValid()) {
        int textureCount = property.GetSrcObjectCount<FbxTexture>();

        // enumerate the textures for this channel
        for (int i = 0; i < textureCount; ++i) {
            // check if this texture is layered
            FbxLayeredTexture* layeredTexture = property.GetSrcObject<FbxLayeredTexture>(i);
            if (layeredTexture) {
                // enumerate the layers of the layered texture
                int numberOfLayers = layeredTexture->GetSrcObjectCount<FbxTexture>();

                for (int j = 0; j < numberOfLayers; ++j) {
                    FbxTexture* texture = layeredTexture->GetSrcObject<FbxTexture>(j);
                    rewriteAndCollectTexture(texture);
                }
            } else {
                FbxTexture* texture = property.GetSrcObject<FbxTexture>(i);
                rewriteAndCollectTexture(texture);
            }
        }
    }

    return true;
}

static const QString BAKED_TEXTURE_DIRECTORY = "textures";
static const QString BAKED_TEXTURE_EXT = ".xtk";

bool FBXBaker::rewriteAndCollectTexture(fbxsdk::FbxTexture* texture) {
    FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);
    if (fileTexture) {
        qCDebug(model_baking) << "Flagging" << fileTexture->GetRelativeFileName() << "for bake and re-mapping to .xtk in FBX";

        // use QFileInfo to easily split up the existing texture filename into its components
        QFileInfo textureFileInfo { fileTexture->GetRelativeFileName() };

        // construct the new baked texture file name
        QString bakedTextureFileName { BAKED_TEXTURE_DIRECTORY + "/" + textureFileInfo.baseName() + BAKED_TEXTURE_EXT };

        // write the new filename into the FBX scene
        fileTexture->SetRelativeFileName(bakedTextureFileName.toLocal8Bit());
    }

    return true;
}
