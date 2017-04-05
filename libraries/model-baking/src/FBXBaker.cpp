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

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <NetworkAccessManager.h>

#include "FBXBaker.h"

Q_LOGGING_CATEGORY(model_baking, "hifi.model-baking");

FBXBaker::FBXBaker(QUrl fbxURL, QString baseOutputPath) :
    _fbxURL(fbxURL),
    _baseOutputPath(baseOutputPath)
{
    // create an FBX SDK manager
    _sdkManager = FbxManager::Create();

    // grab the name of the FBX from the URL, this is used for folder output names
    auto fileName = fbxURL.fileName();
    _fbxName = fileName.left(fileName.indexOf('.'));
}

FBXBaker::~FBXBaker() {
    _sdkManager->Destroy();
}


void FBXBaker::start() {
    // setup the output folder for the results of this bake
    if (!setupOutputFolder()) {
        return;
    }

    // check if the FBX is local or first needs to be downloaded
    if (_fbxURL.isLocalFile()) {
        // load up the local file
        QFile localFBX { _fbxURL.toLocalFile() };

        // make a copy in the output folder
        localFBX.copy(_uniqueOutputPath + _fbxURL.fileName());

        // start the bake now that we have everything in place
        bake();
    } else {
        // remote file, kick off a download
        auto& networkAccessManager = NetworkAccessManager::getInstance();

        QNetworkRequest networkRequest;

        // setup the request to follow re-directs and always hit the network
        networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

        networkRequest.setUrl(_fbxURL);

        qCDebug(model_baking) << "Downloading" << _fbxURL;

        auto networkReply = networkAccessManager.get(networkRequest);
        connect(networkReply, &QNetworkReply::finished, this, &FBXBaker::handleFBXNetworkReply);
    }
}

bool FBXBaker::setupOutputFolder() {
    // construct the output path using the name of the fbx and the base output path
    _uniqueOutputPath =  _baseOutputPath + "/" + _fbxName + "/";

    // make sure there isn't already an output directory using the same name
    int iteration = 0;

    while (QDir(_uniqueOutputPath).exists()) {
        _uniqueOutputPath = _baseOutputPath + "/" + _fbxName + "-" + QString::number(++iteration) + "/";
    }

    qCDebug(model_baking) << "Creating FBX output folder" << _uniqueOutputPath;

    // attempt to make the output folder
    if (!QDir().mkdir(_uniqueOutputPath)) {
        qCWarning(model_baking) << "Failed to created FBX output folder" << _uniqueOutputPath;

        emit finished();
        return false;
    }

    return true;
}

void FBXBaker::handleFBXNetworkReply() {
    QNetworkReply* requestReply = qobject_cast<QNetworkReply*>(sender());

    if (requestReply->error() == QNetworkReply::NoError) {
        qCDebug(model_baking) << "Downloaded" << _fbxURL;

        // grab the contents of the reply and make a copy in the output folder
        QFile copyOfOriginal(_uniqueOutputPath + _fbxURL.fileName());

        qDebug(model_baking) << "Writing copy of original FBX to" << copyOfOriginal.fileName();

        if (!copyOfOriginal.open(QIODevice::WriteOnly) || (copyOfOriginal.write(requestReply->readAll()) == -1)) {

            // add an error to the error list for this FBX stating that a duplicate of the original could not be made
            emit finished();
            return;
        }

        // close that file now that we are done writing to it
        copyOfOriginal.close();

        // kick off the bake process now that everything is ready to go
        bake();
    } else {
        qDebug() << "ERROR DOWNLOADING FBX" << requestReply->errorString();
    }
}

void FBXBaker::bake() {
    // (1) load the scene from the FBX file
    // (2) enumerate the textures found in the scene and bake them
    // (3) export the FBX with re-written texture references
    // (4) enumerate the collected texture paths and bake the textures

    qCDebug(model_baking) << "Baking" << _fbxURL;

    // a failure at any step along the way stops the chain
    importScene() && rewriteAndCollectSceneTextures() && exportScene() && bakeTextures();

    // emit a signal saying that we are done, with whatever errors were produced
    emit finished();
}

bool FBXBaker::importScene() {
    // create an FBX SDK importer
    FbxImporter* importer = FbxImporter::Create(_sdkManager, "");

    // import the copy of the original FBX file
    QString originalCopyPath = _uniqueOutputPath + _fbxURL.fileName();
    bool importStatus = importer->Initialize(originalCopyPath.toLocal8Bit().data());

    if (!importStatus) {
        // failed to initialize importer, print an error and return
        qCCritical(model_baking) << "Failed to import FBX file at" << _fbxURL
            << "- error:" << importer->GetStatus().GetErrorString();

        return false;
    } else {
        qCDebug(model_baking) << "Imported" << _fbxURL << "to FbxScene";
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
                QString bakedTextureFilePath { _uniqueOutputPath + "ktx/" + bakedTextureFileName };

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

    auto rewrittenFBXPath = _uniqueOutputPath + _fbxName + ".ktx.fbx";
    bool exportStatus = exporter->Initialize(rewrittenFBXPath.toLocal8Bit().data());

    if (!exportStatus) {
        // failed to initialize exporter, print an error and return
         qCCritical(model_baking) << "Failed to export FBX file at" << _fbxURL
            << "to" << rewrittenFBXPath << "- error:" << exporter->GetStatus().GetErrorString();

        return false;
    }

    // export the scene
    exporter->Export(_scene);

    qCDebug(model_baking) << "Exported" << _fbxURL << "with re-written paths to" << rewrittenFBXPath;

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
