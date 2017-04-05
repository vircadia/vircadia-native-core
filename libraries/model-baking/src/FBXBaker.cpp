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
#include <QThread>

#include "FBXBaker.h"

Q_LOGGING_CATEGORY(model_baking, "hifi.model-baking");

FBXBaker::FBXBaker(QUrl fbxPath) :
    _fbxPath(fbxPath)
{
    // create an FBX SDK manager
    _sdkManager = FbxManager::Create();
}

FBXBaker::~FBXBaker() {
    _sdkManager->Destroy();
}

void FBXBaker::start() {
    // check if the FBX is local or first needs to be downloaded
    if (_fbxPath.isLocalFile()) {
        // local file, bake now
        bake();
    } else {
        // remote file, kick off a download
    }
}

void FBXBaker::bake() {
    // (1) load the scene from the FBX file
    // (2) enumerate the textures found in the scene and bake them
    // (3) export the FBX with re-written texture references
    // (4) enumerate the collected texture paths and bake the textures

    // a failure at any step along the way stops the chain
    importScene() && rewriteAndCollectSceneTextures() && exportScene() && bakeTextures();

    // emit a signal saying that we are done, with whatever errors were produced
    emit finished(_errorList);
}

bool FBXBaker::importScene() {
    // create an FBX SDK importer
    FbxImporter* importer = FbxImporter::Create(_sdkManager, "");

    // import the FBX file at the given path
    bool importStatus = importer->Initialize(_fbxPath.toLocalFile().toLocal8Bit().data());

    if (!importStatus) {
        // failed to initialize importer, print an error and return
        qCDebug(model_baking) << "Failed to import FBX file at" << _fbxPath << "- error:" << importer->GetStatus().GetErrorString();

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

static const QString BAKED_TEXTURE_DIRECTORY = "textures";
static const QString BAKED_TEXTURE_EXT = ".ktx";

static const QString EXPORT_PATH { "/Users/birarda/code/hifi/lod/test-oven/export/DiscGolfBasket.ktx.fbx" };

bool FBXBaker::rewriteAndCollectSceneTextures() {
    // get a count of the textures used in the scene
    int numTextures = _scene->GetTextureCount();

    // enumerate the textures in the scene
    for (int i = 0; i < numTextures; i++) {
        // grab each file texture
        FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(_scene->GetTexture(i));

        if (fileTexture) {
            // use QFileInfo to easily split up the existing texture filename into its components
            QFileInfo textureFileInfo { fileTexture->GetFileName() };

            // make sure this texture points to something
            if (!textureFileInfo.filePath().isEmpty()) {

                // construct the new baked texture file name and file path
                QString bakedTextureFileName { textureFileInfo.baseName() + BAKED_TEXTURE_EXT };
                QString bakedTextureFilePath { QFileInfo(EXPORT_PATH).absolutePath() + "/textures/" + bakedTextureFileName };

                qCDebug(model_baking).noquote() << "Re-mapping" << fileTexture->GetFileName() << "to" << bakedTextureFilePath;

                // write the new filename into the FBX scene
                fileTexture->SetFileName(bakedTextureFilePath.toLocal8Bit());

                // add the texture to the list of textures needing to be baked
                if (textureFileInfo.exists() && textureFileInfo.isFile()) {
                    // append the URL to the local texture that we have confirmed exists
                    _unbakedTextures.append(QUrl::fromLocalFile(textureFileInfo.absoluteFilePath()));
                } else {

                }
            }
        }
    }

    return true;
}

bool FBXBaker::exportScene() {
    // setup the exporter
    FbxExporter* exporter = FbxExporter::Create(_sdkManager, "");

    bool exportStatus = exporter->Initialize(EXPORT_PATH.toLocal8Bit().data());

    if (!exportStatus) {
        // failed to initialize exporter, print an error and return
         qCDebug(model_baking) << "Failed to export FBX file at" << _fbxPath
            << "to" << EXPORT_PATH << "- error:" << exporter->GetStatus().GetErrorString();

        return false;
    }

    // export the scene
    exporter->Export(_scene);

    return true;
}

bool FBXBaker::bakeTextures() {
    // enumerate the list of unbaked textures
    foreach(const QUrl& textureUrl, _unbakedTextures) {
        qCDebug(model_baking) << "Baking texture at" << textureUrl;

        if (textureUrl.isLocalFile()) {
            // this is a local file that we've already determined is available on the filesystem

            // load the file
            QFile localTexture { textureUrl.toLocalFile() };

            if (!localTexture.open(QIODevice::ReadOnly)) {
                // add an error to the list stating that this texture couldn't be baked because it could not be loaded
            }

            // call the image library to produce a compressed KTX for this image
        } else {
            // this is a remote texture that we'll need to download first
        }
    }

    return true;
}
